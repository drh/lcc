/* x86s running Linux */

#include <string.h>

static char rcsid[] = "$Id$";

#ifndef LCCDIR
#define LCCDIR "/usr/local/lib/lcc/"
#endif

char *suffixes[] = { ".c", ".i", ".s", ".o", ".out", 0 };
char inputs[256] = "";
char *cpp[] = { LCCDIR "cpp",
	"-U__GNUC__", "-D_POSIX_SOURCE", "-D__STDC__=1", "-D__STRICT_ANSI__",
	"-Dunix", "-Di386", "-Dlinux",
	"-D__unix__", "-D__i386__", "-D__linux__",
	"$1", "$2", "$3", 0 };
char *include[] = {"-I" LCCDIR "include", "-I/usr/include", 0 };
char *com[] = {LCCDIR "rcc", "-target=x86/linux", $1", "$2", "$3", 0 };
char *as[] = { "/usr/bin/as", "-o", "$3", "$1", "$2", 0 };
char *ld[] = {
	/*  0 */ "/usr/bin/ld", "-m", "elf_i386", "-dynamic-linker",
	/*  4 */ "/lib/ld-linux.so.1", "-o", "$3",
	/*  7 */ "/usr/lib/crt1.o", "/usr/lib/crti.o", "/usr/lib/crtbegin.o", 
                 "$1", "$2", "-lc",
	/* 13 */ "-L" LCCDIR,
	/* 14 */ "-llcc",
	/* 15 */ "", "/usr/lib/crtend.o", "/usr/lib/crtn.o",
	0 };

extern char *concat(char *, char *);

int option(char *arg) {
  	if (strncmp(arg, "-lccdir=", 8) == 0) {
		cpp[0] = concat(&arg[8], "/cpp");
		include[0] = concat("-I", concat(&arg[8], "/include"));
		ld[13] =  concat("-L", &arg[8]);
		com[0] = concat(&arg[8], "/rcc");
	} else if (strcmp(arg, "-p") == 0 || strcmp(arg, "-pg") == 0) {
		ld[7] = "/usr/lib/gcrt1.o";
		ld[15] = "-lgmon";
	} else if (strcmp(arg, "-b") == 0) 
		;
	else if (strcmp(arg, "-g") == 0)
		;
	else if (strncmp(arg, "-ld=", 4) == 0)
		ld[0] = &arg[4];
	else if (strcmp(arg, "-static") == 0) {
	        ld[3] = "-static";
	        ld[4] = "";
	        ld[13] = "-L/usr/lib/gcc-lib/i486-linux/2.7.2";
	        ld[14] = "-lgcc";
	} else
		return 0;
	return 1;
}
