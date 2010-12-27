BUILDDIR=sparc-solaris
TARGET=sparc/solaris
TSTDIR=$(TARGET)/tst
HOSTFILE=etc/solaris.c
CFLAGS=-g -DSUNDIR=\"/usr/local/opt/SUNWspro/SC5.0/lib/\"
LDFLAGS=-g
CC=/usr/local/opt/SUNWspro/bin/cc
LD=/usr/local/opt/SUNWspro/bin/cc
