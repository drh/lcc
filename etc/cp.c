/* cp src dst
 * copy src to dst
 */
#include <stdio.h>
#include <stdlib.h>

static char rcsid[] = "$Id: cp.c#1 2002/08/30 10:20:51 REDMOND\\drh $";

int main(int argc, char *argv[]) {
	if (argc != 3)
		fprintf(stderr, "usage: %s src dst\n", argv[0]);
	else {
		FILE *in = fopen(argv[1], "r"), *out = fopen(argv[2], "w");
		if (in == NULL)
			fprintf(stderr, "%s: can't read '%s'\n", argv[0], argv[1]);
		else if (out == NULL)
			fprintf(stderr, "%s: can't write '%s'\n", argv[0], argv[2]);
		else {
			static char buf[4096];
			size_t n;
			while ((n = fread(buf, 1, sizeof buf, in)) > 0)
				fwrite(buf, 1, n, out);
			fclose(in);
			fclose(out);
			return EXIT_SUCCESS;
		}
	}
	return EXIT_FAILURE;
}