#include "c.h"

static int curseg;		/* current segment */
static struct structexp {	/* current structure expression: */
	Symbol var;			/* temporary variable */
	int off;			/* offset in var */
	Tree tree;			/* evolving tree */
	struct structexp *link;		/* outer structure expression */
} *current;

static int genasgn ARGS((Tree, struct structexp *));
static void genchar ARGS((Symbol, struct structexp *));
static int genconst ARGS((Tree, int));
static void genspace ARGS((int, struct structexp *));
static int initarray ARGS((int, Type, int));
static int initchar ARGS((int, Type));
static void initend ARGS((int, char []));
static int initfields ARGS((Field, Field));
static int initstruct ARGS((int, Type, int));
static Tree initvalue ARGS((Type));

/* defpointer - initialize a pointer to p or to 0 if p==0 */
void defpointer(p) Symbol p; {
	if (p)
		(*IR->defaddress)(p);
	else {
		Value v;
		(*IR->defconst)(P, (v.p = 0, v));
	}
}

/* genasgn - append tree for assignment of e to evolving structure expression in *sp */
static int genasgn(e, sp) Tree e; struct structexp *sp; {
	Tree p;

	sp->off = roundup(sp->off, e->type->size);
	p = simplify(ADD+P, ptr(e->type), lvalue(idtree(sp->var)), consttree(sp->off, inttype));
	if (isarray(e->type)) {
		p = tree(ASGN+B, e->type, p, e);
		p->u.sym = intconst(e->type->size);
	} else
		p = asgntree(ASGN, retype(rvalue(p), e->type), e);
	sp->tree = tree(RIGHT, voidtype, sp->tree, p);
	sp->off += e->type->size;
	return e->type->size;
}

/* genchar - generate assignment of string constant p to array in *sp */
static void genchar(p, sp) Symbol p; struct structexp *sp; {
	char *s = p->u.c.v.p;
	int n;

	for (n = p->type->size; n > 0 && sp->off%inttype->align; n--)
		genasgn(cast(consttree((*s++)&0377, inttype), chartype), sp);
	if (n > 0) {
		static Value v;
		Type ty = array(chartype, p->type->size - (s - (char *)p->u.c.v.p), 0);
		Tree e;
		v.p = stringn(s, ty->size);
		p = constant(ty, v);
		if (p->u.c.loc == NULL)
			p->u.c.loc = genident(STATIC, ty, GLOBAL);
		e = tree(INDIR+B, ty, pointer(idtree(p)), 0);
		e->u.sym = intconst(ty->size);
		genasgn(e, sp);
	}
}

/* genconst - generate/check constant expression e; return size */
static int genconst(e, def) Tree e; int def; {
	for (;;)
		switch (generic(e->op)) {
		case ADDRG:
			if (def)
				(*IR->defaddress)(e->u.sym);
			return e->type->size;
		case CNST:
			if (e->op == CNST+P && isarray(e->type)) {
				e = cvtconst(e);
				continue;
			}
			if (def)
				(*IR->defconst)(ttob(e->type), e->u.v);
			return e->type->size;
		case RIGHT:
			assert(e->kids[0] || e->kids[1]);
			if (e->kids[1] && e->kids[0])
				error("initializer must be constant\n");
			e = e->kids[1] ? e->kids[1] : e->kids[0];
			continue;
		case CVP:
			if (isarith(e->type))
				error("cast from `%t' to `%t' is illegal in constant expressions\n",
					e->kids[0]->type, e->type);
			/* fall thru */
		case CVC: case CVI: case CVS: case CVU:
		case CVD: case CVF:
			e = e->kids[0];
			continue;
		default:
			error("initializer must be constant\n");
			if (def)
				genconst(consttree(0, inttype), def);
			return inttype->size;
		}
}

/* genspace - generate n bytes of space or 0's */
static void genspace(n, sp) int n; struct structexp *sp; {
	if (sp == 0)
		(*IR->space)(n);
	else if (n <= inttype->size)
		for ( ; n > 0; n--)
			genasgn(cast(consttree(0, inttype), chartype), sp);
	else {
		Tree e;
		static Symbol zeros;
		if (zeros == 0) {
			zeros = install(stringd(genlabel(1)), &globals, GLOBAL, PERM);
			zeros->sclass = STATIC;
			zeros->type = array(chartype, n, 0);
			zeros->generated = 1;
			(*IR->defsymbol)(zeros);
		}
		if (n > zeros->type->size)
			zeros->type = array(chartype, n, 0);
		e = tree(INDIR+B, zeros->type, idtree(zeros), 0);
		e->u.sym = intconst(n);
		genasgn(e, sp);
	}
}

/* initarray - initialize array of ty of <= len bytes; if len == 0, go to } */
static int initarray(len, ty, lev) Type ty; int len, lev; {
	int n = 0;

	do {
		initializer(ty, lev);
		n += ty->size;
		if (len > 0 && n >= len || t != ',')
			break;
		t = gettok();
	} while (t != '}');
	return n;
}

/* initchar - initialize array of <= len ty characters; if len == 0, go to } */
static int initchar(len, ty) Type ty; int len; {
	int n = 0;
	char buf[16], *s = buf;

	do {
		if (current) {
			Type aty;
			Tree e = expr1(0);
			if ((aty = assign(ty, e)) != NULL)
				genasgn(cast(e, aty), current);
			else
				error("invalid initialization type; found `%t' expected `%t'\n",
					e->type, ty);
			++n;
		} else {
			*s++ = initvalue(ty)->u.v.sc;
			if (++n%inttype->size == 0) {
				(*IR->defstring)(inttype->size, buf);
				s = buf;
			}
		}
		if (len > 0 && n >= len || t != ',')
			break;
		t = gettok();
	} while (t != '}');
	if (s > buf)
		(*IR->defstring)(s - buf, buf);
	return n;
}

/* initend - finish off an initialization at level lev; accepts trailing comma */
static void initend(lev, follow) int lev; char follow[]; {
	if (lev == 0 && t == ',')
		t = gettok();
	test('}', follow);
}

/* initfields - initialize <= an unsigned's worth of bit fields in fields p to q */
static int initfields(p, q) Field p, q; {
	unsigned int bits = 0;
	int i, n = 0;

	if (current) {
		Tree e = consttree(0, unsignedtype);
		do {
			Tree x = expr1(0);
			if (fieldsize(p) < 8*p->type->size)
				x = (*optree['&'])(BAND, x,
					consttree(fieldmask(p), unsignedtype));
			e = (*optree['|'])(BOR, e,
				(*optree[LSHIFT])(LSH, x, consttree(fieldright(p), inttype)));
			if (p->link == q)
				break;
			p = p->link;
		} while (t == ',' && (t = gettok()) != 0);
		if (q && (n = q->offset - p->offset) < unsignedtype->size)
			if (IR->little_endian)
				for (i = 0; i < n; i++) {
					genasgn(retype(e, chartype), current);
					e = (*optree[RSHIFT])(RSH, e, consttree(8, inttype));
				}
			else /* a big endian */
				for (i = n - 1; i >= 0; i--) {
					Tree x = (*optree[RSHIFT])(RSH, e,
						consttree(8*(unsignedtype->size - n + i), inttype));
					genasgn(retype(x, chartype), current);
				}
		else
			n = genasgn(e, current);
		return n;
	}
	do {
		i = initvalue(inttype)->u.v.i;
		if (fieldsize(p) < 8*p->type->size) {
			if (p->type == inttype && i >= 0 && (i&~(fieldmask(p)>>1)) !=  0
			||  p->type == inttype && i <  0 && (i| (fieldmask(p)>>1)) != ~(unsigned)0
			||  p->type == unsignedtype      && (i& ~fieldmask(p))     !=  0)
				warning("initializer exceeds bit-field width\n");
			i &= fieldmask(p);
		}
		bits |= i<<fieldright(p);
		if (IR->little_endian) {
			if (fieldsize(p) + fieldright(p) > n)
				n = fieldsize(p) + fieldright(p);
		} else {
			if (fieldsize(p) + fieldleft(p) > n)
				n = fieldsize(p) + fieldleft(p);
		}
		if (p->link == q)
			break;
		p = p->link;
	} while (t == ',' && (t = gettok()) != 0);
	n = (n + 7)/8;
	for (i = 0; i < n; i++) {
		Value v;
		if (IR->little_endian) {
			v.uc = bits;
			bits >>= 8;
		} else {	/* a big endian */
			v.uc = bits>>(8*(unsignedtype->size - 1));
			bits <<= 8;
		}
		(*IR->defconst)(C, v);
	}
	return n;
}

/* initializer - constexpr | { constexpr ( , constexpr )* [ , ] } */
Type initializer(ty, lev) Type ty; int lev; {
	int n = 0;
	Tree e;
	Type aty = NULL;
	static char follow[] = { IF, CHAR, STATIC, 0 };

	ty = unqual(ty);
	if (isscalar(ty)) {
		if (!current)
			needconst++;
		if (t == '{') {
			t = gettok();
			e = expr1(0);
			initend(lev, follow);
		} else
			e = expr1(0);
		e = pointer(e);
		if ((aty = assign(ty, e)) != NULL)
			e = cast(e, aty);
		else
			error("invalid initialization type; found `%t' expected `%t'\n",
				e->type, ty);
		if (current)
			n = genasgn(e, current);
		else {
			n = genconst(e, 1);
			deallocate(STMT);
			needconst--;
		}
	}
	if ((isunion(ty) || isstruct(ty)) && ty->size == 0) {
		static char follow[] = { CHAR, STATIC, 0 };
		error("cannot initialize undefined `%t'\n", ty);
		skipto(';', follow);
		return ty;
	} else if (isunion(ty)) {
		if (t == '{') {
			t = gettok();
			n = initstruct(ty->u.sym->u.s.flist->type->size, ty, lev + 1);
			initend(lev, follow);
		} else {
			if (lev == 0)
				error("missing { in initialization of `%t'\n", ty);
			n = initstruct(ty->u.sym->u.s.flist->type->size, ty, lev + 1);
		}
	} else if (isstruct(ty)) {
		if (t == '{') {
			t = gettok();
			n = initstruct(0, ty, lev + 1);
			test('}', follow);
		} else if (current) {
			e = expr1(0);
			if (assign(ty, e))
				genasgn(e, current);
			else
				error("invalid initialization type; found `%t' expected `%t'\n",
					e->type, ty);
			n = ty->size;
		} else if (lev > 0)
			n = initstruct(ty->size, ty, lev + 1);
		else {
			error("missing { in initialization of `%t'\n", ty);
			n = initstruct(ty->u.sym->u.s.flist->type->size, ty, lev + 1);
		}
	}
	if (isarray(ty))
		aty = unqual(ty->type);
	if (isarray(ty) && ischar(aty)) {
		if (t == SCON) {
			if (ty->size > 0 && ty->size == tsym->type->size - 1)
				tsym->type = array(chartype, ty->size, 0);
			n = tsym->type->size;
			if (current)
				genchar(tsym, current);
			else
				(*IR->defstring)(tsym->type->size, tsym->u.c.v.p);
			t = gettok();
		} else if (t == '{') {
			t = gettok();
			if (t == SCON) {
				ty = initializer(ty, lev + 1);
				initend(lev, follow);
				return ty;
			}
			n = initchar(0, aty);
			test('}', follow);
		} else if (lev > 0 && ty->size > 0)
			n = initchar(ty->size, aty);
		else {	/* eg, char c[] = 0; */
			error("missing { in initialization of `%t'\n", ty);
			n = initchar(1, aty);
		}
	} else if (isarray(ty)) {
		if (t == '{') {
			t = gettok();
			n = initarray(0, aty, lev + 1);
			test('}', follow);
		} else if (lev > 0 && ty->size > 0)
			n = initarray(ty->size, aty, lev + 1);
		else {
			error("missing { in initialization of `%t'\n", ty);
			n = initarray(aty->size, aty, lev + 1);
		}
	}	
	if (ty->size) {
		if (n > ty->size)
			error("too many initializers\n");
		else if (n < ty->size)
			genspace(ty->size - n, current);
	} else if (isarray(ty) && ty->type->size > 0)
		ty = array(ty->type, n/ty->type->size, 0);
	else
		ty->size = n;
	return ty;
}

/* initstruct - initialize a struct ty of <= len bytes; if len == 0, go to } */
static int initstruct(len, ty, lev) Type ty; int len, lev; {
	int a, n = 0;
	Field p = ty->u.sym->u.s.flist;

	do {
		if (p->offset > n) {
			genspace(p->offset - n, current);
			n += p->offset - n;
		}
		if (p->lsb) {
			Field q = p;
			while (q->link && q->link->offset == p->offset)
				q = q->link;
			n += initfields(p, q->link);
			p = q;
		} else {
			initializer(p->type, lev);
			n += p->type->size;
		}
		if (p->link) {
			p = p->link;
			a = p->type->align;
		} else
			a = ty->align;
		if (a && n%a) {
			genspace(a - n%a, current);
			n = roundup(n, a);
		}
		if (len > 0 && n >= len || t != ',')
			break;
		t = gettok();
	} while (t != '}');
	return n;
}

/* initvalue - evaluate a constant expression for a value of integer type ty */
static Tree initvalue(ty) Type ty; {
	Type aty;
	Tree e;

	needconst++;
	e = expr1(0);
	if ((aty = assign(ty, e)) != NULL)
		e = cast(e, aty);
	else {
		error("invalid initialization type; found `%t' expected `%t'\n",
			e->type,  ty);
		e = retype(consttree(0, inttype), ty);
	}
	needconst--;
	if (generic(e->op) != CNST) {
		error("initializer must be constant\n");
		e = retype(consttree(0, inttype), ty);
	}
	return e;
}

/* structexp - in-line structure expression '{' expr ( , expr )* [ , ] '}' */
Tree structexp(ty, t1) Type ty; Symbol t1; {
	struct structexp e;

	e.var = t1;
	e.off = 0;
	e.tree = 0;
	e.link = current;
	current = &e;
	initializer(ty, 0);
	current = e.link;
	return e.tree;
}

/* swtoseg - switch to segment seg, if necessary */
void swtoseg(seg) int seg; {
	if (curseg != seg)
		(*IR->segment)(seg);
	curseg = seg;
}
