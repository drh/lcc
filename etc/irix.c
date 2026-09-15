/* SGI big endian MIPSes running IRIX 5.2 at CS Dept., Princeton University */

#include <string.h>

static char rcsid[] = "$Id$";

#ifndef LCCDIR
#define LCCDIR "/usr/local/lib/lcc/"
#endif

char *cpp[] = { LCCDIR "cpp", "-D__STDC__=1",
	"-DLANGUAGE_C",
	"-DMIPSEB",
	"-DSYSTYPE_SVR4",
	"-D_CFE",
	"-D_LANGUAGE_C",
	"-D_MIPSEB",
	"-D_MIPS_FPSET=16",
	"-D_MIPS_ISA=_MIPS_ISA_MIPS1",
	"-D_MIPS_SIM=_MIPS_SIM_ABI32",
	"-D_MIPS_SZINT=32",
	"-D_MIPS_SZLONG=32",
	"-D_MIPS_SZPTR=32",
	"-D_SGI_SOURCE",
	"-D_SVR4_SOURCE",
	"-D_SYSTYPE_SVR4",
	"-D__host_mips",
	"-D__mips=1",
	"-D__sgi",
	"-D__unix",
	"-Dhost_mips",
	"-Dmips",
	"-Dsgi",
	"-Dunix",
	"$1", "$2", "$3", 0 };
char *com[] =  { LCCDIR "rcc", "-target=mips/irix", "$1", "$2", "$3", "", 0 };
char *include[] = { "-I" LCCDIR "include", "-I/usr/local/include",
	"-I/usr/include", 0 };
char *as[] = { "/usr/bin/as", "-o", "$3", "$1", "-nocpp", "-KPIC", "$2", 0 };
char *ld[] = { "/usr/bin/ld", "-require_dynamic_link", "_rld_new_interface",
	"-elf", "-_SYSTYPE_SVR4", "-Wx,-G", "0", "-g0", "-KPIC",
	"-o", "$3", "/usr/lib/crt1.o",
	"$1", "$2", "", "-lc", "/usr/lib/crtn.o", 0
};
static char *bbexit = LCCDIR "bbexit.o";

extern char *concat(char *, char *);
extern int access(const char *, int);

int option(char *arg) {
	if (strncmp(arg, "-lccdir=", 8) == 0) {
		cpp[0] = concat(&arg[8], "/cpp");
		include[0] = concat("-I", concat(&arg[8], "/include"));
		com[0] = concat(&arg[8], "/rcc");
		bbexit = concat(&arg[8], "/bbexit.o");
	} else if (strcmp(arg, "-g4") == 0
	&& access("/u/drh/lib/mipseb/rcc", 4) == 0
	&& access("/u/drh/book/cdb/mips/irix/cdbld", 4) == 0) {
		com[0] = "/u/drh/lib/mipseb/rcc";
		com[5] = "-g4";
		ld[0] = "/u/drh/book/cdb/mips/irix/cdbld";
		ld[1] = "-o";
		ld[2] = "$3";
		ld[3] = "$1";
		ld[4] = "$2";
		ld[5] = 0;
	} else if (strcmp(arg, "-g") == 0)
		;
	else if (strcmp(arg, "-p") == 0)
		ld[11] = "/usr/lib/mcrt1.o";
	else if (strcmp(arg, "-b") == 0 && access(bbexit, 4) == 0)
		ld[14] = bbexit;
	else
		return 0;
	return 1;
}
