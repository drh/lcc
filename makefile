# $Id$
SHELL=/bin/sh

what:
	-@echo make `sed -n '/^all:/s///p' makefile` triple clean clobber

all:	rcc lburg cpp lcc bprint bbexit.o
rcc:		$(BUILDDIR)/rcc
lburg:		$(BUILDDIR)/lburg
cpp:		$(BUILDDIR)/cpp
lcc:		$(BUILDDIR)/lcc
bprint:		$(BUILDDIR)/bprint
bbexit.o:	$(BUILDDIR)/bbexit.o

RCCOBJS=$(BUILDDIR)/alloc.o \
	$(BUILDDIR)/bind.o \
	$(BUILDDIR)/dag.o \
	$(BUILDDIR)/decl.o \
	$(BUILDDIR)/enode.o \
	$(BUILDDIR)/error.o \
	$(BUILDDIR)/expr.o \
	$(BUILDDIR)/event.o \
	$(BUILDDIR)/init.o \
	$(BUILDDIR)/input.o \
	$(BUILDDIR)/lex.o \
	$(BUILDDIR)/list.o \
	$(BUILDDIR)/main.o \
	$(BUILDDIR)/output.o \
	$(BUILDDIR)/prof.o \
	$(BUILDDIR)/profio.o \
	$(BUILDDIR)/simp.o \
	$(BUILDDIR)/stmt.o \
	$(BUILDDIR)/string.o \
	$(BUILDDIR)/sym.o \
	$(BUILDDIR)/trace.o \
	$(BUILDDIR)/tree.o \
	$(BUILDDIR)/types.o \
	$(BUILDDIR)/null.o \
	$(BUILDDIR)/symbolic.o \
	$(BUILDDIR)/gen.o \
	$(BUILDDIR)/mips.o \
	$(BUILDDIR)/sparc.o \
	$(BUILDDIR)/x86.o

$(BUILDDIR)/rcc:	$(RCCOBJS)
			$(CC) $(CFLAGS) -o $@ $(LDFLAGS) $(RCCOBJS)

$(RCCOBJS):	src/c.h src/token.h src/config.h

$(BUILDDIR)/alloc.o:	src/alloc.c;	$(CC) -c $(CFLAGS) -Isrc -o $@ src/alloc.c
$(BUILDDIR)/bind.o:	src/bind.c;	$(CC) -c $(CFLAGS) -Isrc -o $@ src/bind.c
$(BUILDDIR)/dag.o:	src/dag.c;	$(CC) -c $(CFLAGS) -Isrc -o $@ src/dag.c
$(BUILDDIR)/decl.o:	src/decl.c;	$(CC) -c $(CFLAGS) -Isrc -o $@ src/decl.c
$(BUILDDIR)/enode.o:	src/enode.c;	$(CC) -c $(CFLAGS) -Isrc -o $@ src/enode.c
$(BUILDDIR)/error.o:	src/error.c;	$(CC) -c $(CFLAGS) -Isrc -o $@ src/error.c
$(BUILDDIR)/event.o:	src/event.c;	$(CC) -c $(CFLAGS) -Isrc -o $@ src/event.c
$(BUILDDIR)/expr.o:	src/expr.c;	$(CC) -c $(CFLAGS) -Isrc -o $@ src/expr.c
$(BUILDDIR)/gen.o:	src/gen.c;	$(CC) -c $(CFLAGS) -Isrc -o $@ src/gen.c
$(BUILDDIR)/init.o:	src/init.c;	$(CC) -c $(CFLAGS) -Isrc -o $@ src/init.c
$(BUILDDIR)/input.o:	src/input.c;	$(CC) -c $(CFLAGS) -Isrc -o $@ src/input.c
$(BUILDDIR)/lex.o:	src/lex.c;	$(CC) -c $(CFLAGS) -Isrc -o $@ src/lex.c
$(BUILDDIR)/list.o:	src/list.c;	$(CC) -c $(CFLAGS) -Isrc -o $@ src/list.c
$(BUILDDIR)/main.o:	src/main.c;	$(CC) -c $(CFLAGS) -Isrc -o $@ src/main.c
$(BUILDDIR)/null.o:	src/null.c;	$(CC) -c $(CFLAGS) -Isrc -o $@ src/null.c
$(BUILDDIR)/output.o:	src/output.c;	$(CC) -c $(CFLAGS) -Isrc -o $@ src/output.c
$(BUILDDIR)/prof.o:	src/prof.c;	$(CC) -c $(CFLAGS) -Isrc -o $@ src/prof.c
$(BUILDDIR)/profio.o:	src/profio.c;	$(CC) -c $(CFLAGS) -Isrc -o $@ src/profio.c
$(BUILDDIR)/simp.o:	src/simp.c;	$(CC) -c $(CFLAGS) -Isrc -o $@ src/simp.c
$(BUILDDIR)/stmt.o:	src/stmt.c;	$(CC) -c $(CFLAGS) -Isrc -o $@ src/stmt.c
$(BUILDDIR)/string.o:	src/string.c;	$(CC) -c $(CFLAGS) -Isrc -o $@ src/string.c
$(BUILDDIR)/sym.o:	src/sym.c;	$(CC) -c $(CFLAGS) -Isrc -o $@ src/sym.c
$(BUILDDIR)/symbolic.o:	src/symbolic.c;	$(CC) -c $(CFLAGS) -Isrc -o $@ src/symbolic.c
$(BUILDDIR)/trace.o:	src/trace.c;	$(CC) -c $(CFLAGS) -Isrc -o $@ src/trace.c
$(BUILDDIR)/tree.o:	src/tree.c;	$(CC) -c $(CFLAGS) -Isrc -o $@ src/tree.c
$(BUILDDIR)/types.o:	src/types.c;	$(CC) -c $(CFLAGS) -Isrc -o $@ src/types.c

$(BUILDDIR)/mips.o:	$(BUILDDIR)/mips.c;	$(CC) -c $(CFLAGS) -Isrc -o $@ $(BUILDDIR)/mips.c
$(BUILDDIR)/sparc.o:	$(BUILDDIR)/sparc.c;	$(CC) -c $(CFLAGS) -Isrc -o $@ $(BUILDDIR)/sparc.c
$(BUILDDIR)/x86.o:	$(BUILDDIR)/x86.c;	$(CC) -c $(CFLAGS) -Isrc -o $@ $(BUILDDIR)/x86.c

$(BUILDDIR)/mips.c:	$(BUILDDIR)/lburg src/mips.md;	$(BUILDDIR)/lburg <src/mips.md  >$@
$(BUILDDIR)/sparc.c:	$(BUILDDIR)/lburg src/sparc.md;	$(BUILDDIR)/lburg <src/sparc.md >$@
$(BUILDDIR)/x86.c:	$(BUILDDIR)/lburg src/x86.md;	$(BUILDDIR)/lburg <src/x86.md   >$@

YFLAGS=
$(BUILDDIR)/lburg:	$(BUILDDIR)/lburg.o $(BUILDDIR)/gram.o
			$(CC) -Ilburg -o $@ $(LDFLAGS) $(BUILDDIR)/lburg.o $(BUILDDIR)/gram.o

$(BUILDDIR)/lburg.o:	lburg/lburg.c lburg/lburg.h
			$(CC) -c $(CFLAGS) -Ilburg -o $@ lburg/lburg.c
$(BUILDDIR)/gram.o:	$(BUILDDIR)/gram.c lburg/lburg.h
			$(CC) -c $(CFLAGS) -Ilburg -o $@ $(BUILDDIR)/gram.c

$(BUILDDIR)/gram.c:	lburg/gram.y
			cwd=`pwd`; (cd $(BUILDDIR); \
$(YACC) $(YFLAGS) $$cwd/lburg/gram.y) && mv $(BUILDDIR)/y.tab.c $@


CPPOBJS=$(BUILDDIR)/cpp.o \
	$(BUILDDIR)/lexer.o \
	$(BUILDDIR)/nlist.o \
	$(BUILDDIR)/tokens.o \
	$(BUILDDIR)/macro.o \
	$(BUILDDIR)/eval.o \
	$(BUILDDIR)/include.o \
	$(BUILDDIR)/hideset.o \
	$(BUILDDIR)/unix.o

$(BUILDDIR)/cpp:	$(CPPOBJS)
			$(CC) -o $@ $(LDFLAGS) $(CPPOBJS)

$(CPPOBJS):	cpp/cpp.h

$(BUILDDIR)/cpp.o:	cpp/cpp.c;	$(CC) -c $(CFLAGS) -Icpp -o $@ cpp/cpp.c
$(BUILDDIR)/lexer.o:	cpp/lex.c;	$(CC) -c $(CFLAGS) -Icpp -o $@ cpp/lex.c
$(BUILDDIR)/nlist.o:	cpp/nlist.c;	$(CC) -c $(CFLAGS) -Icpp -o $@ cpp/nlist.c
$(BUILDDIR)/tokens.o:	cpp/tokens.c;	$(CC) -c $(CFLAGS) -Icpp -o $@ cpp/tokens.c
$(BUILDDIR)/macro.o:	cpp/macro.c;	$(CC) -c $(CFLAGS) -Icpp -o $@ cpp/macro.c
$(BUILDDIR)/eval.o:	cpp/eval.c;	$(CC) -c $(CFLAGS) -Icpp -o $@ cpp/eval.c
$(BUILDDIR)/include.o:	cpp/include.c;	$(CC) -c $(CFLAGS) -Icpp -o $@ cpp/include.c
$(BUILDDIR)/hideset.o:	cpp/hideset.c;	$(CC) -c $(CFLAGS) -Icpp -o $@ cpp/hideset.c
$(BUILDDIR)/unix.o:	cpp/unix.c;	$(CC) -c $(CFLAGS) -Icpp -o $@ cpp/unix.c

$(BUILDDIR)/bprint:	etc/bprint.c;	$(CC)    $(CFLAGS) -o $@ etc/bprint.c
$(BUILDDIR)/bbexit.o:	etc/bbexit.c;	$(CC) -c $(CFLAGS) -o $@ etc/bbexit.c

$(BUILDDIR)/lcc:	$(BUILDDIR)/lcc.o $(BUILDDIR)/host.o
			$(CC) -o $@ $(LDFLAGS) $(BUILDDIR)/lcc.o $(BUILDDIR)/host.o

$(BUILDDIR)/lcc.o:	etc/lcc.c;	$(CC) -c $(CFLAGS) -o $@ etc/lcc.c
$(BUILDDIR)/host.o:	$(HOSTFILE);	$(CC) -c $(CFLAGS) -o $@ $(HOSTFILE)

$(BUILDDIR)/include:
		@if [ ! -r $@ ]; then \
		echo $(BUILDDIR)/include is missing--install it before continuing; exit 1; fi

TSTDIR=$(BUILDDIR)/$(TARGET)/tst
T=$(TSTDIR)
test:	$(T) \
	$(T)/8q.s \
	$(T)/array.s \
	$(T)/cf.s \
	$(T)/cq.s \
	$(T)/cvt.s \
	$(T)/fields.s \
	$(T)/front.s \
	$(T)/incr.s \
	$(T)/init.s \
	$(T)/limits.s \
	$(T)/paranoia.s \
	$(T)/sort.s \
	$(T)/spill.s \
	$(T)/stdarg.s \
	$(T)/struct.s \
	$(T)/switch.s \
	$(T)/wf1.s \
	$(T)/yacc.s

$(T):
	-mkdir -p $@

PREREQS=$(BUILDDIR)/rcc $(BUILDDIR)/lcc $(BUILDDIR)/cpp $(BUILDDIR)/include src/run

$(T)/8q.s:	$(PREREQS) tst/8q.c       tst/8q.0;		@src/run $@
$(T)/array.s:	$(PREREQS) tst/array.c    tst/array.0;		@src/run $@
$(T)/cf.s:	$(PREREQS) tst/cf.c       tst/cf.0;		@src/run $@
$(T)/cq.s:	$(PREREQS) tst/cq.c       tst/cq.0;		@src/run $@
$(T)/cvt.s:	$(PREREQS) tst/cvt.c      tst/cvt.0;		@src/run $@
$(T)/fields.s:	$(PREREQS) tst/fields.c   tst/fields.0;		@src/run $@
$(T)/front.s:	$(PREREQS) tst/front.c    tst/front.0;		@src/run $@
$(T)/incr.s:	$(PREREQS) tst/incr.c     tst/incr.0;		@src/run $@
$(T)/init.s:	$(PREREQS) tst/init.c     tst/init.0;		@src/run $@
$(T)/limits.s:	$(PREREQS) tst/limits.c	  tst/limits.0;		@src/run $@
$(T)/paranoia.s:$(PREREQS) tst/paranoia.c tst/paranoia.0;	@src/run $@
$(T)/sort.s:	$(PREREQS) tst/sort.c     tst/sort.0;		@src/run $@
$(T)/spill.s:	$(PREREQS) tst/spill.c    tst/spill.0;		@src/run $@
$(T)/stdarg.s:	$(PREREQS) tst/stdarg.c   tst/stdarg.0;		@src/run $@
$(T)/struct.s:	$(PREREQS) tst/struct.c   tst/struct.0;		@src/run $@
$(T)/switch.s:	$(PREREQS) tst/switch.c   tst/switch.0;		@src/run $@
$(T)/wf1.s:	$(PREREQS) tst/wf1.c      tst/wf1.0;		@src/run $@
$(T)/yacc.s:	$(PREREQS) tst/yacc.c     tst/yacc.0;		@src/run $@

testclean:
		rm -f $(BUILDDIR)/*/*/tst/*.[12so]
		rm -f $(BUILDDIR)/*/*/tst/8q
		rm -f $(BUILDDIR)/*/*/tst/array
		rm -f $(BUILDDIR)/*/*/tst/cf
		rm -f $(BUILDDIR)/*/*/tst/cq
		rm -f $(BUILDDIR)/*/*/tst/cvt
		rm -f $(BUILDDIR)/*/*/tst/fields
		rm -f $(BUILDDIR)/*/*/tst/front
		rm -f $(BUILDDIR)/*/*/tst/inc
		rm -f $(BUILDDIR)/*/*/tst/init
		rm -f $(BUILDDIR)/*/*/tst/limits
		rm -f $(BUILDDIR)/*/*/tst/paranoia
		rm -f $(BUILDDIR)/*/*/tst/sort
		rm -f $(BUILDDIR)/*/*/tst/spill
		rm -f $(BUILDDIR)/*/*/tst/stdarg
		rm -f $(BUILDDIR)/*/*/tst/struct
		rm -f $(BUILDDIR)/*/*/tst/switch
		rm -f $(BUILDDIR)/*/*/tst/wf1
		rm -f $(BUILDDIR)/*/*/tst/yacc

triple:		$(BUILDDIR)/rcc $(BUILDDIR)/lcc $(BUILDDIR)/cpp
		$(BUILDDIR)/lcc -o $(BUILDDIR)/1rcc -d0.6 -Wo-lccdir=$(BUILDDIR) -B$(BUILDDIR)/  -Isrc src/*.c
		$(BUILDDIR)/lcc -o $(BUILDDIR)/2rcc -d0.6 -Wo-lccdir=$(BUILDDIR) -B$(BUILDDIR)/1 -Isrc src/*.c
		strip $(BUILDDIR)/[12]rcc
		dd if=$(BUILDDIR)/1rcc of=$(BUILDDIR)/rcc1 bs=512 skip=1
		dd if=$(BUILDDIR)/2rcc of=$(BUILDDIR)/rcc2 bs=512 skip=1
		if cmp $(BUILDDIR)/rcc[12]; then \
		mv $(BUILDDIR)/2rcc $(BUILDDIR)/rcc; \
		rm -f $(BUILDDIR)/rcc[12]; fi

clean:		testclean
		-rm -f `ls $(BUILDDIR)/*.o|sed /bbexit.o/d`
		-rm -f $(BUILDDIR)/mips.c $(BUILDDIR)/sparc.c $(BUILDDIR)/x86.c $(BUILDDIR)/gram.c
		-rm -f $(BUILDDIR)/rcc[12] $(BUILDDIR)/[12]rcc


clobber:	clean
		-rm -f $(BUILDDIR)/rcc $(BUILDDIR)/lburg $(BUILDDIR)/cpp
		-rm -f $(BUILDDIR)/lcc $(BUILDDIR)/bprint $(BUILDDIR)/bbexit.o
