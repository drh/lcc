#ifndef __ASSERT
#define __ASSERT

void assert(int);

static int _assert(char *e, char *file, unsigned line) {
	extern int write(int, char *, int);
	extern int sprintf(char *, const char *, ...);
	extern void abort(void);
	int i;
	char buf[256];

	sprintf(buf, "assertion failed: %s, file %s, line %d\n", e, file, line);
	for (i = 0; i < (int)sizeof buf; i++)
		if (buf[i] == 0)
			break;
        write(2, buf, i);
	abort();
	return 0;
}
#endif /* __ASSERT */

#undef assert
#ifdef NDEBUG
#define assert(ignore) ((void)0)
#else
extern int _assert(char *, char *, unsigned);
#define assert(e) ((void)((e)||_assert(#e, __FILE__, __LINE__)))
#endif /* NDEBUG */
