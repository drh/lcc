#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cpp.h"

extern	int getopt(int, char **, char *);
extern	char	*optarg;
extern	int	optind;
int	verbose;
int	Cplusplus;

#define	NLSIZE	128

Nlist	*nlist[NLSIZE];

struct	kwtab {
	char	*kw;
	int	val;
	int	flag;
} kwtab[] = {
	"if",		KIF,		ISKW,
	"ifdef",	KIFDEF,		ISKW,
	"ifndef",	KIFNDEF,	ISKW,
	"elif",		KELIF,		ISKW,
	"else",		KELSE,		ISKW,
	"endif",	KENDIF,		ISKW,
	"include",	KINCLUDE,	ISKW,
	"define",	KDEFINE,	ISKW,
	"undef",	KUNDEF,		ISKW,
	"line",		KLINE,		ISKW,
	"error",	KERROR,		ISKW,
	"pragma",	KPRAGMA,	ISKW,
	"eval",		KEVAL,		ISKW,
	"defined",	KDEFINED,	ISKW+ISUNCHANGE,
	"__LINE__",	KLINENO,	ISMAC+ISUNCHANGE,
	"__FILE__",	KFILE,		ISMAC+ISUNCHANGE,
	"__DATE__",	KDATE,		ISMAC+ISUNCHANGE,
	"__TIME__",	KTIME,		ISMAC+ISUNCHANGE,
	"__STDC__",	KSTDC,		ISUNCHANGE,
	NULL
};

unsigned long	namebit[077+1];

void
setup(int argc, char **argv)
{
	struct kwtab *kp;
	Nlist *np;
	Token t;
	int c, fd, i;
	char *fp, *dp;
	Tokenrow tr;
	Source *s;

	for (kp=kwtab; kp->kw; kp++) {
		t.t = kp->kw;
		t.len = strlen(kp->kw);
		np = lookup(&t, 1);
		np->flag = kp->flag;
		np->val = kp->val;
	}
	setsource("", -1, 0);
	while ((c = getopt(argc, argv, "NV+I:D:U:")) != -1)
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
			s = setsource("<cmdarg>", -1, optarg);
			maketokenrow(3, &tr);
			gettokens(&tr, 1);
			doadefine(&tr, c);
			unsetsource();
			break;
		case 'V':
			verbose++;
			break;
		case '+':
			Cplusplus++;
			break;
		default:
			exit(1);
			break;
		}
	dp = ".";
	fp = "<stdin>";
	fd = 0;
	if (optind<argc) {
		if ((fp = strrchr(argv[optind], '/')) != NULL) {
			int len = fp - argv[optind];
			dp = newstring(argv[optind], len+1, 0);
			dp[len] = '\0';
		}
		fp = argv[optind];
		if ((fd = open(fp, 0)) <= 0)
			error(FATAL, "Can't open input file %s", fp);
	}
	if (optind+1<argc) {
		int fdo = creat(argv[optind+1], 0666);
		if (fdo<0)
			error(FATAL, "Can't open output file %s", argv[optind+1]);
		dup2(fdo, 1);
	}
	includelist[NINCLUDE-1].always = 0;
	includelist[NINCLUDE-1].file = dp;
	setsource(fp, fd, NULL);
}

Nlist *
lookup(Token *tp, int install)
{
	unsigned int h;
	Nlist *np;
	char *cp, *cpe;

	h = 0;
	for (cp=tp->t, cpe=cp+tp->len; cp<cpe; )
		h += *cp++;
	h %= NLSIZE;
	np = nlist[h];
	while (np) {
		if (*tp->t==*np->name && tp->len==np->len 
		 && strncmp(tp->t, np->name, tp->len)==0)
			return np;
		np = np->next;
	}
	if (install) {
		np = new(Nlist);
		np->vp = NULL;
		np->ap = NULL;
		np->flag = 0;
		np->len = tp->len;
		np->name = newstring(tp->t, tp->len, 0);
		np->next = nlist[h];
		nlist[h] = np;
		quickset(tp->t[0], tp->len>1? tp->t[1]:0);
		return np;
	}
	return NULL;
}
