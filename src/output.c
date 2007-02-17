#include "c.h"

int outfd = 1;
int errfd = 2;
static char buf1[4*1024], buf2[512];	/* output buffers */
static struct io {
	int fd;				/* file descriptor */
	char *bp;			/* buffer pointer */
	char *buffer;			/* buffer proper */
	char *limit;			/* high water limit */
} iob[] = {
	0, 0, 0, 0,
	1, buf1, buf1, buf1 + sizeof buf1 - 80,
	2, buf2, buf2, buf2 + sizeof buf2 - 80
}, *io[] = {
	&iob[0],			/* used by stringf */
	&iob[1],			/* output */
	&iob[2]				/* standard error & other files; used by fprint */
};
static int fd = 1;			/* current output file */

char *bp = buf1;			/* current output buffer pointer */

void outs(s) char *s; {
	char *p;

	for (p = bp; (*p = *s++) != 0; p++)
		;
	bp = p;
	if (bp > io[fd]->limit)
		outflush();
}
void print VARARGS((char *fmt, ...),
(fmt, va_alist),char *fmt; va_dcl) {
	va_list ap;

	va_init(ap, fmt);
	vprint(fmt, ap);
	va_end(ap);
}
/* outputInit - initialize output system */
void outputInit() {
	io[1]->fd = outfd;
	io[2]->fd = errfd;
}

/* fprint - formatted output to file descriptor f */
void fprint VARARGS((int f, char *fmt, ...),(f, fmt, va_alist),char *fmt; va_dcl) {
	va_list ap;

	va_init(ap, fmt);
	vfprint(f, fmt, ap);
	va_end(ap);
}

/* outflush - flush output buffer */
void outflush() {
	struct io *iop = io[fd];

	assert(fd);
	if (bp > iop->buffer)
		write(iop->fd, iop->buffer, bp - iop->buffer);
	bp = iop->bp = iop->buffer;
}

/* stringf - formatted output to a saved string */
char *stringf VARARGS((char *fmt, ...),(fmt, va_alist),char *fmt; va_dcl) {
	char buf[1024];
	va_list ap;

	va_init(ap, fmt);
	fd = 0;
	io[1]->bp = bp;
	bp = io[0]->bp = io[0]->buffer = buf;
	io[0]->limit = buf + sizeof buf;
	vprint(fmt, ap);
	*bp = 0;
	bp = io[1]->bp;
	fd = 1;
	va_end(ap);
	return string(buf);
}

/* vfprint - formatted output to file descriptor f */
void vfprint(f, fmt, ap) int f; char *fmt; va_list ap; {
	if (f == 1)
		vprint(fmt, ap);
	else {
		fd = 2;
		io[1]->bp = bp;
		io[2]->fd = f == 2 ? errfd : f;
		bp = io[2]->bp;
		vprint(fmt, ap);
		outflush();
		bp = io[1]->bp;
		fd = 1;
	}
}

/* vprint - formatted output to standard output */
void vprint(fmt, ap) char *fmt; va_list ap; {
	for (; *fmt; fmt++)
		if (*fmt == '%')
			switch (*++fmt) {
			case 'c': { *bp++ = va_arg(ap, int);
 } break;
			case 'd': { int n = va_arg(ap, int);
				    unsigned m;
				    char buf[25], *s = buf + sizeof buf;
				    *--s = 0;
				    if (n == INT_MIN)
				    	m = (unsigned)INT_MAX + 1;
				    else if (n < 0)
				    	m = -n;
				    else
				    	m = n;
				    do
				    	*--s = m%10 + '0';
				    while ((m /= 10) != 0);
				    if (n < 0)
				    	*--s = '-';
				    outs(s);
 } break;
			case 'o': { unsigned n = va_arg(ap, unsigned);
				    char buf[25], *s = buf + sizeof buf;
				    *--s = 0;
				    do
				    	*--s = (n&7) + '0';
				    while ((n >>= 3) != 0);
				    outs(s);
 } break;
			case 'x': { unsigned n = va_arg(ap, unsigned);
				    char buf[25], *s = buf + sizeof buf;
				    *--s = 0;
				    do
				    	*--s = "0123456789abcdef"[n&0xf];
				    while ((n >>= 4) != 0);
				    outs(s);
 } break;
			case 's': { char *s = va_arg(ap, char *);
				    if (s)
				    	outs(s);
 } break;
			case 'S': { char *s = va_arg(ap, char *);
				    int n = va_arg(ap, int);
				    if (s)
				    	while (n-- > 0)
				    		*bp++ = *s++;
 } break;
			case 'k': { int t = va_arg(ap, int);
				    static char *tokens[] = {
#define xx(a,b,c,d,e,f,g) g,
#define yy(a,b,c,d,e,f,g) g,
#include "token.h"
				    };
				    assert(tokens[t&0177]);
				    outs(tokens[t&0177]);
 } break;
			case 't': { Type ty = va_arg(ap, Type);
				    outtype(ty ? ty : voidtype);
 } break;
			case 'w': { Coordinate *p = va_arg(ap, Coordinate *);
				    if (p->file && *p->file)
				    	print("%s:", p->file);
				    print("%d", p->y); } break;
			default:  *bp++ = *fmt; break;
			}
		else if ((*bp++ = *fmt) == '\n' && bp > io[fd]->limit)
			outflush();
}

