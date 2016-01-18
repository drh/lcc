/* Copyright Microsoft Corporation. All rights reserved. */

/* x86s running Microsoft.NET CLR */

#include <string.h>

static char rcsid[] = "$Id: msil.c#14 2002/12/11 11:15:24 REDMOND\\drh $";

#ifndef LCCDIR
#define LCCDIR "c:\\PROGRA~1\\lcc.NET\\"
#endif

char *suffixes[] = { ".c;.C", ".i;.I", ".s;.S", ".il;.IL", ".exe", 0 };
char inputs[256] = "";
char *cpp[] = { LCCDIR "cpp", "-D__STDC__=1", "-Dwin32", "-D_WIN32", "-D_MSIL_",
	"$1", "$2", "$3", 0 };
char *include[] = { "-I\"" LCCDIR "include\"", 0 };
char *com[] = { LCCDIR "rcc", "-target=msil/win32", "$1", "$2", "$3", 0 };
char *as[] = { LCCDIR "cp", "$2", "$3", 0 };
char *ld[] = { LCCDIR "illink",
	/*  1 */ "", "", "", "-a",
	/*  5 */"$3", "$1", "$2", "-l", "liblcc.dll", "-l", "msvcrt.dll", 0 };

extern char *concat(char *, char *);
extern char *replace(const char *, int, int);

int option(char *arg) {
	if (strncmp(arg, "-lccdir=", 8) == 0) {
		arg = replace(arg + 8, '/', '\\');
		if (arg[strlen(arg)-1] == '\\')
			arg[strlen(arg)-1] = '\0';
		cpp[0] = concat(arg, "\\cpp");
		include[0] = concat("-I\"", concat(arg, "\\include\""));
		com[0] = concat(arg, "\\rcc");
		as[0] = concat(arg, "\\cp");
		ld[0] = concat(arg, "\\illink");
	} else if (strcmp(arg, "-b") == 0)
		;
	else if (strcmp(arg, "-g") == 0)
		ld[3] = "-debug";
#define xx(a) else if (strncmp(arg, "-" #a "=", sizeof (#a)+1) == 0) \
			  a[0] = &arg[sizeof (#a)+1]
	xx(cpp);
	xx(com);
	xx(as);
#undef xx
	else if (strncmp(arg, "-a=", 3) == 0)
		ld[5] = &arg[3];
	else if (strncmp(arg, "-assembly=", 10) == 0) {
		ld[1] = "-o";
		ld[2] = &arg[10];
	} else
		return 0;
	return 1;
}
