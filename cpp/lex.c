#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cpp.h"

/*
 * lexical FSM encoding
 *   when in state state, and one of the characters
 *   in ch arrives, enter nextstate.
 *   States >= S_SELF are either final, or at least require special action.
 *   In 'fsm' there is a line for each state X charset X nextstate.
 *   List chars that overwrite previous entries later (e.g. C_ALPH
 *   can be overridden by '_' by a later entry; and C_XX is the
 *   the universal set, and should always be first.
 *   States above S_SELF are represented in the big table as negative values.
 *   S_SELF and S_SELFB encode the resulting token type in the upper bits.
 *   These actions differ in that S_SELF doesn't have a lookahead char,
 *   S_SELFB does.
 *
 *   The encoding is blown out into a big table for time-efficiency.
 *   Entries have
 *      nextstate: 6 bits; ?\ marker: 1 bit; tokentype: 9 bits.
 */

#define	MAXSTATE 32
#define	ACT(tok,act)	((tok<<7)+act)
#define	QBSBIT	0100
#define	GETACT(st)	(st>>7)&0x1ff

/* character classes */
#define	C_WS	1
#define	C_ALPH	2
#define	C_NUM	3
#define	C_EOF	4
#define	C_XX	5

enum state {
	START=0, NUM1, NUM2, NUM3, ID1, ST1, ST2, ST3, COM1, COM2, COM3, COM4,
	CC1, CC2, WS1, PLUS1, MINUS1, STAR1, SLASH1, PCT1, SHARP1,
	CIRC1, GT1, GT2, LT1, LT2, OR1, AND1, ASG1, NOT1, DOTS1,
	S_SELF=MAXSTATE, S_SELFB, S_EOF, S_NL, S_EOFSTR,
	S_STNL, S_COMNL, S_EOFCOM, S_COMMENT, S_EOB, S_WS, S_NAME
};

int	tottok;
int	tokkind[256];
struct	fsm {
	int	state;		/* if in this state */
	char	ch[4];		/* and see one of these characters */
	int	nextstate;	/* enter this state if +ve */
};

/*const*/ struct fsm fsm[] = {
	/* start state */
	START,	{ C_XX },	ACT(UNCLASS,S_SELF),
	START,	{ ' ', '\t', '\v' },	WS1,
	START,	{ C_NUM },	NUM1,
	START,	{ '.' },	NUM3,
	START,	{ C_ALPH },	ID1,
	START,	{ 'L' },	ST1,
	START,	{ '"' },	ST2,
	START,	{ '\'' },	CC1,
	START,	{ '/' },	COM1,
	START,	{ EOF },	S_EOF,
	START,	{ '\n' },	S_NL,
	START,	{ '-' },	MINUS1,
	START,	{ '+' },	PLUS1,
	START,	{ '<' },	LT1,
	START,	{ '>' },	GT1,
	START,	{ '=' },	ASG1,
	START,	{ '!' },	NOT1,
	START,	{ '&' },	AND1,
	START,	{ '|' },	OR1,
	START,	{ '#' },	SHARP1,
	START,	{ '%' },	PCT1,
	START,	{ '[' },	ACT(SBRA,S_SELF),
	START,	{ ']' },	ACT(SKET,S_SELF),
	START,	{ '(' },	ACT(LP,S_SELF),
	START,	{ ')' },	ACT(RP,S_SELF),
	START,	{ '*' },	STAR1,
	START,	{ ',' },	ACT(COMMA,S_SELF),
	START,	{ '?' },	ACT(QUEST,S_SELF),
	START,	{ ':' },	ACT(COLON,S_SELF),
	START,	{ ';' },	ACT(SEMIC,S_SELF),
	START,	{ '{' },	ACT(CBRA,S_SELF),
	START,	{ '}' },	ACT(CKET,S_SELF),
	START,	{ '~' },	ACT(TILDE,S_SELF),
	START,	{ '^' },	CIRC1,

	/* saw a digit */
	NUM1,	{ C_XX },	ACT(NUMBER,S_SELFB),
	NUM1,	{ C_NUM, C_ALPH, '.' },	NUM1,
	NUM1,	{ 'E', 'e' },	NUM2,
	NUM1,	{ '_' },	ACT(NUMBER,S_SELFB),

	/* saw possible start of exponent, digits-e */
	NUM2,	{ C_XX },	ACT(NUMBER,S_SELFB),
	NUM2,	{ '+', '-' },	NUM1,
	NUM2,	{ C_NUM, C_ALPH },	NUM1,
	NUM2,	{ '_' },	ACT(NUMBER,S_SELFB),

	/* saw a '.', which could be a number or an operator */
	NUM3,	{ C_XX },	ACT(DOT,S_SELFB),
	NUM3,	{ '.' },	DOTS1,
	NUM3,	{ C_NUM },	NUM1,

	DOTS1,	{ C_XX },	ACT(UNCLASS, S_SELFB),
	DOTS1,	{ C_NUM },	NUM1,
	DOTS1,	{ '.' },	ACT(ELLIPS, S_SELF),

	/* saw a letter or _ */
	ID1,	{ C_XX },	ACT(NAME,S_NAME),
	ID1,	{ C_ALPH, C_NUM },	ID1,

	/* saw L (start of wide string?) */
	ST1,	{ C_XX },	ACT(NAME,S_NAME),
	ST1,	{ C_ALPH, C_NUM },	ID1,
	ST1,	{ '"' },	ST2,
	ST1,	{ '\'' },	CC1,

	/* saw " beginning string */
	ST2,	{ C_XX },	ST2,
	ST2,	{ '"' },	ACT(STRING, S_SELF),
	ST2,	{ '\\' },	ST3,
	ST2,	{ '\n' },	S_STNL,
	ST1,	{ EOF },	S_EOFSTR,

	/* saw \ in string */
	ST3,	{ C_XX },	ST2,
	ST3,	{ '\n' },	S_STNL,
	ST3,	{ EOF },	S_EOFSTR,

	/* saw ' beginning character const */
	CC1,	{ C_XX },	CC1,
	CC1,	{ '\'' },	ACT(CCON, S_SELF),
	CC1,	{ '\\' },	CC2,
	CC1,	{ '\n' },	S_STNL,
	CC1,	{ EOF },	S_EOFSTR,

	/* saw \ in ccon */
	CC2,	{ C_XX },	CC1,
	CC2,	{ '\n' },	S_STNL,
	CC2,	{ EOF },	S_EOFSTR,

	/* saw /, perhaps start of comment */
	COM1,	{ C_XX },	ACT(SLASH, S_SELFB),
	COM1,	{ '=' },	ACT(ASSLASH, S_SELF),
	COM1,	{ '*' },	COM2,
	COM1,	{ '/' },	COM4,

	/* saw "/*", start of comment */
	COM2,	{ C_XX },	COM2,
	COM2,	{ '\n' },	S_COMNL,
	COM2,	{ '*' },	COM3,
	COM2,	{ EOF },	S_EOFCOM,

	/* saw the * possibly ending a comment */
	COM3,	{ C_XX },	COM2,
	COM3,	{ '\n' },	S_COMNL,
	COM3,	{ '*' },	COM3,
	COM3,	{ '/' },	S_COMMENT,

	/* // comment */
	COM4,	{ C_XX },	COM4,
	COM4,	{ '\n' },	S_NL,
	COM4,	{ EOF },	S_EOFCOM,

	/* saw white space, eat it up */
	WS1,	{ C_XX },	S_WS,
	WS1,	{ ' ', '\t', '\v' },	WS1,

	/* saw -, check --, -=, -> */
	MINUS1,	{ C_XX },	ACT(MINUS, S_SELFB),
	MINUS1,	{ '-' },	ACT(MMINUS, S_SELF),
	MINUS1,	{ '=' },	ACT(ASMINUS,S_SELF),
	MINUS1,	{ '>' },	ACT(ARROW,S_SELF),

	/* saw +, check ++, += */
	PLUS1,	{ C_XX },	ACT(PLUS, S_SELFB),
	PLUS1,	{ '+' },	ACT(PPLUS, S_SELF),
	PLUS1,	{ '=' },	ACT(ASPLUS, S_SELF),

	/* saw <, check <<, <<=, <= */
	LT1,	{ C_XX },	ACT(LT, S_SELFB),
	LT1,	{ '<' },	LT2,
	LT1,	{ '=' },	ACT(LEQ, S_SELF),
	LT2,	{ C_XX },	ACT(LSH, S_SELFB),
	LT2,	{ '=' },	ACT(ASLSH, S_SELF),

	/* saw >, check >>, >>=, >= */
	GT1,	{ C_XX },	ACT(GT, S_SELFB),
	GT1,	{ '>' },	GT2,
	GT1,	{ '=' },	ACT(GEQ, S_SELF),
	GT2,	{ C_XX },	ACT(RSH, S_SELFB),
	GT2,	{ '=' },	ACT(ASRSH, S_SELF),

	/* = */
	ASG1,	{ C_XX },	ACT(ASGN, S_SELFB),
	ASG1,	{ '=' },	ACT(EQ, S_SELF),

	/* ! */
	NOT1,	{ C_XX },	ACT(NOT, S_SELFB),
	NOT1,	{ '=' },	ACT(NEQ, S_SELF),

	/* & */
	AND1,	{ C_XX },	ACT(AND, S_SELFB),
	AND1,	{ '&' },	ACT(LAND, S_SELF),
	AND1,	{ '=' },	ACT(ASAND, S_SELF),

	/* | */
	OR1,	{ C_XX },	ACT(OR, S_SELFB),
	OR1,	{ '|' },	ACT(LOR, S_SELF),
	OR1,	{ '=' },	ACT(ASOR, S_SELF),

	/* # */
	SHARP1,	{ C_XX },	ACT(SHARP, S_SELFB),
	SHARP1,	{ '#' },	ACT(DSHARP, S_SELF),

	/* % */
	PCT1,	{ C_XX },	ACT(PCT, S_SELFB),
	PCT1,	{ '=' },	ACT(ASPCT, S_SELF),

	/* * */
	STAR1,	{ C_XX },	ACT(STAR, S_SELFB),
	STAR1,	{ '=' },	ACT(ASSTAR, S_SELF),

	/* ^ */
	CIRC1,	{ C_XX },	ACT(CIRC, S_SELFB),
	CIRC1,	{ '=' },	ACT(ASCIRC, S_SELF),

	-1
};

/* first index is char+1 (to include EOF), second is state */
/* increase #states to power of 2 to encourage use of shift */
short	bigfsm[257][MAXSTATE];

void
expandlex()
{
	/*const*/ struct fsm *fp;
	int i, j, nstate;

	for (fp = fsm; fp->state>=0; fp++) {
		for (i=0; fp->ch[i]; i++) {
			nstate = fp->nextstate;
			if (nstate >= S_SELF)
				nstate = ~nstate;
			switch (fp->ch[i]) {

			case C_XX:		/* random characters */
				for (j=0; j<257; j++)
					bigfsm[j][fp->state] = nstate;
				continue;
			case C_ALPH:
				for (j='a'; j<='z'; j++) {
					bigfsm[j+1][fp->state] = nstate;
					bigfsm[j-'a'+'A'+1][fp->state] = nstate;
				}
				bigfsm['_'+1][fp->state] = nstate;
				continue;
			case C_NUM:
				for (j='0'; j<='9'; j++)
					bigfsm[j+1][fp->state] = nstate;
				continue;
			default:
				bigfsm[fp->ch[i]+1][fp->state] = nstate;
			}
		}
	}
	/* install special cases for ? (trigraphs)  \ (for splicing) and EOB */
	for (i=0; i<MAXSTATE; i++) {
		if (bigfsm['?'+1][i]>0)
			bigfsm['?'+1][i] = ~bigfsm['?'+1][i];
		bigfsm['?'+1][i] &= ~QBSBIT;
		if (bigfsm['\\'+1][i]>0)
			bigfsm['\\'+1][i] = ~bigfsm['\\'+1][i];
		bigfsm['\\'+1][i] &= ~QBSBIT;
		bigfsm[EOB+1][i] = ~S_EOB;
	}
}

void
fixlex()
{
	/* do C++ comments? */
	if (Cplusplus==0)
		bigfsm['/'+1][COM1] = bigfsm['x'+1][COM1];
}

/*
 * fill in a row of tokens from input, terminated by NL or END
 * First token is put at trp->lp.
 * Reset is non-zero when the input buffer can be "rewound."
 * The value is a flag indicating that possible macros have
 * been seen in the row.
 */
int
gettokens(Tokenrow *trp, int reset)
{
	register int c, state, oldstate;
	register char *ip;
	register Token *tp, *maxp;
	Source *s = cursource;
	int c1;
	int nmac = 0;
	extern char outbuf[];

	tp = trp->lp;
	ip = s->inp;
	if (reset) {
		s->lineinc = 0;
		if (ip>=s->inl) {		/* nothing in buffer */
			s->inl = s->inb;
			fillbuf(s);
			ip = s->inb;
		} else if (ip >= s->inb+(3*INS/4)) {
			memmove(s->inb, ip, 4+s->inl-ip);
			s->inl = s->inb+(s->inl-ip);
			ip = s->inb;
		}
	}
	maxp = &trp->bp[trp->max];
	for (;;) {
	   continue2:
		if (tp>=maxp) {
			trp->lp = tp;
			tp = growtokenrow(trp);
			maxp = &trp->bp[trp->max];
		}
		tp->type = UNCLASS;
		tp->hideset = 0;
		tp->t = ip;
		tp->wslen = 0;
		tp->flag = 0;
		state = START;
		c = *(unsigned char *)ip++;
		for (;;) {
			oldstate = state;
			if ((state = bigfsm[c+1][state]) >= 0) {
				c = *(unsigned char *)ip++;
				continue;
			}
			state = ~state;
		reswitch:
			switch (state&0177) {
			case S_SELF:
				tp->type = GETACT(state);
				tp->len = ip - tp->t;
				tp++;
				goto continue2;

			case S_SELFB:
				tp->type = GETACT(state);
				ip -= 1;
				tp->len = ip - tp->t;
				tp++;
				goto continue2;

			case S_NAME:	/* like S_SELFB but with nmac check */
				tp->type = NAME;
				ip -= 1;
				tp->len = ip - tp->t;
				if (tp->len>1)
					nmac |= quicklook(tp->t[0], tp->t[1]);
				else
					nmac |= quicklook(tp->t[0], 0);
				tp++;
				goto continue2;

			case S_WS:
				tp->wslen = ip - tp->t - 1;
				tp->t = ip-1;
				state = START;
				continue;

			default:
				if (c=='?' && state&QBSBIT) { /* check trigraph */
					s->inp = ip;
					if (c1 = trigraph(s)) {
						c = c1;
						ip = s->inp;
						ip[-1] = c;
						memmove(
						  tp->t-tp->wslen+2,
						  tp->t-tp->wslen,
						  ip-tp->t+tp->wslen-3);
						tp->t += 2;
						state = oldstate;
						continue;
					} else
						ip = s->inp;
					state &= ~QBSBIT;
					goto reswitch;
				}
				if (c=='\\' && state&QBSBIT) { /* line-folding */
					s->inp = ip;
					if ((c1 = getch(s)) == '\n') {
						ip = s->inp;
						memmove(
						  tp->t-tp->wslen+2,
						  tp->t-tp->wslen,
						  ip-tp->t+tp->wslen-2);
						tp->t += 2;
						s->lineinc++;
						state = oldstate;
						c = *(unsigned char *)ip++;
						continue;
					} else {
						ip = s->inp;
						if (c1!=EOF)
							ip -= 1;
					}
					state &= ~QBSBIT;
					goto reswitch;
				}
				c = *(unsigned char *)ip++;
				continue;

			case S_EOB:
				if (fillbuf(cursource)==EOF) {
					ip = s->inp;
					c = EOF;
				} else {
					ip = s->inp;
					c = *(unsigned char *)ip++;
				}
				state = oldstate;
				continue;

			case S_EOF:
				tp->type = END;
				tp->len = 0;
				s->inp = ip;
				if (tp!=trp->bp && (tp-1)->type!=NL
				 && cursource->fd!=-1)
					error(WARNING,"No newline at end of file");
				trp->lp = tp+1;
				return nmac;

			case S_NL:
				tp->t = ip-1;
				tp->type = NL;
				tp->len = 1;
				tp->wslen = 0;
				s->lineinc++;
				s->inp = ip;
				trp->lp = tp+1;
				return nmac;

			case S_EOFSTR:
				error(FATAL, "EOF in string or char constant");
				break;

			case S_STNL:
				error(ERROR, "Unterminated string or char const");
				tp->type = STRING;
				break;

			case S_COMNL:
				s->lineinc++;
				state = COM2;
				c = *(unsigned char *)ip++;
				continue;

			case S_EOFCOM:
				error(WARNING, "EOF in comment");
			case S_COMMENT:
				tp->t = ip;
				tp->t[-1] = ' ';
				tp->wslen = 1;
				c = *(unsigned char *)ip++;
				state = START;
				continue;
			}
			break;
		}
		tp->len = ip - tp->t;
		tp++;
	}
}

/* have seen ?; return the trigraph it starts (if any) else 0 */
int
trigraph(Source *s)
{
	int c;

	if ((c = getch(s)) != '?') {
		s->inp -= 1;
		return 0;
	}
	switch(c = getch(s)) {
	case '=':
		return '#';
	case '(':
		return '[';
	case '/':
		return '\\';
	case ')':
		return ']';
	case '\'':
		return '^';
	case '<':
		return '{';
	case '!':
		return '|';
	case '>':
		return '}';
	case '-':
		return '~';
	}
	s->inp -= 2;
	return 0;
}

int
getch(Source *s)
{
	int c;
	if ((c = *(unsigned char *)(s->inp))==EOB) {
		if (fillbuf(s)==EOF)
			return EOF;
		c = *(unsigned char *)(s->inp);
	}
	s->inp += 1;
	return c;
}

int
fillbuf(Source *s)
{
	int n;

	if ((n=read(s->fd, (char *)s->inl, INS/8)) <= 0)
		n = 0;
	s->inp = s->inl;
	s->inl = s->inp+n;
	s->inl[0] = s->inl[1]= s->inl[2]= s->inl[3] = EOB;
	if (n==0) {
		s->inp++;
		return EOF;
	}
	return 0;
}

/*
 * Push down to new source of characters.
 * If fd>0 and str==NULL, then from a file `name';
 * if fd==-1 and str, then from the string.
 */
Source *
setsource(char *name, int fd, char *str)
{
	Source *s = new(Source);
	int len;

	s->line = 1;
	s->lineinc = 0;
	s->fd = fd;
	s->filename = name;
	/* slop at right for EOB */
	if (str) {
		len = strlen(str);
		s->inb = domalloc(len+4);
		s->inp = s->inb;
		strncpy((char *)s->inp, str, len);
	} else {
		s->inb = domalloc(INS+4);
		s->inp = s->inb;
		len = 0;
	}
	s->inl = s->inp+len;
	s->inl[0] = s->inl[1] = EOB;
	s->next = cursource;
	s->ifdepth = 0;
	cursource = s;
	return s;
}

void
unsetsource(void)
{
	Source *s = cursource;

	if (s->fd>=0) {
		close(s->fd);
		dofree(s->inb);
	}
	cursource = s->next;
	dofree(s);
}
