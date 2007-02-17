#include "c.h"

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

static void bbcall ARGS((Symbol, Coordinate *, Tree *));
static void bbentry ARGS((Symbol, Symbol));
static void bbexit ARGS((Symbol, Symbol, Tree));
static int  bbfile ARGS((char *));
static void bbfunc ARGS((Symbol, Symbol));
static void bbincr ARGS((Symbol, Coordinate *, Tree *));
static void bbvars ARGS((Symbol));

int npoints;		/* # of execution points if -b specified */
int ncalled = -1;	/* #times prof.out says current function was called */
static Symbol YYlink;	/* symbol for file's struct _bbdata */
static Symbol YYcounts;	/* symbol for _YYcounts if -b specified */
static List maplist;	/* list of struct map *'s */
static List filelist;	/* list of file names */
static Symbol funclist;	/* list of struct func *'s */
static Symbol afunc;	/* current function's struct func */

/* bbcall - build tree to set _callsite at call site *cp, emit call site data */
static void bbcall(yycounts, cp, e) Symbol yycounts; Coordinate *cp; Tree *e; {
	static Symbol caller;
	Value v;
	union coordinate u;
	Symbol p = genident(STATIC, array(voidptype, 0, 0), GLOBAL);

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
	(*IR->defconst)(U, (v.u = u.coord, v));
	if (caller == 0) {
		caller = mksymbol(EXTERN, "_caller", ptr(voidptype));
		caller->defined = 0;
	}
	*e = right(asgn(caller, idtree(p)), *e);
}

/* bbentry - return tree for _prologue(&afunc, &YYlink)' */
static void bbentry(yylink, f) Symbol yylink, f; {
	static Symbol p;
	
	afunc = genident(STATIC, array(voidptype, 4, 0), GLOBAL);
	if (p == 0) {
		p = mksymbol(EXTERN, "_prologue", ftype(inttype, voidptype));
		p->defined = 0;
	}
	walk(calltree(pointer(idtree(p)), freturn(p->type),
		tree(ARG+P, ptr(unsignedtype), idtree(yylink),
		tree(ARG+P, ptr(unsignedtype), idtree(afunc), 0)), NULL), 0, 0);
}

/* bbexit - return tree for _epilogue(&afunc)' */
static void bbexit(yylink, f, e) Symbol yylink, f; Tree e; {
	static Symbol p;
	
	if (p == 0) {
		p = mksymbol(EXTERN, "_epilogue", ftype(inttype, voidptype));
		p->defined = 0;
	}
	walk(calltree(pointer(idtree(p)), freturn(p->type),
		tree(ARG+P, ptr(unsignedtype), idtree(afunc), NULL), NULL), 0, 0);
}

/* bbfile - add file to list of file names, return its index */
static int bbfile(file) char *file; {
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
static void bbfunc(yylink, f) Symbol yylink, f; {
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
	(*IR->defconst)(U, (v.u = u.coord, v));
	funclist = afunc;
}

/* bbincr - build tree to increment execution point at *cp */
static void bbincr(yycounts, cp, e) Symbol yycounts; Coordinate *cp; Tree *e; {
	struct map *mp = maplist->x;

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
	*e = right(incr('+', rvalue((*optree['+'])(ADD, pointer(idtree(yycounts)),
		consttree(npoints++, inttype))), consttree(1, inttype)), *e);
}

/* bbvars - emit definition for basic block counting data */
static void bbvars(yylink) Symbol yylink; {
	int i, j, n = npoints;
	Value v;
	struct map **mp;
	Symbol coords, files, *p;

	if (!YYcounts && !yylink)
		return;
	if (YYcounts) {
		if (n <= 0)
			n = 1;
		YYcounts->type = array(unsignedtype, n, 0);
		defglobal(YYcounts, BSS);
	}
	files = genident(STATIC, array(ptr(chartype), 1, 0), GLOBAL);
	defglobal(files, LIT);
	for (p = ltov(&filelist, PERM); *p; p++)
		defpointer((*p)->u.c.loc);
	defpointer(NULL);
	coords = genident(STATIC, array(unsignedtype, n, 0), GLOBAL);
	defglobal(coords, LIT);
	for (i = n, mp = ltov(&maplist, PERM); *mp; i -= (*mp)->size, mp++)
		for (j = 0; j < (*mp)->size; j++)
			(*IR->defconst)(U, (v.u = (*mp)->u[j].coord, v));
	if (i > 0)
		(*IR->space)(i*coords->type->type->size);
	defpointer(NULL);
	defglobal(yylink, DATA);
	defpointer(NULL);
	(*IR->defconst)(U, (v.u = n, v));
	defpointer(YYcounts);
	defpointer(coords);
	defpointer(files);
	defpointer(funclist);
}

/* profInit - initialize basic block profiling options */
void profInit(opt) char *opt; {
	if (strncmp(opt, "-a", 2) == 0) {
		if (ncalled == -1 && process(opt[2] ? &opt[2] : "prof.out") > 0)
			ncalled = 0;
	} else if ((strcmp(opt, "-b") == 0 || strcmp(opt, "-C") == 0) && YYlink == 0) {
		YYlink = genident(STATIC, array(unsignedtype, 0, 0), GLOBAL);
		attach((Apply)bbentry, YYlink, &events.entry);
		attach((Apply)bbexit,  YYlink, &events.returns);
		attach((Apply)bbfunc,  YYlink, &events.exit);
		attach((Apply)bbvars,  YYlink, &events.end);
		if (strcmp(opt, "-b") == 0) {
			YYcounts = genident(STATIC, array(unsignedtype, 0, 0), GLOBAL);
			maplist = append(allocate(sizeof (struct map), PERM), maplist);
			((struct map *)maplist->x)->size = 0;
			attach((Apply)bbcall, YYcounts, &events.calls);
			attach((Apply)bbincr, YYcounts, &events.points);
		}
	}
}
