#include "c.h"

static void appendstr ARGS((char *));
static void tracecall ARGS((Symbol, Symbol));
static void tracefinis ARGS((Symbol));
static void tracereturn ARGS((Symbol, Symbol, Tree));
static void tracevalue ARGS((Tree, int));

static char *fmt, *fp, *fmtend;	/* format string, current & limit pointer */
static Tree args;		/* printf arguments */
static Symbol frameno;		/* local holding frame number */

/* appendstr - append str to the evolving format string, expanding it if necessary */
static void appendstr(str) char *str; {
	do
		if (fp == fmtend)
			if (fp) {
				char *s = allocate(2*(fmtend - fmt), FUNC);
				strncpy(s, fmt, fmtend - fmt);
				fp = s + (fmtend - fmt);
				fmtend = s + 2*(fmtend - fmt);
				fmt = s;
			} else {
				fp = fmt = allocate(80, FUNC);
				fmtend = fmt + 80;
			}
	while ((*fp++ = *str++) != 0);
	fp--;
}

/* tracecall - generate code to trace entry to f */
static void tracecall(printer, f) Symbol printer, f; {
	int i;
	Symbol counter = genident(STATIC, inttype, GLOBAL);

	defglobal(counter, BSS);
	(*IR->space)(counter->type->size);
	frameno = genident(AUTO, inttype, level);
	addlocal(frameno);
	appendstr(f->name); appendstr("#");
	tracevalue(asgn(frameno, incr(INCR, idtree(counter), consttree(1, inttype))), 0);
	appendstr("(");
	for (i = 0; f->u.f.callee[i]; i++) {
		if (i)
			appendstr(",");
		appendstr(f->u.f.callee[i]->name); appendstr("=");
		tracevalue(idtree(f->u.f.callee[i]), 0);
	}
	if (variadic(f->type))
		appendstr(",...");
	appendstr(") called\n");
	tracefinis(printer);
}

/* tracefinis - complete & generate the trace call to print */
static void tracefinis(printer) Symbol printer; {
	Tree *ap;
	Symbol p;

	*fp = 0;
	p = mkstr(string(fmt));
	for (ap = &args; *ap; ap = &(*ap)->kids[1])
		;
	*ap = tree(ARG+P, ptr(chartype), pointer(idtree(p->u.c.loc)), 0);
	walk(calltree(pointer(idtree(printer)), freturn(printer->type), args, NULL), 0, 0);
	args = 0;
	fp = fmtend = 0;
}

/* traceinit - initialize for tracing */
void traceInit(print) char *print; {
	static Symbol printer;

	if (!printer) {
		printer = mksymbol(EXTERN, print && *print ? print : "printf",
			ftype(inttype, ptr(qual(CONST, chartype))));
		printer->defined = 0;
		attach((Apply)tracecall,   printer, &events.entry);
		attach((Apply)tracereturn, printer, &events.returns);
	}
}

/* tracereturn - generate code to trace return e */
static void tracereturn(printer, f, e) Symbol printer, f; Tree e; {
	appendstr(f->name); appendstr("#");
	tracevalue(idtree(frameno), 0);
	appendstr(" returned");
	if (freturn(f->type) != voidtype && e) {
		appendstr(" ");
		tracevalue(e, 0);
	}
	appendstr("\n");
	tracefinis(printer);
}

/* tracevalue - append format and argument to print the value of e */
static void tracevalue(e, lev) Tree e; int lev; {
	Type ty = unqual(e->type);

	switch (ty->op) {
	case CHAR:
		appendstr("'\\x%02x'");
		break;
	case SHORT:
		if (ty == unsignedshort)
			appendstr("0x%x");
		else /* fall thru */
	case INT:
			appendstr("%d");
		break;
	case UNSIGNED:
		appendstr("0x%x");
		break;
	case FLOAT: case DOUBLE:
		appendstr("%g");
		break;
	case POINTER:
		if (unqual(ty->type) == chartype) {
			static Symbol null;
			if (null == NULL)
				null = mkstr("(null)");
			tracevalue(cast(e, unsignedtype), lev + 1);
			appendstr(" \"%.30s\"");
			e = condtree(e, e, pointer(idtree(null->u.c.loc)));
		} else {
			appendstr("("); appendstr(typestring(ty, "")); appendstr(")0x%x");
		}
		break;
	case STRUCT: {
		Field q;
		appendstr("("); appendstr(typestring(ty, "")); appendstr("){");
		for (q = ty->u.sym->u.s.flist; q; q = q->link) {
			appendstr(q->name); appendstr("=");
			tracevalue(field(addrof(e), q->name), lev + 1);
			if (q->link)
				appendstr(",");
		}
		appendstr("}");
		return;
		}
	case UNION:
		appendstr("("); appendstr(typestring(ty, "")); appendstr("){...}");
		return;
	case ARRAY:
		if (lev && ty->type->size > 0) {
			int i;
			e = pointer(e);
			appendstr("{");
			for (i = 0; i < ty->size/ty->type->size; i++) {
				Tree p = (*optree['+'])(ADD, e, consttree(i, inttype));
				if (isptr(p->type) && isarray(p->type->type))
					p = retype(p, p->type->type);
				else
					p = rvalue(p);
				if (i)
					appendstr(",");
				tracevalue(p, lev + 1);
			}
			appendstr("}");
		} else
			appendstr(typestring(ty, ""));
		return;
	default:
		assert(0);
	}
	if (ty == floattype)
		e = cast(e, doubletype);
	else if ((isint(ty) || isenum(ty)) && ty->size != inttype->size)
		e = cast(e, promote(ty));
	args = tree(ARG + widen(e->type), e->type, e, args);
}
