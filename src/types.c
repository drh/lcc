#include "c.h"

static Field check ARGS((Type, Type, Field, int));
static Field isfield ARGS((char *, Field));
static Type type ARGS((int, Type, int, int, void *));

static struct entry {
	struct type type;
	struct entry *link;
} *typetable[128];
static int maxlevel;

static Symbol pointersym;

Type chartype;			/* char */
Type doubletype;		/* double */
Type floattype;			/* float */
Type inttype;			/* signed int */
Type longdouble;		/* long double */
Type longtype;			/* long */
Type shorttype;			/* signed short int */
Type signedchar;		/* signed char */
Type unsignedchar;		/* unsigned char */
Type unsignedlong;		/* unsigned long int */
Type unsignedshort;		/* unsigned short int */
Type unsignedtype;		/* unsigned int */
Type voidptype;			/* void* */
Type voidtype;			/* basic types: void */

static Type type(op, ty, size, align, sym)
	int op, size, align; Type ty; void *sym; {
	unsigned h = (op^((unsigned long)ty>>3))
&(NELEMS(typetable)-1);
	struct entry *tn;

	if (op != FUNCTION && (op != ARRAY || size > 0))
		for (tn = typetable[h]; tn; tn = tn->link)
			if (tn->type.op    == op   && tn->type.type  == ty
			&&  tn->type.size  == size && tn->type.align == align
			&&  tn->type.u.sym == sym)
				return &tn->type;
	NEW0(tn, PERM);
	tn->type.op = op;
	tn->type.type = ty;
	tn->type.size = size;
	tn->type.align = align;
	tn->type.u.sym = sym;
	tn->link = typetable[h];
	typetable[h] = tn;
	return &tn->type;
}
void typeInit() {
#define xx(v,name,op,metrics) { \
		Symbol p = install(string(name), &types, GLOBAL, PERM);\
		v = type(op, 0, IR->metrics.size, IR->metrics.align, p);\
		assert(v->align == 0 || v->size%v->align == 0); \
		p->type = v; p->addressed = IR->metrics.outofline; }
	xx(chartype,     "char",          CHAR,    charmetric);
	xx(doubletype,   "double",        DOUBLE,  doublemetric);
	xx(floattype,    "float",         FLOAT,   floatmetric);
	xx(inttype,      "int",           INT,     intmetric);
	xx(longdouble,   "long double",   DOUBLE,  doublemetric);
	xx(longtype,     "long int",      INT,     intmetric);
	xx(shorttype,    "short",         SHORT,   shortmetric);
	xx(signedchar,   "signed char",   CHAR,    charmetric);
	xx(unsignedchar, "unsigned char", CHAR,    charmetric);
	xx(unsignedlong, "unsigned long", UNSIGNED,intmetric);
	xx(unsignedshort,"unsigned short",SHORT,   shortmetric);
	xx(unsignedtype, "unsigned int",  UNSIGNED,intmetric);
#undef xx
	{
		Symbol p;
		p = install(string("void"), &types, GLOBAL, PERM);
		voidtype = type(VOID, NULL, 0, 0, p);
		p->type = voidtype;
	}
	pointersym = install(string("T*"), &types, GLOBAL, PERM);
	pointersym->addressed = IR->ptrmetric.outofline;
	voidptype = ptr(voidtype);
	assert(voidptype->align > 0 && voidptype->size%voidptype->align == 0);
	assert(unsignedtype->size >= voidptype->size);
	assert(inttype->size >= voidptype->size);
}
void rmtypes(lev) int lev; {
	if (maxlevel >= lev) {
		int i;
		maxlevel = 0;
		for (i = 0; i < NELEMS(typetable); i++) {
			struct entry *tn, **tq = &typetable[i];
			while ((tn = *tq) != NULL)
				if (tn->type.op == FUNCTION)
					tq = &tn->link;
				else if (tn->type.u.sym && tn->type.u.sym->scope >= lev)
					*tq = tn->link;
				else {
					if (tn->type.u.sym && tn->type.u.sym->scope > maxlevel)
						maxlevel = tn->type.u.sym->scope;
					tq = &tn->link;
				}

		}
	}
}
Type ptr(ty) Type ty; {
	return type(POINTER, ty, IR->ptrmetric.size,
		IR->ptrmetric.align, pointersym);
}
Type deref(ty) Type ty; {
	if (isptr(ty))
		ty = ty->type;
	else
		error("type error: %s\n", "pointer expected");
	return isenum(ty) ? unqual(ty)->type : ty;
}
Type array(ty, n, a) Type ty; int n, a; {
	assert(ty);
	if (isfunc(ty)) {
		error("illegal type `array of %t'\n", ty);
		return array(inttype, n, 0);
	}
	if (level > GLOBAL && isarray(ty) && ty->size == 0)
		error("missing array size\n");
	if (ty->size == 0) {
		if (unqual(ty) == voidtype)
			error("illegal type `array of %t'\n", ty);
		else if (Aflag >= 2)
			warning("declaring type array of %t' is undefined\n", ty);

	} else if (n > INT_MAX/ty->size) {
		error("size of `array of %t' exceeds %d bytes\n",
			ty, INT_MAX);
		n = 1;
	}
	return type(ARRAY, ty, n*ty->size,
		a ? a : ty->align, NULL);
}
Type atop(ty) Type ty; {
	if (isarray(ty))
		return ptr(ty->type);
	error("type error: %s\n", "array expected");
	return ptr(ty);
}
Type qual(op, ty) int op; Type ty; {
	if (isarray(ty))
		ty = type(ARRAY, qual(op, ty->type), ty->size,
			ty->align, NULL);
	else if (isfunc(ty))
		warning("qualified function type ignored\n");
	else if (isconst(ty)    && op == CONST
	||       isvolatile(ty) && op == VOLATILE)
		error("illegal type `%k %t'\n", op, ty);
	else {
		if (isqual(ty)) {
			op += ty->op;
			ty = ty->type;
		}
		ty = type(op, ty, ty->size, ty->align, NULL);
	}
	return ty;
}
Type func(ty, proto, style) Type ty, *proto; int style; {
	if (ty && (isarray(ty) || isfunc(ty)))
		error("illegal return type `%t'\n", ty);
	ty = type(FUNCTION, ty, 0, 0, NULL);
	ty->u.f.proto = proto;
	ty->u.f.oldstyle = style;
	return ty;
}
Type freturn(ty) Type ty; {
	if (isfunc(ty))
		return ty->type;
	error("type error: %s\n", "function expected");
	return inttype;
}
int variadic(ty) Type ty; {
	if (isfunc(ty) && ty->u.f.proto) {
		int i;
		for (i = 0; ty->u.f.proto[i]; i++)
			;
		return i > 1 && ty->u.f.proto[i-1] == voidtype;
	}
	return 0;
}
Type newstruct(op, tag) int op; char *tag; {
	Symbol p;

	assert(tag);
	if (*tag == 0)
		tag = stringd(genlabel(1));
	else
		if ((p = lookup(tag, types)) != NULL && (p->scope == level
		|| p->scope == PARAM && level == PARAM+1)) {
			if (p->type->op == op && !p->defined)
				return p->type;
			error("redefinition of `%s' previously defined at %w\n",
				p->name, &p->src);
		}
	p = install(tag, &types, level, PERM);
	p->type = type(op, NULL, 0, 0, p);
	if (p->scope > maxlevel)
		maxlevel = p->scope;
	p->src = src;
	return p->type;
}
Field newfield(name, ty, fty) char *name; Type ty, fty; {
	Field p, *q = &ty->u.sym->u.s.flist;

	if (name == NULL)
		name = stringd(genlabel(1));
	for (p = *q; p; q = &p->link, p = *q)
		if (p->name == name)
			error("duplicate field name `%s' in `%t'\n",
				name, ty);
	NEW0(p, PERM);
	*q = p;
	p->name = name;
	p->type = fty;
	if (xref) {							/* omit */
		if (ty->u.sym->u.s.ftab == NULL)			/* omit */
			ty->u.sym->u.s.ftab = table(NULL, level);	/* omit */
		install(name, &ty->u.sym->u.s.ftab, 0, PERM)->src = src;/* omit */
	}								/* omit */
	return p;
}
int eqtype(ty1, ty2, ret) Type ty1, ty2; int ret; {
	if (ty1 == ty2)
		return 1;
	if (ty1->op != ty2->op)
		return 0;
	switch (ty1->op) {
	case CHAR: case SHORT: case UNSIGNED: case INT:
	case ENUM: case UNION: case STRUCT:   case DOUBLE:
		return 0;
	case POINTER:  return eqtype(ty1->type, ty2->type, 1);
	case VOLATILE: case CONST+VOLATILE:
	case CONST:    return eqtype(ty1->type, ty2->type, 1);
	case ARRAY:    if (eqtype(ty1->type, ty2->type, 1)) {
		       	if (ty1->size == ty2->size)
		       		return 1;
		       	if (ty1->size == 0 || ty2->size == 0)
		       		return ret;
		       }
		       return 0;
	case FUNCTION: if (eqtype(ty1->type, ty2->type, 1)) {
		       	Type *p1 = ty1->u.f.proto, *p2 = ty2->u.f.proto;
		       	if (p1 == p2)
		       		return 1;
		       	if (p1 && p2) {
		       		for ( ; *p1 && *p2; p1++, p2++)
					if (eqtype(unqual(*p1), unqual(*p2), 1) == 0)
						return 0;
				if (*p1 == NULL && *p2 == NULL)
					return 1;
		       	} else {
		       		if (variadic(p1 ? ty1 : ty2))
					return 0;
				if (p1 == NULL)
					p1 = p2;
				for ( ; *p1; p1++) {
					Type ty = unqual(*p1);
					if (promote(ty) != (isenum(ty) ? ty->type : ty)
					|| ty == floattype)
						return 0;
				}
				return 1;
		       	}
		       }
		       return 0;
	}
	assert(0); return 0;
}
Type promote(ty) Type ty; {
	ty = unqual(ty);
	if (isunsigned(ty) || ty == longtype)
		return ty;
	else if (isint(ty) || isenum(ty))
		return inttype;
	return ty;
}
Type compose(ty1, ty2) Type ty1, ty2; {
	if (ty1 == ty2)
		return ty1;
	assert(ty1->op == ty2->op);
	switch (ty1->op) {
	case POINTER:
		return ptr(compose(ty1->type, ty2->type));
	case CONST+VOLATILE:
		return qual(CONST, qual(VOLATILE,
			compose(ty1->type, ty2->type)));
	case CONST: case VOLATILE:
		return qual(ty1->op, compose(ty1->type, ty2->type));
	case ARRAY:    { Type ty = compose(ty1->type, ty2->type);
			 if (ty1->size && ty1->type->size && ty2->size == 0)
			 	return array(ty, ty1->size/ty1->type->size, ty1->align);
			 if (ty2->size && ty2->type->size && ty1->size == 0)
			 	return array(ty, ty2->size/ty2->type->size, ty2->align);
			 return array(ty, 0, 0);    }
	case FUNCTION: { Type *p1  = ty1->u.f.proto, *p2 = ty2->u.f.proto;
			 Type ty   = compose(ty1->type, ty2->type);
			 List tlist = NULL;
			 if (p1 == NULL && p2 == NULL)
			 	return func(ty, NULL, 1);
			 if (p1 && p2 == NULL)
			 	return func(ty, p1, ty1->u.f.oldstyle);
			 if (p2 && p1 == NULL)
			 	return func(ty, p2, ty2->u.f.oldstyle);
			 for ( ; *p1 && *p2; p1++, p2++) {
			 	Type ty = compose(unqual(*p1), unqual(*p2));
			 	if (isconst(*p1)    || isconst(*p2))
			 		ty = qual(CONST, ty);
			 	if (isvolatile(*p1) || isvolatile(*p2))
			 		ty = qual(VOLATILE, ty);
			 	tlist = append(ty, tlist);
			 }
			 assert(*p1 == NULL && *p2 == NULL);
			 return func(ty, ltov(&tlist, PERM), 0); }
	}
	assert(0); return NULL;
}
int ttob(ty) Type ty; {
	switch (ty->op) {
	case CONST: case VOLATILE: case CONST+VOLATILE:
		return ttob(ty->type);
	case CHAR: case INT:   case SHORT: case UNSIGNED: 
	case VOID: case FLOAT: case DOUBLE:  return ty->op;
	case POINTER: case FUNCTION:         return POINTER;
	case ARRAY: case STRUCT: case UNION: return STRUCT;
	case ENUM:                           return INT;
	}
	assert(0); return INT;
}
Type btot(op) int op; {
	switch (optype(op)) {
	case F: return floattype;
	case D: return doubletype;
	case C: return chartype;
	case S: return shorttype;
	case I: return inttype;
	case U: return unsignedtype;
	case P: return voidptype;
	}
	assert(0); return 0;
}
int hasproto(ty) Type ty; {
	if (ty == 0)
		return 1;
	switch (ty->op) {
	case CONST: case VOLATILE: case CONST+VOLATILE: case POINTER:
	case ARRAY:
		return hasproto(ty->type);
	case FUNCTION:
		return hasproto(ty->type) && ty->u.f.proto;
	case STRUCT: case UNION:
	case CHAR:   case SHORT: case INT:  case DOUBLE:
	case VOID:   case FLOAT: case ENUM: case UNSIGNED:
		return 1;
	}
	assert(0); return 0;
}
/* check - check ty for ambiguous inherited fields, return augmented field set */
static Field check(ty, top, inherited, off)
Type ty, top; Field inherited; int off; {
	Field p;

	for (p = ty->u.sym->u.s.flist; p; p = p->link)
		if (p->name && isfield(p->name, inherited))
			error("ambiguous field `%s' of `%t' from `%t'\n", p->name, top, ty);
		else if (p->name && !isfield(p->name, top->u.sym->u.s.flist)) {
			Field new;
			NEW(new, FUNC);
			*new = *p;
			new->offset = off + p->offset;
			new->link = inherited;
			inherited = new;
		}
	for (p = ty->u.sym->u.s.flist; p; p = p->link)
		if (p->name == 0)
			inherited = check(p->type, top, inherited,
				off + p->offset);
	return inherited;
}

/* checkfields - check for ambiguous inherited fields in struct/union ty */
void checkfields(ty) Type ty; {
	Field p, inherited = 0;

	for (p = ty->u.sym->u.s.flist; p; p = p->link)
		if (p->name == 0)
			inherited = check(p->type, ty, inherited, p->offset);
}

/* extends - if ty extends fty, return a pointer to field structure */
Field extends(ty, fty) Type ty, fty; {
	Field p, q;

	for (p = unqual(ty)->u.sym->u.s.flist; p; p = p->link)
		if (p->name == 0 && unqual(p->type) == unqual(fty))
			return p;
		else if (p->name == 0 && (q = extends(p->type, fty)) != NULL) {
			static struct field f;
			f = *q;
			f.offset = p->offset + q->offset;
			return &f;
		}
	return 0;
}

/* fieldlist - construct a flat list of fields in type ty */
Field fieldlist(ty) Type ty; {
	Field p, q, t, inherited = 0, *r;

	ty = unqual(ty);
	for (p = ty->u.sym->u.s.flist; p; p = p->link)
		if (p->name == 0)
			inherited = check(p->type, ty, inherited, p->offset);
	if (inherited == 0)
		return ty->u.sym->u.s.flist;
	for (q = 0, p = inherited; p; q = p, p = t) {
		t = p->link;
		p->link = q;
	}
	for (r = &inherited, p = ty->u.sym->u.s.flist; p && q; )
		if (p->name == 0)
			p = p->link;
		else if (p->offset <= q->offset) {
			NEW(*r, FUNC);
			**r = *p;
			r = &(*r)->link;
			p = p->link;
		} else {
			*r = q;
			r = &q->link;
			q = q->link;
		}
	for ( ; p; p = p->link)
		if (p->name) {
			NEW(*r, FUNC);
			**r = *p;
			r = &(*r)->link;
		}
	*r = q;
	return inherited;
}

/* fieldref - find field name of type ty, return entry */
Field fieldref(name, ty) char *name; Type ty; {
	Field p;

	if ((p = isfield(name, unqual(ty)->u.sym->u.s.flist)) != NULL) {
		if (xref) {
			Symbol q;
			assert(unqual(ty)->u.sym->u.s.ftab);
			q = lookup(name, unqual(ty)->u.sym->u.s.ftab);
			assert(q);
			use(q, src);
		}
		return p;
	}
	return 0;
}

/* ftype - return a function type for rty function (ty,...)' */
Type ftype(rty, ty) Type rty, ty; {
	List list = append(ty, NULL);

	list = append(voidtype, list);
	return func(rty, ltov(&list, PERM), 0);
}

/* isfield - if name is a field in flist, return pointer to the field structure */
static Field isfield(name, flist) char *name; Field flist; {
	for ( ; flist; flist = flist->link)
		if (flist->name == name)
			break;
	return flist;
}

/* outtype - output type ty */
void outtype(ty) Type ty; {
	switch (ty->op) {
	case CONST+VOLATILE:
		print("%k %k %t", CONST, VOLATILE, ty->type);
		break;
	case CONST: case VOLATILE:
		print("%k %t", ty->op, ty->type);
		break;
	case STRUCT: case UNION: case ENUM:
		assert(ty->u.sym);
		if (ty->size == 0)
			print("incomplete ");
		assert(ty->u.sym->name);
		if (*ty->u.sym->name >= '1' && *ty->u.sym->name <= '9') {
			Symbol p = findtype(ty);
			if (p == 0)
				print("%k defined at %w", ty->op, &ty->u.sym->src);
			else
				print(p->name);
		} else {
			print("%k %s", ty->op, ty->u.sym->name);
			if (ty->size == 0)
				print(" defined at %w", &ty->u.sym->src);
		}
		break;
	case VOID: case FLOAT: case DOUBLE:
	case CHAR: case SHORT: case INT: case UNSIGNED:
		print(ty->u.sym->name);
		break;
	case POINTER:
		print("pointer to %t", ty->type);
		break;
	case FUNCTION:
		print("%t function", ty->type);
		if (ty->u.f.proto && ty->u.f.proto[0]) {
			int i;
			print("(%t", ty->u.f.proto[0]);
			for (i = 1; ty->u.f.proto[i]; i++)
				if (ty->u.f.proto[i] == voidtype)
					print(",...");
				else
					print(",%t", ty->u.f.proto[i]);
			print(")");
		} else if (ty->u.f.proto && ty->u.f.proto[0] == 0)
			print("(void)");

		break;
	case ARRAY:
		if (ty->size > 0 && ty->type && ty->type->size > 0) {
			print("array %d", ty->size/ty->type->size);
			while (ty->type && isarray(ty->type) && ty->type->type->size > 0) {
				ty = ty->type;
				print(",%d", ty->size/ty->type->size);
			}
		} else
			print("incomplete array");
		if (ty->type)
			print(" of %t", ty->type);
		break;
	default: assert(0);
	}
}

/* printdecl - output a C declaration for symbol p of type ty */
void printdecl(p, ty) Symbol p; Type ty; {
	switch (p->sclass) {
	case AUTO:
		fprint(2, "%s;\n", typestring(ty, p->name));
		break;
	case STATIC: case EXTERN:
		fprint(2, "%k %s;\n", p->sclass, typestring(ty, p->name));
		break;
	case TYPEDEF: case ENUM:
		break;
	default: assert(0);
	}
}

/* printproto - output a prototype declaration for function p */
void printproto(p, callee) Symbol p, callee[]; {
	if (p->type->u.f.proto)
		printdecl(p, p->type);
	else {
		int i;
		List list = 0;
		if (callee[0] == 0)
			list = append(voidtype, list);
		else
			for (i = 0; callee[i]; i++)
				list = append(callee[i]->type, list);
		printdecl(p, func(freturn(p->type), ltov(&list, PERM), 0));
	}
}

/* printtype - print details of type ty on fd */
void printtype(ty, fd) Type ty; int fd; {
	switch (ty->op) {
	case STRUCT: case UNION: {
		Field p;
		fprint(fd, "%k %s size=%d {\n", ty->op, ty->u.sym->name, ty->size);
		for (p = ty->u.sym->u.s.flist; p; p = p->link) {
			fprint(fd, "field %s: offset=%d", p->name, p->offset);
			if (p->lsb)
				fprint(fd, " bits=%d..%d",
					fieldsize(p) + fieldright(p), fieldright(p));
			fprint(fd, " type=%t", p->type);
		}
		fprint(fd, "}\n");
		break;
		}
	case ENUM: {
		int i;
		Symbol p;
		fprint(fd, "enum %s {", ty->u.sym->name);
		for (i = 0; (p = ty->u.sym->u.idlist[i]) != NULL; i++) {
			if (i > 0)
				fprint(fd, ",");
			fprint(fd, "%s=%d", p->name, p->u.value);
		}
		fprint(fd, "}\n");
		break;
		}
	default:
		fprint(fd, "%t\n", ty);
	}
}

/* typestring - return ty as C declaration for str, which may be "" */
char *typestring(ty, str) Type ty; char *str; {
	for ( ; ty; ty = ty->type) {
		Symbol p;
		switch (ty->op) {
		case CONST+VOLATILE:
			if (isptr(ty->type))
				str = stringf("%k %k %s", CONST, VOLATILE, str);
			else
				return stringf("%k %k %s", CONST, VOLATILE, typestring(ty->type, str));
			break;
		case CONST: case VOLATILE:
			if (isptr(ty->type))
				str = stringf("%k %s", ty->op, str);
			else
				return stringf("%k %s", ty->op, typestring(ty->type, str));
			break;
		case STRUCT: case UNION: case ENUM:
			assert(ty->u.sym);
			if ((p = findtype(ty)) != NULL)
				return *str ? stringf("%s %s", p->name, str) : p->name;
			if (*ty->u.sym->name >= '1' && *ty->u.sym->name <= '9')
				warning("unnamed %k in prototype\n", ty->op);
			if (*str)
				return stringf("%k %s %s", ty->op, ty->u.sym->name, str);
			else
				return stringf("%k %s", ty->op, ty->u.sym->name);
		case VOID: case FLOAT: case DOUBLE:
		case CHAR: case SHORT: case INT: case UNSIGNED:
			return *str ? stringf("%s %s", ty->u.sym->name, str) : ty->u.sym->name;
		case POINTER:
			if (unqual(ty->type)->op != CHAR && (p = findtype(ty)) != NULL)
				return *str ? stringf("%s %s", p->name, str) : p->name;
			str = stringf(isarray(ty->type) || isfunc(ty->type) ? "(*%s)" : "*%s", str);
			break;
		case FUNCTION:
			if ((p = findtype(ty)) != NULL)
				return *str ? stringf("%s %s", p->name, str) : p->name;
			if (ty->u.f.proto == 0)
				str = stringf("%s()", str);
			else if (ty->u.f.proto[0]) {
				int i;
				str = stringf("%s(%s", str, typestring(ty->u.f.proto[0], ""));
				for (i = 1; ty->u.f.proto[i]; i++)
					if (ty->u.f.proto[i] == voidtype)
						str = stringf("%s, ...", str);
					else
						str = stringf("%s, %s", str, typestring(ty->u.f.proto[i], ""));
				str = stringf("%s)", str);
			} else
				str = stringf("%s(void)", str);
			break;
		case ARRAY:
			if ((p = findtype(ty)) != NULL)
				return *str ? stringf("%s %s", p->name, str) : p->name;
			if (ty->type && ty->type->size > 0)
				str = stringf("%s[%d]", str, ty->size/ty->type->size);
			else
				str = stringf("%s[]", str);
			break;
		default: assert(0);
		}
	}
	assert(0); return 0;
}

