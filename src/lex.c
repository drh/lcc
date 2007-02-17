#include "c.h"
#include <errno.h>

#define MAXTOKEN 32

enum { BLANK=01,  NEWLINE=02, LETTER=04,
       DIGIT=010, HEX=020,    OTHER=040 };

static unsigned char map[256] = { /* 000 nul */	0,
				   /* 001 soh */	0,
				   /* 002 stx */	0,
				   /* 003 etx */	0,
				   /* 004 eot */	0,
				   /* 005 enq */	0,
				   /* 006 ack */	0,
				   /* 007 bel */	0,
				   /* 010 bs  */	0,
				   /* 011 ht  */	BLANK,
				   /* 012 nl  */	NEWLINE,
				   /* 013 vt  */	BLANK,
				   /* 014 ff  */	BLANK,
				   /* 015 cr  */	0,
				   /* 016 so  */	0,
				   /* 017 si  */	0,
				   /* 020 dle */	0,
				   /* 021 dc1 */	0,
				   /* 022 dc2 */	0,
				   /* 023 dc3 */	0,
				   /* 024 dc4 */	0,
				   /* 025 nak */	0,
				   /* 026 syn */	0,
				   /* 027 etb */	0,
				   /* 030 can */	0,
				   /* 031 em  */	0,
				   /* 032 sub */	0,
				   /* 033 esc */	0,
				   /* 034 fs  */	0,
				   /* 035 gs  */	0,
				   /* 036 rs  */	0,
				   /* 037 us  */	0,
				   /* 040 sp  */	BLANK,
				   /* 041 !   */	OTHER,
				   /* 042 "   */	OTHER,
				   /* 043 #   */	OTHER,
				   /* 044 $   */	0,
				   /* 045 %   */	OTHER,
				   /* 046 &   */	OTHER,
				   /* 047 '   */	OTHER,
				   /* 050 (   */	OTHER,
				   /* 051 )   */	OTHER,
				   /* 052 *   */	OTHER,
				   /* 053 +   */	OTHER,
				   /* 054 ,   */	OTHER,
				   /* 055 -   */	OTHER,
				   /* 056 .   */	OTHER,
				   /* 057 /   */	OTHER,
				   /* 060 0   */	DIGIT,
				   /* 061 1   */	DIGIT,
				   /* 062 2   */	DIGIT,
				   /* 063 3   */	DIGIT,
				   /* 064 4   */	DIGIT,
				   /* 065 5   */	DIGIT,
				   /* 066 6   */	DIGIT,
				   /* 067 7   */	DIGIT,
				   /* 070 8   */	DIGIT,
				   /* 071 9   */	DIGIT,
				   /* 072 :   */	OTHER,
				   /* 073 ;   */	OTHER,
				   /* 074 <   */	OTHER,
				   /* 075 =   */	OTHER,
				   /* 076 >   */	OTHER,
				   /* 077 ?   */	OTHER,
				   /* 100 @   */	0,
				   /* 101 A   */	LETTER|HEX,
				   /* 102 B   */	LETTER|HEX,
				   /* 103 C   */	LETTER|HEX,
				   /* 104 D   */	LETTER|HEX,
				   /* 105 E   */	LETTER|HEX,
				   /* 106 F   */	LETTER|HEX,
				   /* 107 G   */	LETTER,
				   /* 110 H   */	LETTER,
				   /* 111 I   */	LETTER,
				   /* 112 J   */	LETTER,
				   /* 113 K   */	LETTER,
				   /* 114 L   */	LETTER,
				   /* 115 M   */	LETTER,
				   /* 116 N   */	LETTER,
				   /* 117 O   */	LETTER,
				   /* 120 P   */	LETTER,
				   /* 121 Q   */	LETTER,
				   /* 122 R   */	LETTER,
				   /* 123 S   */	LETTER,
				   /* 124 T   */	LETTER,
				   /* 125 U   */	LETTER,
				   /* 126 V   */	LETTER,
				   /* 127 W   */	LETTER,
				   /* 130 X   */	LETTER,
				   /* 131 Y   */	LETTER,
				   /* 132 Z   */	LETTER,
				   /* 133 [   */	OTHER,
				   /* 134 \   */	OTHER,
				   /* 135 ]   */	OTHER,
				   /* 136 ^   */	OTHER,
				   /* 137 _   */	LETTER,
				   /* 140 `   */	0,
				   /* 141 a   */	LETTER|HEX,
				   /* 142 b   */	LETTER|HEX,
				   /* 143 c   */	LETTER|HEX,
				   /* 144 d   */	LETTER|HEX,
				   /* 145 e   */	LETTER|HEX,
				   /* 146 f   */	LETTER|HEX,
				   /* 147 g   */	LETTER,
				   /* 150 h   */	LETTER,
				   /* 151 i   */	LETTER,
				   /* 152 j   */	LETTER,
				   /* 153 k   */	LETTER,
				   /* 154 l   */	LETTER,
				   /* 155 m   */	LETTER,
				   /* 156 n   */	LETTER,
				   /* 157 o   */	LETTER,
				   /* 160 p   */	LETTER,
				   /* 161 q   */	LETTER,
				   /* 162 r   */	LETTER,
				   /* 163 s   */	LETTER,
				   /* 164 t   */	LETTER,
				   /* 165 u   */	LETTER,
				   /* 166 v   */	LETTER,
				   /* 167 w   */	LETTER,
				   /* 170 x   */	LETTER,
				   /* 171 y   */	LETTER,
				   /* 172 z   */	LETTER,
				   /* 173 {   */	OTHER,
				   /* 174 |   */	OTHER,
				   /* 175 }   */	OTHER,
				   /* 176 ~   */	OTHER, };
static struct symbol tval;
static int backslash ARGS((int q));
static Symbol fcon ARGS((void));
static Symbol icon ARGS((unsigned int, int, int));
static void ppnumber ARGS((char *));
Coordinate src;		/* current source coordinate */
int t;
char *token;		/* current token */
Symbol tsym;		/* symbol table entry for current token */

int gettok() {
	for (;;) {
		register unsigned char *rcp = cp;
		while (map[*rcp]&BLANK)
			rcp++;
		if (limit - rcp < MAXTOKEN) {
			cp = rcp;
			fillbuf();
			rcp = cp;
		}
		src.file = file;
		src.x = (char *)rcp - line;
		src.y = lineno;
		cp = rcp + 1;
		switch (*rcp++) {
		case '/': if (*rcp == '*') {
			  	int c = 0;
			  	for (rcp++; *rcp != '/' || c != '*'; )
			  		if (map[*rcp]&NEWLINE) {
			  			if (rcp < limit)
			  				c = *rcp;
			  			cp = rcp + 1;
			  			nextline();
			  			rcp = cp;
			  			if (rcp == limit)
			  				break;
			  		} else
			  			c = *rcp++;
			  	if (rcp < limit)
			  		rcp++;
			  	else
			  		error("unclosed comment\n");
			  	cp = rcp;
			  	continue;
			  }
			  return '/';
		case 'L': if (*rcp == '\'') {
			  	int t = gettok();
			  	assert(t == ICON);
			  	src.x--;
			  	tval.type = unsignedchar;
			  	tval.u.c.v.uc = tval.u.c.v.i;
			  	return t;
			  }
			  if (*rcp != '"')
			  	goto id;
			  cp = rcp + 1;
			  goto scon;
case '<':
	if (*rcp == '=') return cp++, LEQ;
	if (*rcp == '<') return cp++, LSHIFT;
	return '<';
case '>':
	if (*rcp == '=') return cp++, GEQ;
	if (*rcp == '>') return cp++, RSHIFT;
	return '>';
case '-':
	if (*rcp == '>') return cp++, DEREF;
	if (*rcp == '-') return cp++, DECR;
	return '-';
case '=': return *rcp == '=' ? cp++, EQL    : '=';
case '!': return *rcp == '=' ? cp++, NEQ    : '!';
case '|': return *rcp == '|' ? cp++, OROR   : '|';
case '&': return *rcp == '&' ? cp++, ANDAND : '&';
case '+': return *rcp == '+' ? cp++, INCR   : '+';
case ';': case ',': case ':':
case '*': case '~': case '%': case '^': case '?':
case '[': case ']': case '{': case '}': case '(': case ')': 
	return rcp[-1];
		case '\n': case '\v': case '\r': case '\f':
			nextline();
			if (cp == limit) {
				tsym = NULL;
				return EOI;
			}
			continue;

		case 'i':
			if (rcp[0] == 'f'
			&& !(map[rcp[1]]&(DIGIT|LETTER))) {
				cp = rcp + 1;
				return IF;
			}
			if (rcp[0] == 'n'
			&&  rcp[1] == 't'
			&& !(map[rcp[2]]&(DIGIT|LETTER))) {
				cp = rcp + 2;
				tsym = inttype->u.sym;
				return INT;
			}
			goto id;
		case 'h': case 'j': case 'k': case 'm': case 'n': case 'o':
		case 'p': case 'q': case 'x': case 'y': case 'z':
		case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
		case 'G': case 'H': case 'I': case 'J': case 'K':
		case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
		case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
		case 'Y': case 'Z': case '_':
		id:
			if (limit - rcp < MAXLINE) {
				cp = rcp - 1;
				fillbuf();
				rcp = ++cp;
			}
			assert(cp == rcp);
			token = (char *)rcp - 1;
			while (map[*rcp]&(DIGIT|LETTER))
				rcp++;
			token = stringn(token, (char *)rcp - token);
			tsym = lookup(token, identifiers);
			cp = rcp;
			return ID;
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9': {
			unsigned int n = 0;
			if (limit - rcp < MAXLINE) {
				cp = rcp - 1;
				fillbuf();
				rcp = ++cp;
			}
			assert(cp == rcp);
			token = (char *)rcp - 1;
			if (*token == '0' && (*rcp == 'x' || *rcp == 'X')) {
				int d, overflow = 0;
				while (*++rcp) {
					if (map[*rcp]&DIGIT)
						d = *rcp - '0';
					else if (*rcp >= 'a' && *rcp <= 'f')
						d = *rcp - 'a' + 10;
					else if (*rcp >= 'A' && *rcp <= 'F')
						d = *rcp - 'A' + 10;
					else
						break;
					if (n&~((unsigned)-1 >> 4))
						overflow = 1;
					else
						n = (n<<4) + d;
				}
				if ((char *)rcp - token <= 2)
					error("invalid hexadecimal constant `%S'\n", token, (char *)rcp-token);
				cp = rcp;
				tsym = icon(n, overflow, 16);
			} else if (*token == '0') {
				int err = 0, overflow = 0;
				for ( ; map[*rcp]&DIGIT; rcp++) {
					if (*rcp == '8' || *rcp == '9')
						err = 1;
					if (n&~((unsigned)-1 >> 3))
						overflow = 1;
					else
						n = (n<<3) + (unsigned)(*rcp - '0');
				}
if (*rcp == '.' || *rcp == 'e' || *rcp == 'E') {
	cp = rcp;
	tsym = fcon();
	return FCON;
}
				cp = rcp;
				tsym = icon(n, overflow, 8);
				if (err)
					error("invalid octal constant `%S'\n", token, (char*)cp-token);
			} else {
				int overflow = 0;
				for (n = *token - '0'; map[*rcp]&DIGIT; ) {
					int d = *rcp++ - '0';
					if (n > ((unsigned)UINT_MAX - d)/10)
						overflow = 1;
					else
						n = 10*n + d;
				}
if (*rcp == '.' || *rcp == 'e' || *rcp == 'E') {
	cp = rcp;
	tsym = fcon();
	return FCON;
}
				cp = rcp;
				tsym = icon(n, overflow, 10);
			}
			return ICON;
		}
		case '.':
			if (rcp[0] == '.' && rcp[1] == '.') {
				cp += 2;
				return ELLIPSIS;
			}
			if ((map[*rcp]&DIGIT) == 0)
				return '.';
			if (limit - rcp < MAXLINE) {
				cp = rcp - 1;
				fillbuf();
				rcp = ++cp;
			}
			assert(cp == rcp);
			cp = rcp - 1;
			token = (char *)cp;
			tsym = fcon();
			return FCON;
		scon:
		case '\'': case '"': {
			static char cbuf[BUFSIZE+1];
			char *s = cbuf;
			int nbad = 0;
			*s++ = *--cp;
			do {
				cp++;
				while (*cp != cbuf[0]) {
					int c;
					if (map[*cp]&NEWLINE) {
						if (cp < limit)
							break;
						cp++;
						nextline();
						if (cp == limit)
							break;
						continue;
					}
					c = *cp++;
					if (c == '\\') {
						if (map[*cp]&NEWLINE) {
							if (cp < limit)
								break;
							cp++;
							nextline();
						}
						if (limit - cp < MAXTOKEN)
							fillbuf();
						c = backslash(cbuf[0]);
					} else if (map[c] == 0)
						nbad++;
					if (s < &cbuf[sizeof cbuf] - 2)
						*s++ = c;
				}
				if (*cp == cbuf[0])
					cp++;
				else
					error("missing %c\n", cbuf[0]);
			} while (cbuf[0] == '"' && getchr() == '"');
			*s++ = 0;
			if (s >= &cbuf[sizeof cbuf])
				error("%s literal too long\n",
					cbuf[0] == '"' ? "string" : "character");
			if (Aflag >= 2 && cbuf[0] == '"' && s - cbuf - 1 > 509)
				warning("more than 509 characters in a string literal\n");
			if (Aflag >= 2 && nbad)
				warning("%s literal contains non-portable characters\n",
					cbuf[0] == '"' ? "string" : "character");
			token = cbuf;
			tsym = &tval;
			if (cbuf[0] == '"') {
				tval.type = array(chartype, s - cbuf - 1, 0);
				tval.u.c.v.p = cbuf + 1;
				return SCON;
			} else {
				if (s - cbuf > 3)
					warning("excess characters in multibyte character literal `%S' ignored\n", token, (char*)cp-token);

				else if (s - cbuf <= 2)
					error("missing '\n");
				tval.type = inttype;
				tval.u.c.v.i = cbuf[1];
				return ICON;
			}
		}
		case 'a':
			if (rcp[0] == 'u'
			&&  rcp[1] == 't'
			&&  rcp[2] == 'o'
			&& !(map[rcp[3]]&(DIGIT|LETTER))) {
				cp = rcp + 3;
				return AUTO;
			}
			goto id;
		case 'b':
			if (rcp[0] == 'r'
			&&  rcp[1] == 'e'
			&&  rcp[2] == 'a'
			&&  rcp[3] == 'k'
			&& !(map[rcp[4]]&(DIGIT|LETTER))) {
				cp = rcp + 4;
				return BREAK;
			}
			goto id;
		case 'c':
			if (rcp[0] == 'a'
			&&  rcp[1] == 's'
			&&  rcp[2] == 'e'
			&& !(map[rcp[3]]&(DIGIT|LETTER))) {
				cp = rcp + 3;
				return CASE;
			}
			if (rcp[0] == 'h'
			&&  rcp[1] == 'a'
			&&  rcp[2] == 'r'
			&& !(map[rcp[3]]&(DIGIT|LETTER))) {
				cp = rcp + 3;
				tsym = chartype->u.sym;
				return CHAR;
			}
			if (rcp[0] == 'o'
			&&  rcp[1] == 'n'
			&&  rcp[2] == 's'
			&&  rcp[3] == 't'
			&& !(map[rcp[4]]&(DIGIT|LETTER))) {
				cp = rcp + 4;
				return CONST;
			}
			if (rcp[0] == 'o'
			&&  rcp[1] == 'n'
			&&  rcp[2] == 't'
			&&  rcp[3] == 'i'
			&&  rcp[4] == 'n'
			&&  rcp[5] == 'u'
			&&  rcp[6] == 'e'
			&& !(map[rcp[7]]&(DIGIT|LETTER))) {
				cp = rcp + 7;
				return CONTINUE;
			}
			goto id;
		case 'd':
			if (rcp[0] == 'e'
			&&  rcp[1] == 'f'
			&&  rcp[2] == 'a'
			&&  rcp[3] == 'u'
			&&  rcp[4] == 'l'
			&&  rcp[5] == 't'
			&& !(map[rcp[6]]&(DIGIT|LETTER))) {
				cp = rcp + 6;
				return DEFAULT;
			}
			if (rcp[0] == 'o'
			&&  rcp[1] == 'u'
			&&  rcp[2] == 'b'
			&&  rcp[3] == 'l'
			&&  rcp[4] == 'e'
			&& !(map[rcp[5]]&(DIGIT|LETTER))) {
				cp = rcp + 5;
				tsym = doubletype->u.sym;
				return DOUBLE;
			}
			if (rcp[0] == 'o'
			&& !(map[rcp[1]]&(DIGIT|LETTER))) {
				cp = rcp + 1;
				return DO;
			}
			goto id;
		case 'e':
			if (rcp[0] == 'l'
			&&  rcp[1] == 's'
			&&  rcp[2] == 'e'
			&& !(map[rcp[3]]&(DIGIT|LETTER))) {
				cp = rcp + 3;
				return ELSE;
			}
			if (rcp[0] == 'n'
			&&  rcp[1] == 'u'
			&&  rcp[2] == 'm'
			&& !(map[rcp[3]]&(DIGIT|LETTER))) {
				cp = rcp + 3;
				return ENUM;
			}
			if (rcp[0] == 'x'
			&&  rcp[1] == 't'
			&&  rcp[2] == 'e'
			&&  rcp[3] == 'r'
			&&  rcp[4] == 'n'
			&& !(map[rcp[5]]&(DIGIT|LETTER))) {
				cp = rcp + 5;
				return EXTERN;
			}
			goto id;
		case 'f':
			if (rcp[0] == 'l'
			&&  rcp[1] == 'o'
			&&  rcp[2] == 'a'
			&&  rcp[3] == 't'
			&& !(map[rcp[4]]&(DIGIT|LETTER))) {
				cp = rcp + 4;
				tsym = floattype->u.sym;
				return FLOAT;
			}
			if (rcp[0] == 'o'
			&&  rcp[1] == 'r'
			&& !(map[rcp[2]]&(DIGIT|LETTER))) {
				cp = rcp + 2;
				return FOR;
			}
			goto id;
		case 'g':
			if (rcp[0] == 'o'
			&&  rcp[1] == 't'
			&&  rcp[2] == 'o'
			&& !(map[rcp[3]]&(DIGIT|LETTER))) {
				cp = rcp + 3;
				return GOTO;
			}
			goto id;
		case 'l':
			if (rcp[0] == 'o'
			&&  rcp[1] == 'n'
			&&  rcp[2] == 'g'
			&& !(map[rcp[3]]&(DIGIT|LETTER))) {
				cp = rcp + 3;
				tsym = longtype->u.sym;
				return LONG;
			}
			goto id;
		case 'r':
			if (rcp[0] == 'e'
			&&  rcp[1] == 'g'
			&&  rcp[2] == 'i'
			&&  rcp[3] == 's'
			&&  rcp[4] == 't'
			&&  rcp[5] == 'e'
			&&  rcp[6] == 'r'
			&& !(map[rcp[7]]&(DIGIT|LETTER))) {
				cp = rcp + 7;
				return REGISTER;
			}
			if (rcp[0] == 'e'
			&&  rcp[1] == 't'
			&&  rcp[2] == 'u'
			&&  rcp[3] == 'r'
			&&  rcp[4] == 'n'
			&& !(map[rcp[5]]&(DIGIT|LETTER))) {
				cp = rcp + 5;
				return RETURN;
			}
			goto id;
		case 's':
			if (rcp[0] == 'h'
			&&  rcp[1] == 'o'
			&&  rcp[2] == 'r'
			&&  rcp[3] == 't'
			&& !(map[rcp[4]]&(DIGIT|LETTER))) {
				cp = rcp + 4;
				tsym = shorttype->u.sym;
				return SHORT;
			}
			if (rcp[0] == 'i'
			&&  rcp[1] == 'g'
			&&  rcp[2] == 'n'
			&&  rcp[3] == 'e'
			&&  rcp[4] == 'd'
			&& !(map[rcp[5]]&(DIGIT|LETTER))) {
				cp = rcp + 5;
				return SIGNED;
			}
			if (rcp[0] == 'i'
			&&  rcp[1] == 'z'
			&&  rcp[2] == 'e'
			&&  rcp[3] == 'o'
			&&  rcp[4] == 'f'
			&& !(map[rcp[5]]&(DIGIT|LETTER))) {
				cp = rcp + 5;
				return SIZEOF;
			}
			if (rcp[0] == 't'
			&&  rcp[1] == 'a'
			&&  rcp[2] == 't'
			&&  rcp[3] == 'i'
			&&  rcp[4] == 'c'
			&& !(map[rcp[5]]&(DIGIT|LETTER))) {
				cp = rcp + 5;
				return STATIC;
			}
			if (rcp[0] == 't'
			&&  rcp[1] == 'r'
			&&  rcp[2] == 'u'
			&&  rcp[3] == 'c'
			&&  rcp[4] == 't'
			&& !(map[rcp[5]]&(DIGIT|LETTER))) {
				cp = rcp + 5;
				return STRUCT;
			}
			if (rcp[0] == 'w'
			&&  rcp[1] == 'i'
			&&  rcp[2] == 't'
			&&  rcp[3] == 'c'
			&&  rcp[4] == 'h'
			&& !(map[rcp[5]]&(DIGIT|LETTER))) {
				cp = rcp + 5;
				return SWITCH;
			}
			goto id;
		case 't':
			if (rcp[0] == 'y'
			&&  rcp[1] == 'p'
			&&  rcp[2] == 'e'
			&&  rcp[3] == 'd'
			&&  rcp[4] == 'e'
			&&  rcp[5] == 'f'
			&& !(map[rcp[6]]&(DIGIT|LETTER))) {
				cp = rcp + 6;
				return TYPEDEF;
			}
			goto id;
		case 'u':
			if (rcp[0] == 'n'
			&&  rcp[1] == 'i'
			&&  rcp[2] == 'o'
			&&  rcp[3] == 'n'
			&& !(map[rcp[4]]&(DIGIT|LETTER))) {
				cp = rcp + 4;
				return UNION;
			}
			if (rcp[0] == 'n'
			&&  rcp[1] == 's'
			&&  rcp[2] == 'i'
			&&  rcp[3] == 'g'
			&&  rcp[4] == 'n'
			&&  rcp[5] == 'e'
			&&  rcp[6] == 'd'
			&& !(map[rcp[7]]&(DIGIT|LETTER))) {
				cp = rcp + 7;
				return UNSIGNED;
			}
			goto id;
		case 'v':
			if (rcp[0] == 'o'
			&&  rcp[1] == 'i'
			&&  rcp[2] == 'd'
			&& !(map[rcp[3]]&(DIGIT|LETTER))) {
				cp = rcp + 3;
				tsym = voidtype->u.sym;
				return VOID;
			}
			if (rcp[0] == 'o'
			&&  rcp[1] == 'l'
			&&  rcp[2] == 'a'
			&&  rcp[3] == 't'
			&&  rcp[4] == 'i'
			&&  rcp[5] == 'l'
			&&  rcp[6] == 'e'
			&& !(map[rcp[7]]&(DIGIT|LETTER))) {
				cp = rcp + 7;
				return VOLATILE;
			}
			goto id;
		case 'w':
			if (rcp[0] == 'h'
			&&  rcp[1] == 'i'
			&&  rcp[2] == 'l'
			&&  rcp[3] == 'e'
			&& !(map[rcp[4]]&(DIGIT|LETTER))) {
				cp = rcp + 4;
				return WHILE;
			}
			goto id;
		default:
			if ((map[cp[-1]]&BLANK) == 0)
				if (cp[-1] < ' ' || cp[-1] >= 0177)
					error("illegal character `\\0%o'\n", cp[-1]);
				else
					error("illegal character `%c'\n", cp[-1]);
		}
	}
}
static Symbol icon(n, overflow, base)
unsigned n; int overflow, base; {
	if ((*cp=='u'||*cp=='U') && (cp[1]=='l'||cp[1]=='L')
	||  (*cp=='l'||*cp=='L') && (cp[1]=='u'||cp[1]=='U')) {
		tval.type = unsignedlong;
		cp += 2;
	} else if (*cp == 'u' || *cp == 'U') {
		tval.type = unsignedtype;
		cp += 1;
	} else if (*cp == 'l' || *cp == 'L') {
		if (n > (unsigned)~(1<<8*longtype->size - 1))
			tval.type = unsignedlong;
		else
			tval.type = longtype;
		cp += 1;
	} else if (base == 10 && n > (unsigned)~(1<<8*longtype->size - 1))
		tval.type = unsignedlong;
	else if (n > (unsigned)~(1<<8*inttype->size - 1))
		tval.type = unsignedtype;
	else
		tval.type = inttype;
	if (overflow) {
		warning("overflow in constant `%S'\n", token,
			(char*)cp - token);
		n = ~(1<<8*longtype->size - 1);
		tval.type = longtype;
	}
	if (isunsigned(tval.type))
		tval.u.c.v.u = n;
	else
		tval.u.c.v.i = n;		
	ppnumber("integer");
	return &tval;
}
static void ppnumber(which) char *which; {
	unsigned char *rcp = cp--;

	for ( ; (map[*cp]&(DIGIT|LETTER)) || *cp == '.'; cp++)
		if ((cp[0] == 'E' || cp[0] == 'e')
		&&  (cp[1] == '-' || cp[1] == '+'))
			cp++;
	if (cp > rcp)
		error("`%S' is a preprocessing number but an invalid %s constant\n", token,

			(char*)cp-token, which);
}
static Symbol fcon() {
	if (*cp == '.')
		do
			cp++;
		while (map[*cp]&DIGIT);
	if (*cp == 'e' || *cp == 'E') {
		if (*++cp == '-' || *cp == '+')
			cp++;
		if (map[*cp]&DIGIT)
			do
				cp++;
			while (map[*cp]&DIGIT);
		else
			error("invalid floating constant `%S'\n", token,
				(char*)cp - token);
	}

	errno = 0;
	tval.u.c.v.d = strtod(token, NULL);
	if (errno == ERANGE)
		warning("overflow in floating constant `%S'\n", token,
			(char*)cp - token);
	if (*cp == 'f' || *cp == 'F') {
		++cp;
		if (tval.u.c.v.d > FLT_MAX)
			warning("overflow in floating constant `%S'\n", token,
				(char*)cp - token);
		tval.type = floattype;
		tval.u.c.v.f = tval.u.c.v.d;
	} else if (*cp == 'l' || *cp == 'L') {
		cp++;
		tval.type = longdouble;
	} else
		tval.type = doubletype;
	ppnumber("floating");
	return &tval;
}

int getchr() {
	for (;;) {
		while (map[*cp]&BLANK)
			cp++;
		if (!(map[*cp]&NEWLINE))
			return *cp;
		cp++;
		nextline();
		if (cp == limit)
			return EOI;
	}
}
static int backslash(q) int q; {
	int c;

	switch (*cp++) {
	case 'a': return 7;
	case 'b': return '\b';
	case 'f': return '\f';
	case 'n': return '\n';
	case 'r': return '\r';
	case 't': return '\t';
	case 'v': return '\v';
	case '\'': case '"': case '\\': case '\?': break;
	case 'x': {
		int overflow = 0;
		if ((map[*cp]&(DIGIT|HEX)) == 0) {
			if (*cp < ' ' || *cp == 0177)
				error("ill-formed hexadecimal escape sequence\n");
			else
				error("ill-formed hexadecimal escape sequence `\\x%c'\n", *cp);
			if (*cp != q)
				cp++;
			return 0;
		}
		for (c = 0; map[*cp]&(DIGIT|HEX); cp++) {
			if (c&~((unsigned)-1 >> 4))
				overflow++;
			if (map[*cp]&DIGIT)
				c = (c<<4) + *cp - '0';
			else
				c = (c<<4) + (*cp&~040) - 'A' + 10;
		}
		if (c&~0377 || overflow)
			warning("overflow in hexadecimal escape sequence\n");
		return c&0377;
		}
	case '0': case '1': case '2': case '3':
	case '4': case '5': case '6': case '7':
		c = *(cp-1) - '0';
		if (*cp >= '0' && *cp <= '7') {
			c = (c<<3) + *cp++ - '0';
			if (*cp >= '0' && *cp <= '7')
				c = (c<<3) + *cp++ - '0';
		}
		if (c&~0377)
			warning("overflow in octal escape sequence\n");
		return c&0377;
	default:
		if (cp[-1] < ' ' || cp[-1] >= 0177)
			warning("unrecognized character escape sequence\n");
		else
			warning("unrecognized character escape sequence `\\%c'\n", cp[-1]);
	}
	return cp[-1];
}
