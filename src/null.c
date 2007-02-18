#include "c.h"

static Node gen0(Node p) { return p; }
static void address(Symbol q, Symbol p, int n) {}
static void blockbeg0(Env *e) {}
static void blockend0(Env *e) {}
static void defaddress(Symbol p) {}
static void defconst(int suffix, int size, Value v) {}
static void defstring(int len, char *s) {}
static void defsymbol(Symbol p) {}
static void emit0(Node p) {}
static void export(Symbol p) {}
static void function(Symbol f, Symbol caller[], Symbol callee[], int ncalls) {}
static void global(Symbol p) {}
static void import(Symbol p) {}
static void local(Symbol p) {}
static void progbeg(int argc, char *argv[]) {}
static void progend(void) {}
static void segment(int s) {}
static void space(int n) {}

static char rcsid[] = "$Id$";

Interface nullIR = {
        1, 1, 0,        /* char */
        2, 2, 0,        /* short */
        4, 4, 0,        /* int */
        8, 8, 1,        /* long */
        4, 4, 1,        /* float */
        8, 8, 1,        /* double */
        16,16,1,        /* long double */
        4, 4, 0,        /* T* */
        0, 4, 0,        /* struct */
        1,              /* little_endian */
        0,              /* mulops_calls */
        0,              /* wants_callb */
        0,              /* wants_argb */
        1,              /* left_to_right */
        0,              /* wants_dag */
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

