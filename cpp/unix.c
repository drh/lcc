#include "cpp.h"

extern	int getopt(int, char **, char *);
extern	char	*optarg;
extern	int	optind;
int	verbose;
int	Mflag;	/* only print active include files */
char	*objname; /* "src.$O: " */
int	Cplusplus = 1;
int	omitlinekey = 0;
double	frac = 1.;

void
setup(int argc, char **argv)
{
	int c, fd, i;
	char *fp, *dp;
	Tokenrow tr;
	Source *s;
	char *objtype;
	static char nbuf[40];

	setup_kwtab();
	s = setsource("<ecpp>",-1,"__ECPP__");	/* #define __ECPP__ */
	maketokenrow(3, &tr);
	gettokens(&tr, 1);
	doadefine(&tr, 'D');
	unsetsource();

	while ((c = getopt(argc, argv, "MNOV+I:D:U:F:lg")) != -1)
		switch (c) {
		case 'N':
			for (i=0; i<NINCLUDE; i++)
				if (includelist[i].always==1)
					includelist[i].deleted = 1;
			break;
		case 'I':
			for (i=NINCLUDE-2; i>=0; i--) {
				if (includelist[i].file==NULL) {
					includelist[i].always = 1;
					includelist[i].file = optarg;
					break;
				}
			}
			if (i<0)
				error(FATAL, "Too many -I directives");
			break;
		case 'D':
		case 'U':
			setsource("<cmdarg>", -1, optarg);
			maketokenrow(3, &tr);
			gettokens(&tr, 1);
			doadefine(&tr, c);
			unsetsource();
			break;
		case 'M':
			Mflag++;
			break;
		case 'V':
			verbose++;
			break;
		case '+':
			Cplusplus++;
			break;
		case 'F':
			frac = atof(optarg);
			break;
		case 'l':
			omitlinekey++;
			break;
		default:
			break;
		}
	dp = ".";
	fp = "<stdin>";
	fd = 0;
	if (optind<argc) {
		if ((fp = strrchr(argv[optind], '/')) != NULL) {
			int len = fp - argv[optind];
			dp = (char*)newstring((uchar*)argv[optind], len+1, 0);
			dp[len] = '\0';
		}
		fp = (char*)newstring((uchar*)argv[optind], strlen(argv[optind]), 0);
		if ((fd = open(fp, 0)) <= 0)
			error(FATAL, "Can't open input file %s", fp);
	}
	if (optind+1<argc) {
		int fdo = creat(argv[optind+1], 0666);
		if (fdo<0)
			error(FATAL, "Can't open output file %s", argv[optind+1]);
		dup2(fdo, 1);
	}
	if(Mflag)
		setobjname(fp);
	includelist[NINCLUDE-1].always = 0;
	includelist[NINCLUDE-1].file = dp;
	setsource(fp, fd, NULL);
}



/* memmove is defined here because some vendors don't provide it at
   all and others do a terrible job (like calling malloc) */
void *
memmove(void *dp, const void *sp, unsigned int n)
{
	unsigned char *cdp, *csp;

	if (n<=0)
		return 0;
	cdp = dp;
	csp = (unsigned char *)sp;
	if (cdp < csp) {
		do {
			*cdp++ = *csp++;
		} while (--n);
	} else {
		cdp += n;
		csp += n;
		do {
			*--cdp = *--csp;
		} while (--n);
	}
	return 0;
}


/****** Plan9 UTF stuff *******/

extern int chartorune(Rune*,char*);

enum
{
	UTFmax		= 3,		/* maximum bytes per rune */
	Runesync	= 0x80,		/* cannot represent part of a UTF sequence (<) */
	Runeself	= 0x80,		/* rune and UTF sequences are the same (<) */
	Runeerror	= 0x80		/* decoding error in UTF */
};

enum
{
	Bit1	= 7,
	Bitx	= 6,
	Bit2	= 5,
	Bit3	= 4,
	Bit4	= 3,

	T1	= ((1<<(Bit1+1))-1) ^ 0xFF,	/* 0000 0000 */
	Tx	= ((1<<(Bitx+1))-1) ^ 0xFF,	/* 1000 0000 */
	T2	= ((1<<(Bit2+1))-1) ^ 0xFF,	/* 1100 0000 */
	T3	= ((1<<(Bit3+1))-1) ^ 0xFF,	/* 1110 0000 */
	T4	= ((1<<(Bit4+1))-1) ^ 0xFF,	/* 1111 0000 */

	Rune1	= (1<<(Bit1+0*Bitx))-1,		/* 0000 0000 0111 1111 */
	Rune2	= (1<<(Bit2+1*Bitx))-1,		/* 0000 0111 1111 1111 */
	Rune3	= (1<<(Bit3+2*Bitx))-1,		/* 1111 1111 1111 1111 */

	Maskx	= (1<<Bitx)-1,			/* 0011 1111 */
	Testx	= Maskx ^ 0xFF,			/* 1100 0000 */

	Bad	= Runeerror
};

int
chartorune(Rune *rune, char *str)
{
	int c, c1, c2;
	long l;

	/*
	 * one character sequence
	 *	00000-0007F => T1
	 */
	c = *(uchar*)str;
	if(c < Tx) {
		*rune = c;
		return 1;
	}

	/*
	 * two character sequence
	 *	0080-07FF => T2 Tx
	 */
	c1 = *(uchar*)(str+1) ^ Tx;
	if(c1 & Testx)
		goto bad;
	if(c < T3) {
		if(c < T2)
			goto bad;
		l = ((c << Bitx) | c1) & Rune2;
		if(l <= Rune1)
			goto bad;
		*rune = l;
		return 2;
	}

	/*
	 * three character sequence
	 *	0800-FFFF => T3 Tx Tx
	 */
	c2 = *(uchar*)(str+2) ^ Tx;
	if(c2 & Testx)
		goto bad;
	if(c < T4) {
		l = ((((c << Bitx) | c1) << Bitx) | c2) & Rune3;
		if(l <= Rune2)
			goto bad;
		*rune = l;
		return 3;
	}

	/*
	 * bad decoding
	 */
bad:
	*rune = Bad;
	return 1;
}
