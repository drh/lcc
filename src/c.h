#define NEW(p,a) ((p) = allocate(sizeof *(p), (a)))
#define NEW0(p,a) memset(NEW((p),(a)), 0, sizeof *(p))
#ifdef NDEBUG
#define assert(c)
#else
#define assert(c) /*lint -e506 */ \
	((void)((c) || \
	fatal(__FILE__,"assertion failure at line %d\n",__LINE__))) \
	/*lint -restore */
#endif

#define isaddrop(op) \
	((op)==ADDRG+P || (op)==ADDRL+P || (op)==ADDRF+P)

#define	MAXLINE  512
#define	BUFSIZE 4096

#define istypename(t,tsym) (kind[t] == CHAR \
	|| t == ID && tsym && tsym->sclass == TYPEDEF)
#ifdef __LCC__
#ifndef __STDC__
#define __STDC__
#endif
#endif
#ifdef __STDC__
#include <stddef.h>
#include <string.h>
#else
typedef unsigned size_t;
#endif
#ifdef __STDC__
#define ARGS(list) list
#else
#define ARGS(list) ()
#endif
#ifdef __STDC__
#include <stdarg.h>
#define va_init(a,b) va_start(a,b)
#else
#include <varargs.h>
#define va_init(a,b) va_start(a)
#endif
#ifdef __STDC__
#define VARARGS(newlist,oldlist,olddcls) newlist
#else
#define VARARGS(newlist,oldlist,olddcls) oldlist olddcls
#endif
#ifndef __STDC__
#ifndef NULL
#define NULL ((void*)0)
#endif
#endif
#define NELEMS(a) ((int)(sizeof (a)/sizeof ((a)[0])))
#define roundup(x,n) (((x)+((n)-1))&(~((n)-1)))
#define generic(op) ((op)&~15)
#define opindex(op) ((op)>>4)
#define optype(op) ((op)&15)
#define isqual(t)     ((t)->op >= CONST)
#define unqual(t)     (isqual(t) ? (t)->type : (t))

#define isvolatile(t) ((t)->op == VOLATILE \
                    || (t)->op == CONST+VOLATILE)
#define isconst(t)    ((t)->op == CONST \
                    || (t)->op == CONST+VOLATILE)
#define isarray(t)    (unqual(t)->op == ARRAY)
#define isstruct(t)   (unqual(t)->op == STRUCT \
                    || unqual(t)->op == UNION)
#define isunion(t)    (unqual(t)->op == UNION)
#define isfunc(t)     (unqual(t)->op == FUNCTION)
#define isptr(t)      (unqual(t)->op == POINTER)
#define ischar(t)     (unqual(t)->op == CHAR)
#define isint(t)      (unqual(t)->op >= CHAR \
                    && unqual(t)->op <= UNSIGNED)
#define isfloat(t)    (unqual(t)->op <= DOUBLE)
#define isarith(t)    (unqual(t)->op <= UNSIGNED)
#define isunsigned(t) (unqual(t)->op == UNSIGNED)
#define isdouble(t)   (unqual(t)->op == DOUBLE)
#define isscalar(t)   (unqual(t)->op <= POINTER \
                    || unqual(t)->op == ENUM)
#define isenum(t)     (unqual(t)->op == ENUM)
#define fieldsize(p)  (p)->bitsize
#define fieldright(p) ((p)->lsb - 1)
#define fieldleft(p)  (8*(p)->type->size - \
                        fieldsize(p) - fieldright(p))
#define fieldmask(p)  (~(~(unsigned)0<<fieldsize(p)))
#define widen(t) (isint(t) || isenum(t) ? INT : ttob(t))
typedef struct node *Node;

typedef struct list *List;

typedef struct code *Code;

typedef struct swtch *Swtch;

typedef struct symbol *Symbol;

typedef struct coord {
	char *file;
	unsigned x, y;
} Coordinate;
typedef struct table *Table;

typedef union value {
	/* signed */ char sc;
	short ss;
	int i;
	unsigned char uc;
	unsigned short us;
	unsigned int u;
	float f;
	double d;
	void *p;
} Value;
typedef struct tree *Tree;

typedef struct type *Type;

typedef struct field *Field;

typedef struct {
	unsigned printed:1;
	unsigned marked:1;
	unsigned short typeno;
} Xtype;

#include "config.h"
typedef struct metrics {
	unsigned char size, align, outofline;
} Metrics;
typedef struct interface {
	Metrics charmetric;
	Metrics shortmetric;
	Metrics intmetric;
	Metrics floatmetric;
	Metrics doublemetric;
	Metrics ptrmetric;
	Metrics structmetric;
	unsigned little_endian:1;
	unsigned mulops_calls:1;
	unsigned wants_callb:1;
	unsigned wants_argb:1;
	unsigned left_to_right:1;
	unsigned wants_dag:1;

void (*address) ARGS((Symbol p, Symbol q, int n));
void (*blockbeg) ARGS((Env *));
void (*blockend) ARGS((Env *));
void (*defaddress) ARGS((Symbol));
void (*defconst)   ARGS((int ty, Value v));
void (*defstring) ARGS((int n, char *s));
void (*defsymbol) ARGS((Symbol));
void (*emit)     ARGS((Node));
void (*export) ARGS((Symbol));
void (*function) ARGS((Symbol, Symbol[], Symbol[], int));
Node (*gen)      ARGS((Node));
void (*global) ARGS((Symbol));
void (*import) ARGS((Symbol));
void (*local) ARGS((Symbol));
void (*progbeg) ARGS((int argc, char *argv[]));
void (*progend) ARGS((void));
void (*segment) ARGS((int));
void (*space) ARGS((int));
void (*stabblock) ARGS((int, int, Symbol*));
void (*stabend)   ARGS((Coordinate *, Symbol, Coordinate **,                   	Symbol *, Symbol *));
void (*stabfend)  ARGS((Symbol, int));
void (*stabinit)  ARGS((char *, int, char *[]));
void (*stabline)  ARGS((Coordinate *));
void (*stabsym)   ARGS((Symbol));
void (*stabtype)  ARGS((Symbol));
	Xinterface x;
} Interface;
typedef struct binding {
	char *name;
	Interface *ir;
} Binding;

extern Binding bindings[];
extern Interface *IR;
typedef struct {
	List entry;
	List exit;
	List returns;
	List points;
	List calls;
	List end;
} Events;

enum {
#define xx(a,b,c,d,e,f,g) a=b,
#define yy(a,b,c,d,e,f,g)
#include "token.h"
	LAST
};
struct node {
	short op;
	short count;
 	Symbol syms[3];
	Node kids[2];
	Node link;
	Xnode x;
};
enum {
	F=FLOAT,
	D=DOUBLE,
	C=CHAR,
	S=SHORT,
	I=INT,
	U=UNSIGNED,
	P=POINTER,
	V=VOID,
	B=STRUCT
};
enum { CNST=1<<4,
       	CNSTC=CNST+C,
       	CNSTD=CNST+D,
       	CNSTF=CNST+F,
       	CNSTI=CNST+I,
       	CNSTP=CNST+P,
       	CNSTS=CNST+S,
       	CNSTU=CNST+U,
       ARG=2<<4,
       	ARGB=ARG+B,
       	ARGD=ARG+D,
       	ARGF=ARG+F,
       	ARGI=ARG+I,
       	ARGP=ARG+P,
       ASGN=3<<4,
       	ASGNB=ASGN+B,
       	ASGNC=ASGN+C,
       	ASGND=ASGN+D,
       	ASGNF=ASGN+F,
       	ASGNI=ASGN+I,
       	ASGNS=ASGN+S,
       	ASGNP=ASGN+P,
       INDIR=4<<4,
       	INDIRB=INDIR+B,
       	INDIRC=INDIR+C,
       	INDIRD=INDIR+D,
       	INDIRF=INDIR+F,
       	INDIRI=INDIR+I,
       	INDIRS=INDIR+S,
       	INDIRP=INDIR+P,
       CVC=5<<4,
       	CVCI=CVC+I,
       	CVCU=CVC+U,
       CVD=6<<4,
       	CVDF=CVD+F,
       	CVDI=CVD+I,
       CVF=7<<4,
       	CVFD=CVF+D,
       CVI=8<<4,
       	CVIC=CVI+C,
       	CVID=CVI+D,
       	CVIS=CVI+S,
       	CVIU=CVI+U,
       CVP=9<<4,
       	CVPU=CVP+U,
       CVS=10<<4,
       	CVSI=CVS+I,
       	CVSU=CVS+U,
       CVU=11<<4,
       	CVUC=CVU+C,
       	CVUI=CVU+I,
       	CVUP=CVU+P,
       	CVUS=CVU+S,
       NEG=12<<4,
       	NEGD=NEG+D,
       	NEGF=NEG+F,
       	NEGI=NEG+I,
       CALL=13<<4,
       	CALLB=CALL+B,
       	CALLD=CALL+D,
       	CALLF=CALL+F,
       	CALLI=CALL+I,
       	CALLV=CALL+V,
       LOAD=14<<4,
       	LOADB=LOAD+B,
       	LOADC=LOAD+C,
       	LOADD=LOAD+D,
       	LOADF=LOAD+F,
       	LOADI=LOAD+I,
       	LOADP=LOAD+P,
       	LOADS=LOAD+S,
       	LOADU=LOAD+U,
       RET=15<<4,
       	RETD=RET+D,
       	RETF=RET+F,
       	RETI=RET+I,
       ADDRG=16<<4,
       	ADDRGP=ADDRG+P,
       ADDRF=17<<4,
       	ADDRFP=ADDRF+P,
       ADDRL=18<<4,
       	ADDRLP=ADDRL+P,
       ADD=19<<4,
       	ADDD=ADD+D,
       	ADDF=ADD+F,
       	ADDI=ADD+I,
       	ADDP=ADD+P,
       	ADDU=ADD+U,
       SUB=20<<4,
       	SUBD=SUB+D,
       	SUBF=SUB+F,
       	SUBI=SUB+I,
       	SUBP=SUB+P,
       	SUBU=SUB+U,
       LSH=21<<4,
       	LSHI=LSH+I,
       	LSHU=LSH+U,
       MOD=22<<4,
       	MODI=MOD+I,
       	MODU=MOD+U,
       RSH=23<<4,
       	RSHI=RSH+I,
       	RSHU=RSH+U,
       BAND=24<<4,
       	BANDU=BAND+U,
       BCOM=25<<4,
       	BCOMU=BCOM+U,
       BOR=26<<4,
       	BORU=BOR+U,
       BXOR=27<<4,
       	BXORU=BXOR+U,
       DIV=28<<4,
       	DIVD=DIV+D,
       	DIVF=DIV+F,
       	DIVI=DIV+I,
       	DIVU=DIV+U,
       MUL=29<<4,
       	MULD=MUL+D,
       	MULF=MUL+F,
       	MULI=MUL+I,
       	MULU=MUL+U,
       EQ=30<<4,
       	EQD=EQ+D,
       	EQF=EQ+F,
       	EQI=EQ+I,
       GE=31<<4,
       	GED=GE+D,
       	GEF=GE+F,
       	GEI=GE+I,
       	GEU=GE+U,
       GT=32<<4,
       	GTD=GT+D,
       	GTF=GT+F,
       	GTI=GT+I,
       	GTU=GT+U,
       LE=33<<4,
       	LED=LE+D,
       	LEF=LE+F,
       	LEI=LE+I,
       	LEU=LE+U,
       LT=34<<4,
       	LTD=LT+D,
       	LTF=LT+F,
       	LTI=LT+I,
       	LTU=LT+U,
       NE=35<<4,
       	NED=NE+D,
       	NEF=NE+F,
       	NEI=NE+I,
       JUMP=36<<4,
       	JUMPV=JUMP+V,
       LABEL=37<<4,
       	LABELV=LABEL+V };

enum { CODE=1, BSS, DATA, LIT };
enum { PERM=0, FUNC, STMT };
struct list {
	void *x;
	List link;
};

struct code {
	enum { Blockbeg, Blockend, Local, Address, Defpoint,
	       Label,    Start,    Gen,   Jump,    Switch
	} kind;
	Code prev, next;
	union {
		struct {
			int level;
			Symbol *locals;
			Table identifiers, types;
			Env x;
		} block;
		Code begin;
		Symbol var;

		struct {
			Symbol sym;
			Symbol base;
			int offset;
		} addr;
		struct {
			Coordinate src;
			int point;
		} point; 
		Node forest;
		struct {
			Symbol sym;
			Symbol table;
			Symbol deflab;
			int size;
			int *values;
			Symbol *labels;
		} swtch;

	} u;
};
struct symbol {
	char *name;
	int scope;
	Coordinate src;
	Symbol up;
	List uses;
	int sclass;
	unsigned structarg:1;

	unsigned addressed:1;
	unsigned computed:1;
	unsigned temporary:1;
	unsigned generated:1;
	unsigned defined:1;
	Type type;
	float ref;
	union {
		struct {
			int label;
			Symbol equatedto;
		} l;
		struct {
			unsigned cfields:1;
			unsigned vfields:1;
			Table ftab;		/* omit */
			Field flist;
		} s;
		int value;
		Symbol *idlist;
		struct {
			Value v;
			Symbol loc;
		} c;
		struct {
			Coordinate pt;
			int label;
			int ncalls;
			Symbol *callee;
		} f;
		int seg;
		struct {
			Node cse;
		} t;
	} u;
	Xsymbol x;
#ifdef Ysymbol
		Ysymbol y;
#endif

};
enum { CONSTANTS=1, LABELS, GLOBAL, PARAM, LOCAL };
struct tree {
	int op;
	Type type;
	Tree kids[2];
	Node node;
	union {
		Value v;
		Symbol sym;

		Field field;
	} u;
};
enum {
	AND=38<<4,
	NOT=39<<4,
	OR=40<<4,
	COND=41<<4,
	RIGHT=42<<4,
	FIELD=43<<4
};
struct type {
	int op;
	Type type;
	int align;
	int size;
	union {
		Symbol sym;
		struct {
			unsigned oldstyle:1;
			Type *proto;
		} f;
	} u;
	Xtype x;
#ifdef Ytype		/* omit */
	Ytype y;	/* omit */
#endif			/* omit */
};
struct field {
	char *name;
	Type type;
	int offset;
	short bitsize;
	short lsb;
	Field link;
};
extern int nodecount;
extern Symbol cfunc;
extern Symbol retv;
extern Tree (*optree[]) ARGS((int, Tree, Tree));

extern char kind[];
extern int errcnt;
extern int errlimit;
extern int wflag;
extern Events events;
extern float refinc;

extern unsigned char *cp;
extern unsigned char *limit;
extern int infd;
extern char *firstfile;
extern char *file;
extern char *line;
extern int lineno;
extern int t;
extern char *token;
extern Symbol tsym;
extern Coordinate src;
extern char *bp;
extern int Aflag;
extern int Pflag;
extern Symbol YYnull;
extern Symbol YYcheck;
extern int glevel;
extern int xref;

extern int outfd;
extern int errfd;

extern int ncalled;
extern int npoints;

extern int needconst;
extern struct code codehead;
extern Code codelist;
extern Table stmtlabs;
extern float density;
extern Table constants;
extern Table externals;
extern Table globals;
extern Table identifiers;
extern Table labels;
extern Table types;
extern int level;

extern List loci, symbols;

extern List symbols;

extern Type chartype;
extern Type doubletype;
extern Type floattype;
extern Type inttype;
extern Type longdouble;
extern Type longtype;
extern Type shorttype;
extern Type signedchar;
extern Type unsignedchar;
extern Type unsignedlong;
extern Type unsignedshort;
extern Type unsignedtype;
extern Type voidptype;
extern Type voidtype;
extern void  *allocate ARGS((unsigned long n, unsigned a));
extern void deallocate ARGS((unsigned a));
extern void *newarray
	ARGS((unsigned long m, unsigned long n, unsigned a));
extern void walk      ARGS((Tree e, int tlab, int flab));
extern Node listnodes ARGS((Tree e, int tlab, int flab));
extern Node newnode   ARGS((int op, Node left, Node right,
                      		Symbol p));
extern Tree cvtconst ARGS((Tree));
extern void printdag ARGS((Node, int));
extern void compound ARGS((int, Swtch, int));
extern void defglobal ARGS((Symbol, int));
extern void finalize ARGS((void));
extern void program ARGS((void));

extern Tree addrof ARGS((Tree));
extern Tree asgn ARGS((Symbol, Tree));
extern Tree asgntree ARGS((int, Tree, Tree));
extern Type assign ARGS((Type, Tree));
extern Tree bittree ARGS((int, Tree, Tree));
extern Tree call ARGS((Tree, Type, Coordinate));
extern Tree calltree ARGS((Tree, Type, Tree, Symbol));
extern Tree condtree ARGS((Tree, Tree, Tree));
extern Tree consttree ARGS((unsigned int, Type));
extern Tree eqtree ARGS((int, Tree, Tree));
extern int iscallb ARGS((Tree));
extern Tree shtree ARGS((int, Tree, Tree));
extern void typeerror ARGS((int, Tree, Tree));

extern void test ARGS((int tok, char set[]));
extern void expect ARGS((int tok));
extern void skipto ARGS((int tok, char set[]));
extern void error ARGS((char *, ...));
extern int fatal ARGS((char *, char *, int));
extern void warning ARGS((char *, ...));

typedef void (*Apply) ARGS((void *, void *, void *));
extern void attach ARGS((Apply, void *, List *));
extern void apply ARGS((List event, void *arg1, void *arg2));
extern Tree retype ARGS((Tree p, Type ty));
extern Tree rightkid ARGS((Tree p));
extern int hascall ARGS((Tree p));
extern Type binary ARGS((Type, Type));
extern Tree cast ARGS((Tree, Type));
extern Tree cond ARGS((Tree));
extern Tree expr0 ARGS((int));
extern Tree expr ARGS((int));
extern Tree expr1 ARGS((int));
extern Tree field ARGS((Tree, char *));
extern char *funcname ARGS((Tree));
extern Tree idtree ARGS((Symbol));
extern Tree incr ARGS((int, Tree, Tree));
extern Tree lvalue ARGS((Tree));
extern Tree nullcall ARGS((Type, Symbol, Tree, Tree));
extern Tree pointer ARGS((Tree));
extern Tree rvalue ARGS((Tree));
extern Tree value ARGS((Tree));

extern void defpointer ARGS((Symbol));
extern Type initializer ARGS((Type, int));
extern Tree structexp ARGS((Type, Symbol));
extern void swtoseg ARGS((int));

extern void inputInit ARGS((void));
extern void inputstring ARGS((char *));
extern void fillbuf ARGS((void));
extern void nextline ARGS((void));

extern int getchr ARGS((void));
extern int gettok ARGS((void));

extern void outs ARGS((char *));

extern void print ARGS((char *, ...));

extern void emitcode ARGS((void));
extern void gencode  ARGS((Symbol[], Symbol[]));
extern void fprint ARGS((int fd, char *fmt, ...));
extern void outflush ARGS((void));
extern char *stringf ARGS((char *, ...));
extern List append ARGS((void *x, List list));
extern int  length ARGS((List list));
extern void *ltov  ARGS((List *list, unsigned a));
extern Type typename ARGS((void));
extern void checklab ARGS((Symbol p, void *cl));
extern Type enumdcl ARGS((void));
extern int main ARGS((int, char *[]));
extern Symbol mkstr ARGS((char *));
extern Symbol mksymbol ARGS((int, char *,Type));

extern void outflush ARGS((void));
extern void outputInit ARGS((void));
extern void vfprint ARGS((int, char *, va_list));
extern void vprint ARGS((char *, va_list));

void profInit ARGS((char *));

extern int process ARGS((char *));
extern int findfunc ARGS((char *, char *));
extern int findcount ARGS((char *, int, int));

extern Tree constexpr ARGS((int));
extern int intexpr ARGS((int, int));
extern Tree simplify ARGS((int, Type, Tree, Tree));
extern int ispow2 ARGS((unsigned u));

extern void addlocal ARGS((Symbol));
extern Code code ARGS((int));
extern void definelab ARGS((int));
extern void definept ARGS((Coordinate *));
extern void equatelab ARGS((Symbol, Symbol));
extern Node jump ARGS((int));
extern void retcode ARGS((Tree));
extern void statement ARGS((int, Swtch, int));

extern char * string ARGS((char *str));
extern char *stringn ARGS((char *str, int len));
extern char *stringd ARGS((int n));
extern void use ARGS((Symbol p, Coordinate src));
extern void locus ARGS((Table tp, Coordinate *cp));
extern Symbol constant ARGS((Type, Value));
extern void enterscope ARGS((void));
extern void exitscope ARGS((void));
extern Symbol findlabel ARGS((int));
extern Symbol findtype ARGS((Type));
extern void foreach ARGS((Table, int, void (*)(Symbol, void *), void *));
extern Symbol genident ARGS((int, Type, int));
extern int genlabel ARGS((int));
extern Symbol install ARGS((char *, Table *, int, int));
extern Symbol intconst ARGS((int));
extern Symbol lookup ARGS((char *, Table));
extern Symbol mkstr ARGS((char *));
extern Symbol mksymbol ARGS((int, char *, Type));
extern Symbol newtemp ARGS((int, int));
extern Table table ARGS((Table, int));
extern Symbol temporary ARGS((int, Type, int));
extern char *vtoa ARGS((Type, Value));

void traceInit ARGS((char *));

extern int nodeid ARGS((Tree));
extern char *opname ARGS((int));
extern int *printed ARGS((int));
extern void printtree ARGS((Tree, int));
extern Tree right ARGS((Tree, Tree));
extern Tree root ARGS((Tree));
extern Tree texpr ARGS((Tree (*)(int), int, int));
extern Tree tree ARGS((int, Type, Tree, Tree));

extern int hasproto ARGS((Type));
extern void outtype ARGS((Type));
extern void printdecl  ARGS((Symbol p, Type ty));
extern void printproto ARGS((Symbol p, Symbol args[]));
extern char *typestring ARGS((Type ty, char *id));
extern Field fieldref ARGS((char *name, Type ty));
extern Type array ARGS((Type, int, int));
extern Type atop ARGS((Type));
extern Type btot ARGS((int));
extern void checkfields ARGS((Type));
extern Type compose ARGS((Type, Type));
extern Type deref ARGS((Type));
extern int eqtype ARGS((Type, Type, int));
extern Field extends ARGS((Type, Type));
extern Field fieldlist ARGS((Type));
extern Type freturn ARGS((Type));
extern Type ftype ARGS((Type, Type));
extern Type func ARGS((Type, Type *, int));
extern Field newfield ARGS((char *, Type, Type));
extern Type newstruct ARGS((int, char *));
extern void printtype ARGS((Type, int));
extern Type promote ARGS((Type));
extern Type ptr ARGS((Type));
extern Type qual ARGS((int, Type));
extern void rmtypes ARGS((int));
extern int ttob ARGS((Type));
extern void typeInit ARGS((void));
extern int variadic ARGS((Type));

/* limits */
#ifdef __STDC__
#include <limits.h>
#include <float.h>
#else
/*
 * The magnitudes of the values below are greater than or equal to the minimum
 * permitted by the standard (see Appendix D) and are typical for byte-addressed
 * machines with 32-bit integers. These values are suitable for bootstrapping.
 */
#define CHAR_BIT        8
#define MB_LEN_MAX      1

#define UCHAR_MAX       0xff
#define USHRT_MAX       0xffff
#define UINT_MAX        0xffffffff
#define ULONG_MAX       0xffffffffL

#define CHAR_MAX        SCHAR_MAX
#define SCHAR_MAX       0x7f
#define SHRT_MAX        0x7fff
#define INT_MAX         0x7fffffff
#define LONG_MAX        0x7fffffffL

#define CHAR_MIN        SCHAR_MIN
#define SCHAR_MIN       (-SCHAR_MAX-1)
#define SHRT_MIN        (-SHRT_MAX-1)
#define INT_MIN         (-INT_MAX-1)
#define LONG_MIN        (-LONG_MAX-1)

#define FLT_MAX         1e37
#define DBL_MAX         1e37
#endif

extern void exit ARGS((int));
extern void free ARGS((void *));
extern void *malloc ARGS((size_t));
extern void *memset ARGS((void *, int, size_t));

extern int read ARGS((int, unsigned char *, int));

extern double strtod ARGS((const char *, char **));
extern int close ARGS((int));
extern int creat ARGS((char *, int));
extern void *memcpy ARGS((void *, const void *, size_t));
extern int open ARGS((char *, int));
extern long strtol ARGS((const char *, char **, int));
extern int sprintf ARGS((char *, const char *, ...));
extern char *strchr ARGS((const char *, int));
extern int strcmp ARGS((const char *, const char *));
extern size_t strlen ARGS((const char *));
extern int strncmp ARGS((const char *, const char *, size_t));
extern char *strncpy ARGS((char *, const char *, size_t));
extern int write ARGS((int, char *, int));


