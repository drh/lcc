#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "iburg.h"

static char rcsid[] = "$Id$";
static char *prefix = "burm";
static int Iflag = 0, Tflag = 0;
static int ntnumber = 0;
static Nonterm start = 0;
static Term terms;
static Nonterm nts;
static Rule rules;
static int nrules;

static char *stringf(char *fmt, ...);
static void print(char *fmt, ...);
static void ckreach(Nonterm p);
static void emitclosure(Nonterm nts);
static void emitcost(Tree t, char *v);
static void emitdefs(Nonterm nts, int ntnumber);
static void emitfuncs(void);
static void emitheader(void);
static void emitkids(Rule rules, int nrules);
static void emitlabel(Nonterm start);
static void emitleaf(Term p, int ntnumber);
static void emitnts(Rule rules, int nrules);
static void emitrecord(char *pre, Rule r, int cost);
static void emitrule(Nonterm nts);
static void emitstate(Term terms, Nonterm start, int ntnumber);
static void emitstring(Rule rules);
static void emitstruct(Nonterm nts, int ntnumber);
static void emitterms(Term terms);
static void emittest(Tree t, char *v, char *suffix);

int main(int argc, char *argv[]) {
	int c, i;
	Nonterm p;
	
	for (i = 1; i < argc; i++)
		if (strcmp(argv[i], "-I") == 0)
			Iflag = 1;
		else if (strcmp(argv[i], "-T") == 0)
			Tflag = 1;
		else if (strncmp(argv[i], "-p", 2) == 0 && argv[i][2])
			prefix = &argv[i][2];
		else if (strncmp(argv[i], "-p", 2) == 0 && i + 1 < argc)
			prefix = argv[++i];
		else if (*argv[i] == '-' && argv[i][1]) {
			yyerror("usage: %s [-T | -I | -p prefix]... [ [ input ] output \n",
				argv[0]);
			exit(1);
		} else if (infp == NULL) {
			if (strcmp(argv[i], "-") == 0)
				infp = stdin;
			else if ((infp = fopen(argv[i], "r")) == NULL) {
				yyerror("%s: can't read `%s'\n", argv[0], argv[i]);
				exit(1);
			}
		} else if (outfp == NULL) {
			if (strcmp(argv[i], "-") == 0)
				outfp = stdout;
			if ((outfp = fopen(argv[i], "w")) == NULL) {
				yyerror("%s: can't write `%s'\n", argv[0], argv[i]);
				exit(1);
			}
		}
	if (infp == NULL)
		infp = stdin;
	if (outfp == NULL)
		outfp = stdout;
	yyparse();
	if (start)
		ckreach(start);
	for (p = nts; p; p = p->link)
		if (!p->reached)
			yyerror("can't reach non-terminal `%s'\n", p->name);
	emitheader();
	emitdefs(nts, ntnumber);
	emitstruct(nts, ntnumber);
	emitnts(rules, nrules);
	emitterms(terms);
	if (Iflag)
		emitstring(rules);
	emitrule(nts);
	emitclosure(nts);
	if (start)
		emitstate(terms, start, ntnumber);
	print("#ifdef STATE_LABEL\n");
	if (start)
		emitlabel(start);
	emitkids(rules, nrules);
	emitfuncs();
	print("#endif\n");
	if (!feof(infp))
		while ((c = getc(infp)) != EOF)
			putc(c, outfp);
	return errcnt > 0;
}

/* alloc - allocate nbytes or issue fatal error */
void *alloc(int nbytes) {
	void *p = calloc(1, nbytes);

	if (p == NULL) {
		yyerror("out of memory\n");
		exit(1);
	}
	return p;
}

/* stringf - format and save a string */
static char *stringf(char *fmt, ...) {
	va_list ap;
	char *s, buf[512];

	va_start(ap, fmt);
	vsprintf(buf, fmt, ap);
	va_end(ap);
	return strcpy(alloc(strlen(buf) + 1), buf);
}	

struct entry {
	union {
		char *name;
		struct term t;
		struct nonterm nt;
	} sym;
	struct entry *link;
} *table[211];
#define HASHSIZE (sizeof table/sizeof table[0])

/* hash - return hash number for str */
static unsigned hash(char *str) {
	unsigned h = 0;

	while (*str)
		h = (h<<1) + *str++;
	return h;
}

/* lookup - lookup symbol name */
static void *lookup(char *name) {
	struct entry *p = table[hash(name)%HASHSIZE];

	for ( ; p; p = p->link)
		if (strcmp(name, p->sym.name) == 0)
			return &p->sym;
	return 0;
}

/* install - install symbol name */
static void *install(char *name) {
	struct entry *p = alloc(sizeof *p);
	int i = hash(name)%HASHSIZE;

	p->sym.name = name;
	p->link = table[i];
	table[i] = p;
	return &p->sym;
}

/* nonterm - create a new terminal id, if necessary */
Nonterm nonterm(char *id) {
	Nonterm p = lookup(id), *q = &nts;

	if (p && p->kind == NONTERM)
		return p;
	if (p && p->kind == TERM)
		yyerror("`%s' is a terminal\n", id);
	p = install(id);
	p->kind = NONTERM;
	p->number = ++ntnumber;
	if (p->number == 1)
		start = p;
	while (*q && (*q)->number < p->number)
		q = &(*q)->link;
	assert(*q == 0 || (*q)->number != p->number);
	p->link = *q;
	*q = p;
	return p;
}

/* term - create a new terminal id with external symbol number esn */
Term term(char *id, int esn) {
	Term p = lookup(id), *q = &terms;

	if (p)
		yyerror("redefinition of terminal `%s'\n", id);
	else
		p = install(id);
	p->kind = TERM;
	p->esn = esn;
	p->arity = -1;
	while (*q && (*q)->esn < p->esn)
		q = &(*q)->link;
	if (*q && (*q)->esn == p->esn)
		yyerror("duplicate external symbol number `%s=%d'\n",
			p->name, p->esn);
	p->link = *q;
	*q = p;
	return p;
}

/* tree - create & initialize a tree node with the given fields */
Tree tree(char *id, Tree left, Tree right) {
	Tree t = alloc(sizeof *t);
	Term p = lookup(id);
	int arity = 0;

	if (left && right)
		arity = 2;
	else if (left)
		arity = 1;
	if (p == NULL && arity > 0) {
		yyerror("undefined terminal `%s'\n", id);
		p = term(id, -1);
	} else if (p == NULL && arity == 0)
		p = (Term)nonterm(id);
	else if (p && p->kind == NONTERM && arity > 0) {
		yyerror("`%s' is a non-terminal\n", id);
		p = term(id, -1);
	}
	if (p->kind == TERM && p->arity == -1)
		p->arity = arity;
	if (p->kind == TERM && arity != p->arity)
		yyerror("inconsistent arity for terminal `%s'\n", id);
	t->op = p;
	t->nterms = p->kind == TERM;
	if (t->left = left)
		t->nterms += left->nterms;
	if (t->right = right)
		t->nterms += right->nterms;
	return t;
}

/* rule - create & initialize a rule with the given fields */
Rule rule(char *id, Tree pattern, int ern, int cost) {
	Rule r = alloc(sizeof *r), *q;
	Term p = pattern->op;

	nrules++;
	r->lhs = nonterm(id);
	r->packed = ++r->lhs->lhscount;
	for (q = &r->lhs->rules; *q; q = &(*q)->decode)
		;
	*q = r;
	r->pattern = pattern;
	r->ern = ern;
	r->cost = cost;
	if (p->kind == TERM) {
		r->next = p->rules;
		p->rules = r;
	} else if (pattern->left == NULL && pattern->right == NULL) {
		Nonterm p = pattern->op;
		r->chain = p->chain;
	        p->chain = r;
	}
	for (q = &rules; *q && (*q)->ern < r->ern; q = &(*q)->link)
		;
	if (*q && (*q)->ern == r->ern)
		yyerror("duplicate external rule number `%d'\n", r->ern);
	r->link = *q;
	*q = r;
	return r;
}

/* print - formatted output */
static void print(char *fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
	for ( ; *fmt; fmt++)
		if (*fmt == '%')
			switch (*++fmt) {
			case 'd': fprintf(outfp, "%d", va_arg(ap, int)); break;
			case 's': fputs(va_arg(ap, char *), outfp); break;
			case 'P': fprintf(outfp, "%s_", prefix); break;
			case 'T': {
				Tree t = va_arg(ap, Tree);
				print("%S", t->op);
				if (t->left && t->right)
					print("(%T,%T)", t->left, t->right);
				else if (t->left)
					print("(%T)", t->left);
				break;
				}
			case 'R': {
				Rule r = va_arg(ap, Rule);
				print("%S: %T", r->lhs, r->pattern);
				break;
				}
			case 'S': fputs(va_arg(ap, Term)->name, outfp); break;
			case '1': case '2': case '3': case '4': case '5': {
				int n = *fmt - '0';
				while (n-- > 0)
					putc('\t', outfp);
				break;
				}
			default: putc(*fmt, outfp); break;			
			}
		else
			putc(*fmt, outfp);
	va_end(ap);
}

/* reach - mark all non-terminals in tree t as reachable */
static void reach(Tree t) {
	Nonterm p = t->op;

	if (p->kind == NONTERM)
		if (!p->reached)
			ckreach(p);
	if (t->left)
		reach(t->left);
	if (t->right)
		reach(t->right);
}

/* ckreach - mark all non-terminals reachable from p */
static void ckreach(Nonterm p) {
	Rule r;

        p->reached = 1;
	for (r = p->rules; r; r = r->decode)
		reach(r->pattern);
}

/* emitcase - emit one case in function state */
static void emitcase(Term p, int ntnumber) {
	Rule r;

	print("%1case %d: /* %S */\n", p->esn, p);
	switch (p->arity) {
	case 0: case -1:
		if (!Tflag) {
			emitleaf(p, ntnumber);
			return;
		}
		break;
	case 1: print("%2assert(l);\n"); break;
	case 2: print("%2assert(l && r);\n"); break;
	default: assert(0);
	}
	for (r = p->rules; r; r = r->next) {
		switch (p->arity) {
		case 0: case -1:
			print("%2{%1/* %R */\n%3c = ", r);
			break;
		case 1:
			if (r->pattern->nterms > 1) {
				print("%2if (%1/* %R */\n", r);
				emittest(r->pattern->left, "l", " ");
				print("%2) {\n%3c = ");
			} else
				print("%2{%1/* %R */\n%3c = ", r);
			emitcost(r->pattern->left, "l");
			break;
		case 2:
			if (r->pattern->nterms > 1) {
				print("%2if (%1/* %R */\n", r);
				emittest(r->pattern->left,  "l",
					r->pattern->right->nterms ? " && " : " ");
				emittest(r->pattern->right, "r", " ");
				print("%2) {\n%3c = ");
			} else
				print("%2{%1/* %R */\n%3c = ", r);
			emitcost(r->pattern->left,  "l");
			emitcost(r->pattern->right, "r");
			break;
		default: assert(0);
		}
		print("%d;\n", r->cost);
		emitrecord("\t\t\t", r, 0);
		print("%2}\n");
	}
	print("%2break;\n");
}

/* emitclosure - emit the closure functions */
static void emitclosure(Nonterm nts) {
	Nonterm p;

	for (p = nts; p; p = p->link)
		if (p->chain)
			print("static void %Pclosure_%S(struct %Pstate *, int);\n", p);
	print("\n");
	for (p = nts; p; p = p->link)
		if (p->chain) {
			Rule r;
			print("static void %Pclosure_%S(struct %Pstate *p, int c) {\n", p);
			for (r = p->chain; r; r = r->chain)
				emitrecord("\t", r, r->cost);
			print("}\n\n");
		}
}

/* emitcost - emit cost computation for tree t */
static void emitcost(Tree t, char *v) {
	Nonterm p = t->op;

	if (p->kind == TERM) {
		if (t->left)
			emitcost(t->left,  stringf("%s->left",  v));
		if (t->right)
			emitcost(t->right, stringf("%s->right", v));
	} else
		print("%s->cost[%P%S_NT] + ", v, p);
}

/* emitdefs - emit non-terminal defines and data structures */
static void emitdefs(Nonterm nts, int ntnumber) {
	Nonterm p;

	for (p = nts; p; p = p->link)
		print("#define %P%S_NT %d\n", p, p->number);
	print("int %Pmax_nt = %d;\n\n", ntnumber);
	if (Iflag) {
		print("char *%Pntname[] = {\n%10,\n");
		for (p = nts; p; p = p->link)
			print("%1\"%S\",\n", p);
		print("%10\n};\n\n");
	}
}

/* emitfuncs - emit functions to access node fields */
static void emitfuncs(void) {
	print("int %Pop_label(NODEPTR_TYPE p) {\n"
"%1%Passert(p, PANIC(\"NULL tree in %Pop_label\\n\"));\n"
"%1return OP_LABEL(p);\n}\n\n");
	print("int %Pstate_label(NODEPTR_TYPE p) {\n"
"%1%Passert(p, PANIC(\"NULL tree in %Pstate_label\\n\"));\n"
"%1return STATE_LABEL(p);\n}\n\n");
	print("NODEPTR_TYPE %Pchild(NODEPTR_TYPE p, int index) {\n"
"%1%Passert(p, PANIC(\"NULL tree in %Pchild\\n\"));\n"
"%1switch (index) {\n%1case 0:%1return LEFT_CHILD(p);\n"
"%1case 1:%1return RIGHT_CHILD(p);\n%1}\n"
"%1%Passert(0, PANIC(\"Bad index %%d in %Pchild\\n\", index));\n%1return 0;\n}\n\n");
}

/* emitheader - emit initial definitions */
static void emitheader(void) {
	print("#ifndef ALLOC\n#define ALLOC(n) malloc(n)\n#endif\n\n"
"#ifndef %Passert\n#define %Passert(x,y) if (!(x)) { extern void abort(void); y; abort(); }\n#endif\n\n");
	if (Tflag)
		print("static NODEPTR_TYPE %Pnp;\n\n");
}

/* computekids - compute paths to kids in tree t */
static char *computekids(Tree t, char *v, char *bp, int *ip) {
	Term p = t->op;

	if (p->kind == NONTERM) {
		sprintf(bp, "\t\tkids[%d] = %s;\n", (*ip)++, v);
		bp += strlen(bp);
	} else if (p->arity > 0) {
		bp = computekids(t->left, stringf("LEFT_CHILD(%s)", v), bp, ip);
		if (p->arity == 2)
			bp = computekids(t->right, stringf("RIGHT_CHILD(%s)", v), bp, ip);
	}
	return bp;
}

/* emitkids - emit burm_kids */
static void emitkids(Rule rules, int nrules) {
	int i;
	Rule r, *rc = alloc((nrules + 1)*sizeof *rc);
	char **str  = alloc((nrules + 1)*sizeof *str);

	for (i = 0, r = rules; r; r = r->link) {
		int j = 0;
		char buf[1024], *bp = buf;
		*computekids(r->pattern, "p", bp, &j) = 0;
		for (j = 0; str[j] && strcmp(str[j], buf); j++)
			;
		if (str[j] == NULL)
			str[j] = strcpy(alloc(strlen(buf) + 1), buf);
		r->kids = rc[j];
		rc[j] = r;
	}
	print("NODEPTR_TYPE *%Pkids(NODEPTR_TYPE p, int eruleno, NODEPTR_TYPE kids[]) {\n"
"%1%Passert(p, PANIC(\"NULL tree in %Pkids\\n\"));\n"
"%1%Passert(kids, PANIC(\"NULL kids in %Pkids\\n\"));\n"
"%1switch (eruleno) {\n");
	for (i = 0; r = rc[i]; i++) {
		for ( ; r; r = r->kids)
			print("%1case %d: /* %R */\n", r->ern, r);
		print("%s%2break;\n", str[i]);
	}
	print("%1default:\n%2%Passert(0, PANIC(\"Bad external rule number %%d in %Pkids\\n\", eruleno));\n%1}\n%1return kids;\n}\n\n");
}

/* emitlabel - emit the labelling functions */
static void emitlabel(Nonterm start) {
	print("static void %Plabel1(NODEPTR_TYPE p) {\n"
"%1%Passert(p, PANIC(\"NULL tree in %Plabel\\n\"));\n"
"%1switch (%Parity[OP_LABEL(p)]) {\n"
"%1case 0:\n");
	if (Tflag)
		print("%2%Pnp = p;\n");
	print("%2STATE_LABEL(p) = %Pstate(OP_LABEL(p), 0, 0);\n%2break;\n"
"%1case 1:\n%2%Plabel1(LEFT_CHILD(p));\n");
	if (Tflag)
		print("%2%Pnp = p;\n");
	print("%2STATE_LABEL(p) = %Pstate(OP_LABEL(p),\n"
"%3STATE_LABEL(LEFT_CHILD(p)), 0);\n%2break;\n"
"%1case 2:\n%2%Plabel1(LEFT_CHILD(p));\n%2%Plabel1(RIGHT_CHILD(p));\n");
	if (Tflag)
		print("%2%Pnp = p;\n");
	print("%2STATE_LABEL(p) = %Pstate(OP_LABEL(p),\n"
"%3STATE_LABEL(LEFT_CHILD(p)),\n%3STATE_LABEL(RIGHT_CHILD(p)));\n%2break;\n"
"%1}\n}\n\n");
	print(
"int %Plabel(NODEPTR_TYPE p) {\n%1%Plabel1(p);\n"
"%1return ((struct %Pstate *)STATE_LABEL(p))->rule.%P%S ? STATE_LABEL(p) : 0;\n"
"}\n\n", start);
}

/* closure - fill in cost & rule with results of chain rules w/p as rhs */
static void closure(int cost[], Rule rule[], Nonterm p, int c) {
	Rule r;

	for (r = p->chain; r; r = r->chain)
		if (c + r->cost < cost[r->lhs->number]) {
			cost[r->lhs->number] = c + r->cost;
			rule[r->lhs->number] = r;
			closure(cost, rule, r->lhs, c + r->cost);
		}
}

/* emitleaf - emit state code for a leaf */
static void emitleaf(Term p, int ntnumber) {
	int i;
	Rule r;
	static int *cost;
	static Rule *rule;

	if (cost == NULL) {
		cost = alloc((ntnumber + 1)*sizeof *cost);
		rule = alloc((ntnumber + 1)*sizeof *rule);
	}
	for (i = 0; i <= ntnumber; i++) {
		cost[i] = 32767;
		rule[i] = NULL;
	}
	for (r = p->rules; r; r = r->next)
		if (r->pattern->left == NULL && r->pattern->right == NULL) {
			cost[r->lhs->number] = r->cost;
			rule[r->lhs->number] = r;
			closure(cost, rule, r->lhs, r->cost);
		}
	print("%2{\n%3static struct %Pstate z = { %d, 0, 0,\n%4{%10,\n", p->esn);
	for (i = 1; i <= ntnumber; i++)
		if (cost[i] < 32767)
			print("%5%d,%1/* %R */\n", cost[i], rule[i]);
		else
			print("%5%d,\n", cost[i]);
	print("%4},{\n");
	for (i = 1; i <= ntnumber; i++)
		if (rule[i])
			print("%5%d,%1/* %R */\n", rule[i]->packed, rule[i]);
		else
			print("%50,\n");
	print("%4}\n%3};\n%3return (int)&z;\n%2}\n");
}

/* computents - fill in bp with burm_nts vector for tree t */
static char *computents(Tree t, char *bp) {
	if (t) {
		Nonterm p = t->op;
		if (p->kind == NONTERM) {
			sprintf(bp, "%s_%s_NT, ", prefix, p->name);
			bp += strlen(bp);
		} else
			bp = computents(t->right, computents(t->left,  bp));
	}
	return bp;
}

/* emitnts - emit burm_nts ragged array */
static void emitnts(Rule rules, int nrules) {
	Rule r;
	int i, j, *nts = alloc(nrules*sizeof *nts);
	char **str = alloc(nrules*sizeof *str);

	for (i = 0, r = rules; r; r = r->link) {
		char buf[1024];
		*computents(r->pattern, buf) = 0;
		for (j = 0; str[j] && strcmp(str[j], buf); j++)
			;
		if (str[j] == NULL) {
			print("static short %Pnts_%d[] = { %s0 };\n", j, buf);
			str[j] = strcpy(alloc(strlen(buf) + 1), buf);
		}
		nts[i++] = j;
	}
	print("\nshort *%Pnts[] = {\n");
	for (i = j = 0, r = rules; r; r = r->link) {
		for ( ; j < r->ern; j++)
			print("%10,%1/* %d */\n", j);
		print("%1%Pnts_%d,%1/* %d */\n", nts[i++], j++);
	}
	print("};\n\n");
}

/* emitrecord - emit code that tests for a winning match of rule r */
static void emitrecord(char *pre, Rule r, int cost) {
	print("%sif (", pre);
	if (Tflag)
		print("%Ptrace(%Pnp, %d, c + %d, p->cost[%P%S_NT]), ",
			r->ern, cost, r->lhs);
	print("c + %d < p->cost[%P%S_NT]) {\n"
"%s%1p->cost[%P%S_NT] = c + %d;\n%s%1p->rule.%P%S = %d;\n",
		cost, r->lhs, pre, r->lhs, cost, pre, r->lhs,
		r->packed);
	if (r->lhs->chain)
		print("%s%1%Pclosure_%S(p, c + %d);\n", pre, r->lhs, cost);
	print("%s}\n", pre);
}

/* emitrule - emit decoding vectors and burm_rule */
static void emitrule(Nonterm nts) {
	Nonterm p;

	for (p = nts; p; p = p->link) {
		Rule r;
		print("static short %Pdecode_%S[] = {\n%10,\n", p);
		for (r = p->rules; r; r = r->decode)
			print("%1%d,\n", r->ern);
		print("};\n\n");
	}
	print("int %Prule(int state, int goalnt) {\n"
"%1%Passert(goalnt >= 1 && goalnt <= %d, PANIC(\"Bad goal nonterminal %%d in %Prule\\n\", goalnt));\n"
"%1if (!state)\n%2return 0;\n%1switch (goalnt) {\n", ntnumber);
	for (p = nts; p; p = p->link)
		print("%1case %P%S_NT:"
"%1return %Pdecode_%S[((struct %Pstate *)state)->rule.%P%S];\n", p, p, p);
	print("%1default:\n%2%Passert(0, PANIC(\"Bad goal nonterminal %%d in %Prule\\n\", goalnt));\n%1}\n%1return 0;\n}\n\n");
}

/* emitstate - emit state function */
static void emitstate(Term terms, Nonterm start, int ntnumber) {
	int i;
	Term p;

	print("int %Pstate(int op, int left, int right) {\n%1int c;\n"
"%1struct %Pstate *p, *l = (struct %Pstate *)left,\n"
"%2*r = (struct %Pstate *)right;\n\n%1assert(sizeof (int) >= sizeof (void *));\n%1");
	if (!Tflag)
		print("if (%Parity[op] > 0) ");
	print("{\n%2p = (void *)ALLOC(sizeof *p);\n"
"%2%Passert(p, PANIC(\"ALLOC returned NULL in %Pstate\\n\"));\n"
"%2p->op = op;\n%2p->left = l;\n%2p->right = r;\n%2p->rule.%P%S = 0;\n", start);
	for (i = 1; i <= ntnumber; i++)
		print("%2p->cost[%d] =\n", i);
	print("%332767;\n%1}\n%1switch (op) {\n");
	for (p = terms; p; p = p->link)
		emitcase(p, ntnumber);
	print("%1default:\n"
"%2%Passert(0, PANIC(\"Bad operator %%d in %Pstate\\n\", op));\n%1}\n"
"%1return (int)p;\n}\n\n");
}

/* emitstring - emit array of rules and costs */
static void emitstring(Rule rules) {
	Rule r;
	int k;

	print("short %Pcost[][4] = {\n");
	for (k = 0, r = rules; r; r = r->link) {
		for ( ; k < r->ern; k++)
			print("%1{ 0 },%1/* %d */\n", k);
		print("%1{ %d },%1/* %d = %R */\n", r->cost, k++, r);
	}
	print("};\n\nchar *%Pstring[] = {\n");
	for (k = 0, r = rules; r; r = r->link) {
		for ( ; k < r->ern; k++)
			print("%1/* %d */%10,\n", k);
		print("%1/* %d */%1\"%R\",\n", k++, r);
	}
	print("};\n\n");
}

/* emitstruct - emit the definition of the state structure */
static void emitstruct(Nonterm nts, int ntnumber) {
	print("struct %Pstate {\n%1int op;\n%1struct %Pstate *left, *right;\n"
"%1short cost[%d];\n%1struct {\n", ntnumber + 1);
	for ( ; nts; nts = nts->link) {
		int n = 1, m = nts->lhscount;
		while (m >>= 1)
			n++;		
		print("%2unsigned %P%S:%d;\n", nts, n);
	}
	print("%1} rule;\n};\n\n");
}

/* emitterms - emit terminal data structures */
static void emitterms(Term terms) {
	Term p;
	int k;

	print("char %Parity[] = {\n");
	for (k = 0, p = terms; p; p = p->link) {
		for ( ; k < p->esn; k++)
			print("%10,%1/* %d */\n", k);
		print("%1%d,%1/* %d=%S */\n", p->arity < 0 ? 0 : p->arity, k++, p);
	}
	print("};\n\n");
	if (Iflag) {
		print("char *%Popname[] = {\n");
		for (k = 0, p = terms; p; p = p->link) {
			for ( ; k < p->esn; k++)
				print("%1/* %d */%10,\n", k);
			print("%1/* %d */%1\"%S\",\n", k++, p);
		}
		print("};\n\n");
	}
}

/* emittest - emit clause for testing a match */
static void emittest(Tree t, char *v, char *suffix) {
	Term p = t->op;

	if (p->kind == TERM) {
		print("%3%s->op == %d%s/* %S */\n", v, p->esn,
			t->nterms > 1 ? " && " : suffix, p);
		if (t->left)
			emittest(t->left, stringf("%s->left",  v),
				t->right && t->right->nterms ? " && " : suffix);
		if (t->right)
			emittest(t->right, stringf("%s->right", v), suffix);
	}
}
