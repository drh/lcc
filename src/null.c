#include "c.h"

static void address ARGS((Symbol, Symbol, int));
static void blockbeg0 ARGS((Env *));
static void blockend0 ARGS((Env *));
static void defaddress ARGS((Symbol));
static void defconst ARGS((int, Value));
static void defstring ARGS((int, char *));
static void defsymbol ARGS((Symbol));
static void emit0 ARGS((Node));
static void export ARGS((Symbol));
static void function ARGS((Symbol, Symbol [], Symbol [], int));
static Node gen0 ARGS((Node));
static void global ARGS((Symbol));
static void import ARGS((Symbol));
static void local ARGS((Symbol));
static void progbeg ARGS((int, char **));
static void progend ARGS((void));
static void segment ARGS((int));
static void space ARGS((int));

static Node gen0(p) Node p; { return p; }
static void address(q, p, n) Symbol q, p; int n; {}
static void blockbeg0(e) Env *e; {}
static void blockend0(e) Env *e; {}
static void defaddress(p) Symbol p; {}
static void defconst(ty, v) int ty; Value v; {}
static void defstring(len, s) int len; char *s; {}
static void defsymbol(p) Symbol p; {}
static void emit0(p) Node p; {}
static void export(p) Symbol p; {}
static void function(f, caller, callee, ncalls) Symbol f, caller[], callee[]; int ncalls; {}
static void global(p) Symbol p; {}
static void import(p) Symbol p; {}
static void local(p) Symbol p; {}
static void progbeg(argc, argv) int argc; char *argv[]; {}
static void progend() {}
static void segment(s) int s; {}
static void space(n) int n; {}

Interface nullIR = {
	1, 1, 0,	/* char */
	2, 2, 0,	/* short */
	4, 4, 0,	/* int */
	4, 4, 1,	/* float */
	8, 8, 1,	/* double */
	4, 4, 0,	/* T* */
	0, 4, 0,	/* struct */
	1,		/* little_endian */
	0,		/* mulops_calls */
	0,		/* wants_callb */
	0,		/* wants_argb */
	1,		/* left_to_right */
	0,		/* wants_dag */
	address,
	blockbeg0,
	blockend0,
	defaddress,
	defconst,
	defstring,
	defsymbol,
	emit0,
	export,
	function,
	gen0,
	global,
	import,
	local,
	progbeg,
	progend,
	segment,
	space
};
