#include "c.h"

static void printtoken ARGS((void));
int errcnt   = 0;
int errlimit = 20;
char kind[] = {
#define xx(a,b,c,d,e,f,g) f,
#define yy(a,b,c,d,e,f,g) f,
#include "token.h"
};
int wflag;		/* != 0 to suppress warning messages */

void test(tok, set) int tok; char set[]; {
	if (t == tok)
		t = gettok();
	else {
		expect(tok);
		skipto(tok, set);
		if (t == tok)
			t = gettok();
	}
}
void expect(tok) int tok; {
	if (t == tok)
		t = gettok();
	else {
		error("syntax error; found");
		printtoken();
		fprint(2, " expecting `%k'\n", tok);
	}
}
void error VARARGS((char *fmt, ...),
(fmt, va_alist),char *fmt; va_dcl) {
	va_list ap;

	if (errcnt++ >= errlimit) {
		errcnt = -1;
		error("too many errors\n");
		exit(1);
	}
	va_init(ap, fmt);
	if (firstfile != file && firstfile && *firstfile)
		fprint(2, "%s: ", firstfile);
	fprint(2, "%w: ", &src);
	vfprint(2, fmt, ap);
	va_end(ap);
}

void skipto(tok, set) int tok; char set[]; {
	int n;
	char *s;

	assert(set);
	for (n = 0; t != EOI && t != tok; t = gettok()) {
		for (s = set; *s && kind[t] != *s; s++)
			;
		if (kind[t] == *s)
			break;
		if (n++ == 0)
			error("skipping");
		if (n <= 8)
			printtoken();
		else if (n == 9)
			fprint(2, " ...");
	}
	if (n > 8) {
		fprint(2, " up to");
		printtoken();
	}
	if (n > 0)
		fprint(2, "\n");
}
/* fatal - issue fatal error message and exit */
int fatal(name, fmt, n) char *name, *fmt; int n; {
	*bp++ = '\n';
	outflush();
	errcnt = -1;
	error("compiler error in %s--", name);
	fprint(2, fmt, n);
	exit(1);
	return 0;
}

/* printtoken - print current token preceeded by a space */
static void printtoken() {
	switch (t) {
	case ID: fprint(2, " `%s'", token); break;
	case ICON:
		if (*token == '\'') {
			char *s;
	case SCON:	fprint(2, " ");
			for (s = token; *s && s - token < 20; s++)
				if (*s < ' ' || *s >= 0177)
					fprint(2, "\\%o", *s);
				else
					fprint(2, "%c", *s);
			if (*s)
				fprint(2, " ...");
			else
				fprint(2, "%c", *token);
			break;
		} /* else fall thru */
	case FCON: {
		char c = *cp;
		*cp = 0;
		fprint(2, " `%s'", token);
		*cp = c;
		break;
		}
	case '`': case '\'': fprint(2, " \"%k\"", t); break;
	default: fprint(2, " `%k'", t);
	}
}

/* warning - issue warning error message */
void warning VARARGS((char *fmt, ...),(fmt, va_alist),char *fmt; va_dcl) {
	va_list ap;

	va_init(ap, fmt);
	if (wflag == 0) {
		errcnt--;
		error("warning: ");
		vfprint(2, fmt, ap);
	}
	va_end(ap);
}
