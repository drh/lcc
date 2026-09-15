#include "c.h"

static Tree addtree ARGS((int, Tree, Tree));
static Tree andtree ARGS((int, Tree, Tree));
static Tree cmptree ARGS((int, Tree, Tree));
static int compatible ARGS((Type, Type));
static int isnullptr ARGS((Tree e));
static Tree multree ARGS((int, Tree, Tree));
static Tree subtree ARGS((int, Tree, Tree));
#define isvoidptr(ty) \
	(isptr(ty) && unqual(ty->type) == voidtype)

Tree (*optree[]) ARGS((int, Tree, Tree)) = {
#define xx(a,b,c,d,e,f,g) e,
#define yy(a,b,c,d,e,f,g) e,
#include "token.h"
};
Tree call(f, fty, src) Tree f; Type fty; Coordinate src; {
	int n = 0;
	Tree args = NULL, r = NULL;
	Type *proto, rty = unqual(freturn(fty));
	Symbol t3 = NULL;

	if (fty->u.f.oldstyle)
		proto = NULL;
	else
		proto = fty->u.f.proto;
	if (hascall(f))
		r = f;
	if (isstruct(rty))
		{
			t3 = temporary(AUTO, unqual(rty), level);
			if (rty->size == 0)
				error("illegal use of incomplete type `%t'\n", rty);
		}
	if (t != ')')
		for (;;) {
			Tree q = pointer(expr1(0));
			if (proto && *proto && *proto != voidtype)
				{
					Type aty;
					q = value(q);
					aty = assign(*proto, q);
					if (aty)
						q = cast(q, aty);
					else
						error("type error in argument %d to %s; found `%t' expected `%t'\n", n + 1, funcname(f),

							q->type, *proto);
					if ((isint(q->type) || isenum(q->type))
					&& q->type->size != inttype->size)
						q = cast(q, promote(q->type));
					++proto;
				}
			else
				{
					if (!fty->u.f.oldstyle && *proto == NULL)
						error("too many arguments to %s\n", funcname(f));
					q = value(q);
					if (q->type == floattype)
						q = cast(q, doubletype);
					else if (isarray(q->type) || q->type->size == 0)
						error("type error in argument %d to %s; `%t' is illegal\n", n + 1, funcname(f), q->type);

					else
						q = cast(q, promote(q->type));
				}
			if (!IR->wants_argb && isstruct(q->type))
				if (iscallb(q))
					q = addrof(q);
				else {
					Symbol t1 = temporary(AUTO, unqual(q->type), level);
					q = asgn(t1, q);
					q = tree(RIGHT, ptr(t1->type),
						root(q), lvalue(idtree(t1)));
				}
			if (q->type->size == 0)
				q->type = inttype;
			if (hascall(q))
				r = r ? tree(RIGHT, voidtype, r, q) : q;
			args = tree(ARG + widen(q->type), q->type, q, args);
			n++;
			if (Aflag >= 2 && n == 32)
				warning("more than 31 arguments in a call to %s\n",
					funcname(f));
			if (t != ',')
				break;
			t = gettok();
		}
	expect(')');
	if (proto && *proto && *proto != voidtype)
		error("insufficient number of arguments to %s\n",
			funcname(f));
	if (r)
		args = tree(RIGHT, voidtype, r, args);
	if (events.calls)
		apply(events.calls, &src, &f);
	return calltree(f, rty, args, t3);
}
Tree calltree(f, ty, args, t3)
Tree f, args; Type ty; Symbol t3; {
	Tree p;

	if (args)
		f = tree(RIGHT, f->type, args, f);
	if (isstruct(ty))
		assert(t3),
		p = tree(RIGHT, ty,
			tree(CALL+B, ty, f, addrof(idtree(t3))),
			idtree(t3));
	else {
		Type rty = ty;
		if (isenum(ty))
			rty = unqual(ty)->type;
		else if (isptr(ty))
			rty = unsignedtype;
		p = tree(CALL + widen(rty), promote(rty), f, NULL);
		if (isptr(ty) || p->type->size > ty->size)
			p = cast(p, ty);
	}
	return p;
}
int iscallb(e) Tree e; {
	return e->op == RIGHT && e->kids[0] && e->kids[1]
		&& e->kids[0]->op == CALL+B
		&& e->kids[1]->op == INDIR+B
		&& isaddrop(e->kids[1]->kids[0]->op)
		&& e->kids[1]->kids[0]->u.sym->temporary;
}

static Tree addtree(op, l, r) int op; Tree l, r; {
	Type ty = inttype;

	if (isarith(l->type) && isarith(r->type)) {
		ty = binary(l->type, r->type);
		l = cast(l, ty);
		r = cast(r, ty);		
	} else if (isptr(l->type) && isint(r->type))
		return addtree(ADD, r, l);
	else if (  isptr(r->type) && isint(l->type)
	&& !isfunc(r->type->type))
		{
			int n;
			ty = unqual(r->type);
			n = ty->type->size;
			if (n == 0)
				error("unknown size for type `%t'\n", ty->type);
			l = cast(l, promote(l->type));
			if (n > 1)
				l = multree(MUL, consttree(n, inttype), l);
			if (YYcheck && !isaddrop(r->op))		/* omit */
				return nullcall(ty, YYcheck, r, l);	/* omit */
			return simplify(ADD+P, ty, l, r);
		}

	else
		typeerror(op, l, r);
	return simplify(op, ty, l, r);
}

Tree consttree(n, ty) unsigned n; Type ty; {
	Tree p;

	if (isarray(ty))
		ty = atop(ty);
	else assert(ty->op == INT || ty->op == UNSIGNED);
	p = tree(CNST + ttob(ty), ty, NULL, NULL);
	p->u.v.u = n;
	return p;
}
static Tree cmptree(op, l, r) int op; Tree l, r; {
	Type ty;

	if (isarith(l->type) && isarith(r->type)) {
		ty = binary(l->type, r->type);
		l = cast(l, ty);
		r = cast(r, ty);
	} else if (compatible(l->type, r->type)) {
		ty = unsignedtype;
		l = cast(l, ty);
		r = cast(r, ty);
	} else {
		ty = unsignedtype;
		typeerror(op, l, r);
	}
	return simplify(op + ttob(ty), inttype, l, r);
}
static int compatible(ty1, ty2) Type ty1, ty2; {
	return isptr(ty1) && !isfunc(ty1->type)
	    && isptr(ty2) && !isfunc(ty2->type)
	    && eqtype(unqual(ty1->type), unqual(ty2->type), 0);
}
static int isnullptr(e) Tree e; {
	return (isint(e->type) && generic(e->op) == CNST
	        && cast(e, unsignedtype)->u.v.u == 0)
	    || (isvoidptr(e->type) && e->op == CNST+P
	        && e->u.v.p == NULL);
}
Tree eqtree(op, l, r) int op; Tree l, r; {
	Type xty = l->type, yty = r->type;

	if (isptr(xty) && isnullptr(r)
	||  isptr(xty) && !isfunc(xty->type) && isvoidptr(yty)
	||  (isptr(xty) && isptr(yty)
	    && eqtype(unqual(xty->type), unqual(yty->type), 1))) {
		Type ty = unsignedtype;
		l = cast(l, ty);
		r = cast(r, ty);
		return simplify(op + U, inttype, l, r);
	}
	if (isptr(yty) && isnullptr(l)
	||  isptr(yty) && !isfunc(yty->type) && isvoidptr(xty))
		return eqtree(op, r, l);
	return cmptree(op, l, r);
}

Type assign(xty, e) Type xty; Tree e; {
	Type yty = unqual(e->type);

	xty = unqual(xty);
	if (isenum(xty))
		xty = xty->type;
	if (xty->size == 0 || yty->size == 0)
		return NULL;
	if ( isarith(xty) && isarith(yty)
	||  isstruct(xty) && xty == yty)
		return xty;
	if (isstruct(xty) && isstruct(yty) && extends(yty, xty))
		return xty;
	if (isptr(xty) && isstruct(xty->type)
	&&  isptr(yty) && isstruct(yty->type)
	&&  extends(yty->type, xty->type))
		return xty;
	if (isptr(xty) && isnullptr(e))
		return xty;
	if ((isvoidptr(xty) && isptr(yty)
	  || isptr(xty)     && isvoidptr(yty))
	&& (  (isconst(xty->type)    || !isconst(yty->type))
	   && (isvolatile(xty->type) || !isvolatile(yty->type))))
		return xty;

	if ((isptr(xty) && isptr(yty)
	    && eqtype(unqual(xty->type), unqual(yty->type), 1))
	&&  (  (isconst(xty->type)    || !isconst(yty->type))
	    && (isvolatile(xty->type) || !isvolatile(yty->type))))
		return xty;
	if (isptr(xty) && isptr(yty)
	&& (  (isconst(xty->type)    || !isconst(yty->type))
	   && (isvolatile(xty->type) || !isvolatile(yty->type)))) {
		Type lty = unqual(xty->type), rty = unqual(yty->type);
		if (isenum(lty) && rty == inttype
		||  isenum(rty) && lty == inttype) {
			if (Aflag >= 1)
				warning("assignment between `%t' and `%t' is compiler-dependent\n",
					xty, yty);
			return xty;
		}
	}
	return NULL;
}
Tree asgntree(op, l, r) int op; Tree l, r; {
	Type aty, ty;

	r = pointer(r);
	ty = assign(l->type, r);
	if (ty)
		r = cast(r, ty);
	else {
		typeerror(ASGN, l, r);
		if (r->type == voidtype)
			r = retype(r, inttype);
		ty = r->type;
	}
	if (l->op != FIELD)
		l = lvalue(l);
	aty = l->type;
	if (isptr(aty))
		aty = unqual(aty)->type;
	if ( isconst(aty)
	||  isstruct(aty) && unqual(aty)->u.sym->u.s.cfields)
		if (isaddrop(l->op)
		&& !l->u.sym->computed && !l->u.sym->generated)
			error("assignment to const identifier `%s'\n",
				l->u.sym->name);
		else
			error("assignment to const location\n");
	if (l->op == FIELD) {
		int n = 8*l->u.field->type->size - fieldsize(l->u.field);
		if (n > 0 && isunsigned(l->u.field->type))
			r = bittree(BAND, r,
				consttree(fieldmask(l->u.field), unsignedtype));
		else if (n > 0) {
			if (r->op == CNST+I)
				r = consttree(r->u.v.i<<n, inttype);
			else
				r = shtree(LSH, r, consttree(n, inttype));
			r = shtree(RSH, r, consttree(n, inttype));
		}
	}
	if (isstruct(ty) && isaddrop(l->op) && iscallb(r))
		return tree(RIGHT, ty,
			tree(CALL+B, ty, r->kids[0]->kids[0], l),
			idtree(l->u.sym));
	return tree(op + (isunsigned(ty) ? I : ttob(ty)),
		ty, l, r);
}
Tree condtree(e, l, r) Tree e, l, r; {
	Symbol t1;
	Type ty, xty = l->type, yty = r->type;
	Tree p;

	if (isarith(xty) && isarith(yty))
		ty = binary(xty, yty);
	else if (eqtype(xty, yty, 1))
		ty = unqual(xty);
	else if (isptr(xty)   && isnullptr(r))
		ty = xty;
	else if (isnullptr(l) && isptr(yty))
		ty = yty;
	else if (isptr(xty) && !isfunc(xty->type) && isvoidptr(yty)
	||       isptr(yty) && !isfunc(yty->type) && isvoidptr(xty))
		ty = voidptype;
	else if ((isptr(xty) && isptr(yty)
		 && eqtype(unqual(xty->type), unqual(yty->type), 1)))
		ty = xty;
	else {
		typeerror(COND, l, r);
		return consttree(0, inttype);
	}
	if (isptr(ty)) {
		ty = unqual(unqual(ty)->type);
		if (isptr(xty) && isconst(unqual(xty)->type)
		||  isptr(yty) && isconst(unqual(yty)->type))
			ty = qual(CONST, ty);
		if (isptr(xty) && isvolatile(unqual(xty)->type)
		||  isptr(yty) && isvolatile(unqual(yty)->type))
			ty = qual(VOLATILE, ty);
		ty = ptr(ty);
	}
	if (e->op == CNST+D || e->op == CNST+F) {
		e = cast(e, doubletype);
		return cast(e->u.v.d != 0.0 ? l : r, ty);
	}
	if (generic(e->op) == CNST) {
		e = cast(e, unsignedtype);
		return cast(e->u.v.u ? l : r, ty);
	}
	if (ty != voidtype && ty->size > 0) {
		t1 = temporary(REGISTER, unqual(ty), level);
		l = asgn(t1, l);
		r = asgn(t1, r);
	} else
		t1 = NULL;
	p = tree(COND, ty, cond(e),
		tree(RIGHT, ty, root(l), root(r)));
	p->u.sym = t1;
	return p;
}
/* addrof - address of p */
Tree addrof(p) Tree p; {
	Tree q = p;

	for (;;)
		switch (generic(q->op)) {
		case RIGHT:
			assert(q->kids[0] || q->kids[1]);
			q = q->kids[1] ? q->kids[1] : q->kids[0];
			continue;
		case ASGN:
			q = q->kids[1];
			continue;
		case COND: {
			Symbol t1 = q->u.sym;
			q->u.sym = 0;
			q = idtree(t1);
			/* fall thru */
			}
		case INDIR:
			if (p == q)
				return q->kids[0];
			q = q->kids[0];
			return tree(RIGHT, q->type, root(p), q);
		default:
			error("addressable object required\n");
			return value(p);
		}
}

/* andtree - construct tree for l [&& ||] r */
static Tree andtree(op, l, r) int op; Tree l, r; {
	if (!isscalar(l->type) || !isscalar(r->type))
		typeerror(op, l, r);
	return simplify(op, inttype, cond(l), cond(r));
}

/* asgn - generate tree for assignment of expr e to symbol p sans qualifiers */
Tree asgn(p, e) Symbol p; Tree e; {
	if (isarray(p->type))
		e = tree(ASGN+B, p->type, idtree(p),
			tree(INDIR+B, e->type, e, NULL));
	else {
		Type ty = p->type;
		p->type = unqual(p->type);
		if (isstruct(p->type) && p->type->u.sym->u.s.cfields) {
			p->type->u.sym->u.s.cfields = 0;
			e = asgntree(ASGN, idtree(p), e);
			p->type->u.sym->u.s.cfields = 1;
		} else
			e = asgntree(ASGN, idtree(p), e);
		p->type = ty;
	}
	return e;
}

/* bittree - construct tree for l [& | ^ %] r */
Tree bittree(op, l, r) int op; Tree l, r; {
	Type ty = inttype;

	if (isint(l->type) && isint(r->type)) {
 		ty = binary(l->type, r->type);
		l = cast(l, ty);
		r = cast(r, ty);		
		if (op != MOD) {
			l = cast(l, unsignedtype);
			r = cast(r, unsignedtype);
		}
	} else
		typeerror(op, l, r);
	if (op == MOD)
		return simplify(op, ty, l, r);
	return cast(simplify(op, unsignedtype, l, r), ty);
}

/* multree - construct tree for l [* /] r */
static Tree multree(op, l, r) int op; Tree l, r; {
	Type ty = inttype;

	if (isarith(l->type) && isarith(r->type)) {
		ty = binary(l->type, r->type);
		l = cast(l, ty);
		r = cast(r, ty);		
	} else
		typeerror(op, l, r);
	return simplify(op, ty, l, r);
}

/* shtree - construct tree for l [>> <<] r */
Tree shtree(op, l, r) int op; Tree l, r; {
	Type ty = inttype;

	if (isint(l->type) && isint(r->type)) {
		ty = promote(l->type);
		l = cast(l, ty);
		r = cast(r, inttype);
	} else
		typeerror(op, l, r);
	return simplify(op, ty, l, r);
}

/* subtree - construct tree for l - r */
static Tree subtree(op, l, r) int op; Tree l, r; {
	int n;
	Type ty = inttype;

	if (isarith(l->type) && isarith(r->type)) {
		ty = binary(l->type, r->type);
		l = cast(l, ty);
		r = cast(r, ty);		
	} else if (isptr(l->type) && !isfunc(l->type->type) && isint(r->type)) {
		ty = unqual(l->type);
		n = ty->type->size;
		if (n == 0)
			error("unknown size for type `%t'\n", ty->type);
		r = cast(r, promote(r->type));
		if (n > 1)
			r = multree(MUL, consttree(n, inttype), r);
		return simplify(SUB+P, ty, l, r);
	} else if (compatible(l->type, r->type)) {
		ty = unqual(l->type);
		n = ty->type->size;
		if (n == 0)
			error("unknown size for type `%t'\n", ty->type);
		l = simplify(SUB+U, unsignedtype, cast(l, unsignedtype),
			cast(r, unsignedtype));
		return simplify(DIV+I, inttype, cast(l, inttype), consttree(n, inttype));
	} else
		typeerror(op, l, r);
	return simplify(op, ty, l, r);
}

/* typeerror - issue "operands of op have illegal types `l' and `r'" */
void typeerror(op, l, r) int op; Tree l, r; {
	int i;
	static struct { int op; char *name; } ops[] = {
		ASGN, "=",	INDIR, "*",	NEG,  "-",
		ADD,  "+",	SUB,   "-",	LSH,  "<<",
		MOD,  "%",	RSH,   ">>",	BAND, "&",
		BCOM, "~",	BOR,   "|",	BXOR, "^",
		DIV,  "/",	MUL,   "*",	EQ,   "==",
		GE,   ">=",	GT,    ">",	LE,   "<=",
		LT,   "<",	NE,    "!=",	AND,  "&&",
		NOT,  "!",	OR,    "||",	COND, "?:",
		0, 0
	};

	op = generic(op);
	for (i = 0; ops[i].op; i++)
		if (op == ops[i].op)
			break;
	assert(ops[i].name);
	if (r)
		error("operands of %s have illegal types `%t' and `%t'\n",
			ops[i].name, l->type, r->type);
	else
		error("operand of unary %s has illegal type `%t'\n", ops[i].name,
			l->type);
}
