%{
#define INTTMP 0x0100ff00
#define INTVAR 0x40ff0000
#define FLTTMP 0x000f0ff0
#define FLTVAR 0xfff00000

#define INTRET 0x00000004
#define FLTRET 0x00000003

#define readsreg(p) \
	(generic((p)->op)==INDIR && (p)->kids[0]->op==VREG+P)
#define setsrc(d) ((d) && (d)->x.regnode && \
	(d)->x.regnode->set == src->x.regnode->set && \
	(d)->x.regnode->mask&src->x.regnode->mask)

#define relink(a, b) ((b)->x.prev = (a), (a)->x.next = (b))

#include "c.h"
#define NODEPTR_TYPE Node
#define OP_LABEL(p) ((p)->op)
#define LEFT_CHILD(p) ((p)->kids[0])
#define RIGHT_CHILD(p) ((p)->kids[1])
#define STATE_LABEL(p) ((p)->x.state)
static void address     ARGS((Symbol, Symbol, int));
static void blkfetch    ARGS((int, int, int, int));
static void blkloop     ARGS((int, int, int, int, int, int[]));
static void blkstore    ARGS((int, int, int, int));
static void defaddress  ARGS((Symbol));
static void defconst    ARGS((int, Value));
static void defstring   ARGS((int, char *));
static void defsymbol   ARGS((Symbol));
static void doarg       ARGS((Node));
static void emit2       ARGS((Node));
static void export      ARGS((Symbol));
static void clobber     ARGS((Node));
static void function    ARGS((Symbol, Symbol [], Symbol [], int));
static void global      ARGS((Symbol));
static void import      ARGS((Symbol));
static void local       ARGS((Symbol));
static void progbeg     ARGS((int, char **));
static void progend     ARGS((void));
static void segment     ARGS((int));
static void space       ARGS((int));
static void target      ARGS((Node));
extern int      atoi            ARGS((const char *));
static int      bitcount        ARGS((unsigned));
static Symbol   argreg          ARGS((int, int, int, int));

static Symbol ireg[32], freg2[32], d6;
static int tmpregs[] = {3, 9, 10};
static Symbol blkreg;

static int gnum = 8;
static int pic;

static int cseg;
%}
%start stmt
%term ADDD=306 ADDF=305 ADDI=309 ADDP=311 ADDU=310
%term ADDRFP=279
%term ADDRGP=263
%term ADDRLP=295
%term ARGB=41 ARGD=34 ARGF=33 ARGI=37 ARGP=39
%term ASGNB=57 ASGNC=51 ASGND=50 ASGNF=49 ASGNI=53 ASGNP=55 ASGNS=52
%term BANDU=390
%term BCOMU=406
%term BORU=422
%term BXORU=438
%term CALLB=217 CALLD=210 CALLF=209 CALLI=213 CALLV=216
%term CNSTC=19 CNSTD=18 CNSTF=17 CNSTI=21 CNSTP=23 CNSTS=20 CNSTU=22
%term CVCI=85 CVCU=86
%term CVDF=97 CVDI=101
%term CVFD=114
%term CVIC=131 CVID=130 CVIS=132 CVIU=134
%term CVPU=150
%term CVSI=165 CVSU=166
%term CVUC=179 CVUI=181 CVUP=183 CVUS=180
%term DIVD=450 DIVF=449 DIVI=453 DIVU=454
%term EQD=482 EQF=481 EQI=485
%term GED=498 GEF=497 GEI=501 GEU=502
%term GTD=514 GTF=513 GTI=517 GTU=518
%term INDIRB=73 INDIRC=67 INDIRD=66 INDIRF=65 INDIRI=69 INDIRP=71 INDIRS=68
%term JUMPV=584
%term LABELV=600
%term LED=530 LEF=529 LEI=533 LEU=534
%term LOADB=233 LOADC=227 LOADD=226 LOADF=225 LOADI=229 LOADP=231 LOADS=228 LOADU=230
%term LSHI=341 LSHU=342
%term LTD=546 LTF=545 LTI=549 LTU=550
%term MODI=357 MODU=358
%term MULD=466 MULF=465 MULI=469 MULU=470
%term NED=562 NEF=561 NEI=565
%term NEGD=194 NEGF=193 NEGI=197
%term RETD=242 RETF=241 RETI=245
%term RSHI=373 RSHU=374
%term SUBD=322 SUBF=321 SUBI=325 SUBP=327 SUBU=326
%term VREGP=615
%%
reg:  INDIRC(VREGP)     "# read register\n"
reg:  INDIRD(VREGP)     "# read register\n"
reg:  INDIRF(VREGP)     "# read register\n"
reg:  INDIRI(VREGP)     "# read register\n"
reg:  INDIRP(VREGP)     "# read register\n"
reg:  INDIRS(VREGP)     "# read register\n"
stmt: ASGNC(VREGP,reg)  "# write register\n"
stmt: ASGND(VREGP,reg)  "# write register\n"
stmt: ASGNF(VREGP,reg)  "# write register\n"
stmt: ASGNI(VREGP,reg)  "# write register\n"
stmt: ASGNP(VREGP,reg)  "# write register\n"
stmt: ASGNS(VREGP,reg)  "# write register\n"
con: CNSTC  "%a"
con: CNSTI  "%a"
con: CNSTP  "%a"
con: CNSTS  "%a"
con: CNSTU  "%a"
stmt: reg  ""
reg: CVIU(reg)  "%0"  notarget(a)
reg: CVPU(reg)  "%0"  notarget(a)
reg: CVUI(reg)  "%0"  notarget(a)
reg: CVUP(reg)  "%0"  notarget(a)
acon: con     "%0"
acon: ADDRGP  "%a"
addr: ADDI(reg,acon)  "%1($%0)"
addr: ADDU(reg,acon)  "%1($%0)"
addr: ADDP(reg,acon)  "%1($%0)"
addr: acon  "%0"
addr: reg   "($%0)"
addr: ADDRFP  "%a+%F($sp)"
addr: ADDRLP  "%a+%F($sp)"
reg: addr  "la $%c,%0\n"  1
reg: CNSTC  "# reg\n"  range(a, 0, 0)
reg: CNSTS  "# reg\n"  range(a, 0, 0)
reg: CNSTI  "# reg\n"  range(a, 0, 0)
reg: CNSTU  "# reg\n"  range(a, 0, 0)
reg: CNSTP  "# reg\n"  range(a, 0, 0)
stmt: ASGNC(addr,reg)  "sb $%1,%0\n"  1
stmt: ASGNS(addr,reg)  "sh $%1,%0\n"  1
stmt: ASGNI(addr,reg)  "sw $%1,%0\n"  1
stmt: ASGNP(addr,reg)  "sw $%1,%0\n"  1
reg:  INDIRC(addr)     "lb $%c,%0\n"  1
reg:  INDIRS(addr)     "lh $%c,%0\n"  1
reg:  INDIRI(addr)     "lw $%c,%0\n"  1
reg:  INDIRP(addr)     "lw $%c,%0\n"  1
reg: CVCI(INDIRC(addr))  "lb $%c,%0\n"  1
reg: CVSI(INDIRS(addr))  "lh $%c,%0\n"  1
reg: CVCU(INDIRC(addr))  "lbu $%c,%0\n" 1
reg: CVSU(INDIRS(addr))  "lhu $%c,%0\n" 1
reg:  INDIRD(addr)     "l.d $f%c,%0\n"  1
reg:  INDIRF(addr)     "l.s $f%c,%0\n"  1
stmt: ASGND(addr,reg)  "s.d $f%1,%0\n"  1
stmt: ASGNF(addr,reg)  "s.s $f%1,%0\n"  1
reg: DIVI(reg,reg)  "div $%c,$%0,$%1\n"   1
reg: DIVU(reg,reg)  "divu $%c,$%0,$%1\n"  1
reg: MODI(reg,reg)  "rem $%c,$%0,$%1\n"   1
reg: MODU(reg,reg)  "remu $%c,$%0,$%1\n"  1
reg: MULI(reg,reg)  "mul $%c,$%0,$%1\n"   1
reg: MULU(reg,reg)  "mul $%c,$%0,$%1\n"   1
rc:  con            "%0"
rc:  reg            "$%0"

reg: ADDI(reg,rc)   "addu $%c,$%0,%1\n"  1
reg: ADDP(reg,rc)   "addu $%c,$%0,%1\n"  1
reg: ADDU(reg,rc)   "addu $%c,$%0,%1\n"  1
reg: BANDU(reg,rc)  "and $%c,$%0,%1\n"   1
reg: BORU(reg,rc)   "or $%c,$%0,%1\n"    1
reg: BXORU(reg,rc)  "xor $%c,$%0,%1\n"   1
reg: SUBI(reg,rc)   "subu $%c,$%0,%1\n"  1
reg: SUBP(reg,rc)   "subu $%c,$%0,%1\n"  1
reg: SUBU(reg,rc)   "subu $%c,$%0,%1\n"  1
rc5: CNSTI          "%a"                range(a,0,31)
rc5: reg            "$%0"

reg: LSHI(reg,rc5)  "sll $%c,$%0,%1\n"  1
reg: LSHU(reg,rc5)  "sll $%c,$%0,%1\n"  1
reg: RSHI(reg,rc5)  "sra $%c,$%0,%1\n"  1
reg: RSHU(reg,rc5)  "srl $%c,$%0,%1\n"  1
reg: BCOMU(reg)  "not $%c,$%0\n"   1
reg: NEGI(reg)   "negu $%c,$%0\n"  1
reg: LOADC(reg)  "move $%c,$%0\n"  move(a)
reg: LOADS(reg)  "move $%c,$%0\n"  move(a)
reg: LOADI(reg)  "move $%c,$%0\n"  move(a)
reg: LOADP(reg)  "move $%c,$%0\n"  move(a)
reg: LOADU(reg)  "move $%c,$%0\n"  move(a)
reg: ADDD(reg,reg)  "add.d $f%c,$f%0,$f%1\n"  1
reg: ADDF(reg,reg)  "add.s $f%c,$f%0,$f%1\n"  1
reg: DIVD(reg,reg)  "div.d $f%c,$f%0,$f%1\n"  1
reg: DIVF(reg,reg)  "div.s $f%c,$f%0,$f%1\n"  1
reg: MULD(reg,reg)  "mul.d $f%c,$f%0,$f%1\n"  1
reg: MULF(reg,reg)  "mul.s $f%c,$f%0,$f%1\n"  1
reg: SUBD(reg,reg)  "sub.d $f%c,$f%0,$f%1\n"  1
reg: SUBF(reg,reg)  "sub.s $f%c,$f%0,$f%1\n"  1
reg: LOADD(reg)     "mov.d $f%c,$f%0\n"       move(a)
reg: LOADF(reg)     "mov.s $f%c,$f%0\n"       move(a)
reg: NEGD(reg)      "neg.d $f%c,$f%0\n"       1
reg: NEGF(reg)      "neg.s $f%c,$f%0\n"       1
reg: CVCI(reg)  "sll $%c,$%0,24; sra $%c,$%c,24\n"  2
reg: CVSI(reg)  "sll $%c,$%0,16; sra $%c,$%c,16\n"  2
reg: CVCU(reg)  "and $%c,$%0,0xff\n"                1
reg: CVSU(reg)  "and $%c,$%0,0xffff\n"              1
reg: CVIC(reg)  "%0"  notarget(a)
reg: CVIS(reg)  "%0"  notarget(a)
reg: CVUC(reg)  "%0"  notarget(a)
reg: CVUS(reg)  "%0"  notarget(a)
reg: CVIC(reg)  "move $%c,$%0\n"  move(a)
reg: CVIS(reg)  "move $%c,$%0\n"  move(a)
reg: CVIU(reg)  "move $%c,$%0\n"  move(a)
reg: CVPU(reg)  "move $%c,$%0\n"  move(a)
reg: CVUC(reg)  "move $%c,$%0\n"  move(a)
reg: CVUI(reg)  "move $%c,$%0\n"  move(a)
reg: CVUP(reg)  "move $%c,$%0\n"  move(a)
reg: CVUS(reg)  "move $%c,$%0\n"  move(a)
reg: CVDF(reg)  "cvt.s.d $f%c,$f%0\n"  1
reg: CVFD(reg)  "cvt.d.s $f%c,$f%0\n"  1
reg: CVID(reg)  "mtc1 $%0,$f%c; cvt.d.w $f%c,$f%c\n"  2
reg: CVDI(reg)  "trunc.w.d $f2,$f%0,$%c; mfc1 $%c,$f2\n"  2
stmt: LABELV  "%a:\n"
stmt: JUMPV(acon)  "b %0\n"   1
stmt: JUMPV(reg)   ".cpadd $%0\nj $%0\n"  !pic
stmt: JUMPV(reg)   "j $%0\n"  pic
stmt: EQI(reg,reg)  "beq $%0,$%1,%a\n"   1
stmt: GEI(reg,reg)  "bge $%0,$%1,%a\n"   1
stmt: GEU(reg,reg)  "bgeu $%0,$%1,%a\n"  1
stmt: GTI(reg,reg)  "bgt $%0,$%1,%a\n"   1
stmt: GTU(reg,reg)  "bgtu $%0,$%1,%a\n"  1
stmt: LEI(reg,reg)  "ble $%0,$%1,%a\n"   1
stmt: LEU(reg,reg)  "bleu $%0,$%1,%a\n"  1
stmt: LTI(reg,reg)  "blt $%0,$%1,%a\n"   1
stmt: LTU(reg,reg)  "bltu $%0,$%1,%a\n"  1
stmt: NEI(reg,reg)  "bne $%0,$%1,%a\n"   1
stmt: EQD(reg,reg)  "c.eq.d $f%0,$f%1; bc1t %a\n"  2
stmt: EQF(reg,reg)  "c.eq.s $f%0,$f%1; bc1t %a\n"  2
stmt: LED(reg,reg)  "c.le.d $f%0,$f%1; bc1t %a\n"  2
stmt: LEF(reg,reg)  "c.le.s $f%0,$f%1; bc1t %a\n"  2
stmt: LTD(reg,reg)  "c.lt.d $f%0,$f%1; bc1t %a\n"  2
stmt: LTF(reg,reg)  "c.lt.s $f%0,$f%1; bc1t %a\n"  2
stmt: GED(reg,reg)  "c.lt.d $f%0,$f%1; bc1f %a\n"  2
stmt: GEF(reg,reg)  "c.lt.s $f%0,$f%1; bc1f %a\n"  2
stmt: GTD(reg,reg)  "c.le.d $f%0,$f%1; bc1f %a\n"  2
stmt: GTF(reg,reg)  "c.le.s $f%0,$f%1; bc1f %a\n"  2
stmt: NED(reg,reg)  "c.eq.d $f%0,$f%1; bc1f %a\n"  2
stmt: NEF(reg,reg)  "c.eq.s $f%0,$f%1; bc1f %a\n"  2
ar:   ADDRGP     "%a"

reg:  CALLD(ar)  "jal %0\n"  1
reg:  CALLF(ar)  "jal %0\n"  1
reg:  CALLI(ar)  "jal %0\n"  1
stmt: CALLV(ar)  "jal %0\n"  1
ar: reg    "$%0"
ar: CNSTP  "%a"   range(a, 0, 0x0fffffff)
stmt: RETD(reg)  "# ret\n"  1
stmt: RETF(reg)  "# ret\n"  1
stmt: RETI(reg)  "# ret\n"  1
stmt: ARGD(reg)  "# arg\n"  1
stmt: ARGF(reg)  "# arg\n"  1
stmt: ARGI(reg)  "# arg\n"  1
stmt: ARGP(reg)  "# arg\n"  1

stmt: ARGB(INDIRB(reg))       "# argb %0\n"      1
stmt: ASGNB(reg,INDIRB(reg))  "# asgnb %0 %1\n"  1
%%
static void progend(){}
static void progbeg(argc, argv) int argc; char *argv[]; {
	int i;

	{
		union {
			char c;
			int i;
		} u;
		u.i = 0;
		u.c = 1;
		swap = ((int)(u.i == 1)) != ((int)IR->little_endian);
	}
	print(".set reorder\n");
	pic = !IR->little_endian;
	parseflags(argc, argv);
	for (i = 0; i < argc; i++)
		if (strncmp(argv[i], "-G", 2) == 0)
			gnum = atoi(argv[i] + 2);
		else if (strcmp(argv[i], "-pic=1") == 0
		||       strcmp(argv[i], "-pic=0") == 0)
			pic = argv[i][5]-'0';
	for (i = 0; i < 31; i += 2)
		freg2[i] = mkreg("%d", i, 3, FREG);
	for (i = 0; i < 32; i++)
		ireg[i]  = mkreg("%d", i, 1, IREG);
	ireg[29]->x.name = "sp";
	d6 = mkreg("6", 6, 3, IREG);
	rmap[C] = rmap[S] = rmap[P] = rmap[B] = rmap[U] = rmap[I] =
		mkwildcard(ireg);
	rmap[F] = rmap[D] = mkwildcard(freg2);
	tmask[IREG] = INTTMP; tmask[FREG] = FLTTMP;
	vmask[IREG] = INTVAR; vmask[FREG] = FLTVAR;
	blkreg = mkreg("8", 8, 7, IREG);
}
static void target(p) Node p; {
	assert(p);
	switch (p->op) {
	case CNSTC: case CNSTI: case CNSTS: case CNSTU: case CNSTP:
		if (range(p, 0, 0) == 0) {
			setreg(p, ireg[0]);
			p->x.registered = 1;
		}
		break;
	case CALLV:             rtarget(p, 0, ireg[25]); break;
	case CALLD: case CALLF: rtarget(p, 0, ireg[25]);
	                        setreg(p, freg2[0]);     break;
	case CALLI:             rtarget(p, 0, ireg[25]);
	                        setreg(p, ireg[2]);      break;
	case RETD: case RETF:   rtarget(p, 0, freg2[0]); break;
	case RETI:              rtarget(p, 0, ireg[2]);  break;
	case ARGD: case ARGF: case ARGI: case ARGP: {
		static int ty0;
		int ty = optype(p->op);
		Symbol q;

		q = argreg(p->x.argno, p->syms[2]->u.c.v.i, ty, ty0);
		if (p->x.argno == 0)
			ty0 = ty;
		if (q &&
		!((ty == F || ty == D) && q->x.regnode->set == IREG))
			rtarget(p, 0, q);
		break;
		}
	case ASGNB: rtarget(p->kids[1], 0, blkreg); break;
	case ARGB:  rtarget(p->kids[0], 0, blkreg); break;
	}
}
static void clobber(p) Node p; {
	assert(p);
	switch (p->op) {
	case CALLD: case CALLF:
		spill(INTTMP | INTRET, IREG, p);
		spill(FLTTMP,          FREG, p);
		break;
	case CALLI:
		spill(INTTMP,          IREG, p);
		spill(FLTTMP | FLTRET, FREG, p);
		break;
	case CALLV:
		spill(INTTMP | INTRET, IREG, p);
		spill(FLTTMP | FLTRET, FREG, p);
		break;
	}
}
static void emit2(p) Node p; {
	int dst, n, src, ty;
	static int ty0;
	Symbol q;

	switch (p->op) {
	case ARGD: case ARGF: case ARGI: case ARGP:
		ty = optype(p->op);
		if (p->x.argno == 0)
			ty0 = ty;
		q = argreg(p->x.argno, p->syms[2]->u.c.v.i, ty, ty0);
		src = getregnum(p->x.kids[0]);
		if (q == NULL && ty == F)
			print("s.s $f%d,%d($sp)\n", src, p->syms[2]->u.c.v.i);
		else if (q == NULL && ty == D)
			print("s.d $f%d,%d($sp)\n", src, p->syms[2]->u.c.v.i);
		else if (q == NULL)
			print("sw $%d,%d($sp)\n", src, p->syms[2]->u.c.v.i);
		else if (ty == F && q->x.regnode->set == IREG)
			print("mfc1 $%d,$f%d\n", q->x.regnode->number, src);
		else if (ty == D && q->x.regnode->set == IREG)
			print("mfc1.d $%d,$f%d\n", q->x.regnode->number, src);
		break;
	case ASGNB:
		dalign = salign = p->syms[1]->u.c.v.i;
		blkcopy(getregnum(p->x.kids[0]), 0,
			getregnum(p->x.kids[1]), 0,
			p->syms[0]->u.c.v.i, tmpregs);
		break;
	case ARGB:
		dalign = 4;
		salign = p->syms[1]->u.c.v.i;
		blkcopy(29, p->syms[2]->u.c.v.i,
			getregnum(p->x.kids[0]), 0,
			p->syms[0]->u.c.v.i, tmpregs);
		n   = p->syms[2]->u.c.v.i + p->syms[0]->u.c.v.i;
		dst = p->syms[2]->u.c.v.i;
		for ( ; dst <= 12 && dst < n; dst += 4)
			print("lw $%d,%d($sp)\n", (dst/4)+4, dst);
		break;
	}
}
static Symbol argreg(argno, offset, ty, ty0)
int argno, offset, ty, ty0; {
	assert((offset&3) == 0);
	if (offset > 12)
		return NULL;
	else if (argno == 0 && (ty == F || ty == D))
		return freg2[12];
	else if (argno == 1 && (ty == F || ty == D)
	&& (ty0 == F || ty0 == D))
		return freg2[14];
	else if (argno == 1 && ty == D)
		return d6;  /* Pair! */
	else
		return ireg[(offset/4) + 4];
}
static void doarg(p) Node p; {
	static int argno;
	int size;

	if (argoffset == 0)
		argno = 0;
	p->x.argno = argno++;
	size = p->syms[1]->u.c.v.i < 4 ? 4 : p->syms[1]->u.c.v.i;
	p->syms[2] = intconst(mkactual(size,
		p->syms[0]->u.c.v.i));
}
static void local(p) Symbol p; {
	if (askregvar(p, rmap[ttob(p->type)]) == 0)
		mkauto(p);
}
static void function(f, caller, callee, ncalls)
Symbol f, callee[], caller[]; int ncalls; {
	int i, saved, sizefsave, sizeisave, varargs;
	Symbol r, argregs[4];

	usedmask[0] = usedmask[1] = 0;
	freemask[0] = freemask[1] = ~(unsigned)0;
	offset = maxoffset = maxargoffset = 0;
	for (i = 0; callee[i]; i++)
		;
	varargs = variadic(f->type)
		|| i > 0 && strcmp(callee[i-1]->name, "va_alist") == 0;
	for (i = 0; callee[i]; i++) {
		Symbol p = callee[i];
		Symbol q = caller[i];
		assert(q);
		offset = roundup(offset, q->type->align);
		p->x.offset = q->x.offset = offset;
		p->x.name = q->x.name = stringd(offset);
		r = argreg(i, offset, ttob(q->type), ttob(caller[0]->type));
		if (i < 4)
			argregs[i] = r;
		offset = roundup(offset + q->type->size, 4);
		if (varargs)
			p->sclass = AUTO;
		else if (r && ncalls == 0 &&
			 !isstruct(q->type) && !p->addressed &&
			 !(isfloat(q->type) && r->x.regnode->set == IREG)
) {
			p->sclass = q->sclass = REGISTER;
			askregvar(p, r);
			assert(p->x.regnode && p->x.regnode->vbl == p);
			q->x = p->x;
			q->type = p->type;
		}
		else if (askregvar(p, rmap[ttob(p->type)])
			 && r != NULL
			 && (isint(p->type) || p->type == q->type)) {
			assert(q->sclass != REGISTER);
			p->sclass = q->sclass = REGISTER;
			q->type = p->type;
		}
	}
	assert(!caller[i]);
	offset = 0;
	gencode(caller, callee);
	if (ncalls)
		usedmask[IREG] |= ((unsigned)1)<<31;
	usedmask[IREG] &= 0xc0ff0000;
	usedmask[FREG] &= 0xfff00000;
	if (pic && ncalls)
		usedmask[IREG] |= 1<<25;
	maxargoffset = roundup(maxargoffset, 4);
	if (ncalls && maxargoffset < 16)
		maxargoffset = 16;
	sizefsave = 4*bitcount(usedmask[FREG]);
	sizeisave = 4*bitcount(usedmask[IREG]);
	framesize = roundup(maxargoffset + sizefsave
		+ sizeisave + maxoffset, 8);
	segment(CODE);
	print(".align 2\n");
	print(".ent %s\n", f->x.name);
	print("%s:\n", f->x.name);
	i = maxargoffset + sizefsave - framesize;
	print(".frame $sp,%d,$31\n", framesize);
	if (pic)
		print(".set noreorder\n.cpload $25\n.set reorder\n");
	if (framesize > 0)
		print("addu $sp,$sp,%d\n", -framesize);
	if (usedmask[FREG])
		print(".fmask 0x%x,%d\n", usedmask[FREG], i - 8);
	if (usedmask[IREG])
		print(".mask 0x%x,%d\n",  usedmask[IREG],
			i + sizeisave - 4);
	saved = maxargoffset;
	for (i = 20; i <= 30; i += 2)
		if (usedmask[FREG]&(3<<i)) {
			print("s.d $f%d,%d($sp)\n", i, saved);
			saved += 8;
		}
	for (i = 16; i <= 31; i++)
		if (usedmask[IREG]&(1<<i)) {
			if (i == 25)
				print(".cprestore %d\n", saved);
			else
				print("sw $%d,%d($sp)\n", i, saved);
			saved += 4;
		}
	for (i = 0; i < 4 && callee[i]; i++) {
		r = argregs[i];
		if (r && r->x.regnode != callee[i]->x.regnode) {
			Symbol out = callee[i];
			Symbol in  = caller[i];
			int rn = r->x.regnode->number;
			int rs = r->x.regnode->set;
			int tyin = ttob(in->type);

			assert(out && in && r && r->x.regnode);
			assert(out->sclass != REGISTER || out->x.regnode);
			if (out->sclass == REGISTER
			&& (isint(out->type) || out->type == in->type)) {
				int outn = out->x.regnode->number;
				if (rs == FREG && tyin == D)
					print("mov.d $f%d,$f%d\n", outn, rn);
				else if (rs == FREG && tyin == F)
					print("mov.s $f%d,$f%d\n", outn, rn);
				else if (rs == IREG && tyin == D)
					print("mtc1.d $%d,$f%d\n", rn,   outn);
				else if (rs == IREG && tyin == F)
					print("mtc1 $%d,$f%d\n",   rn,   outn);
				else
					print("move $%d,$%d\n",    outn, rn);
			} else {
				int off = in->x.offset + framesize;
				if (rs == FREG && tyin == D)
					print("s.d $f%d,%d($sp)\n", rn, off);
				else if (rs == FREG && tyin == F)
					print("s.s $f%d,%d($sp)\n", rn, off);
				else {
					int i, n = (in->type->size + 3)/4;
					for (i = rn; i < rn+n && i <= 7; i++)
						print("sw $%d,%d($sp)\n", i, off + (i-rn)*4);
				}
			}
		}
	}
	if (varargs && callee[i-1]) {
		i = callee[i-1]->x.offset + callee[i-1]->type->size;
		for (i = roundup(i, 4)/4; i <= 3; i++)
			print("sw $%d,%d($sp)\n", i + 4, framesize + 4*i);
		}
	emitcode();
	saved = maxargoffset;
	for (i = 20; i <= 30; i += 2)
		if (usedmask[FREG]&(3<<i)) {
			print("l.d $f%d,%d($sp)\n", i, saved);
			saved += 8;
		}
	for (i = 16; i <= 31; i++)
		if (usedmask[IREG]&(1<<i)) {
			print("lw $%d,%d($sp)\n", i, saved);
			saved += 4;
		}
	if (framesize > 0)
		print("addu $sp,$sp,%d\n", framesize);
	print("j $31\n");
	print(".end %s\n", f->x.name);
}
static void defconst(ty, v) int ty; Value v; {
	switch (ty) {
	case C: print(".byte %d\n",   v.uc); return;
	case S: print(".half %d\n",   v.ss); return;
	case I: print(".word 0x%x\n", v.i);  return;
	case U: print(".word 0x%x\n", v.u);  return;
	case P: print(".word 0x%x\n", v.p); return;
	case F: print(".word 0x%x\n", *(unsigned *)&v.f); return;
	case D: {
		unsigned *p = (unsigned *)&v.d;
		print(".word 0x%x\n.word 0x%x\n", p[swap], p[!swap]);
		return;
		}
	}
	assert(0);
}
static void defaddress(p) Symbol p; {
	if (pic && p->scope == LABELS)
		print(".gpword %s\n", p->x.name);
	else
		print(".word %s\n", p->x.name);
}
static void defstring(n, str) int n; char *str; {
	char *s;

	for (s = str; s < str + n; s++)
		print(".byte %d\n", (*s)&0377);
}
static void export(p) Symbol p; {
	print(".globl %s\n", p->x.name);
}
static void import(p) Symbol p; {
	if (!isfunc(p->type))
		print(".extern %s %d\n", p->name, p->type->size);
}
static void defsymbol(p) Symbol p; {
	if (p->scope >= LOCAL && p->sclass == STATIC)
		p->x.name = stringf("L.%d", genlabel(1));
	else if (p->generated)
		p->x.name = stringf("L.%s", p->name);
	else
		assert(p->scope != CONSTANTS || isint(p->type) || isptr(p->type)),
		p->x.name = p->name;
}
static void address(q, p, n) Symbol q, p; int n; {
	q->x.offset = p->x.offset + n;
	if (p->scope == GLOBAL
	|| p->sclass == STATIC || p->sclass == EXTERN)
		q->x.name = stringf("%s%s%d", p->x.name,
			n >= 0 ? "+" : "", n);
	else
		q->x.name = stringd(q->x.offset);
}
static void global(p) Symbol p; {
	if (p->u.seg == BSS) {
		if (p->sclass == STATIC || Aflag >= 2)
			print(".lcomm %s,%d\n", p->x.name, p->type->size);
		else
			print( ".comm %s,%d\n", p->x.name, p->type->size);
	} else {
		if (p->u.seg == DATA
		&& (p->type->size == 0 || p->type->size > gnum))
			print(".data\n");
		else if (p->u.seg == DATA)
			print(".sdata\n");
		print(".align %c\n", ".01.2...3"[p->type->align]);
		print("%s:\n", p->x.name);
	}
}
static void segment(n) int n; {
	cseg = n;
	switch (n) {
	case CODE: print(".text\n");  break;
	case LIT:  print(".rdata\n"); break;
	}
}
static void space(n) int n; {
	if (cseg != BSS)
		print(".space %d\n", n);
}
static void blkloop(dreg, doff, sreg, soff, size, tmps)
int dreg, doff, sreg, soff, size, tmps[]; {
	int lab = genlabel(1);

	print("addu $%d,$%d,%d\n", sreg, sreg, size&~7);
	print("addu $%d,$%d,%d\n", tmps[2], dreg, size&~7);
	blkcopy(tmps[2], doff, sreg, soff, size&7, tmps);
	print("L.%d:\n", lab);
	print("addu $%d,$%d,%d\n", sreg, sreg, -8);
	print("addu $%d,$%d,%d\n", tmps[2], tmps[2], -8);
	blkcopy(tmps[2], doff, sreg, soff, 8, tmps);
	print("bltu $%d,$%d,L.%d\n", dreg, tmps[2], lab);
}
static void blkfetch(size, off, reg, tmp)
int size, off, reg, tmp; {
	assert(size == 1 || size == 2 || size == 4);
	if (size == 1)
		print("lbu $%d,%d($%d)\n",  tmp, off, reg);
	else if (salign >= size && size == 2)
		print("lhu $%d,%d($%d)\n",  tmp, off, reg);
	else if (salign >= size)
		print("lw $%d,%d($%d)\n",   tmp, off, reg);
	else if (size == 2)
		print("ulhu $%d,%d($%d)\n", tmp, off, reg);
	else
		print("ulw $%d,%d($%d)\n",  tmp, off, reg);
}
static void blkstore(size, off, reg, tmp)
int size, off, reg, tmp; {
	if (size == 1)
		print("sb $%d,%d($%d)\n",  tmp, off, reg);
	else if (dalign >= size && size == 2)
		print("sh $%d,%d($%d)\n",  tmp, off, reg);
	else if (dalign >= size)
		print("sw $%d,%d($%d)\n",  tmp, off, reg);
	else if (size == 2)
		print("ush $%d,%d($%d)\n", tmp, off, reg);
	else
		print("usw $%d,%d($%d)\n", tmp, off, reg);
}
static void stabinit ARGS((char *, int, char *[]));
static void stabline ARGS((Coordinate *));
static void stabsym ARGS((Symbol));

static char *currentfile;

static int bitcount(mask) unsigned mask; {
	unsigned i, n = 0;

	for (i = 1; i; i <<= 1)
		if (mask&i)
			n++;
	return n;
}

/* stabinit - initialize stab output */
static void stabinit(file, argc, argv) int argc; char *file, *argv[]; {
	if (file) {
		print(".file 2,\"%s\"\n", file);
		currentfile = file;
	}
}

/* stabline - emit stab entry for source coordinate *cp */
static void stabline(cp) Coordinate *cp; {
	if (cp->file && cp->file != currentfile) {
		print(".file 2,\"%s\"\n", cp->file);
		currentfile = cp->file;
	}
	print(".loc 2,%d\n", cp->y);
}

/* stabsym - output a stab entry for symbol p */
static void stabsym(p) Symbol p; {
	if (p == cfunc && IR->stabline)
		(*IR->stabline)(&p->src);
}
Interface mipsebIR = {
	1, 1, 0,  /* char */
	2, 2, 0,  /* short */
	4, 4, 0,  /* int */
	4, 4, 1,  /* float */
	8, 8, 1,  /* double */
	4, 4, 0,  /* T * */
	0, 1, 0,  /* struct */
	0,	/* little_endian */
	0,  /* mulops_calls */
	0,  /* wants_callb */
	1,  /* wants_argb */
	1,  /* left_to_right */
	0,  /* wants_dag */
address,
blockbeg,
blockend,
defaddress,
defconst,
defstring,
defsymbol,
emit,
export,
function,
gen,
global,
import,
local,
progbeg,
progend,
segment,
space,
	0, 0, 0, stabinit, stabline, stabsym, 0,
	{
		4,	/* max_unaligned_load */
		blkfetch, blkstore, blkloop,
		_label,
		_rule,
		_nts,
		_kids,
		_opname,
		_arity,
		_string,
		_templates,
		_isinstruction,
		_ntname,
		emit2,
		doarg,
		target,
		clobber,

	}
}, mipselIR = {
	1, 1, 0,  /* char */
	2, 2, 0,  /* short */
	4, 4, 0,  /* int */
	4, 4, 1,  /* float */
	8, 8, 1,  /* double */
	4, 4, 0,  /* T * */
	0, 1, 0,  /* struct */
	1,	/* little_endian */
	0,  /* mulops_calls */
	0,  /* wants_callb */
	1,  /* wants_argb */
	1,  /* left_to_right */
	0,  /* wants_dag */
address,
blockbeg,
blockend,
defaddress,
defconst,
defstring,
defsymbol,
emit,
export,
function,
gen,
global,
import,
local,
progbeg,
progend,
segment,
space,
	0, 0, 0, stabinit, stabline, stabsym, 0,
	{
		4,	/* max_unaligned_load */
		blkfetch, blkstore, blkloop,
		_label,
		_rule,
		_nts,
		_kids,
		_opname,
		_arity,
		_string,
		_templates,
		_isinstruction,
		_ntname,
		emit2,
		doarg,
		target,
		clobber,

	}
};
