#include "c.h"

static Node *tail;

static Node gen0 ARGS((Node));
static int gen1 ARGS((Node, int));
static void address ARGS((Symbol, Symbol, int));
static void defaddress ARGS((Symbol));
static void defconst ARGS((int, Value));
static void defstring ARGS((int, char *));
static void defsymbol ARGS((Symbol));
static void emit0 ARGS((Node));
static void export ARGS((Symbol));
static void function ARGS((Symbol, Symbol [], Symbol [], int));
static void global ARGS((Symbol));
static void import ARGS((Symbol));
static void local ARGS((Symbol));
static void progbeg ARGS((int, char **));
static void progend ARGS((void));
static void segment ARGS((int));
static void space ARGS((int));
static void stabend ARGS((Coordinate *, Symbol, Coordinate **, Symbol *, Symbol *));
static void stabline ARGS((Coordinate *));
static void sym ARGS((char *, Symbol, char *));
static void symname ARGS((Symbol));

Interface symbolicIR = {
	1, 1, 0,	/* char */
	2, 2, 0,	/* short */
	4, 4, 0,	/* int */
	4, 4, 1,	/* float */
	8, 4, 1,	/* double */
	4, 4, 0,	/* T* */
	0, 4, 0,	/* struct */
	0,		/* little_endian */
	0,		/* mulops_calls */
	0,		/* wants_callb */
	1,		/* wants_argb */
	1,		/* left_to_right */
	1,		/* wants_dag */
	address,
	blockbeg,
	blockend,
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
	space,
	0,	/* stabblock */
	stabend,
	0,	/* stabfend */
	0,	/* stabinit */
	stabline,
	0,	/* stabsym */
	0,	/* stabtype */
};

/* address - initialize q for addressing expression p+n */
static void address(q, p, n) Symbol q, p; int n; {
	q->x.name = stringf("%s%s%d", p->x.name, n > 0 ? "+" : "", n);
}

/* defaddress - initialize an address */
static void defaddress(p) Symbol p; {
	print("defaddress %s\n", p->x.name);
}

/* defconst - define a constant */
static void defconst(ty, v) int ty; Value v; {
	print("defconst ");
	switch (ty) {
	case C: print("char %d\n",       v.uc); break;
	case S: print("short %d\n",      v.ss); break;
	case I: print("int %d\n",        v.i ); break;
	case U: print("unsigned 0x%x\n", v.u ); break;
	case P: print("void* 0x%x\n",    v.p ); break;
	case F: {
		char buf[MAXLINE];
		sprintf(buf, "float %.8e\n", v.f);  /* fix */
		outs(buf);
		break;
		}
	case D: {
		char buf[MAXLINE];
		sprintf(buf, "double %.18e\n", v.d);  /* fix */
		outs(buf);
		break;
		}
	default: assert(0);
	}
}

/* defstring - emit a string constant */
static void defstring(len, s) int len; char *s; {
	int n;

	print("defstring \"");
	for (n = 0; len-- > 0; s++) {
		if (n >= 72) {
			print("\n");
			n = 0;
		}
		if (*s == '"' || *s == '\\') {
			print("\\%c", *s);
			n += 2;
		} else if (*s >= ' ' && *s < 0177) {
			*bp++ = *s;
			n += 1;
		} else {
			print("\\%d%d%d", (*s>>6)&3, (*s>>3)&7, *s&7);
			n += 4;
		}
	}
	print("\"\n");
}

/* defsymbol - define a symbol: initialize p->x */
static void defsymbol(p) Symbol p; {
	if (p->scope == CONSTANTS)
		switch (ttob(p->type)) {
		case U: p->x.name = stringf("0x%x", p->u.c.v.u); break;
		case C: p->x.name = stringf("'\\x%x'", p->u.c.v.uc); break;
		case S: p->x.name = stringd(p->u.c.v.ss); break;
		case I: p->x.name = stringd(p->u.c.v.i); break;
		case P: p->x.name = stringf("0x%x", p->u.c.v.p); break;
		default: assert(0);
		}
	else
		p->x.name = p->name;
	if (glevel > 2 && p->scope >= LOCAL && p->type && isfunc(p->type))
		sym("extern", p, "\n");
}

/* emit0 - emit the dags on list p */
static void emit0(p) Node p; {
	for (; p; p = p->x.next)
		if (p->op == LABEL+V) {
			assert(p->syms[0]);
			print("%s:\n", p->syms[0]->x.name);
		} else {
			int i;
			print("node%c%d %s count=%d", p->x.listed ? '\'' : '#', p->x.inst,
				opname(p->op), p->count);
			for (i = 0; i < NELEMS(p->kids) && p->kids[i]; i++)
				print(" #%d", p->kids[i]->x.inst);
			for (i = 0; i < NELEMS(p->syms) && p->syms[i]; i++) {
				if (p->syms[i]->x.name)
					print(" %s", p->syms[i]->x.name);
				if (p->syms[i]->name != p->syms[i]->x.name)
					print(" (%s)", p->syms[i]->name);
			}
			if (generic(p->op) == CALL && p->syms[0] && p->syms[0]->type)
				print(" {%t}", p->syms[0]->type);
			print("\n");
		}
}

/* export - announce p as exported */
static void export(p) Symbol p; {
	print("export %s\n", p->x.name);
}

/* function - generate code for a function */
static void function(f, caller, callee, ncalls)
Symbol f, caller[], callee[]; int ncalls; {
	int i;

	sym("function", f, ncalls ? (char *)0 : "\n");
	if (ncalls)
		print(" ncalls=%d\n", ncalls);
	offset = 0;
	for (i = 0; caller[i] && callee[i]; i++) {
		offset = roundup(offset, caller[i]->type->align);
		caller[i]->x.name = caller[i]->name;
		callee[i]->x.name = callee[i]->name;
		caller[i]->x.offset = callee[i]->x.offset = offset;
		sym("caller's parameter", caller[i], "\n");
		sym("callee's parameter", callee[i], "\n");
		offset += caller[i]->type->size;
	}
	maxoffset = offset = 0;
	gencode(caller, callee);
	print("maxoffset=%d\n", maxoffset);
	emitcode();
	print("end %s\n", f->x.name);
}

/* gen0 - generate code for the dags on list p */
static Node gen0(p) Node p; {
	int n;
	Node nodelist;

	tail = &nodelist;
	for (n = 0; p; p = p->link) {
		switch (generic(p->op)) {	/* check for valid nodelist */
		case CALL:
			assert(IR->wants_dag || p->count == 0);
			break;
		case ARG:
		case ASGN: case JUMP: case LABEL: case RET:
		case EQ: case GE: case GT: case LE: case LT: case NE:
			assert(p->count == 0);
			break;
		case INDIR:
			assert(IR->wants_dag && p->count > 0);
			break;
		default:
			assert(0);
		}
		p->x.listed = 1;
		n = gen1(p, n);
	}
	*tail = 0;
	return nodelist;
}

/* gen1 - generate code for *p */
static int gen1(p, n) Node p; int n; {
	if (p && p->x.inst == 0) {
		p->x.inst = ++n;
		n = gen1(p->kids[0], n);
		n = gen1(p->kids[1], n);
		*tail = p;
		tail = &p->x.next;
	}
	return n;
}

/* global - announce a global */
static void global(p) Symbol p; {
	sym("global", p, "\n");
}

/* import - import a symbol */
static void import(p) Symbol p; {
	print("import %s\n", p->x.name);
}

/* local - local variable */
static void local(p) Symbol p; {
	offset = roundup(offset, p->type->align);
	p->x.name = p->name;
	p->x.offset = offset;
	sym("local", p, "\n");
	offset += p->type->size;
}

/* progbeg - beginning of program */
static void progbeg(argc, argv) int argc; char *argv[]; {
	int i;

	for (i = 0; i < argc; i++)
		if (strncmp(argv[i], "-little_endian=", 15) == 0)
			IR->little_endian = argv[i][15] - '0';
		else if (strncmp(argv[i], "-mulops_calls=", 18) == 0)
			IR->mulops_calls = argv[i][18] - '0';
		else if (strncmp(argv[i], "-wants_callb=", 13) == 0)
			IR->wants_callb = argv[i][13] - '0';
		else if (strncmp(argv[i], "-wants_argb=", 12) == 0)
			IR->wants_argb = argv[i][12] - '0';
		else if (strncmp(argv[i], "-left_to_right=", 15) == 0)
			IR->left_to_right = argv[i][15] - '0';
		else if (strncmp(argv[i], "-wants_dag=", 11) == 0)
			IR->wants_dag = argv[i][11] - '0';
}

/* progend - end of program */
static void progend() {}

/* segment - switch to segment s */
static void segment(s) int s; {
	print("segment %s\n", &"text\0bss\0.data\0lit\0.sym\0."[5*s-5]);
}

/* space - initialize n bytes of space */
static void space(n) int n; {
	print("space %d\n", n);
}

/* sym - print symbol table entry for p, followed by str */
static void sym(kind, p, str) char *kind, *str; Symbol p; {
	assert(kind);
	if (glevel > 2) {
		print("%s ", kind);
		symname(p);
	} else
		print("%s %s", kind, p->name);
	if (p->name != p->x.name)
		print(" (%s)", p->x.name);
	print(" type=%t class=%k scope=", p->type, p->sclass);
	switch (p->scope) {
	case CONSTANTS: print("CONSTANTS"); break;
	case LABELS:    print("LABELS");    break;
	case GLOBAL:    print("GLOBAL");    break;
	case PARAM:     print("PARAM");     break;
	case LOCAL:     print("LOCAL");     break;
	default:
		if (p->scope > LOCAL)
			print("LOCAL+%d", p->scope - LOCAL);
		else
			print("%d", p->scope);
	}
	if (p->scope >= PARAM && p->sclass != STATIC)
		print(" offset=%d", p->x.offset);
	print(" ref=%d", (int)(1000*p->ref));
	if (glevel > 2) {
		print(" up=");
		symname(p->up);
	}
	if (str)
		print(str);
}

/* symname - print prefix, p's name, declaration source coordinate, suffix */
static void symname(p) Symbol p; {
	if (p)
		print("%s@%w.%d", p->name, &p->src, p->src.x);
	else
		print("0");
}

/* stabend - finalize stab output */
static void stabend(cp, p, cpp, sp, stab) Coordinate *cp, **cpp; Symbol p, *sp, *stab; {
	int i;

	symname(p);
	print("\n");
	if (cpp && sp)
		for (i = 0; cpp[i] && sp[i]; i++) {
			print("%w.%d: ", cpp[i], cpp[i]->x);
			symname(sp[i]);
			print("\n");
		}
}

/* stabline - emit line number information for source coordinate *cp */
static void stabline(cp) Coordinate *cp; {
	if (cp->file)
		print("%s:", cp->file);
	print("%d.%d:\n", cp->y, cp->x);
}
