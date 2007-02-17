#include "c.h"

static char prec[] = {
#define xx(a,b,c,d,e,f,g) c,
#define yy(a,b,c,d,e,f,g) c,
#include "token.h"
};
static int oper[] = {
#define xx(a,b,c,d,e,f,g) d,
#define yy(a,b,c,d,e,f,g) d,
#include "token.h"
};
float refinc = 1.0;
static Tree expr2 ARGS((void));
static Tree expr3 ARGS((int));
static Tree nullcheck ARGS((Tree));
static Tree postfix ARGS((Tree));
static Tree unary ARGS((void));
static Tree primary ARGS((void));
static Type super ARGS((Type ty));

Tree expr(tok) int tok; {
	static char stop[] = { IF, ID, '}', 0 };
	Tree p = expr1(0);

	while (t == ',') {
		Tree q;
		t = gettok();
		q = pointer(expr1(0));
		p = tree(RIGHT, q->type, root(value(p)), q);
	}
	if (tok)	
		test(tok, stop);
	return p;
}
Tree expr0(tok) int tok; {
	return root(expr(tok));
}
Tree expr1(tok) int tok; {
	static char stop[] = { IF, ID, 0 };
	Tree p = expr2();

	if (t == '='
	|| (prec[t] >=  6 && prec[t] <=  8)
	|| (prec[t] >= 11 && prec[t] <= 13)) {
		int op = t;
		t = gettok();
		if (oper[op] == ASGN)
			p = asgntree(ASGN, p, value(expr1(0)));
		else
			{
				expect('=');
				p = incr(op, p, expr1(0));
			}
	}
	if (tok)	
		test(tok, stop);
	return p;
}
Tree incr(op, v, e) int op; Tree v, e; {
	return asgntree(ASGN, v, (*optree[op])(oper[op], v, e));
}
static Tree expr2() {
	Tree p = expr3(4);

	if (t == '?') {
		Tree l, r;
		Coordinate pts[2];
		if (Aflag > 1 && isfunc(p->type))
			warning("%s used in a conditional expression\n",
				funcname(p));
		p = pointer(p);
		t = gettok();
		pts[0] = src;
		l = pointer(expr(':'));
		pts[1] = src;
		r = pointer(expr2());
		p = condtree(p, l, r);
		if (events.points)
			if (p->op == COND) {
				Symbol t1 = p->u.sym;
				assert(p->kids[1]);
				assert(p->kids[1]->op == RIGHT);
				apply(events.points, &pts[0], &p->kids[1]->kids[0]);
				apply(events.points, &pts[1], &p->kids[1]->kids[1]);
				p = tree(COND, p->type, p->kids[0],
					tree(RIGHT, p->type,
						p->kids[1]->kids[0],
						p->kids[1]->kids[1]));
				p->u.sym = t1;
			}
	}
	return p;
}
Tree value(p) Tree p; {
	int op = generic(rightkid(p)->op);

	if (op==AND || op==OR || op==NOT || op==EQ || op==NE
	||  op== LE || op==LT || op== GE || op==GT)
		p = condtree(p, consttree(1, inttype),
			consttree(0, inttype));
	return p;
}
static Tree expr3(k) int k; {
	int k1;
	Tree p = unary();

	for (k1 = prec[t]; k1 >= k; k1--)
		while (prec[t] == k1 && *cp != '=') {
			Tree r;
			Coordinate pt;
			int op = t;
			t = gettok();
			pt = src;
			p = pointer(p);
			if (op == ANDAND || op == OROR) {
				r = pointer(expr3(k1));
				if (events.points)
					apply(events.points, &pt, &r);
			} else
				r = pointer(expr3(k1 + 1));
			p = (*optree[op])(oper[op], p, r); 
		}
	return p;
}
static Tree unary() {
	Tree p;

	switch (t) {
	case '*':    t = gettok(); p = unary(); p = pointer(p);
						  if (isptr(p->type)
						  && (isfunc(p->type->type) || isarray(p->type->type)))
						  	p = retype(p, p->type->type);
						  else {
						  	if (YYnull)
						  		p = nullcheck(p);
						  	p = rvalue(p);
						  } break;
	case '&':    t = gettok(); p = unary(); if (isarray(p->type) || isfunc(p->type))
						  	p = retype(p, ptr(p->type));
						  else
						  	p = lvalue(p);
						  if (isaddrop(p->op) && p->u.sym->sclass == REGISTER)
						  	error("invalid operand of unary &; `%s' is declared register\n", p->u.sym->name);

						  else if (isaddrop(p->op))
						  	p->u.sym->addressed = 1;
 break;
	case '+':    t = gettok(); p = unary(); p = pointer(p);
						  if (isarith(p->type))
						  	p = cast(p, promote(p->type));
						  else
						  	typeerror(ADD, p, NULL);  break;
	case '-':    t = gettok(); p = unary(); p = pointer(p);
						  if (isarith(p->type)) {
						  	p = cast(p, promote(p->type));
						  	if (isunsigned(p->type)) {
						  		warning("unsigned operand of unary -\n");
						  		p = simplify(NEG, inttype, cast(p, inttype), NULL);
						  		p = cast(p, unsignedtype);
						  	} else
						  		p = simplify(NEG, p->type, p, NULL);
						  } else
						  	typeerror(SUB, p, NULL); break;
	case '~':    t = gettok(); p = unary(); p = pointer(p);
						  if (isint(p->type)) {
						  	Type ty = promote(p->type);
						  	p = simplify(BCOM, ty, cast(p, ty), NULL);
						  } else
						  	typeerror(BCOM, p, NULL);  break;
	case '!':    t = gettok(); p = unary(); p = pointer(p);
						  if (isscalar(p->type))
						  	p = simplify(NOT, inttype, cond(p), NULL);
						  else
						  	typeerror(NOT, p, NULL); break;
	case INCR:   t = gettok(); p = unary(); p = incr(INCR, pointer(p), consttree(1, inttype)); break;
	case DECR:   t = gettok(); p = unary(); p = incr(DECR, pointer(p), consttree(1, inttype)); break;
	case SIZEOF: t = gettok(); { Type ty;
				     p = NULL;
				     if (t == '(') {
				     	t = gettok();
				     	if (istypename(t, tsym)) {
				     		ty = typename();
				     		expect(')');
				     	} else {
				     		p = postfix(expr(')'));
				     		ty = p->type;
				     	}
				     } else {
				     	p = unary();
				     	ty = p->type;
				     }
				     assert(ty);
				     if (isfunc(ty) || ty->size == 0)
				     	error("invalid type argument `%t' to `sizeof'\n", ty);
				     else if (p && rightkid(p)->op == FIELD)
				     	error("`sizeof' applied to a bit field\n");
				     p = consttree(ty->size, unsignedtype); } break;
	case '(':
		t = gettok();
		if (istypename(t, tsym)) {
			Type ty, ty1 = typename(), pty;
			expect(')');
			ty = unqual(ty1);
			if (isenum(ty)) {
				Type ty2 = ty->type;
				if (isconst(ty1))
					ty2 = qual(CONST, ty2);
				if (isvolatile(ty1))
					ty2 = qual(VOLATILE, ty2);
				ty1 = ty2;
				ty = ty->type;
			}
			p = pointer(unary());
			pty = p->type;
			if (isenum(pty))
				pty = pty->type;
			if (isarith(pty) && isarith(ty)
			||  isptr(pty)   && isptr(ty))
				p = cast(p, ty);
			else if (isptr(pty) && isint(ty)
			||       isint(pty) && isptr(ty)) {
				if (Aflag >= 1 && ty->size < pty->size)
					warning("conversion from `%t' to `%t' is compiler dependent\n", p->type, ty);

				p = cast(p, ty);
			} else if (ty != voidtype) {
				error("cast from `%t' to `%t' is illegal\n",
					p->type, ty1);
				ty1 = inttype;
			}
			p = retype(p, ty1);
			if (generic(p->op) == INDIR)
				p = tree(RIGHT, ty, NULL, p);
		} else
			p = postfix(expr(')'));
		break;
	default:
		p = postfix(primary());
	}
	return p;
}

static Tree postfix(p) Tree p; {
	for (;;)
		switch (t) {
		case INCR:  p = tree(RIGHT, p->type,
			    	tree(RIGHT, p->type,
			    		p,
			    		incr(t, p, consttree(1, inttype))),
			    	p);
			    t = gettok(); break;
		case DECR:  p = tree(RIGHT, p->type,
			    	tree(RIGHT, p->type,
			    		p,
			    		incr(t, p, consttree(1, inttype))),
			    	p);
			    t = gettok(); break;
		case '[':   {
			    	Tree q;
			    	t = gettok();
			    	q = expr(']');
			    	if (YYnull)
			    		if (isptr(p->type))
			    			p = nullcheck(p);
			    		else if (isptr(q->type))
			    			q = nullcheck(q);
			    	p = (*optree['+'])(ADD, pointer(p), pointer(q));
			    	if (isptr(p->type) && isarray(p->type->type))
			    		p = retype(p, p->type->type);
			    	else
			    		p = rvalue(p);
			    } break;
		case '(':   {
			    	Type ty;
			    	Coordinate pt;
			    	p = pointer(p);
			    	if (isptr(p->type) && isfunc(p->type->type))
			    		ty = p->type->type;
			    	else {
			    		error("found `%t' expected a function\n", p->type);
			    		ty = func(voidtype, NULL, 1);
			    		p = retype(p, ptr(ty));
			    	}
			    	pt = src;
			    	t = gettok();
			    	p = call(p, ty, pt);
			    } break;
		case '.':   t = gettok();
			    if (t == ID) {
			    	if (isstruct(p->type)) {
			    		Tree q = addrof(p);
			    		p = field(q, token);
			    		q = rightkid(q);
			    		if (isaddrop(q->op) && q->u.sym->temporary) {
			    			p = tree(RIGHT, p->type, p, NULL);
			    			p->u.sym = q->u.sym;
			    		}
			    	} else
			    		error("left operand of . has incompatible type `%t'\n",
			    			p->type);
			    	t = gettok();
			    } else
			    	error("field name expected\n"); break;
		case DEREF: t = gettok();
			    p = pointer(p);
			    if (t == ID) {
			    	if (isptr(p->type) && isstruct(p->type->type)) {
			    		if (YYnull)
			    			p = nullcheck(p);
			    		p = field(p, token);
			    	} else
			    		error("left operand of -> has incompatible type `%t'\n", p->type);

			    	t = gettok();
			    } else
			    	error("field name expected\n"); break;
		default:
			return p;
		}
}
static Tree primary() {
	Tree p;

	assert(t != '(');
	switch (t) {
	case ICON:
	case FCON: p = tree(CNST + ttob(tsym->type), tsym->type, NULL, NULL);
		   p->u.v = tsym->u.c.v;
 break;
	case SCON: tsym->u.c.v.p = stringn(tsym->u.c.v.p, tsym->type->size);
		   tsym = constant(tsym->type, tsym->u.c.v); 
		   if (tsym->u.c.loc == NULL)
		   	tsym->u.c.loc = genident(STATIC, tsym->type, GLOBAL);
		   p = idtree(tsym->u.c.loc); break;
	case ID:   if (tsym == NULL)
		   	{
				Symbol q = install(token, &identifiers, level, level < LOCAL ? PERM : FUNC);
				q->src = src;
				t = gettok();
				if (t == '(') {
					Symbol r;
					q->sclass = EXTERN;
					q->type = func(inttype, NULL, 1);
					if (Aflag >= 1)
						warning("missing prototype\n");
					(*IR->defsymbol)(q);
				 	if ((r = lookup(q->name, externals)) != NULL) {
						q->defined = r->defined;
						q->temporary = r->temporary;
						q->generated = r->generated;
						q->computed = r->computed;
						q->addressed = r->addressed;
					} else {
						r = install(q->name, &externals, GLOBAL, PERM);
						r->src = q->src;
						r->type = q->type;
						r->sclass = EXTERN;
					}
				} else {
					error("undeclared identifier `%s'\n", q->name);
					q->sclass = AUTO;
					q->type = inttype;
					if (q->scope == GLOBAL)
						(*IR->defsymbol)(q);
					else
						addlocal(q);
				}
				if (xref)
					use(q, src);
				return idtree(q);
			}
		   if (xref)
		   	use(tsym, src);
		   if (tsym->sclass == ENUM)
		   	p = consttree(tsym->u.value, inttype);
		   else {
		   	if (tsym->sclass == TYPEDEF)
		   		error("illegal use of type name `%s'\n", tsym->name);
		   	p = idtree(tsym);
		   } break;
	default:
		error("illegal expression\n");
		p = consttree(0, inttype);
	}
	t = gettok();
	return p;
}
Tree idtree(p) Symbol p; {
	int op;
	Tree e;
	Type ty = p->type ? unqual(p->type) : voidtype;

	p->ref += refinc;
	if (p->scope  == GLOBAL
	||  p->sclass == STATIC || p->sclass == EXTERN)
		op = ADDRG+P;
	else if (p->scope == PARAM) {
		op = ADDRF+P;
		if (isstruct(p->type) && !IR->wants_argb)
			{
				e = tree(op, ptr(ptr(p->type)), NULL, NULL);
				e->u.sym = p;
				return rvalue(rvalue(e));
			}
	} else
		op = ADDRL+P;
	if (isarray(ty) || isfunc(ty)) {
		e = tree(op, p->type, NULL, NULL);
		e->u.sym = p;
	} else {
		e = tree(op, ptr(p->type), NULL, NULL);
		e->u.sym = p;
		e = rvalue(e);
	}
	return e;
}

Tree rvalue(p) Tree p; {
	Type ty = deref(p->type);

	ty = unqual(ty);
	if (YYnull && !isaddrop(p->op))		/* omit */
		p = nullcheck(p);		/* omit */
	return tree(INDIR + (isunsigned(ty) ? I : ttob(ty)),
		ty, p, NULL);
}
Tree lvalue(p) Tree p; {
	if (generic(p->op) != INDIR) {
		error("lvalue required\n");
		return value(p);
	} else if (unqual(p->type) == voidtype)
		warning("`%t' used as an lvalue\n", p->type);
	return p->kids[0];
}
Tree retype(p, ty) Tree p; Type ty;{
	Tree q;

	if (p->type == ty)
		return p;
	q = tree(p->op, ty, p->kids[0], p->kids[1]);
	q->u = p->u;
	return q;
}
Tree rightkid(p) Tree p; {
	while (p && p->op == RIGHT)
		if (p->kids[1])
			p = p->kids[1];
		else if (p->kids[0])
			p = p->kids[0];
		else
			assert(0);
	assert(p);
	return p;
}
int hascall(p) Tree p; {
	if (p == 0)
		return 0;
	if (generic(p->op) == CALL || (IR->mulops_calls &&
	  (p->op == DIV+I || p->op == MOD+I || p->op == MUL+I
	|| p->op == DIV+U || p->op == MOD+U || p->op == MUL+U)))
		return 1;
	return hascall(p->kids[0]) || hascall(p->kids[1]);
}
Type binary(xty, yty) Type xty, yty; {
	if (isdouble(xty) || isdouble(yty))
		return doubletype;
	if (xty == floattype || yty == floattype)
		return floattype;
	if (isunsigned(xty) || isunsigned(yty))
		return unsignedtype;
	return inttype;
}
Tree pointer(p) Tree p; {
	if (isarray(p->type))
		/* assert(p->op != RIGHT || p->u.sym == NULL), */
		p = retype(p, atop(p->type));
	else if (isfunc(p->type))
		p = retype(p, ptr(p->type));
	return p;
}
Tree cond(p) Tree p; {
	int op = generic(rightkid(p)->op);

	if (op == AND || op == OR || op == NOT
	||  op == EQ  || op == NE
	||  op == LE  || op == LT || op == GE || op == GT)
		return p;
	p = pointer(p);
	p = cast(p, promote(p->type));
	return (*optree[NEQ])(NE, p, consttree(0, inttype));
}
Tree cast(p, type) Tree p; Type type; {
	Type pty, ty = unqual(type);

	p = value(p);
	if (p->type == type)
		return p;
	pty = unqual(p->type);
	if (pty == ty)
		return retype(p, type);
	switch (pty->op) {
	case CHAR:    p = simplify(CVC, super(pty), p, NULL); break;
	case SHORT:   p = simplify(CVS, super(pty), p, NULL); break;
	case FLOAT:   p = simplify(CVF, doubletype, p, NULL); break;
	case INT:     p = retype(p, inttype);                 break;
	case DOUBLE:  p = retype(p, doubletype);              break;
	case ENUM:    p = retype(p, inttype);                 break;
	case UNSIGNED:p = retype(p, unsignedtype);            break;
	case POINTER:
		if (isptr(ty)) {
			if (isfunc(pty->type) && !isfunc(ty->type)
			|| !isfunc(pty->type) &&  isfunc(ty->type))
				warning("conversion from `%t' to `%t' is compiler dependent\n", p->type, ty);

			return retype(p, type);
		} else
			p = simplify(CVP, unsignedtype, p, NULL);
		break;
	default: assert(0);
	}
	{
		Type sty = super(ty);
		pty = p->type;
		if (pty != sty)
			if (pty == inttype)
				p = simplify(CVI, sty, p, NULL);
			else if (pty == doubletype)
				if (sty == unsignedtype) {
					Tree c = tree(CNST+D, doubletype, NULL, NULL);
					c->u.v.d = (double)INT_MAX + 1;
					p = condtree(
						simplify(GE, doubletype, p, c),
						(*optree['+'])(ADD,
							cast(cast(simplify(SUB, doubletype, p, c), inttype), unsignedtype),
							consttree((unsigned)INT_MAX + 1, unsignedtype)),
						simplify(CVD, inttype, p, NULL));
				} else
					p = simplify(CVD, sty, p, NULL);
			else if (pty == unsignedtype)
				if (sty == doubletype) {
					Tree two = tree(CNST+D, doubletype, NULL, NULL);
					two->u.v.d = 2.;
					p = (*optree['+'])(ADD,
						(*optree['*'])(MUL,
							two,
							simplify(CVU, inttype,
								simplify(RSH, unsignedtype,
									p, consttree(1, inttype)), NULL)),
						simplify(CVU, inttype,
							simplify(BAND, unsignedtype,
								p, consttree(1, unsignedtype)), NULL));
				} else
					p = simplify(CVU, sty, p, NULL);
			else assert(0);
	}
	if (ty == signedchar || ty == chartype || ty == shorttype)
		p = simplify(CVI, type, p, NULL);
	else if (isptr(ty)
	|| ty == unsignedchar || ty == unsignedshort)
		p = simplify(CVU, type, p, NULL);
	else if (ty == floattype)
		p = simplify(CVD, type, p, NULL);
	else
		p = retype(p, type);
	return p;
}
static Type super(ty) Type ty; {
	if (ty == signedchar || ty == chartype || isenum(ty)
	||  ty == shorttype  || ty == inttype  || ty == longtype)
		return inttype;
	if (isptr(ty)
	|| ty == unsignedtype  || ty == unsignedchar
	|| ty == unsignedshort || ty == unsignedlong)
		return unsignedtype;
	if (ty == floattype || ty == doubletype || ty == longdouble)
		return doubletype;
	assert(0);
	return NULL;
}
Tree field(p, name) Tree p; char *name; {
	Field q;
	Type ty1, ty = p->type;

	if (isptr(ty))
		ty = deref(ty);
	ty1 = ty;
	ty = unqual(ty);
	if ((q = fieldref(name, ty)) != NULL) {
		if (isarray(q->type)) {
			ty = q->type->type;
			if (isconst(ty1) && !isconst(ty))
				ty = qual(CONST, ty);
			if (isvolatile(ty1) && !isvolatile(ty))
				ty = qual(VOLATILE, ty);
			ty = array(ty, q->type->size/ty->size, q->type->align);
		} else {
			ty = q->type;
			if (isconst(ty1) && !isconst(ty))
				ty = qual(CONST, ty);
			if (isvolatile(ty1) && !isvolatile(ty))
				ty = qual(VOLATILE, ty);
			ty = ptr(ty);
		}
		if (YYcheck && !isaddrop(p->op) && q->offset > 0)	/* omit */
			p = nullcall(ty, YYcheck, p, consttree(q->offset, inttype));	/* omit */
		else					/* omit */
		p = simplify(ADD+P, ty, p, consttree(q->offset, inttype));

		if (q->lsb) {
			p = tree(FIELD, ty->type, rvalue(p), NULL);
			p->u.field = q;
		} else if (!isarray(q->type))
			p = rvalue(p);

	} else {
		error("unknown field `%s' of `%t'\n", name, ty);
		p = rvalue(retype(p, ptr(inttype)));
	}
	return p;
}
/* funcname - return name of function f or a function' */
char *funcname(f) Tree f; {
	if (isaddrop(f->op))
		return stringf("`%s'", f->u.sym->name);
	return "a function";
}
static Tree nullcheck(p) Tree p; {
	if (!needconst && YYnull && isptr(p->type)) {
		p = value(p);
		if (strcmp(YYnull->name, "_YYnull") == 0) {
			Symbol t1 = temporary(REGISTER, voidptype, level);
			p = tree(RIGHT, p->type,
				tree(OR, voidtype,
					cond(asgn(t1, cast(p, voidptype))),
					calltree(pointer(idtree(YYnull)), voidtype,
						tree(ARG+I, inttype,
							consttree(lineno, inttype), NULL), NULL)),
				idtree(t1));
		}
		else
			p = nullcall(p->type, YYnull, p, consttree(0, inttype));

	}
	return p;
}
Tree nullcall(pty, f, p, e) Type pty; Symbol f; Tree p, e; {
	Tree fp, r;
	Type ty;

	if (isarray(pty))
		return retype(nullcall(atop(pty), f, p, e), pty);
	ty = unqual(unqual(p->type)->type);
	if (file && *file)
		fp = idtree(mkstr(file)->u.c.loc);
	else
		fp = cast(consttree(0, inttype), voidptype);
	r = calltree(pointer(idtree(f)), pty,
		tree(	ARG+I, inttype, consttree(lineno, inttype), tree(
			ARG+P, ptr(chartype), fp, tree(
			ARG+I, unsignedtype, consttree(ty->align, unsignedtype), tree(
			ARG+I, unsignedtype, consttree(ty->size, unsignedtype), tree(
			ARG+I, inttype, e, tree(
			ARG+P, p->type, p, NULL) ))))),
		NULL);
	if (hascall(e))
		r = tree(RIGHT, r->type, e, r);
	if (hascall(p))
		r = tree(RIGHT, r->type, p, r);
	return r;
}
