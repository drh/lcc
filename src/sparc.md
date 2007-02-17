%{
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
static int imm ARGS((Node));
static void rename ARGS((void));
static void defsymbol2 ARGS((Symbol));
static void global2 ARGS((Symbol));
static void segment2 ARGS((int));

static Symbol greg[32];
static Symbol *oreg = &greg[8], *ireg = &greg[24];
static Symbol freg[32], freg2[32];

static int retstruct;

static int pflag = 0;

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
reg: ADDRGP  "set %a,%%%c\n"  1
stk13: ADDRFP  "%a"                  imm(a)
stk13: ADDRLP  "%a"                  imm(a)
reg:   stk13   "add %0,%%fp,%%%c\n"  1
stk: ADDRFP  "set %a,%%%c\n"                      2
stk: ADDRLP  "set %a,%%%c\n"                      2
reg: ADDRFP  "set %a,%%%c\nadd %%%c,%%fp,%%%c\n"  3
reg: ADDRLP  "set %a,%%%c\nadd %%%c,%%fp,%%%c\n"  3
con13: CNSTC  "%a"  imm(a)
con13: CNSTI  "%a"  imm(a)
con13: CNSTP  "%a"  imm(a)
con13: CNSTS  "%a"  imm(a)
con13: CNSTU  "%a"  imm(a)
base: ADDI(reg,con13)  "%%%0+%1"
base: ADDP(reg,con13)  "%%%0+%1"
base: ADDU(reg,con13)  "%%%0+%1"
base: reg    "%%%0"
base: con13  "%0"
base: stk13  "%%fp+%0"
addr: base           "%0"
addr: ADDI(reg,reg)  "%%%0+%%%1"
addr: ADDP(reg,reg)  "%%%0+%%%1"
addr: ADDU(reg,reg)  "%%%0+%%%1"
addr: stk            "%%fp+%%%0"
reg:  INDIRC(addr)     "ldsb [%0],%%%c\n"  1
reg:  INDIRS(addr)     "ldsh [%0],%%%c\n"  1
reg:  INDIRI(addr)     "ld [%0],%%%c\n"    1
reg:  INDIRP(addr)     "ld [%0],%%%c\n"    1
reg:  INDIRF(addr)     "ld [%0],%%f%c\n"   1
stmt: ASGNC(addr,reg)  "stb %%%1,[%0]\n"   1
stmt: ASGNS(addr,reg)  "sth %%%1,[%0]\n"   1
stmt: ASGNI(addr,reg)  "st %%%1,[%0]\n"    1
stmt: ASGNP(addr,reg)  "st %%%1,[%0]\n"    1
stmt: ASGNF(addr,reg)  "st %%f%1,[%0]\n"   1
addrl: ADDRLP            "%%%fp+%a"          imm(a)

reg:   INDIRD(addrl)     "ldd [%0],%%f%c\n"  1
stmt:  ASGND(addrl,reg)  "std %%f%1,[%0]\n"  1
reg:  INDIRD(base)     "ld2 [%0],%%f%c\n"  2
stmt: ASGND(base,reg)  "st2 %%f%1,[%0]\n"  2
spill:  ADDRLP          "%a" !imm(a)

stmt: ASGNC(spill,reg)  "set %0,%%g1\nstb %%%1,[%%fp+%%g1]\n"
stmt: ASGNS(spill,reg)  "set %0,%%g1\nsth %%%1,[%%fp+%%g1]\n"
stmt: ASGNI(spill,reg)  "set %0,%%g1\nst %%%1,[%%fp+%%g1]\n"
stmt: ASGNP(spill,reg)  "set %0,%%g1\nst %%%1,[%%fp+%%g1]\n"
stmt: ASGNF(spill,reg)  "set %0,%%g1\nst %%f%1,[%%fp+%%g1]\n"
stmt: ASGND(spill,reg)  "set %0,%%g1\nstd %%f%1,[%%fp+%%g1]\n"
reg: CVCI(INDIRC(addr))  "ldsb [%0],%%%c\n"  1
reg: CVSI(INDIRS(addr))  "ldsh [%0],%%%c\n"  1
reg: CVCU(INDIRC(addr))  "ldub [%0],%%%c\n"  1
reg: CVSU(INDIRS(addr))  "lduh [%0],%%%c\n"  1
reg: CVIC(reg)  "mov %%%0,%%%c\n"  move(a)
reg: CVIS(reg)  "mov %%%0,%%%c\n"  move(a)
reg: CVIU(reg)  "mov %%%0,%%%c\n"  move(a)
reg: CVPU(reg)  "mov %%%0,%%%c\n"  move(a)
reg: CVUC(reg)  "mov %%%0,%%%c\n"  move(a)
reg: CVUI(reg)  "mov %%%0,%%%c\n"  move(a)
reg: CVUP(reg)  "mov %%%0,%%%c\n"  move(a)
reg: CVUS(reg)  "mov %%%0,%%%c\n"  move(a)
reg: CVIC(reg)  "%0"  notarget(a)
reg: CVIS(reg)  "%0"  notarget(a)
reg: CVUC(reg)  "%0"  notarget(a)
reg: CVUS(reg)  "%0"  notarget(a)
reg: LOADC(reg)  "mov %%%0,%%%c\n"  move(a)
reg: LOADI(reg)  "mov %%%0,%%%c\n"  move(a)
reg: LOADP(reg)  "mov %%%0,%%%c\n"  move(a)
reg: LOADS(reg)  "mov %%%0,%%%c\n"  move(a)
reg: LOADU(reg)  "mov %%%0,%%%c\n"  move(a)
reg: CNSTC  "# reg\n"  range(a, 0, 0)
reg: CNSTI  "# reg\n"  range(a, 0, 0)
reg: CNSTP  "# reg\n"  range(a, 0, 0)
reg: CNSTS  "# reg\n"  range(a, 0, 0)
reg: CNSTU  "# reg\n"  range(a, 0, 0)
reg: con  "set %0,%%%c\n"  1
rc: con13  "%0"
rc: reg    "%%%0"
reg: ADDI(reg,rc)   "add %%%0,%1,%%%c\n"  1
reg: ADDP(reg,rc)   "add %%%0,%1,%%%c\n"  1
reg: ADDU(reg,rc)   "add %%%0,%1,%%%c\n"  1
reg: BANDU(reg,rc)  "and %%%0,%1,%%%c\n"  1
reg: BORU(reg,rc)   "or %%%0,%1,%%%c\n"   1
reg: BXORU(reg,rc)  "xor %%%0,%1,%%%c\n"  1
reg: SUBI(reg,rc)   "sub %%%0,%1,%%%c\n"  1
reg: SUBP(reg,rc)   "sub %%%0,%1,%%%c\n"  1
reg: SUBU(reg,rc)   "sub %%%0,%1,%%%c\n"  1
rc5: CNSTI  "%a"    range(a, 0, 31)
rc5: reg    "%%%0"
reg: LSHI(reg,rc5)  "sll %%%0,%1,%%%c\n"  1
reg: LSHU(reg,rc5)  "sll %%%0,%1,%%%c\n"  1
reg: RSHI(reg,rc5)  "sra %%%0,%1,%%%c\n"  1
reg: RSHU(reg,rc5)  "srl %%%0,%1,%%%c\n"  1
reg: BANDU(reg,BCOMU(rc))  "andn %%%0,%1,%%%c\n"  1
reg: BORU(reg,BCOMU(rc))   "orn %%%0,%1,%%%c\n"   1
reg: BXORU(reg,BCOMU(rc))  "xnor %%%0,%1,%%%c\n"  1
reg: NEGI(reg)   "neg %%%0,%%%c\n"  1
reg: BCOMU(reg)  "not %%%0,%%%c\n"  1
reg: CVCI(reg)  "sll %%%0,24,%%%c; sra %%%c,24,%%%c\n"  2
reg: CVSI(reg)  "sll %%%0,16,%%%c; sra %%%c,16,%%%c\n"  2
reg: CVCU(reg)  "and %%%0,0xff,%%%c\n"                   1
reg: CVSU(reg)  "set 0xffff,%%g1; and %%%0,%%g1,%%%c\n"  2
addrg: ADDRGP        "%a"
stmt:  JUMPV(addrg)  "ba %0; nop\n"   2
stmt:  JUMPV(addr)   "jmp %0; nop\n"  2
stmt:  LABELV        "%a:\n"
stmt: EQI(reg,rc)  "cmp %%%0,%1; be %a; nop\n"    3
stmt: GEI(reg,rc)  "cmp %%%0,%1; bge %a; nop\n"   3
stmt: GEU(reg,rc)  "cmp %%%0,%1; bgeu %a; nop\n"  3
stmt: GTI(reg,rc)  "cmp %%%0,%1; bg %a; nop\n"    3
stmt: GTU(reg,rc)  "cmp %%%0,%1; bgu %a; nop\n"   3
stmt: LEI(reg,rc)  "cmp %%%0,%1; ble %a; nop\n"   3
stmt: LEU(reg,rc)  "cmp %%%0,%1; bleu %a; nop\n"  3
stmt: LTI(reg,rc)  "cmp %%%0,%1; bl %a; nop\n"    3
stmt: LTU(reg,rc)  "cmp %%%0,%1; blu %a; nop\n"   3
stmt: NEI(reg,rc)  "cmp %%%0,%1; bne %a; nop\n"   3
call: ADDRGP           "%a"
call: addr             "%0"
reg:  CALLD(call)      "call %0; nop\n"                2
reg:  CALLF(call)      "call %0; nop\n"                2
reg:  CALLI(call)      "call %0; nop\n"                2
stmt: CALLV(call)      "call %0; nop\n"                2
stmt: CALLB(call,reg)  "call %0; st %%%1,[%%sp+64]; unimp %b&0xfff\n"  3

stmt: RETD(reg)  "# ret\n"  1
stmt: RETF(reg)  "# ret\n"  1
stmt: RETI(reg)  "# ret\n"  1
stmt: ARGI(reg)  "st %%%0,[%%sp+4*%c+68]\n"  1
stmt: ARGP(reg)  "st %%%0,[%%sp+4*%c+68]\n"  1
stmt: ARGD(reg)  "# ARGD\n"  1
stmt: ARGF(reg)  "# ARGF\n"  1

reg: DIVI(reg,rc)   "sra %%%0,31,%%g1; wr %%g0,%%g1,%%y; nop; nop; nop; sdiv %%%0,%1,%%%c\n"       6

reg: DIVU(reg,rc)   "wr %%g0,%%g0,%%y; nop; nop; nop; udiv %%%0,%1,%%%c\n"       5

reg: MODI(reg,rc)   "sra %%%0,31,%%g1; wr %%g0,%%g1,%%y; nop; nop; nop; sdiv %%%0,%1,%%g1\n; smul %%g1,%1,%%g1; sub %%%0,%%g1,%%%c\n"  8


reg: MODU(reg,rc)   "wr %%g0,%%g0,%%y; nop; nop; nop; udiv %%%0,%1,%%g1\n; umul %%g1,%1,%%g1; sub %%%0,%%g1,%%%c\n"  7


reg: MULI(rc,reg)   "smul %%%1,%0,%%%c\n"  1
reg: MULU(rc,reg)   "umul %%%1,%0,%%%c\n"  1
reg: ADDD(reg,reg)  "faddd %%f%0,%%f%1,%%f%c\n"  1
reg: ADDF(reg,reg)  "fadds %%f%0,%%f%1,%%f%c\n"  1
reg: DIVD(reg,reg)  "fdivd %%f%0,%%f%1,%%f%c\n"  1
reg: DIVF(reg,reg)  "fdivs %%f%0,%%f%1,%%f%c\n"  1
reg: MULD(reg,reg)  "fmuld %%f%0,%%f%1,%%f%c\n"  1
reg: MULF(reg,reg)  "fmuls %%f%0,%%f%1,%%f%c\n"  1
reg: SUBD(reg,reg)  "fsubd %%f%0,%%f%1,%%f%c\n"  1
reg: SUBF(reg,reg)  "fsubs %%f%0,%%f%1,%%f%c\n"  1
reg: NEGF(reg)   "fnegs %%f%0,%%f%c\n"  1
reg: LOADF(reg)  "fmovs %%f%0,%%f%c\n"  1
reg: CVDF(reg)   "fdtos %%f%0,%%f%c\n"  1
reg: CVFD(reg)   "fstod %%f%0,%%f%c\n"  1
reg: CVDI(reg)  "fdtoi %%f%0,%%f0; st %%f0,[%%sp+64]; ld [%%sp+64],%%%c\n"  3

reg: CVID(reg)  "st %%%0,[%%sp+64]; ld [%%sp+64],%%f%c; fitod %%f%c,%%f%c\n"  3

rel: EQD(reg,reg)  "fcmped %%f%0,%%f%1; nop; fbue"
rel: EQF(reg,reg)  "fcmpes %%f%0,%%f%1; nop; fbue"
rel: GED(reg,reg)  "fcmped %%f%0,%%f%1; nop; fbuge"
rel: GEF(reg,reg)  "fcmpes %%f%0,%%f%1; nop; fbuge"
rel: GTD(reg,reg)  "fcmped %%f%0,%%f%1; nop; fbug"
rel: GTF(reg,reg)  "fcmpes %%f%0,%%f%1; nop; fbug"
rel: LED(reg,reg)  "fcmped %%f%0,%%f%1; nop; fbule"
rel: LEF(reg,reg)  "fcmpes %%f%0,%%f%1; nop; fbule"
rel: LTD(reg,reg)  "fcmped %%f%0,%%f%1; nop; fbul"
rel: LTF(reg,reg)  "fcmpes %%f%0,%%f%1; nop; fbul"
rel: NED(reg,reg)  "fcmped %%f%0,%%f%1; nop; fbne"
rel: NEF(reg,reg)  "fcmpes %%f%0,%%f%1; nop; fbne"

stmt: rel  "%0 %a; nop\n"  4
reg:  LOADD(reg)  "# LOADD\n"  2

reg:  NEGD(reg)  "# NEGD\n"  2

stmt:  ASGNB(reg,INDIRB(reg))  "# ASGNB\n"

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
	parseflags(argc, argv);
	for (i = 0; i < argc; i++)
		if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "-pg") == 0)
			pflag = 1;
	for (i = 0; i < 8; i++) {
		greg[i +  0] = mkreg(stringf("g%d", i), i +  0, 1, IREG);
		greg[i +  8] = mkreg(stringf("o%d", i), i +  8, 1, IREG);
		greg[i + 16] = mkreg(stringf("l%d", i), i + 16, 1, IREG);
		greg[i + 24] = mkreg(stringf("i%d", i), i + 24, 1, IREG);
	}
	for (i = 0; i < 32; i++)
		freg[i]  = mkreg("%d", i, 1, FREG);
	for (i = 0; i < 31; i += 2)
		freg2[i] = mkreg("%d", i, 3, FREG);
	rmap[C] = rmap[S] = rmap[P] = rmap[B] = rmap[U] = rmap[I] =
		mkwildcard(greg);
	rmap[F] = mkwildcard(freg);
	rmap[D] = mkwildcard(freg2);
	tmask[IREG] = 0x3fff3e00;
	vmask[IREG] = 0x3ff00000;
	tmask[FREG]  = ~(unsigned)0;
	vmask[FREG]  = 0;
}
static void target(p) Node p; {
	assert(p);
	switch (p->op) {
	case CNSTC: case CNSTI: case CNSTS: case CNSTU: case CNSTP:
		if (range(p, 0, 0) == 0) {
			setreg(p, greg[0]);
			p->x.registered = 1;
		}
		break;
	case CALLB:
		assert(p->syms[1] && p->syms[1]->type && isfunc(p->syms[1]->type));
		p->syms[1] = intconst(freturn(p->syms[1]->type)->size);
		break;
	case CALLD: setreg(p, freg2[0]);     break;
	case CALLF: setreg(p, freg[0]);      break;
	case CALLI:
	case CALLV: setreg(p, oreg[0]);      break;
	case RETD:  rtarget(p, 0, freg2[0]); break;
	case RETF:  rtarget(p, 0, freg[0]);  break;
	case RETI:
		rtarget(p, 0, ireg[0]);
		p->kids[0]->x.registered = 1;
		break;
	case ARGI: case ARGP:
		if (p->syms[RX]->u.c.v.i < 6) {
			rtarget(p, 0, oreg[p->syms[RX]->u.c.v.i]);
			p->op = LOAD+optype(p->op);
			setreg(p, oreg[p->syms[RX]->u.c.v.i]);
		}
		break;
	}
}
static void clobber(p) Node p; {
	assert(p);
	switch (p->op) {
	case CALLB: case CALLD: case CALLF: case CALLI:
		spill(~(unsigned)3, FREG, p);
		break;
	case CALLV:
		spill(oreg[0]->x.regnode->mask, IREG, p);
		spill(~(unsigned)3, FREG, p);
		break;
	case ARGF:
		if (p->syms[2]->u.c.v.i <= 6)
			spill((1<<(p->syms[2]->u.c.v.i + 8)), IREG, p);
		break;
	case ARGD:
		if (p->syms[2]->u.c.v.i <= 5)
			spill((3<<(p->syms[2]->u.c.v.i + 8))&0xff00, IREG, p);
		break;
	}
}
static int imm(p) Node p; {
	return range(p, -4096, 4095);
}
static void doarg(p) Node p; {
	assert(p && p->syms[0] && p->op != ARG+B);
	p->syms[RX] = intconst(mkactual(4,
		p->syms[0]->u.c.v.i)/4);
}
static void emit2(p) Node p; {
	switch (p->op) {
	case ARGF: {
		int n = p->syms[RX]->u.c.v.i;
		print("st %%f%d,[%%sp+4*%d+68]\n",
			getregnum(p->x.kids[0]), n);
		if (n <= 5)
			print("ld [%%sp+4*%d+68],%%o%d\n", n, n);
		break;
	}
	case ARGD: {
		int n = p->syms[RX]->u.c.v.i;
		int src = getregnum(p->x.kids[0]);
		print("st %%f%d,[%%sp+4*%d+68]\n", src, n);
		print("st %%f%d,[%%sp+4*%d+68]\n", src+1, n+1);
		if (n <= 5)
			print("ld [%%sp+4*%d+68],%%o%d\n", n, n);
		if (n <= 4)
			print("ld [%%sp+4*%d+68],%%o%d\n", n+1, n+1);
		break;
	}
	case LOADD: {
		int dst = getregnum(p);
		int src = getregnum(p->x.kids[0]);
		print("fmovs %%f%d,%%f%d; ", src,   dst);
		print("fmovs %%f%d,%%f%d\n", src+1, dst+1);
		break;
	}
	case NEGD: {
		int dst = getregnum(p);
		int src = getregnum(p->x.kids[0]);
		print("fnegs %%f%d,%%f%d; ", src,   dst);
		print("fmovs %%f%d,%%f%d\n", src+1, dst+1);
		break;
	}
	case ASGNB: {
		static int tmpregs[] = { 1, 2, 3 };
		dalign = salign = p->syms[1]->u.c.v.i;
		blkcopy(getregnum(p->x.kids[0]), 0,
		        getregnum(p->x.kids[1]), 0,
		        p->syms[0]->u.c.v.i, tmpregs);
		break;
	}
	}
}
static void local(p) Symbol p; {
	if (retstruct) {
		p->x.name = stringd(4*16);
		p->x.offset = 4*16;
		retstruct = 0;
		return;
	}
	if (isscalar(p->type) && !p->addressed && !isfloat(p->type))
		p->sclass = REGISTER;
	if (glevel && glevel != 3) p->sclass = AUTO;	/* glevel!=3 is for Norman's debugger */
	if (askregvar(p, rmap[ttob(p->type)]) == 0)
		mkauto(p);
}
static void function(f, caller, callee, ncalls)
Symbol f, callee[], caller[]; int ncalls; {
	int autos = 0, i, leaf, reg, varargs;

	for (i = 0; callee[i]; i++)
		;
	varargs = variadic(f->type)
		|| i > 0 && strcmp(callee[i-1]->name,
			"__builtin_va_alist") == 0;
usedmask[0] = usedmask[1] = 0;
freemask[0] = freemask[1] = ~(unsigned)0;
	for (i = 0; i < 8; i++)
		ireg[i]->x.regnode->vbl = NULL;
	offset = 68;
	maxargoffset = 24;
	reg = 0;
	for (i = 0; callee[i]; i++) {
		Symbol p = callee[i], q = caller[i];
		int size = roundup(q->type->size, 4);
		assert(q);
		if (isfloat(p->type) || reg >= 6) {
			p->x.offset = q->x.offset = offset;
			p->x.name = q->x.name = stringd(offset);
			p->sclass = q->sclass = AUTO;
			autos++;
		}
		else if (p->addressed || varargs)
			{
				p->x.offset = offset;
				p->x.name = stringd(p->x.offset);
				p->sclass = AUTO;
				q->sclass = REGISTER;
				askregvar(q, ireg[reg]);
				assert(q->x.regnode);
				autos++;
			}
		else if (glevel && glevel != 3) {
							p->x.offset = offset;
							p->x.name = stringd(p->x.offset);
							p->sclass = AUTO;
							q->sclass = REGISTER;
							askregvar(q, ireg[reg]);
							assert(q->x.regnode);
							autos++;
						}

		else {
			p->sclass = q->sclass = REGISTER;
			askregvar(p, ireg[reg]);
			assert(p->x.regnode);
			q->x.name = p->x.name;
		}
		offset += size;
		reg += isstruct(p->type) ? 1 : size/4;
	}
	assert(caller[i] == 0);
	offset = maxoffset = 0;
	retstruct = isstruct(freturn(f->type));
	gencode(caller, callee);
	maxargoffset = roundup(maxargoffset, 4);
	framesize = roundup(maxoffset + maxargoffset + 4*(16+1), 8);
	assert(!varargs || autos);
	leaf = (!ncalls
		&& !maxoffset && !autos
		&& !isstruct(freturn(f->type))
		&& !(usedmask[IREG]&0x00ffff01)
		&& !(usedmask[FREG]&~(unsigned)3)
		&& !pflag && !glevel);
	print(".align 4\n.proc 4\n%s:\n", f->x.name);
	if (leaf) {
		for (i = 0; caller[i] && callee[i]; i++) {
			Symbol p = caller[i], q = callee[i];
			if (p->sclass == REGISTER && q->sclass == REGISTER)
				assert(q->x.regnode),
				assert(q->x.regnode->set == IREG),
				assert(q->x.regnode->number >= 24),
				assert(q->x.regnode->number <= 31),
				p->x.name = greg[q->x.regnode->number - 16]->x.name;
			}
		rename();
	} else if (framesize <= 4095)
		print("save %%sp,%d,%%sp\n", -framesize);
	else
		print("set %d,%%g1; save %%sp,%%g1,%%sp\n", -framesize);
	if (varargs)
		for (; reg < 6; reg++)
			print("st %%i%d,[%%fp+%d]\n", reg, 4*reg + 68);
	else
		offset = 4*(16 + 1);
		reg = 0;
		for (i = 0; caller[i]; i++) {
			Symbol p = caller[i];
			if (isdouble(p->type) && reg <= 4) {
				print("st %%r%d,[%%fp+%d]\n",
					ireg[reg++]->x.regnode->number, offset);
				print("st %%r%d,[%%fp+%d]\n",
					ireg[reg++]->x.regnode->number, offset + 4);
			} else if (isfloat(p->type) && reg <= 5)
				print("st %%r%d,[%%fp+%d]\n",
					ireg[reg++]->x.regnode->number, offset);
			else
				reg++;
			offset += roundup(p->type->size, 4);
		}
if (pflag) {
	int lab = genlabel(1);
	print("set L%d,%%o0; call mcount; nop\n", lab);
	print(".seg \"data\"\n.align 4; L%d:.word 0\n.seg \"text\"\n", lab);
}
	emitcode();
	if (isstruct(freturn(f->type)))
		print("jmp %%i7+12; restore\n");
	else if (!leaf)
		print("ret; restore\n");
	else {
		rename();
		print("retl; nop\n");
	}
}
#define exch(x, y, t) (((t) = x), ((x) = (y)), ((y) = (t)))

static void rename() {
	int i;

	for (i = 0; i < 8; i++) {
		char *ptmp;
		int itmp;
		if (ireg[i]->x.regnode->vbl)
			ireg[i]->x.regnode->vbl->x.name = oreg[i]->x.name;
		exch(ireg[i]->x.name, oreg[i]->x.name, ptmp);
		exch(ireg[i]->x.regnode->number,
			oreg[i]->x.regnode->number, itmp);
	}
}
static void defconst(ty, v) int ty; Value v; {
	switch (ty) {
	case C: print(".byte %d\n",   v.uc); return;
	case S: print(".half %d\n",   v.ss); return;
	case I: print(".word %d\n",   v.i ); return;
	case U: print(".word 0x%x\n", v.u ); return;
	case P: print(".word 0x%x\n", v.p ); return;
	case F:
		print(".word 0x%x\n", *(unsigned *)&v.f);
		return;
case D: {
	unsigned *p = (unsigned *)&v.d;
	print(".word 0x%x\n.word 0x%x\n", p[swap], p[!swap]);
	return;
	}
	}
	assert(0);
}

static void defaddress(p) Symbol p; {
	print(".word %s\n", p->x.name);
}

static void defstring(n, str) int n; char *str; {
	char *s;

	for (s = str; s < str + n; s++)
		print(".byte %d\n", (*s)&0377);
}

static void address(q, p, n) Symbol q, p; int n; {
	q->x.offset = p->x.offset + n;
	if (p->scope == GLOBAL || p->sclass == STATIC || p->sclass == EXTERN)
		q->x.name = stringf("%s%s%d", p->x.name, n >= 0 ? "+" : "", n);
	else
		q->x.name = stringd(q->x.offset);
}
static void export(p) Symbol p; {
	print(".global %s\n", p->x.name);
}
static void import(p) Symbol p; {}
static void defsymbol(p) Symbol p; {
	if (p->scope >= LOCAL && p->sclass == STATIC)
		p->x.name = stringf("%d", genlabel(1));
	else
		assert(p->scope != CONSTANTS || isint(p->type) || isptr(p->type)),
		p->x.name = p->name;
	if (p->scope >= LABELS)
		p->x.name = stringf(p->generated ? "L%s" : "_%s",
			p->x.name);
}
static void segment(n) int n; {
	cseg = n;
	switch (n) {
	case CODE: print(".seg \"text\"\n"); break;
	case BSS:  print(".seg \"bss\"\n");  break;
	case DATA: print(".seg \"data\"\n"); break;
	case LIT:  print(".seg \"text\"\n"); break;
	}
}
static void space(n) int n; {
	if (cseg != BSS)
		print(".skip %d\n", n);
}
static void global(p) Symbol p; {
	print(".align %d\n", p->type->align);
	assert(p->u.seg);
	if (p->u.seg == BSS
	&& (p->sclass == STATIC || Aflag >= 2))
		print(".reserve %s,%d\n", p->x.name, p->type->size);
	else if (p->u.seg == BSS)
		print(".common %s,%d\n",  p->x.name, p->type->size);
	else
		print("%s:\n", p->x.name);
}
static void blkfetch(k, off, reg, tmp)
int k, off, reg, tmp; {
	assert(k == 1 || k == 2 || k == 4);
	assert(salign >= k);
	if (k == 1)
		print("ldub [%%r%d+%d],%%r%d\n", reg, off, tmp);
	else if (k == 2)
		print("lduh [%%r%d+%d],%%r%d\n", reg, off, tmp);
	else
		print("ld [%%r%d+%d],%%r%d\n",   reg, off, tmp);
}
static void blkstore(k, off, reg, tmp)
int k, off, reg, tmp; {
	assert(k == 1 || k == 2 || k == 4);
	assert(dalign >= k);
	if (k == 1)
		print("stb %%r%d,[%%r%d+%d]\n", tmp, reg, off);
	else if (k == 2)
		print("sth %%r%d,[%%r%d+%d]\n", tmp, reg, off);
	else
		print("st %%r%d,[%%r%d+%d]\n",  tmp, reg, off);
}
static void blkloop(dreg, doff, sreg, soff, size, tmps)
int dreg, doff, sreg, soff, size, tmps[]; {
	if ((size&~7) < 4096) {
		print("add %%r%d,%d,%%r%d\n", sreg, size&~7, sreg);
		print("add %%r%d,%d,%%r%d\n", dreg, size&~7, tmps[2]);
	} else {
		print("set %d,%%r%d\n", size&~7, tmps[2]);
		print("add %%r%d,%%r%d,%%r%d\n", sreg, tmps[2], sreg);
		print("add %%r%d,%%r%d,%%r%d\n", dreg, tmps[2], tmps[2]);
	}
	blkcopy(tmps[2], doff, sreg, soff, size&7, tmps);
	print("1: dec 8,%%r%d\n", tmps[2]);
	blkcopy(tmps[2], doff, sreg, soff - 8, 8, tmps);
	print("cmp %%r%d,%%r%d; ", tmps[2], dreg);
	print("bgt 1b; ");
	print("dec 8,%%r%d\n", sreg);
}
static void defsymbol2(p) Symbol p; {
	if (p->scope >= LOCAL && p->sclass == STATIC)
		p->x.name = stringf(".%d", genlabel(1));
	else
		assert(p->scope != CONSTANTS || isint(p->type) || isptr(p->type)),
		p->x.name = p->name;
	if (p->scope >= LABELS)
		p->x.name = stringf(p->generated ? ".L%s" : "%s",
			p->x.name);
}

static void global2(p) Symbol p; {
	assert(p->u.seg);
	if (p->sclass != STATIC && !p->generated)
		print(".type %s,#%s\n", p->x.name,
			isfunc(p->type) ? "function" : "object");
	if (p->u.seg == BSS && p->sclass == STATIC)
		print(".local %s\n.common %s,%d,%d\n", p->x.name, p->x.name,
			p->type->size, p->type->align);
	else if (p->u.seg == BSS && Aflag >= 2)
		print(".align %d\n%s:.skip %d\n", p->type->align, p->x.name,
			p->type->size);
	else if (p->u.seg == BSS)
		print(".common %s,%d,%d\n", p->x.name, p->type->size, p->type->align);
	else
		print(".align %d\n%s:\n", p->type->align, p->x.name);
}

static void segment2(n) int n; {
	cseg = n;
	switch (n) {
	case CODE: print(".section \".text\"\n");   break;
	case BSS:  print(".section \".bss\"\n");    break;
	case DATA: print(".section \".data\"\n");   break;
	case LIT:  print(".section \".rodata\"\n"); break;
	}
}
#ifdef sparc
#include <stab.h>
static char *currentfile;       /* current file name */
static int ntypes;
static int nextlab = 1;
extern Interface solarisIR;

static void stabblock ARGS((int, int, Symbol*));
static void stabfend ARGS((Symbol, int));
static void stabinit ARGS((char *, int, char *[]));
static void stabline ARGS((Coordinate *));
static void stabsym ARGS((Symbol));
static void stabtype ARGS((Symbol));

static void asgncode ARGS((Type, int));
static void dbxout ARGS((Type));
static int dbxtype ARGS((Type));
static int emittype ARGS((Type, int, int));

/* asgncode - assign type code to ty */
static void asgncode(ty, lev) Type ty; int lev; {
	if (ty->x.marked || ty->x.typeno)
		return;
	ty->x.marked = 1;
	switch (ty->op) {
	case VOLATILE: case CONST: case VOLATILE+CONST:
		asgncode(ty->type, lev);
		ty->x.typeno = ty->type->x.typeno;
		break;
	case POINTER: case FUNCTION: case ARRAY:
		asgncode(ty->type, lev + 1);
		/* fall thru */
	case VOID: case CHAR: case SHORT: case INT: case UNSIGNED:
	case FLOAT: case DOUBLE:
		break;
	case STRUCT: case UNION: {
		Field p;
		for (p = fieldlist(ty); p; p = p->link)
			asgncode(p->type, lev + 1);
		/* fall thru */
	case ENUM:
		if (ty->x.typeno == 0)
			ty->x.typeno = ++ntypes;
		if (lev > 0 && (*ty->u.sym->name < '0' || *ty->u.sym->name > '9'))
			dbxout(ty);
		break;
		}
	default:
		assert(0);
	}
}

/* dbxout - output .stabs entry for type ty */
static void dbxout(ty) Type ty; {
	ty = unqual(ty);
	if (!ty->x.printed) {
		int col = 0;
		print(".stabs \""), col += 8;
		if (ty->u.sym && !(isfunc(ty) || isarray(ty) || isptr(ty)))
			print("%s", ty->u.sym->name), col += strlen(ty->u.sym->name);
		print(":%c", isstruct(ty) || isenum(ty) ? 'T' : 't'), col += 2;
		emittype(ty, 0, col);
		print("\",%d,0,0,0\n", N_LSYM);
	}
}

/* dbxtype - emit a stabs entry for type ty, return type code */
static int dbxtype(ty) Type ty; {
	asgncode(ty, 0);
	dbxout(ty);
	return ty->x.typeno;
}

/*
 * emittype - emit ty's type number, emitting its definition if necessary.
 * Returns the output column number after emission; col is the approximate
 * output column before emission and is used to emit continuation lines for long
 * struct, union, and enum types. Continuations are not emitted for other types,
 * even if the definition is long. lev is the depth of calls to emittype.
 */
static int emittype(ty, lev, col) Type ty; int lev, col; {
	int tc = ty->x.typeno;

	if (isconst(ty) || isvolatile(ty)) {
		col = emittype(ty->type, lev, col);
		ty->x.typeno = ty->type->x.typeno;
		ty->x.printed = 1;
		return col;
	}
	if (tc == 0) {
		ty->x.typeno = tc = ++ntypes;
/*              fprint(2,"`%t'=%d\n", ty, tc); */
	}
	print("%d", tc), col += 3;
	if (ty->x.printed)
		return col;
	ty->x.printed = 1;
	switch (ty->op) {
	case VOID:	/* void is defined as itself */
		print("=%d", tc), col += 1+3;
		break;
	case CHAR:	/* unsigned char is a subrange of int */
		if (ty == unsignedchar)
			print("=r1;0;255;"), col += 10;
		else	/* following pcc, char is a subrange of itself */
			print("=r%d;-128;127;", tc), col += 2+3+10;
		break;
	case SHORT:	/* short is a subrange of int */
		if (ty == unsignedshort)
			print("=r1;0;65535;"), col += 12;
		else	/* signed */
			print("=r1;-32768;32767;"), col += 17;
		break;
	case INT:	/* int is a subrange of itself */
		print("=r1;%d;%d;", INT_MIN, INT_MAX), col += 4+11+1+10+1;
		break;
	case UNSIGNED:	/* unsigned is a subrange of int */
		print("=r1;0;-1;"), col += 9;
		break;
	case FLOAT: case DOUBLE:	/* float, double get sizes instead of ranges */
		print("=r1;%d;0;", ty->size), col += 4+1+3;
		break;
	case POINTER:
		print("=*"), col += 2;
		col = emittype(ty->type, lev + 1, col);
		break;
	case FUNCTION:
		print("=f"), col += 2;
		col = emittype(ty->type, lev + 1, col);
		break;
	case ARRAY:	/* array includes subscript as an int range */
		if (ty->size && ty->type->size)
			print("=ar1;0;%d;", ty->size/ty->type->size - 1), col += 7+3+1;
		else
			print("=ar1;0;-1;"), col += 10;
		col = emittype(ty->type, lev + 1, col);
		break;
	case STRUCT: case UNION: {
		Field p;
		if (!ty->u.sym->defined) {
			print("=x%c%s:", ty->op == STRUCT ? 's' : 'u', ty->u.sym->name);
			col += 2+1+strlen(ty->u.sym->name)+1;
			break;
		}
		if (lev > 0 && (*ty->u.sym->name < '0' || *ty->u.sym->name > '9')) {
			ty->x.printed = 0;
			break;
		}
		print("=%c%d", ty->op == STRUCT ? 's' : 'u', ty->size), col += 1+1+3;
		for (p = fieldlist(ty); p; p = p->link) {
			if (p->name)
				print("%s:", p->name), col += strlen(p->name)+1;
			else
				print(":"), col += 1;
			col = emittype(p->type, lev + 1, col);
			if (p->lsb)
				print(",%d,%d;", 8*p->offset +
					(IR->little_endian ? fieldright(p) : fieldleft(p)),
					fieldsize(p));
			else
				print(",%d,%d;", 8*p->offset, 8*p->type->size);
			col += 1+3+1+3+1;	/* accounts for ,%d,%d; */
			if (col >= 80 && p->link) {
				print("\\\\\",%d,0,0,0\n.stabs \"", N_LSYM);
				col = 8;
			}
		}
		print(";"), col += 1;
		break;
		}
	case ENUM: {
		Symbol *p;
		if (lev > 0 && (*ty->u.sym->name < '0' || *ty->u.sym->name > '9')) {
			ty->x.printed = 0;
			break;
		}
		print("=e"), col += 2;
		for (p = ty->u.sym->u.idlist; *p; p++) {
			print("%s:%d,", (*p)->name, (*p)->u.value), col += strlen((*p)->name)+3;
			if (col >= 80 && p[1]) {
				print("\\\\\",%d,0,0,0\n.stabs \"", N_LSYM);
				col = 8;
			}
		}
		print(";"), col += 1;
		break;
		}
	default:
		assert(0);
	}
	return col;
}

/* stabblock - output a stab entry for '{' or '}' at level lev */
static void stabblock(brace, lev, p) int brace, lev; Symbol *p; {
	if (brace == '{')
		while (*p)
			stabsym(*p++);
	if (IR == &solarisIR) {
		print(".stabn 0x%x,0,%d,.LL%d-%s\n", brace == '{' ? N_LBRAC : N_RBRAC, lev, nextlab, cfunc->x.name);
		print(".LL%d:\n", nextlab++);
	} else
		print(".stabd 0x%x,0,%d\n", brace == '{' ? N_LBRAC : N_RBRAC, lev);
}

/* stabfend - end of function p */
static void stabfend(p, line) Symbol p; int line; {
	if (IR == &solarisIR) {
		print(".type %s,#function\n", p->x.name);
		print(".size %s,.-%s\n", p->x.name, p->x.name);
	}
}

/* stabinit - initialize stab output */
static void stabinit(file, argc, argv) int argc; char *file, *argv[]; {
	typedef void (*Closure) ARGS((Symbol, void *));

	if (file && *file) {
		(*IR->segment)(CODE);
		print("Ltext:.stabs \"%s\",0x%x,0,0,Ltext\n", file, N_SO);
		currentfile = file;
	}
	dbxtype(inttype);
	dbxtype(chartype);
	dbxtype(doubletype);
	dbxtype(floattype);
	dbxtype(longdouble);
	dbxtype(longtype);
	dbxtype(shorttype);
	dbxtype(signedchar);
	dbxtype(unsignedchar);
	dbxtype(unsignedlong);
	dbxtype(unsignedshort);
	dbxtype(unsignedtype);
	dbxtype(voidtype);
	foreach(types, GLOBAL, (Closure)stabtype, NULL);
}

/* stabline - emit stab entry for source coordinate *cp */
static void stabline(cp) Coordinate *cp; {
	if (cp->file && cp->file != currentfile) {
		int lab = genlabel(1);
		print("L%d: .stabs \"%s\",0x%x,0,0,L%d\n", lab,
				cp->file, N_SOL, lab);
		currentfile = cp->file;
	}
	if (IR == &solarisIR) {
		print(".stabn 0x%x,0,%d,.LL%d-%s\n", N_SLINE, cp->y, nextlab, cfunc->x.name);
		print(".LL%d:\n", nextlab++);
	} else
		print(".stabd 0x%x,0,%d\n", N_SLINE, cp->y);
}

/* stabsym - output a stab entry for symbol p */
static void stabsym(p) Symbol p; {
	int code, tc, sz = p->type->size;

	if (p->generated || p->computed)
		return;
	if (isfunc(p->type)) {
		print(".stabs \"%s:%c%d\",%d,0,0,%s\n", p->name,
			p->sclass == STATIC ? 'f' : 'F', dbxtype(freturn(p->type)),
			N_FUN, p->x.name);
		return;
	}
	if (!IR->wants_argb && p->scope == PARAM && p->structarg) {
		assert(isptr(p->type) && isstruct(p->type->type));
		tc = dbxtype(p->type->type);
		sz = p->type->type->size;
	} else
		tc = dbxtype(p->type);
	if (p->sclass == AUTO && p->scope == GLOBAL || p->sclass == EXTERN) {
		print(".stabs \"%s:G", p->name);
		code = N_GSYM;
	} else if (p->sclass == STATIC) {
		print(".stabs \"%s:%c%d\",%d,0,0,%s\n", p->name, p->scope == GLOBAL ? 'S' : 'V',
			tc, p->u.seg == BSS ? N_LCSYM : N_STSYM, p->x.name);
		return;
	} else if (p->sclass == REGISTER) {
		if (p->scope > PARAM) {
			int r = p->x.regnode->number;
			if (p->x.regnode->set == FREG)
				r += 32;	/* floating point */
			print(".stabs \"%s:r%d\",%d,0,", p->name, tc, N_RSYM);
			print("%d,%d\n", sz, r);
		}
		return;
	} else if (p->scope == PARAM) {
		print(".stabs \"%s:p", p->name);
		code = N_PSYM;
	} else if (p->scope >= LOCAL) {
		print(".stabs \"%s:", p->name);
		code = N_LSYM;
	} else
		assert(0);
	print("%d\",%d,0,0,%s\n", tc, code,
		p->scope >= PARAM && p->sclass != EXTERN ? p->x.name : "0");
}

/* stabtype - output a stab entry for type *p */
static void stabtype(p) Symbol p; {
	if (p->type) {
		if (p->sclass == 0)
			dbxtype(p->type);
		else if (p->sclass == TYPEDEF)
			print(".stabs \"%s:t%d\",%d,0,0,0\n", p->name, dbxtype(p->type), N_LSYM);
	}
}
#else
#define stabblock 0
#define stabfend 0
#define stabinit 0
#define stabline 0
#define stabsym 0
#define stabtype 0
#endif
Interface sparcIR = {
	1, 1, 0,  /* char */
	2, 2, 0,  /* short */
	4, 4, 0,  /* int */
	4, 4, 1,  /* float */
	8, 8, 1,  /* double */
	4, 4, 0,  /* T * */
	0, 1, 0,  /* struct */
	0,  /* little_endian */
	0,  /* mulops_calls */
	1,  /* wants_callb */
	0,  /* wants_argb */
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
	stabblock, 0, 0, stabinit, stabline, stabsym, stabtype,
	{
		1,  /* max_unaligned_load */
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

Interface solarisIR = {
	1, 1, 0,  /* char */
	2, 2, 0,  /* short */
	4, 4, 0,  /* int */
	4, 4, 1,  /* float */
	8, 8, 1,  /* double */
	4, 4, 0,  /* T * */
	0, 1, 0,  /* struct */
	0,	/* little_endian */
	0,	/* mulops_calls */
	1,	/* wants_callb */
	0,	/* wants_argb */
	1,	/* left_to_right */
	0,	/* wants_dag */
	address,
	blockbeg,
	blockend,
	defaddress,
	defconst,
	defstring,
	defsymbol2,
	emit,
	export,
	function,
	gen,
	global2,
	import,
	local,
	progbeg,
	progend,
	segment2,
	space,
	stabblock, 0, stabfend, stabinit, stabline, stabsym, stabtype,
	{
		1,	/* max_unaligned_load */
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
