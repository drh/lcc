#define	INS	32768		/* input buffer */
#define	OBS	4096		/* outbut buffer */
#define	NARG	32		/* Max number arguments to a macro */
#define	NINCLUDE 32		/* Max number of include directories (-I) */
#define	NIF	10		/* depth of nesting of #if */
#define	EOF	(-1)

enum toktype { END, UNCLASS, NAME, NUMBER, STRING, CCON, NL, WS, DSHARP,
		EQ, NEQ, LEQ, GEQ, LSH, RSH, LAND, LOR, PPLUS, MMINUS,
		ARROW, SBRA, SKET, LP, RP, DOT, AND, STAR, PLUS, MINUS,
		TILDE, NOT, SLASH, PCT, LT, GT, CIRC, OR, QUEST,
		COLON, ASGN, COMMA, SHARP, SEMIC, CBRA, CKET,
		ASPLUS, ASMINUS, ASSTAR, ASSLASH, ASPCT, ASCIRC, ASLSH,
		ASRSH, ASOR, ASAND, ELLIPS,
		DSHARP1, NAME1, DEFINED, UMINUS };

enum kwtype { KIF, KIFDEF, KIFNDEF, KELIF, KELSE, KENDIF, KINCLUDE, KDEFINE,
		KUNDEF, KLINE, KERROR, KPRAGMA, KDEFINED,
		KLINENO, KFILE, KDATE, KTIME, KSTDC, KEVAL };

#define	ISDEFINED	01	/* has #defined value */
#define	ISKW		02	/* is PP keyword */
#define	ISUNCHANGE	04	/* can't be #defined in PP */
#define	ISMAC		010	/* builtin macro, e.g. __LINE__ */

#define	EOB	0xFF		/* sentinel for end of input buffer */
#define	XPWS	1		/* token flag: white space to assure token sep. */

typedef struct token {
	unsigned char	type;
	unsigned char 	flag;
	unsigned short	hideset;
	unsigned short	wslen;
	unsigned short	len;
	char	*t;
} Token;

typedef struct tokenrow {
	Token	*tp;		/* current one to scan */
	Token	*bp;		/* base (allocated value) */
	Token	*lp;		/* last+1 token used */
	int	max;		/* number allocated */
} Tokenrow;

typedef struct source {
	char	*filename;	/* name of file of the source */
	int	line;		/* current line number */
	int	lineinc;	/* adjustment for \\n lines */
	char	*inb;		/* input buffer */
	char	*inp;		/* input pointer */
	char	*inl;		/* end of input */
	int	fd;		/* input source */
	int	ifdepth;	/* conditional nesting in include */
	struct	source *next;	/* stack for #include */
} Source;

typedef struct nlist {
	struct nlist *next;
	char	*name;
	int	len;
	Tokenrow *vp;		/* value as macro */
	Tokenrow *ap;		/* list of argument names, if any */
	char	val;		/* value as preprocessor name */
	char	flag;		/* is defined, is pp name */
} Nlist;

typedef	struct	includelist {
	char	deleted;
	char	always;
	char	*file;
} Includelist;

#define	new(t)	(t *)domalloc(sizeof(t))
#define	quicklook(a,b)	(namebit[(a)&077] & (1<<((b)&037)))
#define	quickset(a,b)	namebit[(a)&077] |= (1<<((b)&037))
extern	unsigned long namebit[077+1];

enum errtype { WARNING, ERROR, FATAL };

void	expandlex(void);
void	fixlex(void);
void	setup(int, char **);
int	gettokens(Tokenrow *, int);
int	comparetokens(Tokenrow *, Tokenrow *);
Source	*setsource(char *, int, char *);
void	unsetsource(void);
void	puttokens(Tokenrow *);
void	process(Tokenrow *);
void	*domalloc(int);
void	dofree(void *);
void	error(enum errtype, char *, ...);
void	flushout(void);
int	fillbuf(Source *);
int	getch(Source *);
int	trigraph(Source *);
Nlist	*lookup(Token *, int);
void	control(Tokenrow *);
void	dodefine(Tokenrow *);
void	doadefine(Tokenrow *, int);
void	doinclude(Tokenrow *);
void	doif(Tokenrow *, enum kwtype);
void	expand(Tokenrow *, Nlist *);
void	builtin(Tokenrow *, int);
int	gatherargs(Tokenrow *, Tokenrow **, int *);
void	substargs(Nlist *, Tokenrow *, Tokenrow **);
void	expandrow(Tokenrow *, char *);
void	maketokenrow(int, Tokenrow *);
Tokenrow *copytokenrow(Tokenrow *, Tokenrow *);
Token	*growtokenrow(Tokenrow *);
Tokenrow *normtokenrow(Tokenrow *);
void	adjustrow(Tokenrow *, int);
void	movetokenrow(Tokenrow *, Tokenrow *);
void	insertrow(Tokenrow *, int, Tokenrow *);
void	peektokens(Tokenrow *, char *);
void	doconcat(Tokenrow *);
Tokenrow *stringify(Tokenrow *);
int	lookuparg(Nlist *, Token *);
long	eval(Tokenrow *, int);
void	genline(void);
void	setempty(Tokenrow *);
void	makespace(Tokenrow *);
char	*outnum(char *, int);
int	digit(int);
char	*newstring(char *, int, int);
int	checkhideset(int, Nlist *);
void	prhideset(int);
int	newhideset(int, Nlist *);
int	unionhideset(int, int);
void	iniths(void);
#define	rowlen(tokrow)	((tokrow)->lp - (tokrow)->bp)

extern	char *outp;
extern	Token	nltoken;
extern	Source *cursource;
extern	char *curtime;
extern	int incdepth;
extern	int ifdepth;
extern	int ifsatisfied[NIF];
extern	int skipping;
extern	int verbose;
extern	int Cplusplus;
extern	Includelist includelist[NINCLUDE];
