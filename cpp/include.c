#include <stdlib.h>
#include <string.h>
#include "cpp.h"

#define min(a,b) ((a)<(b)?a:b);

Includelist	includelist[NINCLUDE] = {
	{ 0, 1, "/usr/include" },
};

void
doinclude(Tokenrow *trp)
{
	char fname[256], iname[256];
	Includelist *ip;
	int angled, len, fd, i;

	trp->tp += 1;
	if (trp->tp>=trp->lp)
		goto syntax;
	if (trp->tp->type!=STRING && trp->tp->type!=LT) {
		len = trp->tp - trp->bp;
		expandrow(trp, "<include>");
		trp->tp = trp->bp+len;
	}
	if (trp->tp->type==STRING) {
		len = min(sizeof(fname)-1, trp->tp->len-2);
		strncpy(fname, trp->tp->t+1, len);
		angled = 0;
	} else {
		len = 0;
		trp->tp++;
		while (trp->tp->type!=GT) {
			if (trp->tp>trp->lp || len+trp->tp->len+2 >= sizeof(fname))
				goto syntax;
			strncpy(fname+len, trp->tp->t, trp->tp->len);
			len += trp->tp->len;
			trp->tp++;
		}
		angled = 1;
	}
	trp->tp += 2;
	if (trp->tp < trp->lp || len==0)
		goto syntax;
	fname[len] = '\0';
	if (fname[0]=='/') {
		fd = open(fname, 0);
		strcpy(iname, fname);
	} else for (fd=-1,i=NINCLUDE-1; i>=0; i--) {
		ip = &includelist[i];
		if (ip->file==NULL || ip->deleted || (angled && ip->always==0))
			continue;
		if (strlen(fname)+strlen(ip->file)+2 > sizeof(iname))
			continue;
		strcpy(iname, ip->file);
		strcat(iname, "/");
		strcat(iname, fname);
		if ((fd = open(iname, 0)) >= 0)
			break;
	}
	if (fd >= 0) {
		if (++incdepth > 10)
			error(FATAL, "#include too deeply nested");
		setsource(newstring(iname, strlen(iname), 0), fd, NULL);
		genline();
	} else {
		trp->tp = trp->bp+2;
		error(ERROR, "Could not find include file %r", trp);
	}
	return;
syntax:
	error(ERROR, "Syntax error in #include");
	return;
}

/*
 * Generate a line directive for cursource
 */
void
genline(void)
{
	static Token ta = { UNCLASS, NULL, 0, 0 };
	static Tokenrow tr = { &ta, &ta, &ta+1, 1 };
	char *p;

	ta.t = p = outp;
	strcpy(p, "#line ");
	p += sizeof("#line ")-1;
	p = outnum(p, cursource->line);
	*p++ = ' '; *p++ = '"';
	strcpy(p, cursource->filename);
	p += strlen(p);
	*p++ = '"'; *p++ = '\n';
	ta.len = p-outp;
	outp = p;
	tr.tp = tr.bp;
	puttokens(&tr);
}
