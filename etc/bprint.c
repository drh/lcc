#include "profio.c"
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* bprint [ -c | -Idir... | -f | -b | -n ] [ file... ]
 * annotate listings of files with prof.out data
 */

static char rcsid2[] = "$Id$";

#define NDIRS (sizeof dirs/sizeof dirs[0] - 1)
#define NELEMS(a) ((int)(sizeof (a)/sizeof ((a)[0])))

char *progname;
int number;
char *dirs[20];
int fcount;

void *alloc(unsigned);
void emitdata(char *);
void printfile(struct file *, int);
void printfuncs(struct file *, int);

void *allocate(unsigned long n, unsigned a) { return alloc(n); }

void *newarray(unsigned long m, unsigned long n, unsigned a) {
	return alloc(m*n);
}

int main(int argc, char *argv[]) {
	int i;
	struct file *p;
	void (*f)(struct file *, int) = printfile;

	progname = argv[0];
	if ((i = process("prof.out")) <= 0) {
		fprintf(stderr, "%s: can't %s `%s'\n", progname,
			i == 0 ? "open" : "interpret", "prof.out");
		exit(1);
	}
	for (i = 1; i < argc && *argv[i] == '-'; i++)
		if (strcmp(argv[i], "-c") == 0) {
			emitdata("prof.out"); 
			exit(0);
		} else if (strcmp(argv[i], "-b") == 0)
			f = printfile;
		else if (strcmp(argv[i], "-f") == 0) {
			fcount++;
			f = printfuncs;
		} else if (strcmp(argv[i], "-n") == 0)
			number++;
		else if (strncmp(argv[i], "-I", 2) == 0) {
			int j;
			for (j = 0; j < NDIRS && dirs[j]; j++)
				;
			if (j < NDIRS)
				dirs[j] = &argv[i][2];
			else
				fprintf(stderr, "%s: too many -I options\n", progname);
		} else {
			fprintf(stderr, "usage: %s [ -c | -b | -n | -f | -Idir... ] [ file... ]\n", progname);
			exit(1);
		}
	for (p = filelist; p; p = p->link)
		qsort(p->counts, p->count, sizeof *p->counts, compare);
	if (i < argc) {
		int nf = i < argc - 1 ? 1 : 0;
		for ( ; i < argc; i++, nf ? nf++ : 0)
			if (p = findfile(string(argv[i])))
				(*f)(p, nf);
			else
				fprintf(stderr, "%s: no data for `%s'\n", progname, argv[i]);
	} else {
		int nf = filelist && filelist->link ? 1 : 0;
		for (p = filelist; p; p = p->link, nf ? nf++ : 0)
			(*f)(p, nf);
	}
	return 0;
}

/* alloc - allocate n bytes or die */
void *alloc(unsigned n) {
	void *new = malloc(n);

	assert(new);
	return new;
}

/* emitdata - write prof.out data to file */
void emitdata(char *file) {
	FILE *fp;

	if (fp = fopen(file, "w")) {
		struct file *p;
		for (p = filelist; p; p = p->link) {
			int i;
			struct func *q;
			struct caller *r;
			fprintf(fp, "1\n%s\n", p->name);
			for (i = 0, q = p->funcs; q; i++, q = q->link)
				if (r = q->callers)
					for (i--; r; r = r->link)
						i++;
			fprintf(fp, "%d\n", i);
			for (q = p->funcs; q; q = q->link)
				if (q->count.count == 0 || !q->callers)
					fprintf(fp, "%s 1 %d %d %d ? ? 0 0\n", q->name, q->count.x,
						q->count.y, q->count.count);
				else
					for (r = q->callers; r; r = r->link)
						fprintf(fp, "%s 1 %d %d %d %s %s %d %d\n", q->name, q->count.x,
							q->count.y, r->count, r->name, r->file, r->x, r->y);
			fprintf(fp, "%d\n", p->count);
			for (i = 0; i < p->count; i++)
				fprintf(fp, "1 %d %d %d\n", p->counts[i].x,
					p->counts[i].y, p->counts[i].count);
		}
		fclose(fp);
	} else
		fprintf(stderr, "%s: can't create `%s'\n", progname, file);
}

/* openfile - open name for reading, searching -I directories */
FILE *openfile(char *name) {
	int i;
	FILE *fp;

	if (*name != '/')	
		for (i = 0; dirs[i]; i++) {
			char buf[200];
			sprintf(buf, "%s/%s", dirs[i], name);
			if (fp = fopen(buf, "r"))
				return fp;
		}
	return fopen(name, "r");
}

/* printfile - print annotated listing for p */
void printfile(struct file *p, int nf) {
	int lineno;
	FILE *fp;
	char *s, buf[512];
	struct count *u = p->counts, *r, *uend;

	if (u == 0 || p->count <= 0)
		return;
	uend = &p->counts[p->count];
	if ((fp = openfile(p->name)) == NULL) {
		fprintf(stderr, "%s: can't open `%s'\n", progname, p->name);
		return;
	}
	if (nf)
		printf("%s%s:\n\n", nf == 1 ? "" : "\f", p->name);
	for (lineno = 1; fgets(buf, sizeof buf, fp); lineno++) {
		if (number)
			printf("%d\t", lineno);
		while (u < uend && u->y < lineno)
			u++;
		for (s = buf; *s; ) {
			char *t = s + 1;
			while (u < uend && u->y == lineno && u->x < s - buf)
				u++;
			if (isalnum(*s) || *s == '_')
				while (isalnum(*t) || *t == '_')
					t++;
			while (u < uend && u->y == lineno && u->x < t - buf) {
				printf("<%d>", u->count);
				for (r = u++; u < uend && u->x == r->x && u->y == r->y && u->count == r->count; u++)
					;
			}
			while (s < t)
				putchar(*s++);
		}
		if (*s)
			printf("%s", s);
	}
	fclose(fp);
}

/* printfuncs - summarize data for functions in p */
void printfuncs(struct file *p, int nf) {
	struct func *q;

	if (nf)
		printf("%s:\n", p->name);
	for (q = p->funcs; q; q = q->link)
		if (fcount <= 1 || q->count.count == 0 || !q->callers)
			printf("%d\t%s\n", q->count.count, q->name);
		else {
			struct caller *r;
			for (r = q->callers; r; r = r->link)
				printf("%d\t%s\tfrom %s\tin %s:%d.%d\n", r->count, q->name, r->name,
					r->file, r->y, r->x + 1);
		}
		
}

/* string - save a copy of str, if necessary */
char *string(const char *str) {
	static struct string { struct string *link; char str[1]; } *list;
	struct string *p;

	for (p = list; p; p = p->link)
		if (strcmp(p->str, str) == 0)
			return p->str;
	p = alloc(strlen(str) + sizeof *p);
	strcpy(p->str, str);
	p->link = list;
	list = p;
	return p->str;
}
