#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <windows.h>

#ifndef EXPORT
#define EXPORT
#endif

static char rcsid[] = "$Id: echo.c#3 2002/08/30 10:25:36 REDMOND\\drh $";

EXPORT void *__echo(int argc, char *argv[]) {
	int i;
	char **args = calloc(argc + 2, sizeof args[0]);
	args[0] = _strdup(GetCommandLine());
	for (i = 0; args[0][i] != ' ' && args[0][i] != '\0'; i++)
		;
	args[0][i] = '\0';
	for (i = 0; i < argc; i++)
		args[i+1] = _strdup(argv[i]);
	args[i+1] = 0;
	return args;
}

#define xx(f) EXPORT void *__get##f(void) { return f; }
xx(stdin)
xx(stdout)
xx(stderr)
#undef xx

EXPORT int *__errno(void) { return &errno; }

EXPORT char *getcwd(char *buf, size_t n) {
	extern char *_getcwd(char *, size_t);
	return _getcwd(buf, n);
}

EXPORT int getpid(char *buf, size_t n) {
	extern int _getpid(void);
	return _getpid();
}

EXPORT int access(char *buf, int mode) {
	extern int _access(char *, int);
	return _access(buf, mode);
}
