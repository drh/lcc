/* DEC little endian MIPSes running ULTRIX V4.3
at CS Dept., Princeton University */

#include <string.h>

static char rcsid[] = "$Id: ultrix.c,v 1.10 1997/06/11 23:00:24 drh Exp $";

#ifndef LCCDIR
#define LCCDIR "/usr/local/lib/lcc/"
#endif

char *suffixes[] = { ".c", ".i", ".s", ".o", ".out", 0 };
char inputs[256] = "";
char *cpp[] = { LCCDIR "cpp", "-D__STDC__=1",
	"-DLANGUAGE_C", "-D_LANGUAGE_C", "-D__LANGUAGE_C",
	"-D_unix", "-D__unix", "-Dunix", "-Dultrix", "-D_ultrix", "-D__ultrix",
	"-Dmips", "-D_mips", "-D__mips",
	"-Dhost_mips", "-D_host_mips", "-D__host_mips",
	"-DMIPSEL", "-D_MIPSEL", "-D__MIPSEL",
	"$1", "$2", "$3", 0 };
char *com[] =  { LCCDIR "rcc", "-target=mips/ultrix", "$1", "$2", "$3", "", 0 };
char *include[] = { "-I" LCCDIR "include", "-I/usr/local/include",
	"-I/usr/include", 0 };
char *as[] =  { "/bin/as", "-o", "$3", "", "$1", "-nocpp", "-EL", "$2", 0 };
char *ld[] =  { "/usr/lib/cmplrs/c89/ld", "-o", "$3",
	"-nocount", "/usr/lib/cmplrs/c89/crt0.o", "-count",
	"$1", "$2", "", "", "-nocount", "-L" LCCDIR, "-llcc", "-lm", "-lcP", "-lc", 0 };

extern char *concat(char *, char *);

int option(char *arg) {
	if (strncmp(arg, "-lccdir=", 8) == 0) {
		cpp[0] = concat(&arg[8], "/cpp");
		include[0] = concat("-I", concat(&arg[8], "/include"));
		com[0] = concat(&arg[8], "/rcc");
		ld[11] = concat("-L", &arg[8]);
	} else if (strcmp(arg, "-g") == 0)
		as[3] = "-g";
	else if (strcmp(arg, "-p") == 0
	&& strcmp(ld[4], "/usr/lib/cmplrs/c89/crt0.o") == 0)
		ld[4] = "/usr/lib/cmplrs/c89/pcrt0.o";
	else if (strcmp(arg, "-b") == 0)
		;
	else
		return 0;
	return 1;
}
