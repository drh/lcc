#include "c.h"

/* ops [ {csilhfdxp}=n ]...
 * prints lcc dag operator set for a given set of type sizes.
 */

static char rcsid[] = "$Id$";

static char list[] = { 'c', 's', 'i', 'l', 'h', 'f', 'd', 'x', 'p', 0 };
static int sizes[] = {  1,   2,   4,   4,   8,   4,   8,  16,   8 };

static int doop(int op, int type, const char *sz, const char *opname) {
	int count = 0;
	static int last;

	if (op == LOAD)
		return 0;
	if (last != 0 && last != op)
		printf("\n");
	last = op;
	if (type == B || type == V) {
		printf(" %s=%d", opname, op + type);
		count++;
	} else {
		int i, done = 0;
		const char *s;
		for (i = 0; sz[i] != '\0' && (s = strchr(list, sz[i])) != NULL; i++) {
			int n = sizes[s-list];
			if ((done&(1<<n)) == 0) {
				printf(" %s%d=%d", opname, n, op + type + sizeop(n));
				count++;
			}
			done |= 1<<n;
		}
	}
	printf("\n");
	return count;
}

int main(int argc, char *argv[]) {
	int i, count = 0;

	for (i = 1; i < argc; i++) {
		char c, *s;
		int n;
		if (sscanf(argv[i], "%c=%d", &c, &n) == 2
		&& n > 0 && (s = strchr(list, c)) != NULL)
			sizes[s-list] = n;
		else {
			fprintf(stderr, "usage: %s [ {csilhfdxp}=n ]...\n", argv[0]);
			exit(EXIT_FAILURE);
		}
	}
#define gop(x,n)
#define op(x,t,s) count += doop(x,t,#s,#x #t);
#include "ops.h"
#undef gop
#undef op
	fprintf(stderr, "%d operators\n", count);
	return EXIT_SUCCESS;
}
