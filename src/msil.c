/* Copyright Microsoft Corporation. All rights reserved. */

#include "c.h"
#include <time.h>

#define I(f) msil_##f
static char rcsid[] = "$Id: msil.c#42 2002/11/12 15:35:23 REDMOND\\drh $";

static const char *uname;	/* unique name prefix */
static int maxstack;		/* maximum amount of stack space used, in bytes */
static int maxlocals;		/* maximum amount of local space used, in bytes */
static Table ILtypes;		/* generated MSIL types */

/* emittype - emit .class declarations for type ty, tag id; return id */
static const char *emittype(const char *id, Type ty) {
	Symbol p = lookup(id, ILtypes);
	if (p == NULL) {
		p = install(id, &ILtypes, 0, PERM);
		p->type = ty;
		print(".class private value explicit ansi sealed '%s' { .pack 1 .size %d }\n",
		      id, ty->size);
	}
	return id;
}

static const char *gettypename(Type), *signature(Type, const char *);

/* getfieldtype - return "short" type name for use in struct/union type names */
static const char *getfieldtype(Type ty) {
	assert(ty);
	switch (ty->op) {
	case VOID:     return "v";
	case INT:      return stringf("i%d", ty->size);
	case UNSIGNED: return stringf("u%d", ty->size);
	case FLOAT:    return stringf("f%d", ty->size);
	case POINTER:  return stringf("p");
	case ARRAY:    return stringf("%s[]", getfieldtype(ty->type));
	default: return gettypename(ty);
	}
}

/* gettypename - return IL type name corresponding to ty */
static const char *gettypename(Type ty) {
	assert(ty);
	switch (ty->op) {
	case VOID:     return "void";
	case INT:      return stringf("int%d", 8*ty->size);
	case UNSIGNED: return stringf("int%d", 8*ty->size);
	case FLOAT:    return stringf("float%d", 8*ty->size);
	case POINTER:
		if (isfunc(ty->type))
			return stringf("method %s", signature(ty->type, "*"));
		else
			return stringf("void*");
	case ENUM: case CONST: case VOLATILE: case CONST+VOLATILE:
		return gettypename(ty->type);
	case ARRAY:
		return emittype(stringf("%s[]", gettypename(ty->type)), ty);
	case UNION: case STRUCT: {
		Field p = fieldlist(ty);
		const char *id = getfieldtype(p->type);
		while ((p = p->link) != NULL)
			id = stringf("%s%s", id, getfieldtype(p->type));
		id = stringf("%s%s{%s}", ty->op == STRUCT ? "s" : "u",
			ty->u.sym->name, id);
		return emittype(id, ty);
		}
	case FUNCTION: break;
	default: fprint(stderr, "bad type code %d\n", ty->op); assert(0);
	}
	return "";
}

static const char *va_list_type = "valuetype [mscorlib]System.ArgIterator";

/* isva_list - return 1 if IL name if ty is va_list; otherwise 0 */
static int isva_list(Type ty) {
	assert(ty);
	return isptr(ty) && isstruct(ty->type)
		&& strcmp(ty->type->u.sym->name, "__va_list") == 0;
}

/* gettype - return IL type corresponding to ty, defining it, if necessary */
static const char *gettype(Type ty) {
	assert(ty);
	switch (ty->op) {
	case POINTER:
		if (isva_list(ty))
			return va_list_type;
		/* else fall thru */
	case VOID: case INT: case UNSIGNED: case FLOAT: case ENUM:
		return gettypename(ty);
	case CONST: case VOLATILE: case CONST+VOLATILE:
		return gettype(ty->type);
	case ARRAY: case UNION: case STRUCT:
		return stringf("valuetype '%s'", gettypename(ty));
	case FUNCTION: break;
	default: assert(0);
	}
	return "";
}

/* widen - promote char and short to int, if necessary */
static Type widen(Type ty) {
	if (isint(ty) && ty->size < IR->intmetric.size)
		ty = inttype;
	return ty;
}

/* signature - return MSIL signature of type ty, include id if ty is a function */
static const char *signature(Type ty, const char *id) {
	assert(ty);
	if (isfunc(ty)) {
		char buf[1024], *s = buf;
		const char *sig;
		assert(id);
		if (ty->u.f.proto != NULL && ty->u.f.proto[0] != NULL) {
			int i;
			for (i = 0; ty->u.f.proto[i] != NULL; i++) {
				int len;
				if (i == 0 && isstruct(freturn(ty))) {
					sig = signature(freturn(ty), NULL);
					len = strlen(sig);
					assert(s + len + 2 < buf + sizeof buf);
					strcpy(s, sig);
					s[len] = ',';
					s += len + 1;
				}
				if (i > 0 && ty->u.f.proto[i] == voidtype) {
					/* sig = "..."; */
					break;
				} else
					sig = signature(widen(ty->u.f.proto[i]), NULL);
				len = strlen(sig);
				assert(s + len + 1 < buf + sizeof buf);
				if (i > 0)
					*s++ = ',';
				strcpy(s, sig);
				s += len;
			}
		}
		*s = 0;
		if (isstruct(freturn(ty)))
			sig = signature(voidtype, NULL);
		else
			sig = signature(widen(freturn(ty)), NULL);
		if (variadic(ty))
			return stringf("vararg %s %s(%s)", sig, id, buf);
		else
			return stringf("%s %s(%s)", sig, id, buf);
	} else if (isstruct(ty))
		return signature(ptr(ty), NULL);
	else
		return gettype(ty);
}

static void I(segment)(int n) {}

static void I(blockbeg)(Env *e) {}

static void I(blockend)(Env *e) {}

static int lc;		/* location counter for initialized data */
static Symbol current;	/* current global being initialized */
List initbuffer, databuffer;

/* appendstr - append formatted string to list, return list */
static List appendstr(List list, const char *fmt, ...) {
	char buf[1024];
	va_list ap;

	va_start(ap, fmt);
	vfprint(NULL, buf, fmt, ap);
	va_end(ap);
	return append(string(buf), list);
}

/* printstr - append formatted string to databuffer */
static void printstr(const char *fmt, ...) {
	char buf[1024];
	va_list ap;

	va_start(ap, fmt);
	vfprint(NULL, buf, fmt, ap);
	va_end(ap);
	databuffer = append(string(buf), databuffer);
}

/* printlist - print the strings in *list */
static void printlist(List *list) {
	char **strs = ltov(list, STMT);
	int i;

	for (i = 0; strs[i] != NULL; i++)
		print("%s", strs[i]);
}

/* datalabel - return p's data label, generating it if necessary */
static const char *datalabel(Symbol p) {
#define dlabel x.usecount
	switch (p->sclass) {
		case STATIC:
			if (p->dlabel == 0)
				p->dlabel = genlabel(1);
			return stringf("$%s%d", uname, p->dlabel);
		case AUTO: case EXTERN:
			return stringf("$%s", p->name);
		default: assert(0);
	}
	return NULL;
#undef dlabel
}

/* closedata - close current .data initialization, if necessary */
static void closedata(void) {
	if (lc > 0) {
		assert(current);
		print(".data %s = {\n", datalabel(current));
		printlist(&databuffer);
		print("\n}\n");
	}
	lc = 0;
	current = NULL;
}

/* address - initialize q for addressing expression p+n */
static void I(address)(Symbol q, Symbol p, long n) {
	q->name = p->name;
	q->x.name = p->x.name;
	q->x.offset = n;
	q->type = p->type;
}

#define print printstr
static void I(defaddress)(Symbol p) {
	if (lc > 0)
		print(",\n");
	if (!isfunc(p->type) && (!p->computed || p->x.offset == 0))
		print("&(%s)", datalabel(p));
	else {
		initbuffer = appendstr(initbuffer, "ldsflda %s %s\n",
			gettype(current->type), current->x.name);
		if (lc > 0)
			initbuffer = appendstr(initbuffer,
				"ldc.i4 %d\n"
				"add\n", lc);
		if (isfunc(p->type)) {
			initbuffer = appendstr(initbuffer, "ldftn %s\n" , signature(p->type, p->x.name));
			if (p->sclass != STATIC) {
				int lab = genlabel(1);
				initbuffer = appendstr(initbuffer,
					"ldsfld int8 __is_unmanaged_%s\n"
					"brfalse $L%d\n"
					"call void* __getMUThunk(void*)\n"
					"$L%d:\n", p->name, lab, lab);
			}
		} else {
			initbuffer = appendstr(initbuffer, "ldsflda %s %s\n",
				gettype(p->type), p->x.name);
			if (p->x.offset != 0)
				initbuffer = appendstr(initbuffer,
					"ldc.i4 %d\n"
					"add\n", p->x.offset);
		}
		initbuffer = appendstr(initbuffer, "stind.i4\n");
		print("int%d (0)", 8*voidptype->size);
	}
	lc += voidptype->size;
}

static char *tostring(long double d) {
	static char buf[128];

	if (d == 0.0) {
		float f = d;
		int *p = (int *)&f;
		if (*p < 0)
			sprintf(buf, "-0.0");
		else
			sprintf(buf, "0.0");
	} else
		sprintf(buf, "%e", d);
	return buf;
}

static void I(defconst)(int suffix, int size, Value v) {
	if (lc > 0)
		print(",\n");
	switch (suffix) {
	case I: print("int%d (%d)", 8*size, v.i); break;
	case U: print("int%d (%u)", 8*size, (long)v.u); break;
	case P: print("int%d (%p)", 8*size, v.p); break;
	case F: print("float%d (%s)", 8*size, tostring(v.d)); break;
	default: assert(0);
	}
	lc += size;
}

static void I(defstring)(int len, char *str) {
	int i;

	if (lc > 0)
		print(",\n");
	print("bytearray (");
	for (i = 0; i < len; i++) {
		if (i > 0 && i%30 == 0)
			print("\n");
		print(" %x", (unsigned char)str[i]);
	}
	print(" )");
	lc += len;
}
#undef print

static void I(space)(int n) {
	static Value v = { 0 };

	for ( ; n >= 4; n -= 4)
		(*IR->defconst)(I, 4, v);
	for ( ; n >= 2; n -= 2)
		(*IR->defconst)(I, 2, v);
	for ( ; n >  0; n -= 1)
		(*IR->defconst)(I, 1, v);
}

static void I(defsymbol)(Symbol p) {
	if (p->sclass == STATIC && p->generated)
		p->x.name = stringf("$%s_%s", uname, p->name);
	else if (p->sclass == STATIC)
		p->x.name = stringf("$%s%d_%s", uname, genlabel(1), p->name);
	else if (p->scope == LABELS)
		p->x.name = stringf("$L%s", p->name);
	else if (p->generated)
		p->x.name = stringf("$%s", p->name);
	else
		p->x.name = stringf("'%s'", p->name);
}

static void I(global)(Symbol p) {
	closedata();
	gettypename(p->type);
	print(".field public static %s %s at %s\n", gettype(p->type), p->x.name, datalabel(p));
	current = p;
}

static char suffix(int op, const char *map) {
	if (!map)
		map = "iupr";
	switch (optype(op)) {
	case I: return map[0];
	case U: return map[1];
	case P: return map[2];
	case F: return map[3];
	default: assert(0);
	}
	return 0;
}

/* builtin - is p a built-in function? */
static int builtin(Symbol p) {
	return strcmp(p->name, "__va_start") == 0 ||strcmp(p->name, "__va_arg") == 0;
}

/* emitbuiltin - emit code for builtin operator, which is identified by a call */
static void emitbuiltin(Symbol p, Node call) {
	if (strcmp(p->name, "__va_start") == 0) {
		print("arglist\n");
		print("call instance void [mscorlib]System.ArgIterator::.ctor(valuetype [mscorlib]System.RuntimeArgumentHandle)\n");
	} else if (strcmp(p->name, "__va_arg") == 0) {
		print("call instance typedref [mscorlib]System.ArgIterator::GetNextArg()\n");
		assert(call->syms[1]);
		switch (call->syms[1]->u.c.v.i) {
		case FLOAT: print("refanyval float64\n"); break;
		case INT: case UNSIGNED: print("refanyval int32\n"); break;
		case POINTER: print("refanyval void*\n"); break;
		case STRUCT: case UNION: print("refanyval void*\nldind.i4\n"); break;
		default:error("invalid use of va_arg\n");
		}
	} else
		assert(0);
}

/* emitcallsig - emit call signature sig for function f or type ty */
static void emitcallsig(Node a, List sig, Symbol f, Type ty) {
	int i, n;
	Type *argv = ltov(&sig, FUNC);

	if (variadic(ty))
		print("vararg ");
	if (specific(a->op) == CALL+V)
		print("void");
	else
		print("%s", signature(btot(a->op, opsize(a->op)), NULL));
	if (f != NULL)
		print(" %s", f->x.name);
	print("(");
	for (n = 0; ty->u.f.proto && ty->u.f.proto[n] != NULL; n++)
		;
	for (i = 0; argv[i] != NULL; i++) {
		if (i > 0)
			print(",");		
		if (variadic(ty) && i > 0 && i < n && ty->u.f.proto[i] == voidtype)
			print("...,");
		if (isptr(argv[i]) && i < n && isva_list(ty->u.f.proto[i]))
			print("%s", va_list_type);
		else if (isptr(argv[i]) && i < n && isptr(ty->u.f.proto[i]))
			print("%s", signature(ty->u.f.proto[i], NULL));
		else
			print("%s", signature(argv[i], NULL));
	}
	print(")\n");
}

/* checkconvert - emit a conv.r? the value on the tos is "internal F type" */
static void checkconvert(Node p) {
	int op = specific(p->op);
	if (op == ADD+F || op == SUB+F || op == MUL+F || op == DIV+F || op == NEG+F)
		print("conv.r%d\n", opsize(p->op));
}

/* getcallee - get the name of the next function in a CALL node, or NULL */
static Symbol getcallee(Node p) {
	Node a;

	while (p && generic(p->op) != CALL)
		p = p->x.next;
	a = p->kids[0];
	if (generic(a->op) == ADDRG)
		return a->syms[0];
	return NULL;
}

/* argtype - get the type of the nth argument in function type ty, or NULL */
static Type getargtype(int n, Type ty) {
	int i;

	assert(isfunc(ty));
	for (i = 0; ty->u.f.proto && ty->u.f.proto[i] != NULL; i++)
		if (i == n)
			return ty->u.f.proto[i];
	return NULL;
}

static List callsig = NULL;

static void emitnode(Node p) {
	int size;

	if (!p)
		return;
	size = roundup(opsize(p->op), 4);
	switch (specific(p->op)) {
	case CNST+I: print("ldc.i%d %D\n", size, p->syms[0]->u.c.v.i); return;
	case CNST+U: print("ldc.i%d %U\n", size, p->syms[0]->u.c.v.u); return;
	case CNST+P: print("ldc.i%d %p\n", size, p->syms[0]->u.c.v.p); return;
	case CNST+F: print("ldc.r%d %s\n", size, tostring(p->syms[0]->u.c.v.d)); return;
	case CVF+F:  print("conv.r%d\n", opsize(p->op)); return;
	case CVF+I:  print("conv.i%d\n", opsize(p->op)); return;
	case CVI+F:  print("conv.r%d\n", opsize(p->op)); return;
	case CVI+I:  print("conv.i%d\n", opsize(p->op)); return;
	case CVU+P:
	case CVP+U:
	case CVU+U:
	case CVI+U:  print("conv.u%d\n", opsize(p->op)); return;
	case CVU+I:  print("conv.i%d\n", opsize(p->op)); return;
	case ARG+F:
	case ARG+I:	
	case ARG+U:
		checkconvert(p->x.prev);
		assert(opsize(p->op) >= IR->intmetric.size);
		callsig = append(btot(p->op, opsize(p->op)), callsig);
		return;
	case ARG+P: {
		Symbol f = getcallee(p->x.next);
		Type aty;
		checkconvert(p->x.prev);
		assert(opsize(p->op) >= IR->intmetric.size);
		callsig = append(btot(p->op, opsize(p->op)), callsig);
		if (f != NULL && !builtin(f) && (aty = getargtype(p->x.argno, f->type))
		&& isptr(aty) && isfunc(aty->type)) {
			Symbol lab = findlabel(genlabel(1));
			print("ldsfld int8 __is_unmanaged_%s\n", f->name);
			print("brfalse %s\n", lab->x.name);
			print("call void* __getUMThunk(void*)\n");
			print("%s:\n", lab->x.name);
		}
		return;
		}
	case INDIR+B: return;
	case INDIR+I:
	case INDIR+U:
	case INDIR+P:
	case INDIR+F: {
		Node a = p->kids[0];
		if (isaddrop(a->op) && isscalar(a->syms[0]->type)) {
			if (generic(a->op) == ADDRL)
				print("ldloc %d\n", a->syms[0]->x.offset);
			else if (generic(a->op) == ADDRF)
				print("ldarg %d\n", a->syms[0]->x.offset);
			else /* generic(a->op) == ADDRG) */
				print("ldsfld %s %s\n", gettype(a->syms[0]->type), a->syms[0]->x.name);
		} else
			print("ldind.%c%d\n", suffix(p->op, "iuur"), opsize(p->op));
		return;
		}
	case ASGN+I:
	case ASGN+U:
	case ASGN+P:
	case ASGN+F: {
		Node a = p->kids[0];
		if (isaddrop(a->op) && isscalar(a->syms[0]->type)) {
			if (generic(a->op) == ADDRL) {
				checkconvert(p->x.prev);
				print("stloc %d\n", a->syms[0]->x.offset);
			} else if (generic(a->op) == ADDRF) {
				checkconvert(p->x.prev);
				print("starg %d\n", a->syms[0]->x.offset);
			} else /* generic(a->op) == ADDRG) */
				print("stsfld %s %s\n", gettype(a->syms[0]->type), a->syms[0]->x.name);
		} else
			print("stind.%c%d\n", suffix(p->op, "iiir"), opsize(p->op));
		return;
		}
	case CALL+I:
	case CALL+U:
	case CALL+F:
	case CALL+P:
	case CALL+V: {
		Symbol f = getcallee(p);
		if (f != NULL) {
			if (builtin(f))
				emitbuiltin(f, p);
			else {
				Type ty = freturn(f->type);
				print("call ");
				emitcallsig(p, callsig, f, f->type);
				if (isptr(ty) && isfunc(ty->type)) {
					Symbol lab = findlabel(genlabel(1));
					print("ldsfld int8 __is_unmanaged_%s\n", f->name);
					print("brfalse %s\n", lab->x.name);
					print("call void* __getMUThunk(void*)\n");
					print("%s:\n", lab->x.name);
				}
			}
			callsig = NULL;
			return;
		}
		break;
		}
	case JUMP+V:
		assert(generic(p->kids[0]->op) == ADDRG);
		print("br %s\n", p->kids[0]->syms[0]->x.name);
		return;
	case DIV+U: print("div.un\n"); return;
	case MOD+U: print("rem.un\n"); return;
	case RSH+U: print("shr.un\n"); return;
	case GE+U:  print("bge.un %s\n", p->syms[0]->x.name); return;
	case GT+U:  print("bgt.un %s\n", p->syms[0]->x.name); return;
	case LE+U:  print("ble.un %s\n", p->syms[0]->x.name); return;
	case LT+U:  print("blt.un %s\n", p->syms[0]->x.name); return;
	}
	switch (generic(p->op)) {
	case ADDRG:
		if (p->syms[0]->type && isfunc(p->syms[0]->type))
			print("ldftn %s\n", signature(p->syms[0]->type, p->syms[0]->x.name));
		else if (p->syms[0]->scope != LABELS)
			print("ldsflda %s %s\n", gettype(p->syms[0]->type), p->syms[0]->x.name);
		break;
	case ADDRF: print("ldarga %d\n", p->syms[0]->x.offset); break;
	case ADDRL: print("ldloca %d\n", p->syms[0]->x.offset); break;
	case NEG:   print("neg\n"); break;
	case BCOM:  print("not\n"); break;
	case CALL:
		print("calli ");
		emitcallsig(p, callsig, NULL, p->syms[0]->type);
		callsig = NULL;
		break;
	case ASGN:
		assert(specific(p->op) == ASGN+B);
		print("ldc.i4 %d\ncpblk\n", p->syms[0]->u.c.v.i);
		break;
	case RET:   checkconvert(p->x.prev); break;
	case SUB:   print("sub\n"); break;
	case ADD:   print("add\n"); break;
	case LSH:   print("shl\n"); break;
	case MOD:   print("rem\n"); break;
	case RSH:   print("shr\n"); break;
	case BAND:  print("and\n"); break;
	case BOR:   print("or\n");  break;
	case BXOR:  print("xor\n"); break;
	case DIV:   print("div\n"); break;
	case MUL:   print("mul\n"); break;
	case EQ:    print("beq %s\n", p->syms[0]->x.name); break;
	case GE:    print("bge %s\n", p->syms[0]->x.name); break;
	case GT:    print("bgt %s\n", p->syms[0]->x.name); break;
	case LE:    print("ble %s\n", p->syms[0]->x.name); break;
	case LT:    print("blt %s\n", p->syms[0]->x.name); break;
	case NE:    print("bne.un %s\n", p->syms[0]->x.name); break;
	case LABEL: print("%s:\n", p->syms[0]->x.name); break;
	default: assert(0);
	}
}

static void I(emit)(Node p) {
	callsig = NULL;
	for ( ; p != NULL; p = p->x.next) {
		emitnode(p);
		if (generic(p->op) == CALL && p->op != CALL+V && p->x.listed)
			print("pop\n");
	}
}

static List locals = NULL;

static void I(local)(Symbol p) {
	p->x.offset = maxlocals/4;
	maxlocals = roundup(maxlocals + p->type->align, 4);
	locals = append(p, locals);
	if (isarray(p->type) && p->type->type->size > 0)
		emittype(stringf("%s[%d]", gettypename(p->type->type),
			p->type->size/p->type->type->size), p->type);
	else
		gettypename(p->type);
}

static void I(export)(Symbol p) {
}

static int reg, argcount, argno;

static void I(function)(Symbol f, Symbol caller[], Symbol callee[], int ncalls) {
	int i;

	closedata();
	maxstack = maxlocals = 0;
	for (i = 0; caller[i] && callee[i]; i++) {
		caller[i]->x.offset = i;
		callee[i]->sclass = caller[i]->sclass;
		callee[i]->x.offset = i;
	}
	reg = 0;
	gencode(caller, callee);
	print(".method public hidebysig static %s cil managed {\n", signature(f->type, f->x.name));
	{
		Symbol *argv = ltov(&locals, FUNC);
		for (i = 0; argv[i] != NULL; i++) {
			print(".locals ([%d] ", argv[i]->x.offset);
			if (isarray(argv[i]->type) && argv[i]->type->type->size > 0)
				print("valuetype '%s[%d]'", gettypename(argv[i]->type->type),
				      argv[i]->type->size/argv[i]->type->type->size);
			else
				print("%s", gettype(argv[i]->type));
			print(" '%s')\n", argv[i]->name);
		}
	}
	assert(maxstack%4 == 0);
	maxstack = roundup(maxstack, voidptype->size);
	print(".maxstack %d\n", maxstack/voidptype->size);
	emitcode();
	print("ret\n}\n");
}

static void insert(Node p, Node left, Node right) {
	p->x.prev = left;
	p->x.next = right;
	left->x.next = p;
	right->x.prev = p;
}

static struct node head;

/* max - return maximum of a and b */
#undef max
static int max(int a, int b) {
	return a > b ? a : b;
}

static int visit(Node p) {
	int size, left, right;

	assert(p);
	size = roundup(opsize(p->op), 4);
	switch (generic(p->op)) {
	case ADDRG:
	case ADDRF:
	case ADDRL:
	case CNST:
		reg += size;
		break;
	case JUMP: {
		Type rty = freturn(cfunc->type);
		assert(generic(p->kids[0]->op) == ADDRG);
		if (p->kids[0]->syms[0]->u.l.label == cfunc->u.f.label
		&& rty != voidtype && !isstruct(rty))
			reg -= widen(rty)->size;
		}
		break;
	case INDIR:
		if (specific(p->op) == INDIR+B)
			size = visit(p->kids[0]);
		else {
			Node a = p->kids[0];
			if (isaddrop(a->op) && isscalar(a->syms[0]->type))
				reg += size;
			else {
				reg -= visit(p->kids[0]);
				reg += size;
			}
		}
		break;
	case NEG:
	case BCOM:
	case CVF:
		reg -= visit(p->kids[0]);
		reg += size;
		break;
	case CVI:
	case CVP:
	case CVU:
		left = visit(p->kids[0]);
		if (specific(p->op) != CVI+I && p->syms[0]->u.c.v.i != size) {
			int n = size >= p->syms[0]->u.c.v.i ? size : p->syms[0]->u.c.v.i;
			reg += n;
			maxstack = max(maxstack, reg);
			reg -= n + left;
		} else
			reg -= left;
		reg += size;
		break;
	case ASGN:
		if (specific(p->op) == ASGN+B) {
			left = visit(p->kids[0]); right = visit(p->kids[1]);
			reg += 4;
			maxstack = max(maxstack, reg);
			reg -= left + right + 4;
		} else {
			Node a = p->kids[0];
			if (isaddrop(a->op) && isscalar(a->syms[0]->type))
				reg -= visit(p->kids[1]);
			else {
				left = visit(p->kids[0]); right = visit(p->kids[1]);
				reg -= left + right;
			}
		}
		size = 0;
		break;
	case ARG:
		if (argcount == 0)
			argno = 0;
		p->x.argno = argno++;
		argcount += visit(p->kids[0]);
		if (specific(p->op) == ARG+P)
			maxstack = max(maxstack, reg + inttype->size);
		break;
	case CALL:
		if (generic(p->kids[0]->op) != ADDRG) {
			left = visit(p->kids[0]);
			reg -= argcount + left;
		} else {
			Symbol f = p->kids[0]->syms[0];
			Type ty = freturn(f->type);
			if (strcmp(f->name, "__va_arg") == 0) {
				Node e;
				assert((specific(head.x.prev->op) == ARG+I
				    && specific(head.x.prev->x.prev->op) == CNST+I));
				p->syms[1] = head.x.prev->x.prev->syms[0];
				e = head.x.prev->x.prev->x.prev;
				insert(e, e->x.prev, &head);
			}
			reg -= argcount;
			if (isptr(ty) && isfunc(ty->type))
				maxstack = max(maxstack, reg + inttype->size);			
		}
		if (p->op != CALL+V)
			reg += size;
		argcount = 0;
		break;
	case RET:
		if (p->op != RET+V)
			visit(p->kids[0]);
		break;
	case ADD:
	case SUB:
	case LSH:
	case MOD:
	case RSH:
	case BAND:
	case BOR:
	case BXOR:
	case DIV:
	case MUL:
		left = visit(p->kids[0]); right = visit(p->kids[1]);
		reg -= left + right;
		reg += size;
		break;
	case EQ:
	case GE:
	case GT:
	case LE:
	case LT:
	case NE:
		left = visit(p->kids[0]); right = visit(p->kids[1]);
		reg -= left + right;
		size = 0;
		break;
	case LABEL:
		break;
	default: assert(0);
	}
	assert(reg >= 0);
	if (reg > maxstack)
		maxstack = reg;
	insert(p, head.x.prev, &head);
	return size;
}

static Node I(gen)(Node p) {
	head.x.next = head.x.prev = &head;
	argcount = 0;
	for ( ; p != NULL; p = p->link) {
		visit(p);
		if (generic(p->op) == CALL && p->op != CALL+V) {
			p->x.listed = 1;
			reg -= opsize(p->op);
		}
	}
	assert(argcount == 0);
	head.x.prev->x.next = NULL;
	for (p = head.x.next; p != NULL; p = p->x.next)
		switch (generic(p->op)) {
		case ADDRG: gettype(p->syms[0]->type); break;
		case INDIR: case ASGN:
			if (generic(p->kids[0]->op) == ADDRG)
				gettype(p->kids[0]->syms[0]->type);
			break;
		}
	return head.x.next;
}

static void I(import)(Symbol p) {
	closedata();
}

static void I(progbeg)(int argc, char *argv[]){
	int i;

	density = 1.1;
	ILtypes = newtable(PERM);
	{
		extern int getpid(void);
		time_t t, prev = time(NULL);
		while ((t = time(NULL)) == prev)
			;
		uname = stringf("%x_%x_", t, getpid());
	}
	for (i = 0; i < argc; i++)
		if (strncmp(argv[i], "-uname=", 7) == 0)
			uname = argv[i] + 7;
	print("//");
	if (firstfile != NULL)
		print(" file=%s", firstfile);
	print(" uname=%s\n", uname);
}

static void I(progend)(void) {
	closedata();
	if (initbuffer != NULL) {
		print(".method public hidebysig static void $$%s_init() cil managed {\n"
		      ".maxstack 3\n", uname);
		printlist(&initbuffer);
		print("ret\n}\n");
		print("//$$INIT call void $$%s_init()\n", uname);
	}
}

Interface msilIR = {
	1, 1, 0,	/* char */
	2, 2, 0,	/* short */
	4, 4, 0,	/* int */
	4, 4, 0,	/* long */
	4, 4, 0,	/* long long */
	4, 4, 0,	/* float */
	8, 4, 0,	/* double */
	8, 4, 0,	/* long double */
	4, 4, 0,	/* T * */
	0, 1, 0,	/* struct; so that ARGB keeps stack aligned */
	1,		/* little_endian */
	0,		/* mulops_calls */
	0,		/* wants_callb */
	0,		/* wants_argb */
	1,		/* left_to_right */
	0,		/* wants_dag */
	0,		/* unsigned_char */
	0,		/* I(address) */
	I(blockbeg),
	I(blockend),
	I(defaddress),
	I(defconst),
	I(defstring),
	I(defsymbol),
	I(emit),
	I(export),
	I(function),
	I(gen),
	I(global),
	I(import),
	I(local),
	I(progbeg),
	I(progend),
	I(segment),
	I(space),
	0,		/* I(stabblock) */
	0,		/* I(stabend) */
	0,		/* I(stabfend) */
	0,		/* I(stabinit) */
	0,		/* I(stabline) */
	0,		/* I(stabsym) */
	0,		/* I(stabtype) */
};

Tree constexpr1(int tok) {
	Tree t;

	if (needconst && IR == &msilIR)
		IR->address = I(address);
	t = expr1(tok);
	if (IR == &msilIR)
		IR->address = NULL;
	return t;
}
#undef I
