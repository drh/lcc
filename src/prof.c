#include "c.h"

static char rcsid[] = "$Id$";

struct callsite {
	char *file, *name;
	union coordinate {
		unsigned int coord;
		struct { unsigned int y:16,x:10,index:6; } le;
		struct { unsigned int index:6,x:10,y:16; } be;
	} u;
};
struct func {
	struct func *link;
	struct caller *callers;
	char *name;
	union coordinate src;
};
struct map {		/* source code map; 200 coordinates/map */
	int size;
	union coordinate u[200];
};

int npoints;		/* # of execution points if -b specified */
int ncalled = -1;	/* #times prof.out says current function was called */
static Symbol YYlink;	/* symbol for file's struct _bbdata */
static Symbol YYcounts;	/* symbol for _YYcounts if -b specified */
static List maplist;	/* list of struct map *'s */
static List filelist;	/* list of file names */
static Symbol funclist;	/* list of struct func *'s */
static Symbol afunc;	/* current function's struct func */

/* bbpad - emit space, if necessary, to make size%align == 0; return new size */
static int bbpad(int size, int align) {
	if (size%align) {
		(*IR->space)(align - size%align);
		size = roundup(size, align);
	}
	return size;
}

/* bbcall - build tree to set _callsite at call site *cp, emit call site data */
static void bbcall(Symbol yycounts, Coordinate *cp, Tree *e) {
	static Symbol caller;
	Value v;
	union coordinate u;
	Symbol p = genident(STATIC, array(voidptype, 0, 0), GLOBAL);
	Tree t;

	defglobal(p, LIT);
	defpointer(cp->file ? mkstr(cp->file)->u.c.loc : (Symbol)0);
	defpointer(mkstr(cfunc->name)->u.c.loc);
	if (IR->little_endian) {
		u.le.x = cp->x;
		u.le.y = cp->y;
	} else {
		u.be.x = cp->x;
		u.be.y = cp->y;
	}
	(*IR->defconst)(U, unsignedtype->size, (v.u = u.coord, v));
	bbpad(2*voidptype->size + unsignedtype->size, p->type->align);	
	if (caller == 0) {
		caller = mksymbol(EXTERN, "_caller", ptr(voidptype));
		caller->defined = 0;
	}
	for (t = *e; generic(t->op) != CALL; t = t->kids[0])
		assert(t->op == RIGHT || !t->kids[1]);
	assert(generic(t->op) == CALL);
	t = tree(t->op, t->type,
		tree(RIGHT, t->kids[0]->type,
			t->kids[0],
			tree(RIGHT, t->kids[0]->type, asgn(caller, idtree(p)), t->kids[0])),
		t->kids[1]);
	for ( ; generic((*e)->op) != CALL; e = &(*e)->kids[0])
		;
	*e = t;
}

/* bbentry - return tree for _prologue(&afunc, &YYlink)' */
static void bbentry(Symbol yylink, Symbol f, void *ignore) {
	static Symbol prologue;

	afunc = genident(STATIC, array(voidptype, 4, 0), GLOBAL);
	if (prologue == 0) {
		prologue = mksymbol(EXTERN, "_prologue", ftype(inttype, voidptype, voidptype, NULL));
		prologue->defined = 0;
	}
	walk(vcall(prologue, voidtype, pointer(idtree(afunc)), pointer(idtree(yylink)), NULL), 0, 0);
}

/* bbexit - return tree for _epilogue(&afunc)' */
static void bbexit(Symbol yylink, Symbol f, Tree e) {
	static Symbol epilogue;

	if (epilogue == 0) {
		epilogue = mksymbol(EXTERN, "_epilogue", ftype(inttype, voidptype, NULL));
		epilogue->defined = 0;
	}
	walk(vcall(epilogue, voidtype, pointer(idtree(afunc)), NULL), 0, 0);
}

/* bbfile - add file to list of file names, return its index */
static int bbfile(char *file) {
	if (file) {
		List lp;
		int i = 1;
		if ((lp = filelist) != NULL)
			do {
				lp = lp->link;
				if (((Symbol)lp->x)->u.c.v.p == file)
					return i;
				i++;
			} while (lp != filelist);
		filelist = append(mkstr(file), filelist);
		return i;
	}
	return 0;
}

/* bbfunc - emit function name and src coordinates */
static void bbfunc(Symbol yylink, Symbol f, void *ignore) {
	Value v;
	union coordinate u;

	defglobal(afunc, DATA);
	defpointer(funclist);
	defpointer(NULL);
	defpointer(mkstr(f->name)->u.c.loc);
	if (IR->little_endian) {
		u.le.x = f->u.f.pt.x;
		u.le.y = f->u.f.pt.y;
		u.le.index = bbfile(f->u.f.pt.file);
	} else {
		u.be.x = f->u.f.pt.x;
		u.be.y = f->u.f.pt.y;
		u.be.index = bbfile(f->u.f.pt.file);
	}
	(*IR->defconst)(U, unsignedtype->size, (v.u = u.coord, v));
	bbpad(3*voidptype->size + unsignedtype->size, afunc->type->align);
	funclist = afunc;
}

/* bbincr - build tree to increment execution point at *cp */
static void bbincr(Symbol yycounts, Coordinate *cp, Tree *e) {
	struct map *mp = maplist->x;
	Tree t;

	if (needconst)
		return;
	/* append *cp to source map */
	if (mp->size >= NELEMS(mp->u)) {
		NEW(mp, PERM);
		mp->size = 0;
		maplist = append(mp, maplist);
	}
	if (IR->little_endian) {
		mp->u[mp->size].le.x = cp->x;
		mp->u[mp->size].le.y = cp->y;
		mp->u[mp->size++].le.index = bbfile(cp->file);
	} else {
		mp->u[mp->size].be.x = cp->x;
		mp->u[mp->size].be.y = cp->y;
		mp->u[mp->size++].be.index = bbfile(cp->file);
	}
	t = incr('+', rvalue((*optree['+'])(ADD, pointer(idtree(yycounts)),
		consttree(npoints++, inttype))), consttree(1, inttype));
	if (*e)
		*e = tree(RIGHT, (*e)->type, t, *e);
	else
		*e = t;
}

/* bbvars - emit definition for basic block counting data */
static void bbvars(Symbol yylink, void *ignore, void *ignore2) {
	int i, j, n = npoints;
	Value v;
	struct map **mp;
	Symbol coords, files, *p;

	if (!YYcounts && !yylink)
		return;
	if (YYcounts) {
		if (n <= 0)
			n = 1;
		YYcounts->type = array(inttype, n, 0);
		defglobal(YYcounts, BSS);
		(*IR->space)(YYcounts->type->size);
	}
	files = genident(STATIC, array(charptype, 1, 0), GLOBAL);
	defglobal(files, LIT);
	for (p = ltov(&filelist, PERM); *p; p++)
		defpointer((*p)->u.c.loc);
	defpointer(NULL);
	coords = genident(STATIC, array(unsignedtype, n, 0), GLOBAL);
	defglobal(coords, LIT);
	for (i = n, mp = ltov(&maplist, PERM); *mp; i -= (*mp)->size, mp++)
		for (j = 0; j < (*mp)->size; j++)
			(*IR->defconst)(U, unsignedtype->size, (v.u = (*mp)->u[j].coord, v));
	if (i > 0)
		(*IR->space)(i*coords->type->type->size);
	(*IR->defconst)(U, unsignedtype->size, (v.u = 0, v));
	defglobal(yylink, DATA);
	defpointer(NULL);
	(*IR->defconst)(U, inttype->size, (v.u = n, v));
	bbpad(voidptype->size + inttype->size, yylink->type->align);
	defpointer(YYcounts);
	defpointer(coords);
	defpointer(files);
	defpointer(funclist);
}

/* profInit - initialize basic block profiling options */
void profInit(char *arg) {
	if (strncmp(arg, "-a", 2) == 0) {
		if (ncalled == -1
		&& process(arg[2] ? &arg[2] : "prof.out") > 0)
			ncalled = 0;
	} else if ((strcmp(arg, "-b") == 0
	         || strcmp(arg, "-C") == 0) && YYlink == 0) {
		YYlink = genident(STATIC, array(voidptype, 0, 0), GLOBAL);
		attach((Apply)bbentry, YYlink, &events.entry);
		attach((Apply)bbexit,  YYlink, &events.returns);
		attach((Apply)bbfunc,  YYlink, &events.exit);
		attach((Apply)bbvars,  YYlink, &events.end);
		if (strcmp(arg, "-b") == 0) {
			YYcounts = genident(STATIC, array(inttype, 0, 0), GLOBAL);
			maplist = append(allocate(sizeof (struct map), PERM), maplist);
			((struct map *)maplist->x)->size = 0;
			attach((Apply)bbcall, YYcounts, &events.calls);
			attach((Apply)bbincr, YYcounts, &events.points);
		}
	}
}
