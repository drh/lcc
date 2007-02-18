/* DEC little endian MIPSes running ULTRIX V4.3
at CS Dept., Princeton University */

#include <string.h>

static char rcsid[] = "$Id$";

#ifndef LCCDIR
#define LCCDIR "/usr/local/lib/lcc/"
#endif

char *cpp[] = { LCCDIR "cpp", "-D__STDC__=1",
	"-DLANGUAGE_C", "-D_LANGUAGE_C", "-D__LANGUAGE_C",
	"-D_unix", "-D__unix", "-Dultrix", "-D_ultrix", "-D__ultrix",
	"-Dmips", "-D_mips", "-D__mips",
	"-Dhost_mips", "-D_host_mips", "-D__host_mips",
	"-DMIPSEL", "-D_MIPSEL", "-D__MIPSEL",
	"$1", "$2", "$3", 0 };
char *com[] =  { LCCDIR "rcc", "-target=mips/ultrix", "$1", "$2", "$3", "", 0 };
char *include[] = { "-I" LCCDIR "include", "-I/usr/local/include",
	"-I/usr/include", 0 };
char *as[] =  { "/bin/as", "-o", "$3", "", "$1", "-nocpp", "-EL", "$2", 0 };
char *ld[] =  { "/usr/bin/ld", "-o", "$3", "/usr/lib/crt0.o",
	"$1", "$2", "", "", "-lm", "-lc", 0 };
static char *libprefix = "/cmnusr/local/lib/ldb/";
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
	&& access("/u/drh/lib/mipsel/rcc", 4) == 0
	&& access("/u/drh/book/cdb/mips/ultrix/cdbld", 4) == 0) {
		com[0] = "/u/drh/lib/mipsel/rcc";
		com[5] = "-g4";
		ld[0] = "/u/drh/book/cdb/mips/ultrix/cdbld";
		ld[1] = "-o";
		ld[2] = "$3";
		ld[3] = "$1";
		ld[4] = "$2";
		ld[5] = 0;
	} else if (strcmp(arg, "-g") == 0)
		as[3] = "-g";
	else if (strcmp(arg, "-p") == 0
	&& strcmp(ld[3], "/usr/lib/crt0.o") == 0) {
		ld[3] = "/usr/lib/mcrt0.o";
		ld[7] = "/usr/lib/libprof1.a";
	} else if (strcmp(arg, "-b") == 0 && access(bbexit, 4) == 0)
		ld[6] = bbexit;
	else if (strncmp(arg, "-B", 2) == 0)
		libprefix = arg + 2;
	else if (strcmp(arg, "-G") == 0
	&& strcmp(ld[3], "/usr/lib/crt0.o") == 0) {
	        com[0] = concat(libprefix, "mipsel/rcc");
		com[1] = "";
		ld[0]  = "/cmnusr/local/lib/ldb/ldb-ld";
   	        ld[3]  = concat(libprefix, "mipsel/crt0.o");
		ld[7]  = concat(libprefix, "mipsel/Cnub.o");
	} else
		return 0;
	return 1;
}
