#include "c.h"

#define equalp(x) v.x == p->sym.u.c.v.x

struct table {
	int level;
	Table previous;
	struct entry {
		struct symbol sym;
		struct entry *link;
	} *buckets[256];
	Symbol all;
};
#define HASHSIZE NELEMS(((Table)0)->buckets)
static struct table
	cns = { CONSTANTS },
	ext = { GLOBAL },
	ids = { GLOBAL },
	tys = { GLOBAL };
Table constants   = &cns;
Table externals   = &ext;
Table identifiers = &ids;
Table globals     = &ids;
Table types       = &tys;
Table labels;
int level = GLOBAL;
List loci, symbols;

Table table(tp, level) Table tp; int level; {
	Table new;

	NEW0(new, FUNC);
	new->previous = tp;
	new->level = level;
	if (tp)
		new->all = tp->all;
	return new;
}
void foreach(tp, lev, apply, cl) Table tp; int lev;
void (*apply) ARGS((Symbol, void *)); void *cl; {
	assert(tp);
	while (tp && tp->level > lev)
		tp = tp->previous;
	if (tp && tp->level == lev) {
		Symbol p;
		Coordinate sav;
		sav = src;
		for (p = tp->all; p && p->scope == lev; p = p->up) {
			src = p->src;
			(*apply)(p, cl);
		}
		src = sav;
	}
}
void enterscope() {
	++level;
}
void exitscope() {
	rmtypes(level);
	if (types->level == level)
		types = types->previous;
	if (identifiers->level == level) {
		if (Aflag >= 2) {
			int n = 0;
			Symbol p;
			for (p = identifiers->all; p && p->scope == level; p = p->up)
				if (++n > 127) {
					warning("more than 127 identifiers declared in a block\n");
					break;
				}
		}
		identifiers = identifiers->previous;
	}
	assert(level >= GLOBAL);
	--level;
}
Symbol install(name, tpp, level, arena)
char *name; Table *tpp; int level, arena; {
	Table tp = *tpp;
	struct entry *p;
	unsigned h = (unsigned long)name&(HASHSIZE-1);

	assert(level == 0 || level >= tp->level);
	if (level > 0 && tp->level < level)
		tp = *tpp = table(tp, level);
	NEW0(p, arena);
	p->sym.name = name;
	p->sym.scope = level;
	p->sym.up = tp->all;
	tp->all = &p->sym;
	p->link = tp->buckets[h];
	tp->buckets[h] = p;
	return &p->sym;
}
Symbol lookup(name, tp) char *name; Table tp; {
	struct entry *p;
	unsigned h = (unsigned long)name&(HASHSIZE-1);

	assert(tp);
	do
		for (p = tp->buckets[h]; p; p = p->link)
			if (name == p->sym.name)
				return &p->sym;
	while ((tp = tp->previous) != NULL);
	return NULL;
}
int genlabel(n) int n; {
	static int label = 1;

	label += n;
	return label - n;
}
Symbol findlabel(lab) int lab; {
	struct entry *p;
	unsigned h = lab&(HASHSIZE-1);

	for (p = labels->buckets[h]; p; p = p->link)
		if (lab == p->sym.u.l.label)
			return &p->sym;
	NEW0(p, FUNC);
	p->sym.name = stringd(lab);
	p->sym.scope = LABELS;
	p->sym.up = labels->all;
	labels->all = &p->sym;
	p->link = labels->buckets[h];
	labels->buckets[h] = p;
	p->sym.generated = 1;
	p->sym.u.l.label = lab;
	(*IR->defsymbol)(&p->sym);
	return &p->sym;
}
Symbol constant(ty, v) Type ty; Value v; {
	struct entry *p;
	unsigned h = v.u&(HASHSIZE-1);

	ty = unqual(ty);
	for (p = constants->buckets[h]; p; p = p->link)
		if (eqtype(ty, p->sym.type, 1))
			switch (ty->op) {
			case CHAR:     if (equalp(uc)) return &p->sym; break;
			case SHORT:    if (equalp(ss)) return &p->sym; break;
			case INT:      if (equalp(i))  return &p->sym; break;
			case UNSIGNED: if (equalp(u))  return &p->sym; break;
			case FLOAT:    if (equalp(f))  return &p->sym; break;
			case DOUBLE:   if (equalp(d))  return &p->sym; break;
			case ARRAY: case FUNCTION:
			case POINTER:  if (equalp(p))  return &p->sym; break;
			default: assert(0);
			}
	NEW0(p, PERM);
	p->sym.name = vtoa(ty, v);
	p->sym.scope = CONSTANTS;
	p->sym.type = ty;
	p->sym.sclass = STATIC;
	p->sym.u.c.v = v;
	p->link = constants->buckets[h];
	p->sym.up = constants->all;
	constants->all = &p->sym;
	constants->buckets[h] = p;
	if (ty->u.sym && !ty->u.sym->addressed)
		(*IR->defsymbol)(&p->sym);
	p->sym.defined = 1;
	return &p->sym;
}
Symbol intconst(n) int n; {
	Value v;

	v.i = n;
	return constant(inttype, v);
}
Symbol genident(scls, ty, lev) int scls, lev; Type ty; {
	Symbol p;

	NEW0(p, lev >= LOCAL ? FUNC : PERM);
	p->name = stringd(genlabel(1));
	p->scope = lev;
	p->sclass = scls;
	p->type = ty;
	p->generated = 1;
	if (lev == GLOBAL)
		(*IR->defsymbol)(p);
	return p;
}

Symbol temporary(scls, ty, lev) Type ty; int scls, lev; {
	Symbol p = genident(scls, ty, lev);

	p->temporary = 1;
	return p;
}
Symbol newtemp(sclass, tc) int sclass, tc; {
	Symbol p = temporary(sclass, btot(tc), LOCAL);

	(*IR->local)(p);
	p->defined = 1;
	return p;
}

void locus(tp, cp) Table tp; Coordinate *cp; {
	loci    = append(cp, loci);
	symbols = append(tp->all, symbols);
}

void use(p, src) Symbol p; Coordinate src; {
	Coordinate *cp;

	NEW(cp, PERM);
	*cp = src;
	p->uses = append(cp, p->uses);
}
/* findtype - find type ty in identifiers */
Symbol findtype(ty) Type ty; {
	Table tp = identifiers;
	int i;
	struct entry *p;

	assert(tp);
	do
		for (i = 0; i < HASHSIZE; i++)
			for (p = tp->buckets[i]; p; p = p->link)
				if (p->sym.type == ty && p->sym.sclass == TYPEDEF)
					return &p->sym;
	while ((tp = tp->previous) != NULL);
	return NULL;
}

/* mkstr - make a string constant */
Symbol mkstr(str) char *str; {
	Value v;
	Symbol p;

	v.p = str;
	p = constant(array(chartype, strlen(v.p) + 1, 0), v);
	if (p->u.c.loc == NULL)
		p->u.c.loc = genident(STATIC, p->type, GLOBAL);
	return p;
}

/* mksymbol - make a symbol for name, install in &globals if sclass==EXTERN */
Symbol mksymbol(sclass, name, ty) int sclass; char *name; Type ty; {
	Symbol p;

	if (sclass == EXTERN)
		p = install(string(name), &globals, GLOBAL, PERM);
	else {
		NEW0(p, PERM);
		p->name = string(name);
		p->scope = GLOBAL;
	}
	p->sclass = sclass;
	p->type = ty;
	(*IR->defsymbol)(p);
	p->defined = 1;
	return p;
}

/* vtoa - return string for the constant v of type ty */
char *vtoa(ty, v) Type ty; Value v; {
	char buf[50];

	ty = unqual(ty);
	switch (ty->op) {
	case CHAR:
		return stringd(v.uc);
	case SHORT:
		return stringd(v.ss);
	case INT:
		return stringd(v.i);
	case UNSIGNED:
		if ((v.u&~0x7fff) == 0)
			return stringd(v.u);
		else
			return stringf("0x%x", v.u);
	case FLOAT:
		sprintf(buf, "%.8g", v.f);
		return string(buf);
	case DOUBLE:
		sprintf(buf, "%.18g", v.d);
		return string(buf);
	case ARRAY:
		if (ty->type->op == CHAR)
			return v.p;
		/* else fall thru */
	case POINTER: case FUNCTION:
		return stringf("0x%x", v.p);
	default:assert(0);
	}
	return NULL;
}
