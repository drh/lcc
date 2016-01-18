#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
#endif
#ifndef EXPORT
#define EXPORT
#endif

static char rcsid[] = "$Id: thunks.c#5 2002/08/30 14:24:46 REDMOND\\drh $";

struct thunk {
	struct thunk *link;
	void (*entrypoint)();
	unsigned char code[16];
};
static struct thunk *thunktable[237];

#define HASH(f) ((int)(((unsigned)f)>>2)%(sizeof thunktable/sizeof thunktable[0]))

static void *newthunk(void (*f)(), void *thunkcode, size_t size, size_t offset) {
	int i = HASH(f);
	struct thunk *tp = malloc(sizeof *tp);

	if (tp == NULL)
		return NULL;	/* cause an exception when dereferenced */
	tp->link = thunktable[i];
	thunktable[i] = tp;
	tp->entrypoint = f;
	memcpy(tp->code, thunkcode, size);
	*(int *)(tp->code + offset) = (char *)f - (tp->code + offset + 4);
	return tp->code;
}

#define NOP 0x90

EXPORT void *__getUMThunk(void (*f)()) {
	static struct thunkcode {		/* must be <= 16 bytes */
		unsigned char code[12];
		int offset;
	} thunkcode = {
		NOP,  NOP,  NOP,
		0x8B, 0x4C, 0x24, 0x04,		/* mov ecx,DWORD PTR [esp+4] */
		0x8B, 0x54, 0x24, 0x08,		/* mov edx,DWORD PTR [esp+8] */
		0xE9,						/* jmp rel32 */
		0
	};
	int i = HASH(f);
	struct thunk *tp = thunktable[i];

	for ( ; tp != NULL; tp = tp->link)
		if (tp->entrypoint == f)
			return tp->code;
	return newthunk(f, &thunkcode, sizeof thunkcode, offsetof(struct thunkcode,offset));
}

EXPORT void *__getMUThunk(void (*f)()) {
	static struct thunkcode {		/* must be <= 16 bytes */
		unsigned char code[4];
		int offset;
		unsigned char retcode[4];
	} thunkcode = {
		NOP,
		0x52,						/* push	edx */
		0x51,						/* push ecx */
		0xE8,						/* call rel32 */
		0,
		0x83, 0xC4, 0x08,			/* add esp,8 */
		0xC3						/* ret */
	};
	int i = HASH(f);
	struct thunk *tp = thunktable[i];

	for ( ; tp != NULL; tp = tp->link)
		if (tp->entrypoint == f)
			return tp->code;
	return newthunk(f, &thunkcode, sizeof thunkcode, offsetof(struct thunkcode,offset));
}
