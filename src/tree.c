#include "c.h"

static int where = STMT;
static int nid = 1;		/* identifies trees & nodes in debugging output */
static struct nodeid {
	int printed;
	Tree node;
} ids[500];			/* if ids[i].node == p, then p's id is i */

static void printtree1 ARGS((Tree, int, int));

Tree tree(op, type, left, right)
int op; Type type; Tree left, right; {
	Tree p;

	NEW0(p, where);
	p->op = op;
	p->type = type;
	p->kids[0] = left;
	p->kids[1] = right;
	return p;
}

Tree texpr(f, tok, a) Tree (*f) ARGS((int)); int tok, a; {
	int save = where;
	Tree p;

	where = a;
	p = (*f)(tok);
	where = save;
	return p;
}
/* right - return (RIGHT, root(p), q) or just p/q if q/p==0;
   if ty==NULL, use q->type, or q->type/p->type */
Tree right(p, q) Tree p, q; {
	assert(p || q);
	if (p && q)
		return tree(RIGHT, q->type, root(p), q);
	else if (p)
		return p;
	else
		return q;
}

Tree root(p) Tree p; {
	if (p == NULL)
		return p;
	switch (generic(p->op)) {
	case COND: {
		Tree q = p->kids[1];
		assert(q && q->op == RIGHT);
		if (p->u.sym && q->kids[0] && generic(q->kids[0]->op) == ASGN)
			q->kids[0] = root(q->kids[0]->kids[1]);
		else
			q->kids[0] = root(q->kids[0]);
		if (p->u.sym && q->kids[1] && generic(q->kids[1]->op) == ASGN)
			q->kids[1] = root(q->kids[1]->kids[1]);
		else
			q->kids[1] = root(q->kids[1]);
		p->u.sym = 0;
		if (q->kids[0] == 0 && q->kids[1] == 0)
			p = root(p->kids[0]);
		}
		break;
	case AND: case OR:
		if ((p->kids[1] = root(p->kids[1])) == 0)
			p = root(p->kids[0]);
		break;
	case NOT:
		return root(p->kids[0]);
	case RIGHT:
		if (p->kids[1] == 0)
			return root(p->kids[0]);
		if (p->kids[0] && p->kids[0]->op == CALL+B
		&&  p->kids[1] && p->kids[1]->op == INDIR+B)
			/* avoid premature release of the CALL+B temporary */
			return p->kids[0];
		if (p->kids[0] && p->kids[0]->op == RIGHT
		&&  p->kids[1] == p->kids[0]->kids[0])
			/* de-construct e++ construction */
			return p->kids[0]->kids[1];
		/* fall thru */
	case EQ:  case NE:  case GT:   case GE:  case LE:  case LT: 
	case ADD: case SUB: case MUL:  case DIV: case MOD:
	case LSH: case RSH: case BAND: case BOR: case BXOR:
		p = tree(RIGHT, p->type, root(p->kids[0]), root(p->kids[1]));
		return p->kids[0] || p->kids[1] ? p : 0;
	case INDIR:
		if (p->type->size == 0 && unqual(p->type) != voidtype)
			warning("reference to `%t' elided\n", p->type);
		if (isptr(p->kids[0]->type) && isvolatile(p->kids[0]->type->type))
			warning("reference to `volatile %t' elided\n", p->type);
		/* fall thru */
	case CVI: case CVF:  case CVD:   case CVU: case CVC: case CVS: case CVP:
	case NEG: case BCOM: case FIELD:
		return root(p->kids[0]);
	case ADDRL: case ADDRG: case ADDRF: case CNST:
		if (needconst)
			return p;
		return NULL;
	case ARG: case ASGN: case CALL: case JUMP: case LABEL:
		break;
	default: assert(0);
	}
	return p;
}

char *opname(op) int op; {
	static char *opnames[] = {
	"",
	"CNST",
	"ARG",
	"ASGN",
	"INDIR",
	"CVC",
	"CVD",
	"CVF",
	"CVI",
	"CVP",
	"CVS",
	"CVU",
	"NEG",
	"CALL",
	"LOAD",
	"RET",
	"ADDRG",
	"ADDRF",
	"ADDRL",
	"ADD",
	"SUB",
	"LSH",
	"MOD",
	"RSH",
	"BAND",
	"BCOM",
	"BOR",
	"BXOR",
	"DIV",
	"MUL",
	"EQ",
	"GE",
	"GT",
	"LE",
	"LT",
	"NE",
	"JUMP",
	"LABEL",
	"AND",
	"NOT",
	"OR",
	"COND",
	"RIGHT",
	"FIELD"
	}, typenames[] = " FDCSIUPVB";
	char *name;

	if (opindex(op) > 0 && opindex(op) < NELEMS(opnames))
		name = opnames[opindex(op)];
	else
		name = stringd(opindex(op));
	if (op >= AND && op <= FIELD)
		return name;
	else if (optype(op) > 0 && optype(op) < sizeof (typenames) - 1)
		return stringf("%s%c", name, typenames[optype(op)]);
	else
		return stringf("%s+%d", name, optype(op));
}

int nodeid(p) Tree p; {
	int i = 1;

	ids[nid].node = p;
	while (ids[i].node != p)
		i++;
	if (i == nid)
		ids[nid++].printed = 0;
	return i;
}

/* printed - return pointer to ids[id].printed */
int *printed(id) int id; {
	if (id)
		return &ids[id].printed;
	nid = 1;
	return 0;
}

/* printtree - print tree p on fd */
void printtree(p, fd) Tree p; int fd; {
	(void)printed(0);
	printtree1(p, fd, 1);
}

/* printtree1 - recursively print tree p */
static void printtree1(p, fd, lev) Tree p; int fd, lev; {
	int i;
	static char blanks[] = "                                         ";

	if (p == 0 || *printed(i = nodeid(p)))
		return;
	fprint(fd, "#%d%s%s", i, &"   "[i < 10 ? 0 : i < 100 ? 1 : 2],
		 &blanks[sizeof blanks - lev]);
	fprint(fd, "%s %t", opname(p->op), p->type);
	*printed(i) = 1;
	for (i = 0; i < NELEMS(p->kids); i++)
		if (p->kids[i])
			fprint(fd, " #%d", nodeid(p->kids[i]));
	if (p->op == FIELD && p->u.field)
		fprint(fd, " %s %d..%d", p->u.field->name,
			fieldsize(p->u.field) + fieldright(p->u.field), fieldright(p->u.field));
	else if (generic(p->op) == CNST)
		fprint(fd, " %s", vtoa(p->type, p->u.v));
	else if (p->u.sym)
		fprint(fd, " %s", p->u.sym->name);
	if (p->node)
		fprint(fd, " node=0x%x", p->node);
	fprint(fd, "\n");
	for (i = 0; i < NELEMS(p->kids); i++)
		printtree1(p->kids[i], fd, lev + 1);
}
