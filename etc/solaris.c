/* SPARCs running Solaris 2.5.1 at CS Dept., Princeton University */

#include <string.h>

static char rcsid[] = "$Id$";

#ifndef LCCDIR
#define LCCDIR "/usr/local/lib/lcc/"
#endif
#ifndef SUNDIR
#define SUNDIR "/opt/SUNWspro/SC4.0/lib/"
#endif

char *cpp[] = { LCCDIR "cpp",
	"-D__STDC__=1", "-Dsparc", "-D__sparc__", "-Dsun", "-D__sun__",
	"$1", "$2", "$3", 0 };
char *include[] = { "-I" LCCDIR "include", "-I/usr/local/include",
	"-I/usr/include", 0 };
char *com[] = { LCCDIR "rcc", "-target=sparc/solaris",
	"$1", "$2", "$3", 0 };
char *as[] = { "/usr/ccs/bin/as", "-Qy", "-s", "-o", "$3", "$1", "$2", 0 };
char *ld[] = { "/usr/ccs/bin/ld", "-o", "$3", "$1",
	SUNDIR "crti.o", SUNDIR "crt1.o",
	SUNDIR "values-xa.o", "$2", "",
	"-Y", "P," SUNDIR ":/usr/ccs/lib:/usr/lib", "-Qy",
	"-lm", "-lc", "", SUNDIR "crtn.o", 0 };
static char *bbexit = LCCDIR "bbexit.o";

extern char *concat(char *, char *);
extern int access(const char *, int);

int option(char *arg) {
	if (strncmp(arg, "-lccdir=", 8) == 0) {
		cpp[0] = concat(&arg[8], "/cpp");
		include[0] = concat("-I", concat(&arg[8], "/include"));
		com[0] = concat(&arg[8], "/rcc");
		bbexit = concat(&arg[8], "/bbexit.o");
	} else if (strcmp(arg, "-g") == 0)
		;
	else if (strcmp(arg, "-p") == 0) {
		ld[5] = SUNDIR "mcrt1.o";
		ld[10] = "P," SUNDIR "libp:/usr/ccs/lib/libp:/usr/lib/libp:"
			 SUNDIR ":/usr/ccs/lib:/usr/lib";
	} else if (strcmp(arg, "-b") == 0 && access(bbexit, 4) == 0)
		ld[8] = bbexit;
	else
		return 0;
	return 1;
}
