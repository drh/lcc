#include "c.h"

static void pragma ARGS((void));
static void resynch ARGS((void));

static int bsize;
static unsigned char buffer[MAXLINE+1 + BUFSIZE+1];
int infd;		/* input file descriptor */
unsigned char *cp;	/* current input character */
char *file;		/* current input file name */
char *firstfile;	/* first input file */
unsigned char *limit;	/* points to last character + 1 */
char *line;		/* current line */
int lineno;		/* line number of current line */

void inputInit() {
	limit = cp = &buffer[MAXLINE+1];
	bsize = -1;
	lineno = 0;
	file = NULL;
	fillbuf();
	if (cp >= limit)
		cp = limit;
	nextline();
}
void nextline() {
	do {
		if (cp >= limit) {
			fillbuf();
			if (cp >= limit)
				cp = limit;
			if (cp == limit)
				return;
		} else
			lineno++;
		for (line = (char *)cp; *cp==' ' || *cp=='\t'; cp++)
			;
	} while (*cp == '\n' && cp == limit);
	if (*cp == '#') {
		resynch();
		nextline();
	}
}
void fillbuf() {
	if (bsize == 0)
		return;
	if (cp >= limit)
		cp = &buffer[MAXLINE+1];
	else
		{
			int n = limit - cp;
			unsigned char *s = &buffer[MAXLINE+1] - n;
			assert(s >= buffer);
			line = (char *)s - ((char *)cp - line);
			while (cp < limit)
				*s++ = *cp++;
			cp = &buffer[MAXLINE+1] - n;
		}
	bsize = read(infd, &buffer[MAXLINE+1], BUFSIZE);
	if (bsize < 0) {
		error("read error\n");
		exit(1);
	}
	limit = &buffer[MAXLINE+1+bsize];
	*limit = '\n';
}
/* inputstring - arrange to read str as next input */
void inputstring(str) char *str; {
	limit = cp = &buffer[MAXLINE+1];
	while ((*limit++ = *str++) != 0)
		;
	*--limit = '\n';
	bsize = 0;
}

/* pragma - handle #pragma ref id... */
static void pragma() {
	if ((t = gettok()) == ID && strcmp(token, "ref") == 0)
		for (;;) {
			while (*cp == ' ' || *cp == '\t')
				cp++;
			if (*cp == '\n' || *cp == 0)
				break;
			if ((t = gettok()) == ID && tsym) {
				tsym->ref++;
				use(tsym, src);
			}	
		}
}

/* resynch - set line number/file name in # n [ "file" ] and #pragma ... */
static void resynch() {
	for (cp++; *cp == ' ' || *cp == '\t'; )
		cp++;
	if (limit - cp < MAXLINE)
		fillbuf();
	if (strncmp((char *)cp, "pragma", 6) == 0) {
		cp += 6;
		pragma();
	} else if (*cp >= '0' && *cp <= '9') {
	line:	for (lineno = 0; *cp >= '0' && *cp <= '9'; )
			lineno = 10*lineno + *cp++ - '0';
		lineno--;
		while (*cp == ' ' || *cp == '\t')
			cp++;
		if (*cp == '"') {
			file = (char *)++cp;
			while (*cp && *cp != '"' && *cp != '\n')
				cp++;
			file = stringn(file, (char *)cp - file);
			if (*cp == '\n')
				warning("missing \" in preprocessor line\n");
			if (firstfile == 0)
				firstfile = file;
		}
	} else if (strncmp((char *)cp, "line", 4) == 0) {
		for (cp += 4; *cp == ' ' || *cp == '\t'; )
			cp++;
		if (*cp >= '0' && *cp <= '9')
			goto line;
		if (Aflag >= 2)
			warning("unrecognized control line\n");
	} else if (Aflag >= 2 && *cp != '\n')
		warning("unrecognized control line\n");
	while (*cp)
		if (*cp++ == '\n')
			if (cp == limit + 1)
				nextline();
			else
				break;
}

