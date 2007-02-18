#include "c.h"
#include <stdio.h>

static char rcsid[] = "$Id$";

static Node *tail;
static int off, maxoff;

/* symname - print prefix, p's name, declaration source coordinate, suffix */
static void symname(Symbol p) {
        if (p)
                print("%s@%w.%d", p->name, &p->src, p->src.x);
        else
                print("0");
}

/* sym - print symbol table entry for p, followed by str */
static void sym(char *kind, Symbol p, char *str) {
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
        print(" ref=%f", p->ref);
        if (p->temporary && p->u.t.cse)
                print(" cse");
        if (glevel > 2) {
                print(" up=");
                symname(p->up);
        }
        if (str)
                print(str);
}

/* address - initialize q for addressing expression p+n */
static void address(Symbol q, Symbol p, int n) {
        q->x.name = stringf("%s%s%d", p->x.name, n > 0 ? "+" : "", n);
}

/* blockbeg1 - start a block */
static void blockbeg1(Env *e) {
        e->offset = off;
}

/* blockend1 - start a block */
static void blockend1(Env *e) {
        if (off > maxoff)
                maxoff = off;
        off = e->offset;
}

/* defaddress - initialize an address */
static void defaddress(Symbol p){
        print("defaddress %s\n", p->x.name);
}

/* defconst - define a constant */
static void defconst(int suffix, int size, Value v) {
        print("defconst ");
        switch (suffix) {
        case I:
                if (size > sizeof (int))
                        print("int.%d %D\n", size, v.i);
                else
                        print("int.%d %d\n", size, (int)v.i);
                break;
        case U:
                if (size > sizeof (unsigned))
                        print("unsigned.%d %U\n", size, v.u);
                else
                        print("unsigned.%d %u\n", size, (unsigned)v.u);
                break;
        case P: print("void*.%d %p\n", size, v.p); break;
        case F: print("float.%d %g\n", size, (double)v.d); break;
        default: assert(0);
        }
}

/* defstring - emit a string constant */
static void defstring(int len, char *s) {
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
                        print("%c", *s);
                        n += 1;
                } else {
                        print("\\%d%d%d", (*s>>6)&3, (*s>>3)&7, *s&7);
                        n += 4;
                }
        }
        print("\"\n");
}

/* defsymbol - define a symbol: initialize p->x */
static void defsymbol(Symbol p) {
        p->x.name = p->name;
        if (glevel > 2 && p->scope >= LOCAL && p->type && isfunc(p->type))
                sym("extern", p, "\n");
}

/* emit0 - emit the dags on list p */
static void emit0(Node p){
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
static void export(Symbol p) {
        print("export %s\n", p->x.name);
}

/* function - generate code for a function */
static void function(Symbol f, Symbol caller[], Symbol callee[], int ncalls) {
        int i;

        sym("function", f, ncalls ? (char *)0 : "\n");
        if (ncalls)
                print(" ncalls=%d\n", ncalls);
        off = 0;
        for (i = 0; caller[i] && callee[i]; i++) {
                off = roundup(off, caller[i]->type->align);
                caller[i]->x.name = caller[i]->name;
                callee[i]->x.name = callee[i]->name;
                caller[i]->x.offset = callee[i]->x.offset = off;
                sym("caller's parameter", caller[i], "\n");
                sym("callee's parameter", callee[i], "\n");
                off += caller[i]->type->size;
        }
        maxoff = off = 0;
        gencode(caller, callee);
        print("maxoff=%d\n", maxoff);
        emitcode();
        print("end %s\n", f->x.name);
}

/* gen1 - generate code for *p */
static int gen1(Node p, int n) {
        if (p && p->x.inst == 0) {
                p->x.inst = ++n;
                n = gen1(p->kids[0], n);
                n = gen1(p->kids[1], n);
                *tail = p;
                tail = &p->x.next;
        }
        return n;
}

/* gen0 - generate code for the dags on list p */
static Node gen0(Node p) {
        int n;
        Node nodelist;

        tail = &nodelist;
        for (n = 0; p; p = p->link) {
                switch (generic(p->op)) {       /* check for valid forest */
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
                check(p);
                p->x.listed = 1;
                n = gen1(p, n);
        }
        *tail = 0;
        return nodelist;
}

/* global - announce a global */
static void global(Symbol p) {
        sym("global", p, "\n");
}

/* import - import a symbol */
static void import(Symbol p) {
        print("import %s\n", p->x.name);
}

/* local - local variable */
static void local(Symbol p) {
        off = roundup(off, p->type->align);
        p->x.name = p->name;
        p->x.offset = off;
        sym(p->temporary ? "temporary" : "local", p, "\n");
        off += p->type->size;
}

/* progbeg - beginning of program */
static void progbeg(int argc, char *argv[]) {
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
static void progend(void) {}

/* segment - switch to segment s */
static void segment(int s) {
        print("segment %s\n", &"text\0bss\0.data\0lit\0.sym\0."[5*s-5]);
}

/* space - initialize n bytes of space */
static void space(int n) {
        print("space %d\n", n);
}

/* stabend - finalize stab output */
static void stabend(Coordinate *cp, Symbol p, Coordinate **cpp, Symbol *sp, Symbol *stab) {
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
static void stabline(Coordinate *cp) {
        if (cp->file)
                print("%s:", cp->file);
        print("%d.%d:\n", cp->y, cp->x);
}

Interface symbolicIR = {
        1, 1, 0,        /* char */
        2, 2, 0,        /* short */
        4, 4, 0,        /* int */
        4, 4, 0,        /* long */
        4, 4, 1,        /* float */
        8, 8, 1,        /* double */
        8, 8, 1,        /* long double */
        4, 4, 0,        /* T* */
        0, 4, 0,        /* struct */
        0,              /* little_endian */
        0,              /* mulops_calls */
        0,              /* wants_callb */
        1,              /* wants_argb */
        1,              /* left_to_right */
        1,              /* wants_dag */
        address,
        blockbeg1,
        blockend1,
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
        0,      /* stabblock */
        stabend,
        0,      /* stabfend */
        0,      /* stabinit */
        stabline,
        0,      /* stabsym */
        0,      /* stabtype */
};

Interface symbolic64IR = {
        1, 1, 0,        /* char */
        2, 2, 0,        /* short */
        4, 4, 0,        /* int */
        8, 8, 0,        /* long */
        4, 4, 1,        /* float */
        8, 8, 1,        /* double */
        8, 8, 1,        /* long double */
        8, 8, 0,        /* T* */
        0, 1, 0,        /* struct */
        1,              /* little_endian */
        0,              /* mulops_calls */
        0,              /* wants_callb */
        1,              /* wants_argb */
        1,              /* left_to_right */
        1,              /* wants_dag */
        address,
        blockbeg1,
        blockend1,
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
        0,      /* stabblock */
        stabend,
        0,      /* stabfend */
        0,      /* stabinit */
        stabline,
        0,      /* stabsym */
        0,      /* stabtype */
};

