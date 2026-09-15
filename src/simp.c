#include "c.h"

#define foldcnst(TYPE,VAR,OP,RTYPE) \
	if (l->op == CNST+TYPE && r->op == CNST+TYPE) {\
		p = tree(CNST+ttob(RTYPE), RTYPE, NULL, NULL);\
		p->u.v.VAR = l->u.v.VAR OP r->u.v.VAR;\
		return p; }
#define commute(L,R) \
	if (generic(R->op) == CNST && generic(L->op) != CNST) {\
		Tree t = L; L = R; R = t; }
#define xfoldcnst(TYPE,VAR,OP,RTYPE,FUNC,MIN,MAX,needconst)\
	if (l->op == CNST+TYPE && r->op == CNST+TYPE\
	&& FUNC((double)l->u.v.VAR,(double)r->u.v.VAR,\
		(double)MIN,(double)MAX, needconst)) {\
		p = tree(CNST+ttob(RTYPE), RTYPE, NULL, NULL);\
		p->u.v.VAR = l->u.v.VAR OP r->u.v.VAR;\
		return p; }
#define cvtcnst(FTYPE,TTYPE,EXPR) \
	if (l->op == CNST+FTYPE) {\
		p = tree(CNST+ttob(TTYPE), TTYPE, NULL, NULL);\
		EXPR;\
		return p; }
#define xcvtcnst(FTYPE,TTYPE,VAR,MIN,MAX,EXPR) \
	if (l->op == CNST+FTYPE) {\
		if (needconst && (VAR < MIN || VAR > MAX))\
			warning("overflow in constant expression\n");\
		if (needconst || VAR >= MIN && VAR <= MAX) {\
			p = tree(CNST+ttob(TTYPE), TTYPE, NULL, NULL);\
			EXPR;\
			return p; } }
#define identity(X,Y,TYPE,VAR,VAL) \
	if (X->op == CNST+TYPE && X->u.v.VAR == VAL)\
		return Y
#define zerofield(OP,TYPE,VAR) \
	if (l->op == FIELD\
	&&  r->op == CNST+TYPE && r->u.v.VAR == 0)\
		return eqtree(OP, bittree(BAND, l->kids[0],\
			consttree(\
				fieldmask(l->u.field)<<fieldright(l->u.field),\
				unsignedtype)), r);
#define cfoldcnst(TYPE,VAR,OP,RTYPE) \
	if (l->op == CNST+TYPE && r->op == CNST+TYPE) {\
		p = tree(CNST+ttob(RTYPE), RTYPE, NULL, NULL);\
		p->u.v.i = l->u.v.VAR OP r->u.v.VAR;\
		return p; }
#define foldaddp(L,R,RTYPE,VAR) \
	if (L->op == CNST+P && R->op == CNST+RTYPE) {\
		p = tree(CNST+P, ty, NULL, NULL);\
		p->u.v.p = (char *)L->u.v.p + R->u.v.VAR;\
		return p; }
#define ufoldcnst(TYPE,EXP) if (l->op == CNST+TYPE) return EXP
#define sfoldcnst(TYPE,VAR,OP,RTYPE) \
	if (l->op == CNST+TYPE && r->op == CNST+I \
	&& r->u.v.i >= 0 && r->u.v.i < 8*l->type->size) { \
		p = tree(CNST+ttob(RTYPE), RTYPE, NULL, NULL); \
		p->u.v.VAR = l->u.v.VAR OP r->u.v.i; return p; }
#define geu(L,R,V) \
	if (R->op == CNST+U && R->u.v.u == 0) { \
		warning("result of unsigned comparison is constant\n"); \
		return tree(RIGHT, inttype, root(L), consttree(V, inttype)); }
#define idempotent(OP) if (l->op == OP) return l->kids[0];
#define utod(x)	(2.*(int)((unsigned)(x)>>1)+(int)((x)&1))

int needconst;
static int add ARGS((double, double, double, double, int));
static Tree addrtree ARGS((Tree, int, Type));
static int div ARGS((double, double, double, double, int));
static int mul ARGS((double, double, double, double, int));
static int sub ARGS((double, double, double, double, int));
Tree constexpr(tok) int tok; {
	Tree p;

	needconst++;
	p = expr1(tok);
	needconst--;
	return p;
}

int intexpr(tok, n) int tok, n; {
	Tree p = constexpr(tok);

	needconst++;
	if (generic(p->op) == CNST && isint(p->type))
		n = cast(p, inttype)->u.v.i;
	else
		error("integer expression must be constant\n");
	needconst--;
	return n;
}
Tree simplify(op, ty, l, r) int op; Type ty; Tree l, r; {
	int n;
	Tree p;

	if (optype(op) == 0)
		op += ttob(ty);
	switch (op) {
		case ADD+U:
			foldcnst(U,u,+,unsignedtype);
			commute(r,l);
			identity(r,l,U,u,0);
			break;
		case ADD+I:
			xfoldcnst(I,i,+,inttype,add,INT_MIN,INT_MAX,needconst);
			commute(r,l);
			identity(r,l,I,i,0);
			break;
		case CVC+I:
			cvtcnst(C,inttype, p->u.v.i =
				(l->u.v.sc&0200 ? (~0<<8) : 0)|(l->u.v.sc&0377));
			break;
		case CVU+S:
			cvtcnst(U,unsignedshort,p->u.v.us = l->u.v.u); break;
		case CVP+U:
			cvtcnst(P,unsignedtype, p->u.v.u  = (unsigned long)l->u.v.p);
			break;
		case CVI+U:
			cvtcnst(I,unsignedtype, p->u.v.u  = l->u.v.i); break;

		case CVC+U:  cvtcnst(C, unsignedtype,p->u.v.u  = l->u.v.uc); break;
		case CVF+D:  cvtcnst(F,   doubletype,p->u.v.d  = l->u.v.f);  break;
		case CVI+D:  cvtcnst(I,   doubletype,p->u.v.d  = l->u.v.i);  break;
		case CVS+I:  cvtcnst(S,      inttype,p->u.v.i  = l->u.v.ss); break;
		case CVS+U:  cvtcnst(S, unsignedtype,p->u.v.u  = l->u.v.us); break;
		case CVU+C:  cvtcnst(U, unsignedchar,p->u.v.uc = l->u.v.u);  break;
		case CVU+D:  cvtcnst(U,   doubletype,p->u.v.d  = utod(l->u.v.u)); break;
		case CVU+I:
			if (needconst && l->u.v.u > INT_MAX)
				warning("overflow in constant expression\n");
			if (needconst || l->u.v.u <= INT_MAX)
				cvtcnst(U,   inttype,p->u.v.i  = l->u.v.u);
			break;
		case CVU+P:  cvtcnst(U,    voidptype,p->u.v.p  = (void *)l->u.v.u);  break;
		case CVI+C:
			xcvtcnst(I, chartype,l->u.v.i,SCHAR_MIN,SCHAR_MAX,
				p->u.v.sc = l->u.v.i); break;
		case CVD+F:
			xcvtcnst(D,floattype,l->u.v.d, -FLT_MAX,FLT_MAX,
				p->u.v.f  = l->u.v.d); break;
		case CVD+I:
			xcvtcnst(D,  inttype,l->u.v.d,  INT_MIN,INT_MAX,
				p->u.v.i  = l->u.v.d); break;
		case CVI+S:
			xcvtcnst(I,shorttype,l->u.v.i, SHRT_MIN,SHRT_MAX,
				p->u.v.ss = l->u.v.i); break;

		case BAND+U:
			foldcnst(U,u,&,unsignedtype);
			commute(r,l);
			identity(r,l,U,u,(~(unsigned)0));
			if (r->op == CNST+U && r->u.v.u == 0)
				return tree(RIGHT, unsignedtype, root(l),
					consttree(0, unsignedtype));
			break;

		case MUL+U:
			commute(l,r);
			if (l->op == CNST+U && (n = ispow2(l->u.v.u)) != 0)
				return simplify(LSH+U, unsignedtype, r,
					consttree(n, inttype));
			foldcnst(U,u,*,unsignedtype);
			identity(r,l,U,u,1);
			break;
		case NE+I:
			cfoldcnst(I,i,!=,inttype);
			commute(r,l);
			zerofield(NE,I,i);
			break;

		case EQ+I:
			cfoldcnst(I,i,==,inttype);
			commute(r,l);
			zerofield(EQ,I,i);
			break;
		case ADD+P:
			foldaddp(l,r,I,i);
			foldaddp(l,r,U,u);
			foldaddp(r,l,I,i);
			foldaddp(r,l,U,u);
			commute(r,l);
			identity(r,retype(l,ty),I,i,0);
			identity(r,retype(l,ty),U,u,0);
			if (isaddrop(l->op)
			&& (r->op == CNST+I || r->op == CNST+U))
				return addrtree(l, cast(r, inttype)->u.v.i, ty);
			if (l->op == ADD+P && isaddrop(l->kids[1]->op)
			&& (r->op == CNST+I || r->op == CNST+U))
				return simplify(ADD+P, ty, l->kids[0],
					addrtree(l->kids[1], cast(r, inttype)->u.v.i, ty));
			if ((l->op == ADD+I || l->op == SUB+I)
			&& l->kids[1]->op == CNST+I && isaddrop(r->op))
				return simplify(ADD+P, ty, l->kids[0],
					simplify(generic(l->op)+P, ty, r, l->kids[1]));
			if (l->op == ADD+P && generic(l->kids[1]->op) == CNST
			&& generic(r->op) == CNST)
				return simplify(ADD+P, ty, l->kids[0],
					(*optree['+'])(ADD, l->kids[1], r));
			if (l->op == ADD+I && generic(l->kids[1]->op) == CNST
			&&  r->op == ADD+P && generic(r->kids[1]->op) == CNST)
				return simplify(ADD+P, ty, l->kids[0],
					simplify(ADD+P, ty, r->kids[0],
					(*optree['+'])(ADD, l->kids[1], r->kids[1])));
			if (l->op == RIGHT && l->kids[1])
				return tree(RIGHT, ty, l->kids[0],
					simplify(ADD+P, ty, l->kids[1], r));
			else if (l->op == RIGHT && l->kids[0])
				return tree(RIGHT, ty,
					simplify(ADD+P, ty, l->kids[0], r), NULL);
			break;

		case ADD+D:
			xfoldcnst(D,d,+,doubletype,add,-DBL_MAX,DBL_MAX,0);
			commute(r,l);
			break;
		case ADD+F:
			xfoldcnst(F,f,+,floattype,add,-FLT_MAX,FLT_MAX,0);
			commute(r,l);
			break;
		case AND+I:
			op = AND;
			ufoldcnst(I,l->u.v.i ? cond(r) : l);	/* 0&&r => 0, 1&&r => r */
			break;
		case OR+I:
			op = OR;
			/* 0||r => r, 1||r => 1 */
			ufoldcnst(I,l->u.v.i ? consttree(1, inttype) : cond(r));
			break;
		case BCOM+I:
			ufoldcnst(I,consttree(~l->u.v.i, inttype));
			idempotent(BCOM+U);
			op = BCOM+U;
			break;
		case BCOM+U:
			ufoldcnst(U,consttree(~l->u.v.u, unsignedtype));
			idempotent(BCOM+U);
			break;
		case BOR+U:
			foldcnst(U,u,|,unsignedtype);
			commute(r,l);
			identity(r,l,U,u,0);
			break;
		case BXOR+U:
			foldcnst(U,u,^,unsignedtype);
			commute(r,l);
			identity(r,l,U,u,0);
			break;
		case DIV+D:
			xfoldcnst(D,d,/,doubletype,div,-DBL_MAX,DBL_MAX,0);
			break;
		case DIV+F:
			xfoldcnst(F,f,/,floattype,div,-FLT_MAX,FLT_MAX,0);
			break;
		case DIV+I:
			identity(r,l,I,i,1);
#ifdef mips
			if (l->op == CNST+I && r->op == CNST+I && r->u.v.i == -1
			&& !div((double)l->u.v.i, (double)r->u.v.i, (double)INT_MIN, (double)INT_MAX, 0))
				break;
#endif
			xfoldcnst(I,i,/,inttype,div,INT_MIN,INT_MAX,needconst);
			break;
		case DIV+U:		
			identity(r,l,U,u,1);
			if (r->op == CNST+U && r->u.v.u == 0)
				break;
			if (r->op == CNST+U && (n = ispow2(r->u.v.u)) != 0)
				return simplify(RSH+U, unsignedtype, l, consttree(n, inttype));
			foldcnst(U,u,/,unsignedtype);
			break;
		case EQ+D:
			cfoldcnst(D,d,==,inttype);
			commute(r,l);
			break;
		case EQ+F:
			cfoldcnst(F,f,==,inttype);
			commute(r,l);
			break;
		case EQ+U:
			cfoldcnst(U,u,==,inttype);
			commute(r,l);
			zerofield(EQ,U,u);
			op = EQ+I;
			break;
		case GE+D: cfoldcnst(D,d,>=,inttype); break;
		case GE+F: cfoldcnst(F,f,>=,inttype); break;
		case GE+I: cfoldcnst(I,i,>=,inttype); break;
		case GE+U:
			geu(l,r,1);	/* l >= 0 => (l,1) */
			cfoldcnst(U,u,>=,inttype);
			if (l->op == CNST+U && l->u.v.u == 0)	/* 0 >= r => r == o */
				return tree(EQ+I, ty, r, l);
			break;
		case GT+D: cfoldcnst(D,d, >,inttype); break;
		case GT+F: cfoldcnst(F,f, >,inttype); break;
		case GT+I: cfoldcnst(I,i, >,inttype); break;
		case GT+U:
			geu(r,l,0);	/* 0 > r => (r,0) */
			cfoldcnst(U,u, >,inttype);
			if (r->op == CNST+U && r->u.v.u == 0)	/* l > 0 => l != 0 */
				return tree(NE+I, ty, l, r);
			break;
		case LE+D: cfoldcnst(D,d,<=,inttype); break;
		case LE+F: cfoldcnst(F,f,<=,inttype); break;
		case LE+I: cfoldcnst(I,i,<=,inttype); break;
		case LE+U:
			geu(r,l,1);	/* 0 <= r => (r,1) */
			cfoldcnst(U,u,<=,inttype);
			if (r->op == CNST+U && r->u.v.u == 0)	/* l <= 0 => l == 0 */
				return tree(EQ+I, ty, l, r);
			break;
		case LSH+I:
			identity(r,l,I,i,0);
			if (l->op == CNST+I && r->op == CNST+I
			&& r->u.v.i >= 0 && r->u.v.i < 8*l->type->size
			&& mul((double)l->u.v.i, (double)(1<<r->u.v.i), (double)INT_MIN, (double)INT_MAX, needconst))
				return consttree(l->u.v.i<<r->u.v.i, inttype);
			if (r->op == CNST+I && (r->u.v.i >= 8*l->type->size || r->u.v.i < 0)) {
				warning("shift by %d is undefined\n", r->u.v.i);
				break;
			}
			break;
		case LSH+U:
			identity(r,l,I,i,0);
			sfoldcnst(U,u,<<,unsignedtype);
			if (r->op == CNST+I && (r->u.v.i >= 8*l->type->size || r->u.v.i < 0)) {
				warning("shift by %d is undefined\n", r->u.v.i);
				break;
			}
			break;
		case LT+D: cfoldcnst(D,d, <,inttype); break;
		case LT+F: cfoldcnst(F,f, <,inttype); break;
		case LT+I: cfoldcnst(I,i, <,inttype); break;
		case LT+U:
			geu(l,r,0);	/* l < 0 => (l,0) */
			cfoldcnst(U,u, <,inttype);
			if (l->op == CNST+U && l->u.v.u == 0)	/* 0 < r => r != 0 */
				return tree(NE+I, ty, r, l);
			break;
		case MOD+I:
			if (r->op == CNST+I && r->u.v.i == 1)	/* l%1 => (l,0) */
				return tree(RIGHT, inttype, root(l), consttree(0, inttype));
			if (r->op == CNST+I && r->u.v.i == 0)
				break;
#ifdef mips
			if (l->op == CNST+I && r->op == CNST+I && r->u.v.i == -1
			&& !div((double)l->u.v.i, (double)r->u.v.i, (double)INT_MIN, (double)INT_MAX, 0))
				break;
#endif
			xfoldcnst(I,i,%,inttype,div,INT_MIN,INT_MAX,needconst);
			break;
		case MOD+U:		
			if (r->op == CNST+U && ispow2(r->u.v.u))	/* l%2^n => l&(2^n-1) */
				return bittree(BAND, l,
					consttree(r->u.v.u - 1, unsignedtype));
			if (r->op == CNST+U && r->u.v.u == 0)
				break;
			foldcnst(U,u,%,unsignedtype);
			break;
		case MUL+D:
			xfoldcnst(D,d,*,doubletype,mul,-DBL_MAX,DBL_MAX,0);
			commute(l,r);
			break;
		case MUL+F:
			xfoldcnst(F,f,*,floattype,mul,-FLT_MAX,FLT_MAX,0);
			commute(l,r);
			break;
		case MUL+I:
			commute(l,r);
			if (l->op == CNST+I && r->op == ADD+I && r->kids[1]->op == CNST+I)
				/* c1*(x + c2) => c1*x + c1*c2 */
				return simplify(ADD+I, inttype, simplify(MUL+I, inttype, l, r->kids[0]),
					simplify(MUL+I, inttype, l, r->kids[1]));
			if (l->op == CNST+I && r->op == SUB+I && r->kids[1]->op == CNST+I)
				/* c1*(x - c2) => c1*x - c1*c2 */
				return simplify(SUB+I, inttype, simplify(MUL+I, inttype, l, r->kids[0]),
					simplify(MUL+I, inttype, l, r->kids[1]));
			if (l->op == CNST+I && l->u.v.i > 0 && (n = ispow2(l->u.v.i)) != 0)
				/* 2^n * r => r<<n */
				return simplify(LSH+I, inttype, r, consttree(n, inttype));
			xfoldcnst(I,i,*,inttype,mul,INT_MIN,INT_MAX,needconst);
			identity(r,l,I,i,1);
			break;
		case NE+D:
			cfoldcnst(D,d,!=,inttype);
			commute(r,l);
			break;
		case NE+F:
			cfoldcnst(F,f,!=,inttype);
			commute(r,l);
			break;
		case NE+U:
			cfoldcnst(U,u,!=,inttype);
			commute(r,l);
			zerofield(NE,U,u);
			op = NE+I;
			break;
		case NEG+D:
			ufoldcnst(D,(p = tree(CNST+D, doubletype, NULL, NULL), p->u.v.d = -l->u.v.d, p));
			idempotent(NEG+D);
			break;
		case NEG+F:
			ufoldcnst(F,(p = tree(CNST+F, floattype, NULL, NULL), p->u.v.f = -l->u.v.f, p));
			idempotent(NEG+F);
			break;
		case NEG+I:
			if (l->op == CNST+I) {
				if (needconst && l->u.v.i == INT_MIN)
					warning("overflow in constant expression\n");
				if (needconst || l->u.v.i != INT_MIN)
					return consttree(-l->u.v.i, inttype);
			}
			idempotent(NEG+I);
			break;
		case NOT+I:
			op = NOT;
			ufoldcnst(I,consttree(!l->u.v.i, inttype));
			break;
		case RSH+I:
			identity(r,l,I,i,0);
			if (l->op == CNST+I && r->op == CNST+I
			&& r->u.v.i >= 0 && r->u.v.i < 8*l->type->size) {
				int n = l->u.v.i>>r->u.v.i;
				if (l->u.v.i < 0)
					n |= ~0<<(8*l->type->size - r->u.v.i);
				return consttree(n, inttype);
			}
			if (r->op == CNST+I && (r->u.v.i >= 8*l->type->size || r->u.v.i < 0)) {
				warning("shift by %d is undefined\n", r->u.v.i);
				break;
			}
			break;
		case RSH+U:
			identity(r,l,I,i,0);
			sfoldcnst(U,u,>>,unsignedtype);
			if (r->op == CNST+I && (r->u.v.i >= 8*l->type->size || r->u.v.i < 0)) {
				warning("shift by %d is undefined\n", r->u.v.i);
				break;
			}
			break;
		case SUB+D:
			xfoldcnst(D,d,-,doubletype,sub,-DBL_MAX,DBL_MAX,0);
			break;
		case SUB+F:
			xfoldcnst(F,f,-,floattype,sub,-FLT_MAX,FLT_MAX,0);
			break;
		case SUB+I:
			xfoldcnst(I,i,-,inttype,sub,INT_MIN,INT_MAX,needconst);
			identity(r,l,I,i,0);
			break;
		case SUB+U:
			foldcnst(U,u,-,unsignedtype);
			identity(r,l,U,u,0);
			break;
		case SUB+P:
			if (l->op == CNST+P && r->op == CNST+P)
				return consttree((char *)l->u.v.p - (char *)r->u.v.p, inttype);
			if (r->op == CNST+I || r->op == CNST+U)
				return simplify(ADD+P, ty, l,
					consttree(r->op == CNST+I ? -r->u.v.i : -(int)r->u.v.u, inttype));
			if (isaddrop(l->op) && r->op == ADD+I && r->kids[1]->op == CNST+I)
				/* l - (x + c) => l-c - x */
				return simplify(SUB+P, ty,
					simplify(SUB+P, ty, l, r->kids[1]), r->kids[0]);
			break;
		default:assert(0);
	}
	return tree(op, ty, l, r);
}
static int add(x, y, min, max, needconst)
double x, y, min, max; int needconst; {
	int cond = x == 0 || y == 0
	|| x < 0 && y < 0 && x >= min - y
	|| x < 0 && y > 0
	|| x > 0 && y < 0
	|| x > 0 && y > 0 && x <= max - y;
	if (!cond && needconst) {
		warning("overflow in constant expression\n");
		cond = 1;
	}
	return cond;
}
static Tree addrtree(e, n, ty) Tree e; int n; Type ty; {
	Symbol p = e->u.sym, q;

	NEW0(q, FUNC);
	q->name = stringd(genlabel(1));
	q->sclass = p->sclass;
	q->scope = p->scope;
	assert(isptr(ty) || isarray(ty));
	q->type = isptr(ty) ? ty->type : ty;
	q->temporary = p->temporary;
	q->generated = p->generated;
	q->addressed = p->addressed;
	q->computed = 1;
	q->defined = 1;
	q->ref = 1;
	if (p->scope  == GLOBAL
	||  p->sclass == STATIC || p->sclass == EXTERN) {
		if (p->sclass == AUTO)
			q->sclass = STATIC;
		(*IR->address)(q, p, n);
	} else {
		Code cp;
		addlocal(p);
		cp = code(Address);
		cp->u.addr.sym = q;
		cp->u.addr.base = p;
		cp->u.addr.offset = n;
	}
	e = tree(e->op, ty, NULL, NULL);
	e->u.sym = q;
	return e;
}

/* div - return 1 if min <= x/y <= max, 0 otherwise */
static int div(x, y, min, max, needconst)
double x, y, min, max; int needconst; {
	int cond;

	if (x < 0) x = -x;
	if (y < 0) y = -y;
	cond = y != 0 && (y > 1 || x <= max*y);
	if (!cond && y != 0 && needconst) {
		warning("overflow in constant expression\n");
		cond = 1;
	}
	return cond;
}

/* ispow2 - if u > 1 && u == 2^n, return n, otherwise return 0 */
int ispow2(u) unsigned u; {
	int n;

	if (u > 1 && (u&(u-1)) == 0)
		for (n = 0; u; u >>= 1, n++)
			if (u&1)
				return n;
	return 0;
}

/* mul - return 1 if min <= x*y <= max, 0 otherwise */
static int mul(x, y, min, max, needconst)
double x, y, min, max; int needconst; {
	int cond = x > -1 && x <= 1 || y > -1 && y <= 1
	|| x < 0 && y < 0 && -x <= max/-y
	|| x < 0 && y > 0 &&  x >= min/y
	|| x > 0 && y < 0 &&  x >= min/y
	|| x > 0 && y > 0 &&  x <= max/y;
	if (!cond && needconst) {
		warning("overflow in constant expression\n");
		cond = 1;
	}
	return cond;
}

/* sub - return 1 if min <= x-y <= max, 0 otherwise */
static int sub(x, y, min, max, needconst)
double x, y, min, max; int needconst; {
	return add(x, -y, min, max, needconst);
}
