#include "c.h"

#define iscall(p) (generic((p)->op) == CALL \
	|| IR->mulops_calls \
	&& ((p)->op==DIV+I || (p)->op==MOD+I || (p)->op==MUL+I \
	||  (p)->op==DIV+U || (p)->op==MOD+U || (p)->op==MUL+U))
static Node forest;
static struct dag {
	struct node node;
	struct dag *hlink;
} *buckets[16];
int nodecount;
static Tree firstarg;
static Node *tail;

static int depth = 0;
static Node asgnnode ARGS((Symbol, Node));
static struct dag *dagnode ARGS((int, Node, Node, Symbol));
static Symbol equated ARGS((Symbol));
static void fixup ARGS((Node));
static void labelnode ARGS((int));
static void list ARGS((Node));
static void kill ARGS((Symbol));
static Node node ARGS((int, Node, Node, Symbol));
static void printdag1 ARGS((Node, int, int));
static void printnode ARGS((Node, int, int));
static void reset ARGS((void));
static Node tmpnode ARGS((Node));
static void typestab ARGS((Symbol, void *));
static Node undag ARGS((Node));
static Node visit ARGS((Node, int));
static void unlist ARGS((void));
void walk(tp, tlab, flab) Tree tp; int tlab, flab; {
	listnodes(tp, tlab, flab);
	if (forest) {
		code(Gen)->u.forest = forest->link;
		forest->link = NULL;
		forest = NULL;
	}
	reset();
	deallocate(STMT);
}

static Node node(op, l, r, sym)
int op; Node l, r; Symbol sym; {
	int i;
	struct dag *p;

	i = (opindex(op)^((unsigned long)sym>>2))&(NELEMS(buckets)-1);
	for (p = buckets[i]; p; p = p->hlink)
		if (p->node.op      == op && p->node.syms[0] == sym
		&&  p->node.kids[0] == l  && p->node.kids[1] == r)
			return &p->node;
	p = dagnode(op, l, r, sym);
	p->hlink = buckets[i];
	buckets[i] = p;
	++nodecount;
	return &p->node;
}
static struct dag *dagnode(op, l, r, sym)
int op; Node l, r; Symbol sym; {
	struct dag *p;

	NEW0(p, FUNC);
	p->node.op = op;
	if ((p->node.kids[0] = l) != NULL)
		++l->count;
	if ((p->node.kids[1] = r) != NULL)
		++r->count;
	p->node.syms[0] = sym;
	return p;
}
Node newnode(op, l, r, sym) int op; Node l, r; Symbol sym; {
	return &dagnode(op, l, r, sym)->node;
}
static void kill(p) Symbol p; {
	int i;
	struct dag **q;

	for (i = 0; i < NELEMS(buckets); i++)
		for (q = &buckets[i]; *q; )
			if (generic((*q)->node.op) == INDIR &&
			    (!isaddrop((*q)->node.kids[0]->op)
			     || (*q)->node.kids[0]->syms[0] == p)) {
				*q = (*q)->hlink;
				--nodecount;
			} else
				q = &(*q)->hlink;
}
static void reset() {
	if (nodecount > 0)
		memset(buckets, 0, sizeof buckets);
	nodecount = 0;
}
Node listnodes(tp, tlab, flab) Tree tp; int tlab, flab; {
	Node p = NULL, l, r;

	assert(tlab || flab || tlab == 0 && flab == 0);
	if (tp == NULL)
		return NULL;
	if (tp->node)
		return tp->node;
	switch (generic(tp->op)) {
	case AND:   { if (depth++ == 0) reset();
		      if (flab) {
		      	listnodes(tp->kids[0], 0, flab);
		      	listnodes(tp->kids[1], 0, flab);
		      } else {
		      	listnodes(tp->kids[0], 0, flab = genlabel(1));
		      	listnodes(tp->kids[1], tlab, 0);
		      	labelnode(flab);
		      }
		      depth--; } break;
	case OR:    { if (depth++ == 0)
		      	reset();
		      if (tlab) {
		      	listnodes(tp->kids[0], tlab, 0);
		      	listnodes(tp->kids[1], tlab, 0);
		      } else {
		      	tlab = genlabel(1);
		      	listnodes(tp->kids[0], tlab, 0);
		      	listnodes(tp->kids[1], 0, flab);
		      	labelnode(tlab);
		      }
		      depth--;
 } break;
	case NOT:   { return listnodes(tp->kids[0], flab, tlab); }
	case COND:  { Tree q = tp->kids[1];
		      assert(tlab == 0 && flab == 0);
		      if (tp->u.sym)
		      	addlocal(tp->u.sym);
		      flab = genlabel(2);
		      listnodes(tp->kids[0], 0, flab);
		      assert(q && q->op == RIGHT);
		      reset();
		      listnodes(q->kids[0], 0, 0);
if (forest->op == LABELV) {
	equatelab(forest->syms[0], findlabel(flab + 1));
	unlist();
}
		      list(jump(flab + 1));
		      labelnode(flab);
		      listnodes(q->kids[1], 0, 0);
if (forest->op == LABELV) {
	equatelab(forest->syms[0], findlabel(flab + 1));
	unlist();
}
		      labelnode(flab + 1);

		      if (tp->u.sym)
		      	p = listnodes(idtree(tp->u.sym), 0, 0); } break;
	case CNST:  { Type ty = unqual(tp->type);
		      assert(ty->u.sym);
		      if (tlab || flab) {
		      	assert(ty == inttype);
		      	if (tlab && tp->u.v.i != 0)
		      		list(jump(tlab));
		      	else if (flab && tp->u.v.i == 0)
		      		list(jump(flab));
		      }
		      else if (ty->u.sym->addressed)
		      	p = listnodes(cvtconst(tp), 0, 0);
		      else
		      	p = node(tp->op, NULL, NULL, constant(ty, tp->u.v)); } break;
	case RIGHT: { if (   tp->kids[0] && tp->kids[1]
			  &&  generic(tp->kids[1]->op) == ASGN
			  && (generic(tp->kids[0]->op) == INDIR
			  && tp->kids[0]->kids[0] == tp->kids[1]->kids[0]
			  || (tp->kids[0]->op == FIELD
			  &&  tp->kids[0] == tp->kids[1]->kids[0]))) {
		      	assert(tlab == 0 && flab == 0);
			if (generic(tp->kids[0]->op) == INDIR) {
				p = listnodes(tp->kids[0], 0, 0);
				list(p);
				listnodes(tp->kids[1], 0, 0);
			}
			else {
				assert(generic(tp->kids[0]->kids[0]->op) == INDIR);
				list(listnodes(tp->kids[0]->kids[0], 0, 0));
				p = listnodes(tp->kids[0], 0, 0);
				listnodes(tp->kids[1], 0, 0);
			}
		      } else if (tp->kids[1]) {
		      	listnodes(tp->kids[0], 0, 0);
		      	p = listnodes(tp->kids[1], tlab, flab);
		      } else
		      	p = listnodes(tp->kids[0], tlab, flab); } break;
	case JUMP:  { assert(tlab == 0 && flab == 0);
		      assert(tp->u.sym == 0);
		      assert(tp->kids[0]);
		      l = listnodes(tp->kids[0], 0, 0);
		      list(newnode(JUMPV, l, NULL, NULL));
		      reset(); } break;
	case CALL:  { Tree save = firstarg;
		      firstarg = NULL;
		      assert(tlab == 0 && flab == 0);
		      if (tp->op == CALL+B && !IR->wants_callb) {
		      	Tree arg0 = tree(ARG+P, tp->kids[1]->type,
				tp->kids[1], NULL);
			if (IR->left_to_right)
				firstarg = arg0;
			l = listnodes(tp->kids[0], 0, 0);
			if (!IR->left_to_right || firstarg) {
				firstarg = NULL;
				listnodes(arg0, 0, 0);
			}
		      	p = newnode(CALLV, l, NULL, NULL);
		      } else {
		      	l = listnodes(tp->kids[0], 0, 0);
		      	r = listnodes(tp->kids[1], 0, 0);
		      	p = newnode(tp->op, l, r, NULL);
		      }
		      NEW0(p->syms[0], FUNC);
		      assert(isptr(tp->kids[0]->type));
		      assert(isfunc(tp->kids[0]->type->type));
		      p->syms[0]->type = tp->kids[0]->type->type;
		      list(p);
		      reset();
		      cfunc->u.f.ncalls++;
		      firstarg = save;
 } break;
	case ARG:   { assert(tlab == 0 && flab == 0);
		      if (IR->left_to_right)
		      	listnodes(tp->kids[1], 0, 0);
		      if (firstarg) {
		      	Tree arg = firstarg;
		      	firstarg = NULL;
		      	listnodes(arg, 0, 0);
		      }
		      l = listnodes(tp->kids[0], 0, 0);
		      list(newnode(tp->op, l, NULL, NULL));
		      forest->syms[0] = intconst(tp->type->size);
		      forest->syms[1] = intconst(tp->type->align);
		      if (!IR->left_to_right)
		      	listnodes(tp->kids[1], 0, 0); } break;
	case EQ:  case NE: case GT: case GE: case LE:
	case LT:    { assert(tp->u.sym == 0);
		      assert(errcnt || tlab || flab);
		      l = listnodes(tp->kids[0], 0, 0);
		      r = listnodes(tp->kids[1], 0, 0);
		      if (tlab)
		      	assert(flab == 0),
		      	list(newnode(tp->op, l, r, findlabel(tlab)));
		      else if (flab) {
		      	int op = generic(tp->op);
		      	switch (generic(op)) {
		      	case EQ: op = NE + optype(tp->op); break;
		      	case NE: op = EQ + optype(tp->op); break;
		      	case GT: op = LE + optype(tp->op); break;
		      	case LT: op = GE + optype(tp->op); break;
		      	case GE: op = LT + optype(tp->op); break;
		      	case LE: op = GT + optype(tp->op); break;
		      	default: assert(0);
		      	}
		      	list(newnode(op, l, r, findlabel(flab)));
		      }
		      if (forest && forest->syms[0])
		      	forest->syms[0]->ref++; } break;
	case ASGN:  { assert(tlab == 0 && flab == 0);
		      if (tp->kids[0]->op == FIELD) {
		      	Tree  x = tp->kids[0]->kids[0];
			Field f = tp->kids[0]->u.field;
			assert(generic(x->op) == INDIR);
			reset();
			l = listnodes(lvalue(x), 0, 0);
			if (fieldsize(f) < 8*f->type->size) {
				unsigned int fmask = fieldmask(f);
				unsigned int  mask = fmask<<fieldright(f);
				Tree q = tp->kids[1];
				if (q->op == CNST+I && q->u.v.i == 0
				||  q->op == CNST+U && q->u.v.u == 0)
					q = bittree(BAND, x, consttree(~mask, unsignedtype));
				else if (q->op == CNST+I && (q->u.v.i&fmask) == fmask
				||       q->op == CNST+U && (q->u.v.u&fmask) == fmask)
					q = bittree(BOR, x, consttree(mask, unsignedtype));
				else {
					listnodes(q, 0, 0);
					q = bittree(BOR,
						bittree(BAND, rvalue(lvalue(x)),
							consttree(~mask, unsignedtype)),
						bittree(BAND, shtree(LSH, cast(q, unsignedtype),
							consttree(fieldright(f), inttype)),
							consttree(mask, unsignedtype)));
				}
				r = listnodes(q, 0, 0);
			} else
				r = listnodes(tp->kids[1], 0, 0);
		      } else {
		      	l = listnodes(tp->kids[0], 0, 0);
		      	r = listnodes(tp->kids[1], 0, 0);
		      }
		      list(newnode(tp->op, l, r, NULL));
		      forest->syms[0] = intconst(tp->kids[1]->type->size);
		      forest->syms[1] = intconst(tp->kids[1]->type->align);
		      if (isaddrop(tp->kids[0]->op)
		      && !tp->kids[0]->u.sym->computed)
		      	kill(tp->kids[0]->u.sym);
		      else
		      	reset();
		      p = listnodes(tp->kids[1], 0, 0); } break;
	case BOR: case BAND: case BXOR:
	case ADD: case SUB:  case RSH:
	case LSH:   { assert(tlab == 0 && flab == 0);
		      l = listnodes(tp->kids[0], 0, 0);
		      r = listnodes(tp->kids[1], 0, 0);
		      p = node(tp->op, l, r, NULL); } break;
	case DIV: case MUL:
	case MOD:   { assert(tlab == 0 && flab == 0);
		      l = listnodes(tp->kids[0], 0, 0);
		      r = listnodes(tp->kids[1], 0, 0);
		      p = node(tp->op, l, r, NULL);
		      if (IR->mulops_calls && isint(tp->type)) {
		      	list(p);
		      	cfunc->u.f.ncalls++;
		      } } break;
	case RET:   { assert(tlab == 0 && flab == 0);
		      l = listnodes(tp->kids[0], 0, 0);
		      list(newnode(tp->op, l, NULL, NULL)); } break;
	case CVC: case CVD: case CVF: case CVI:
	case CVP: case CVS: case CVU: case BCOM:
	case NEG:   { assert(tlab == 0 && flab == 0);
		      l = listnodes(tp->kids[0], 0, 0);
		      p = node(tp->op, l, NULL, NULL); } break;
	case INDIR: { Type ty = tp->kids[0]->type;
		      assert(tlab == 0 && flab == 0);
		      l = listnodes(tp->kids[0], 0, 0);
		      if (isptr(ty))
		      	ty = unqual(ty)->type;
		      if (isvolatile(ty)
		      || (isstruct(ty) && unqual(ty)->u.sym->u.s.vfields))
		      	p = newnode(tp->op, l, NULL, NULL);
		      else
		      	p = node(tp->op, l, NULL, NULL); } break;
	case FIELD: { Tree q = shtree(RSH,
		      	shtree(LSH, tp->kids[0],
		      		consttree(fieldleft(tp->u.field), inttype)),
		      	consttree(8*tp->type->size - fieldsize(tp->u.field),
		      		inttype));
		      assert(tlab == 0 && flab == 0);
		      p = listnodes(q, 0, 0); } break;
	case ADDRG:
	case ADDRF: { assert(tlab == 0 && flab == 0);
		      p = node(tp->op, NULL, NULL, tp->u.sym);
 } break;
	case ADDRL: { assert(tlab == 0 && flab == 0);
		      if (tp->u.sym->temporary)
		      	addlocal(tp->u.sym);
		      p = node(tp->op, NULL, NULL, tp->u.sym); } break;
	default:assert(0);
	}
	tp->node = p;
	return p;
}
static void list(p) Node p; {
	if (p && p->link == NULL) {
		if (forest) {
			p->link = forest->link;
			forest->link = p;
		} else
			p->link = p;
		forest = p;
	}
}
static void labelnode(lab) int lab; {
	assert(lab);
	if (forest && forest->op == LABELV)
		equatelab(findlabel(lab), forest->syms[0]);
	else
		list(newnode(LABELV, NULL, NULL, findlabel(lab)));
	reset();
}
static void unlist() {
	Node p;

	assert(forest);
	assert(forest != forest->link);
	p = forest->link;
	while (p->link != forest)
		p = p->link;
	p->link = forest->link;
	forest = p;
}
Tree cvtconst(p) Tree p; {
	Symbol q = constant(p->type, p->u.v);
	Tree e;

	if (q->u.c.loc == NULL)
		q->u.c.loc = genident(STATIC, p->type, GLOBAL);
	if (isarray(p->type)) {
		e = tree(ADDRG+P, atop(p->type), NULL, NULL);
		e->u.sym = q->u.c.loc;
	} else
		e = idtree(q->u.c.loc);
	return e;
}
void gencode(caller, callee) Symbol caller[], callee[]; {
	Code cp;
	Coordinate save;

	save = src;
	{
		int i;
		Symbol p, q;
		cp = codehead.next->next;
		codelist = codehead.next;
		for (i = 0; (p = callee[i]) != NULL
		         && (q = caller[i]) != NULL; i++)
			if (p->sclass != q->sclass || p->type != q->type)
				walk(asgn(p, idtree(q)), 0, 0);
		codelist->next = cp;
		cp->prev = codelist;
	}
	if (glevel && IR->stabsym) {
		int i;
		Symbol p, q;
		for (i = 0; (p = callee[i]) != NULL
		         && (q = caller[i]) != NULL; i++) {
			(*IR->stabsym)(p);
			if (p->sclass != q->sclass || p->type != q->type)
				(*IR->stabsym)(q);
		}
		swtoseg(CODE);
	}
	cp = codehead.next;
	for ( ; errcnt <= 0 && cp; cp = cp->next)
		switch (cp->kind) {
		case Address:  (*IR->address)(cp->u.addr.sym, cp->u.addr.base,
			       	cp->u.addr.offset); break;
		case Blockbeg: {
			       	Symbol *p = cp->u.block.locals;
			       	(*IR->blockbeg)(&cp->u.block.x);
			       	for ( ; *p; p++)
			       		if ((*p)->ref != 0.0)
			       			(*IR->local)(*p);
			       		else if (glevel) (*IR->local)(*p);
			       }
 break;
		case Blockend: (*IR->blockend)(&cp->u.begin->u.block.x); break;
		case Defpoint: src = cp->u.point.src; break;
		case Gen: case Jump:
		case Label:    if (!IR->wants_dag)
			       	cp->u.forest = undag(cp->u.forest);
			       fixup(cp->u.forest);
			       cp->u.forest = (*IR->gen)(cp->u.forest); break;
		case Local:    (*IR->local)(cp->u.var); break;
		case Switch:   break;
		default: assert(0);
		}
	src = save;
}
static void fixup(p) Node p; {
	for ( ; p; p = p->link)
		switch (generic(p->op)) {
		case JUMP:
			if (p->kids[0]->op == ADDRG+P)
				p->kids[0]->syms[0] =
					equated(p->kids[0]->syms[0]);
			break;
		case LABEL: assert(p->syms[0] == equated(p->syms[0])); break;
		case EQ: case GE: case GT: case LE: case LT: case NE:
			assert(p->syms[0]);
			p->syms[0] = equated(p->syms[0]);
		}
}
static Symbol equated(p) Symbol p; {
	{ Symbol q; for (q = p->u.l.equatedto; q; q = q->u.l.equatedto) assert(p != q); }
	while (p->u.l.equatedto)
		p = p->u.l.equatedto;
	return p;
}
void emitcode() {
	Code cp;
	Coordinate save;

	save = src;
	cp = codehead.next;
	for ( ; errcnt <= 0 && cp; cp = cp->next)
		switch (cp->kind) {
		case Address: break;
		case Blockbeg: if (glevel && IR->stabblock) {
			       	(*IR->stabblock)('{',  cp->u.block.level - LOCAL, cp->u.block.locals);
			       	swtoseg(CODE);
			       }
 break;
		case Blockend: if (glevel && IR->stabblock) {
			       	Code bp = cp->u.begin;
			       	foreach(bp->u.block.identifiers, bp->u.block.level, typestab, NULL);
			       	foreach(bp->u.block.types,       bp->u.block.level, typestab, NULL);
			       	(*IR->stabblock)('}', bp->u.block.level - LOCAL, bp->u.block.locals);
			       	swtoseg(CODE);
			       }
 break;
		case Defpoint: src = cp->u.point.src;
			       if (glevel > 0 && IR->stabline) {
			       	(*IR->stabline)(&cp->u.point.src); swtoseg(CODE); } break;
		case Gen: case Jump:
		case Label:    if (cp->u.forest)
			       	(*IR->emit)(cp->u.forest); break;
		case Local:    if (glevel && IR->stabsym) {
			       	(*IR->stabsym)(cp->u.var);
			       	swtoseg(CODE);
			       } break;
		case Switch:   {	int i;
			       	defglobal(cp->u.swtch.table, LIT);
			       	(*IR->defaddress)(equated(cp->u.swtch.labels[0]));
			       	for (i = 1; i < cp->u.swtch.size; i++) {
			       		long k = cp->u.swtch.values[i-1];
			       		while (++k < cp->u.swtch.values[i])
			       			assert(k < LONG_MAX),
			       			(*IR->defaddress)(equated(cp->u.swtch.deflab));
			       		(*IR->defaddress)(equated(cp->u.swtch.labels[i]));
			       	}
			       	swtoseg(CODE);
			       } break;
		default: assert(0);
		}
	src = save;
}

static Node undag(forest) Node forest; {
	Node p;

	tail = &forest;
	for (p = forest; p; p = p->link)
		if (generic(p->op) == INDIR
		|| iscall(p) && p->count >= 1)
			assert(p->count >= 1),
			visit(p, 1);
		else {
			assert(p->count == 0),
			visit(p, 1);
			*tail = p;
			tail = &p->link;
		}
	*tail = NULL;
	return forest;
}
static Node visit(p, listed) Node p; int listed; {
	if (p)
		if (p->syms[2])
			p = tmpnode(p);
		else if (p->count <= 1 && !iscall(p)
		||       p->count == 0 &&  iscall(p)) {
			p->kids[0] = visit(p->kids[0], 0);
			p->kids[1] = visit(p->kids[1], 0);
		}

		else if (p->op == ADDRLP || p->op == ADDRFP) {
			assert(!listed);
			p = newnode(p->op, NULL, NULL, p->syms[0]);
			p->count = 1;
		}
		else if (generic(p->op) == INDIR && !listed
		&& (p->kids[0]->op == ADDRLP || p->kids[0]->op == ADDRFP)
		&&  p->kids[0]->syms[0]->sclass == REGISTER) {
			p = newnode(p->op, newnode(p->kids[0]->op, NULL, NULL,
				p->kids[0]->syms[0]), NULL, NULL);
			p->count = 1;
		}
		else if (p->op == INDIRB) {
			--p->count;
			p = newnode(p->op, p->kids[0], NULL, NULL);
			p->count = 1;
			p->kids[0] = visit(p->kids[0], 0);
			p->kids[1] = visit(p->kids[1], 0);
		} else {
			p->kids[0] = visit(p->kids[0], 0);
			p->kids[1] = visit(p->kids[1], 0);
			p->syms[2] = temporary(REGISTER, btot(p->op), LOCAL);
			assert(!p->syms[2]->defined);
			p->syms[2]->ref = 1;
			p->syms[2]->u.t.cse = p;
			(*IR->local)(p->syms[2]);
			p->syms[2]->defined = 1;

			*tail = asgnnode(p->syms[2], p);
			tail = &(*tail)->link;
			if (!listed)
				p = tmpnode(p);
		};
	return p;
}
static Node tmpnode(p) Node p; {
	Symbol tmp = p->syms[2];

	assert(tmp);
	if (--p->count == 0)
		p->syms[2] = NULL;
	p = newnode(INDIR + (isunsigned(tmp->type) ? I : ttob(tmp->type)),
		newnode(ADDRLP, NULL, NULL, tmp), NULL, NULL);
	p->count = 1;
	return p;
}

static Node asgnnode(tmp, p) Symbol tmp; Node p; {
	p = newnode(ASGN + (isunsigned(tmp->type) ? I : ttob(tmp->type)),
		newnode(ADDRLP, NULL, NULL, tmp), p, NULL);
	p->syms[0] = intconst(tmp->type->size);
	p->syms[1] = intconst(tmp->type->align);
	return p;
}
/* printdag - print dag p on fd, or the node list if p == 0 */
void printdag(p, fd) Node p; int fd; {
	printed(0);
	if (p == 0) {
		if ((p = forest) != NULL)
			do {
				p = p->link;
				printdag1(p, fd, 0);
			} while (p != forest);
	} else if (*printed(nodeid((Tree)p)))
		fprint(fd, "node'%d printed above\n", nodeid((Tree)p));
	else
		printdag1(p, fd, 0);
}

/* printdag1 - recursively print dag p */
static void printdag1(p, fd, lev) Node p; int fd, lev; {
	int id, i;

	if (p == 0 || *printed(id = nodeid((Tree)p)))
		return;
	*printed(id) = 1;
	for (i = 0; i < NELEMS(p->kids); i++)
		printdag1(p->kids[i], fd, lev + 1);
	printnode(p, fd, lev);
}

/* printnode - print fields of dag p */
static void printnode(p, fd, lev) Node p; int fd, lev; {
	if (p) {
		int i, id = nodeid((Tree)p);
		fprint(fd, "%c%d%s", lev == 0 ? '\'' : '#', id,
			&"   "[id < 10 ? 0 : id < 100 ? 1 : 2]);
		fprint(fd, "%s count=%d", opname(p->op), p->count);
		for (i = 0; i < NELEMS(p->kids) && p->kids[i]; i++)
			fprint(fd, " #%d", nodeid((Tree)p->kids[i]));
		for (i = 0; i < NELEMS(p->syms) && p->syms[i]; i++)
			fprint(fd, " %s", p->syms[i]->name);
		fprint(fd, "\n");
	}
}

/* typestab - emit stab entries for p */
static void typestab(p, cl) Symbol p; void *cl; {
	if (!isfunc(p->type) && (p->sclass == EXTERN || p->sclass == STATIC) && IR->stabsym)
		(*IR->stabsym)(p);
	else if ((p->sclass == TYPEDEF || p->sclass == 0) && IR->stabtype)
		(*IR->stabtype)(p);
}

