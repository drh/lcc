/* x86s running MS Windows NT 4.0 */

#include <string.h>

static char rcsid[] = "$Id$";

#ifndef LCCDIR
#define LCCDIR "/usr/local/lib/lcc/"
#endif
#ifndef VCCDIR
#define VCCDIR "/program files/devstudio/vc/"
#endif

char *suffixes[] = { ".c", ".i", ".asm", ".obj", ".exe", 0 };
char inputs[256] = "";
char *cpp[] = { LCCDIR "cpp.exe",
	"-D__STDC__=1", "$1", "$2", "$3", 0 };
char *include[] = { "-I" LCCDIR "include", "\"-I" VCCDIR "include\"", 0 };
char *com[] = { LCCDIR "rcc.exe", "-target=sparc/solaris",
	"$1", "$2", "$3", 0 };
char *as[] = { "\"/program files/MASM611/bin/ml.exe\"", "-nologo", "-c",
	"-Fo$3", "$1", "$2", 0 };
char *ld[] = { "\"" VCCDIR "bin/link.exe\"", "-nologo", 
	"-align:0x1000", "-subsystem:console", "-entry:mainCRTStartup",
	"$2", "-OUT:$3", "$1", "", "libc.lib", "kernel32.lib", 0 };
static char *bbexit = LCCDIR "bbexit.obj";

extern char *concat(char *, char *);
extern int access(const char *, int);

int option(char *arg) {
	if (strncmp(arg, "-lccdir=", 8) == 0) {
		cpp[0] = concat(&arg[8], "\\cpp.exe");
		include[0] = concat("-I", concat(&arg[8], "\\include"));
		com[0] = concat(&arg[8], "\\rcc.exe");
		bbexit = concat(&arg[8], "\\bbexit.obj");
	} else if (strcmp(arg, "-g") == 0)
		;
	else if (strcmp(arg, "-b") == 0 && access(bbexit, 4) == 0)
		ld[9] = bbexit;
	else if (strncmp(arg, "-ld=", 4) == 0)
		ld[0] = &arg[4];
	else
		return 0;
	return 1;
}
