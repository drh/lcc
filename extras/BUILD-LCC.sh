#! /bin/sh
#!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
# NB: Change this with each new release of lcc
VERSION=lcc-4.1
#!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
# /usr/local/src/lcc/lcc-4.0/BUILD-LCC.sh, Thu Jul  3 07:56:57 1997
# Edit by Nelson H. F. Beebe <beebe@plot79.math.utah.edu>
#=======================================================================
# This is a UNIX shell script to build and install lcc 3.6 or later on
# each supported architecture.  If you follow the widely-used GNU (and
# others) default of using /usr/local/{bin,include,lib,man/man1} as
# the location of installed files, then NO customizations of this file
# will be necessary.
#
# Usage:
#
# csh/tcsh:
#	[setenv CC=...]
#	[setenv CFLAGS=...]
#	[setenv TMPDIR=...]
#	./BUILD-LCC.sh [--exec-prefix=path] [ target [ host ] ]
#
# bash/sh/ksh:
#	[CC=...] [CFLAGS=...] [TMPDIR=...] \
#		./BUILD-LCC.sh [--exec-prefix=path] [ target [ host ] ]
#
# For all supported UNIX systems, target and host can be omitted: they
# will be determined automatically from output of the uname command.
# 
# The value of CC must NOT contain any blanks; if you need extra
# options, e.g. "cc -std1", specify them like this: CC=cc CFLAGS=-std1.
#
# You can use the --exec-prefix option to provide a replacement for
# the default /usr/local installation path.  Files will be installed
# in directories lib/lcc-VERSION, man/man1, and bin under that path.
#
# This script has been used at *.math.utah.edu for successful builds
# like this:
#
#	DEC Alpha OSF/1 3.2:	./BUILD-LCC.sh alpha-osf
#	Intel GNU/Linux 2.0.35  ./BUILD-LCC.sh x86-linux
#	SGI MIPS IRIX 5.3:	./BUILD-LCC.sh mips-irix
#	SGI MIPS IRIX 6.3:	./BUILD-LCC.sh mips-irix
#	Sun SPARC Solaris 5.2:	./BUILD-LCC.sh sparc-solaris
#
# These targets were available in a 4.1 pretest, but have been
# dropped in the final distribution:
#
#	DEC MIPS ULTRIX 4.3:	./BUILD-LCC.sh mips-ultrix
#	Sun SPARC SunOS 4.1.3:	./BUILD-LCC.sh sparc-sun
#
# In each of these cases, a bare ./BUILD-LCC.sh also works.
# 
# If your default C compiler does not support ANSI/ISO Standard C,
# then you may need to set the CC environment to point to such a
# compiler (lcc, gcc, c89, ...).  This script will automatically check
# for a number of such compilers.  Without an already installed
# Standard-conforming compiler, lcc cannot be bootstrapped onto your
# machine without major source code modifications.  You may be able to
# use cross-compilation to build lcc on another system where you do
# have Standard C support, or you may be able to find a prebuilt
# binary distribution in the lcc archives that you can install without
# the need for compilation.
#
# This script makes one change from the recommended installation paths
# suggested in doc/install.html: it compiles into the lcc executable
# program version-specific paths to its associated installed, and
# installs the binaries under names xxx and xxx-y.z (y.z is the lcc
# version number) using hard links.  That way, multiple versions of
# lcc can remain installed on the system.  This practice is also
# common with GNUware.
#
# All of the installation tests, including cross-compilation tests,
# are performed; check the output carefully to ensure that they
# produce no errors, other than as documented in doc/install.html.
#
# An ls -l listing is produced for all installed files, so that
# you have feedback about what has been changed on your system.
#
# The source directory tree is treated as read-only; all building is
# done under TMPDIR, which defaults to /usr/tmp if it is not globally
# defined before invoking this script.  This permits builds from
# CD-ROMs and network-mounted read-only file systems, and also permits
# simultaneous builds on multiple architectures/hosts.  NB: On Sun
# Solaris 2.x at least, TMPDIR cannot be /tmp, because ld generates an
# error; /usr/tmp works fine, though it loses the speed benefits of an
# in-memory file system.  The amount of free space required under TMPDIR
# is about 11MB.
#
# [11-Sep-1998] -- update for lcc 4.1
# [15-Jul-1997] -- update for lcc 4.0
# [21-Oct-1996] -- update for lcc 3.6 (installation process changed
#		   significantly), and add support for command-line
#		   specification of TARGET and HOST values
# [30-Apr-1996] -- update with check for existence of GNU cpp, and
#		   make sure CC is not set
# [29-Aug-1995] -- original version
#=======================================================================

exec_prefix=/usr/local	

if test $# -gt 0
then
    # Support GNU configure-style --exec-prefix option
    case "$1" in
	-exec-prefix* | --exec_prefix* | --exec-prefix* | --exec-prefi* \
	| --exec-pref* | --exec-pre* | --exec-pr* | --exec-p* | --exec-* \
	| --exec* | --exe* | --ex*)
	    exec_prefix=`echo $1 | awk -F= '{print $2}'`
	    shift
	    ;;
    esac
fi

if test $# -gt 0 -a "xx$1" != "xx"
then
    TARGET=$1
else
    # Try to set TARGET and HOST automatically based on uname output
    os=`uname -s || echo UNKNOWN-OS`
    if test "xx$os" = "xxOSF1"
    then
	TARGET=alpha-osf
    elif test "xx$os" = "xxULTRIX"
    then
	TARGET=mips-ultrix
    elif test "xx$os" = "xxIRIX"
    then
	TARGET=mips-irix
    elif test "xx$os" = "xxIRIX64"
    then
	TARGET=mips-irix
    elif test "xx$os" = "xxSunOS"
    then
	majorversion=`uname -r | awk -F. '{print $1}'`
	if test "xx$majorversion" = "xx4"
	then
	    TARGET=sparc-sun
	    # SunOS 4.x has limited support for Standard C, and
	    # lacks definitions of two required symbols
	    CFLAGS="-DEXIT_SUCCESS=0 -DEXIT_FAILURE=1 $CFLAGS"
	elif test "xx$majorversion" = "xx5"
	then
	    CFLAGS='-DSUNDIR=\"/opt/SUNWspro/SC4.2/lib/\" '"$CFLAGS"
	    TARGET=sparc-solaris
	else
	    echo "Warning: unrecognized SunOS version $majorversion: assuming compatible with 5"
	    TARGET=sparc-solaris	
	fi
    elif test "xx$os" = "xxLinux"
    then
	TARGET=x86-linux
    else
	echo "There is no support for TARGET=$os in this version of lcc"
	exit 1
    fi
fi

if test $# -gt 1 -a "xx$2" != "xx"
then
    HOST=$2
else
    HOST=`echo $TARGET | tr - ' ' | awk '{print $2}'`
fi

echo ==================== Building $VERSION for TARGET=$TARGET HOST=$HOST
echo ==================== with exec_prefix=$exec_prefix

VERSIONNUMBER=`echo $VERSION | sed -e s/lcc-//`

# Where the installed files will go:
INSTALLBINDIR=$exec_prefix/bin
INSTALLLIBDIR=$exec_prefix/lib/$VERSION
INSTALLINCDIR=$INSTALLLIBDIR/include
INSTALLMANDIR=$exec_prefix/man

# rcp supports the -p (preserve file time stamp) flag, but not
# all cp implementations do, so use it instead
CP="rcp -p"

RM="rm -f"

TARGETOS=`echo $TARGET | tr - /`

# First do a sanity check to make sure that we can find suitable
# TARGET and HOST values, either from the command line, or internally
# by default.
if test ! -r etc/$HOST.c
then
    echo "Cannot find driver host file etc/$HOST.c"
    echo "Usage: $0 target host"
    echo "Target = one of: alpha-osf mips-irix mips-ultrix sparc-solaris x86-linux"
    echo "Host   = one of: `/bin/ls -1 etc | sed -e 's/[.][ch]//g'`"
    exit 1
fi

TARGETOS=`echo $TARGET | tr - /`
if test ! -d $TARGETOS
then
    echo "Cannot find target/os directory $TARGETOS"
    echo "Usage: $0 target host"
    echo "Target = one of: alpha-osf mips-irix mips-ultrix sparc-solaris x86-linux"
    echo "Host   = one of: `/bin/ls -1 etc | sed -e 's/[.][ch]//g'`"
    exit 1
fi

if test "xx$TMPDIR" = "xx"
then
    TMPDIR=/usr/tmp
fi

echo ========== "The build requires about 11MB of free space under $TMPDIR"
echo ========== "Check that you have enough:"
# We try to use the -k flag for df to get sensible size units, but not
# all df implementations support it, so we try bare df if df -k fails:
df -k $TMPDIR 2>/dev/null || df $TMPDIR

# NB: Cannot do this in /tmp, because ld step to create lcc fails on
# Sun Solaris 5.2, sigh...
BUILDDIR=$TMPDIR/$VERSION/$TARGETOS; export BUILDDIR

echo ========== Creating temporary build directory tree in $BUILDDIR
mkdir -p $BUILDDIR

if test "xx$os" = "xxLinux"
then
    TMPDIRNAME=/tmp/lcc.dirname.$$
    dirname `find /usr/lib/gcc-lib/*/[0-9]* /usr/local/lib/gcc-lib/*/[0-9]* -name cpp 2>/dev/null | head -1` >$TMPDIRNAME
    echo Using gcc release from `cat $TMPDIRNAME`
    if test -h $BUILDDIR/gcc
    then
	rm -f $BUILDDIR/gcc
    fi
    ln -s `cat $TMPDIRNAME` $BUILDDIR/gcc
    rm -f $TMPDIRNAME
fi

echo ========== Checking for ANSI/ISO Standard C compiler
if test "xx$CC" = "xx"
then
    # Try lcc last, because if a prior build installed lcc, but there
    # were errors in the tests, we prefer not to use it when a
    # correctly-working alternative is available.
    # Try c89 first, because on DEC Alpha OSF/1 3.2, cc produces
    # a core-dumping rcc, sigh...
    COMPILERS="c89 cc gcc lcc"
else
    COMPILERS="$CC"
fi
echo "#include <stdio.h>"   >$BUILDDIR/try.c
echo "#include <stdlib.h>" >>$BUILDDIR/try.c
echo 'int main(void) { printf("Hello" " " "there"); return (0); }' >>$BUILDDIR/try.c
CC=
echo ========== COMPILERS=$COMPILERS
for cc in $COMPILERS
do
    echo ========== Trying CC="$cc $CFLAGS" for compilation of Standard C test file
    echo =============== $cc $CFLAGS -c -o $BUILDDIR/try.o $BUILDDIR/try.c
    if $cc $CFLAGS -c -o $BUILDDIR/try.o $BUILDDIR/try.c 2>/dev/null
    then
	CC=$cc
	export CC
	$RM $BUILDDIR/try.c $BUILDDIR/try.o
	break
    fi
done

if test "xx$CC" = "xx"
then
    echo ========== Cannot find an ANSI/ISO Standard C compiler on your system
    exit 1
else
    echo ========== Using ANSI/ISO Standard C compiler: CC="$CC $CFLAGS"
fi

# Delete include files (from a possibly-failed prior installation)
$RM -r $BUILDDIR/include

# and recreate an empty include file directory
mkdir -p $BUILDDIR/include

# then copy in the needed include files
$CP include/$TARGETOS/*.h $BUILDDIR/include

echo ========== Making lcc driver ...
echo make CC="$CC $CFLAGS" LD="$CC $CFLAGS" HOSTFILE=etc/$HOST.c lcc CFLAGS="'"-DLCCDIR=\"$INSTALLLIBDIR/\""'"
make CC="$CC $CFLAGS" LD="$CC $CFLAGS" HOSTFILE=etc/$HOST.c lcc CFLAGS="'"-DLCCDIR=\"$INSTALLLIBDIR/\""'"

echo ========== Building compiler and accessories ...
# We need LD here to be a Standard C compiler; on DEC Ultrix, LD
# defaults to cc, which fails because it is for K&R C
echo make CC="$CC $CFLAGS" LD="$CC $CFLAGS" all
make CC="$CC $CFLAGS" LD="$CC $CFLAGS" all

echo ========== Compiler test 1 ...
echo make CC="$CC $CFLAGS" TARGET=$TARGETOS test
make CC="$CC $CFLAGS" TARGET=$TARGETOS test

echo ========== Compiler test 2 ...
# The /opt/SUNWspro/SC4.0/include/cc directory is needed for stab.h
# referred to in src/sparc.c:
# echo make CC="$CC $CFLAGS" triple CFLAGS=-I/opt/SUNWspro/SC4.0/include/cc
# make CC="$CC $CFLAGS" triple CFLAGS=-I/opt/SUNWspro/SC4.0/include/cc
echo make CC="$CC $CFLAGS" triple
make CC="$CC $CFLAGS" triple

echo ========== Compiler test 3 ...

echo make CC="$CC $CFLAGS" TARGET=$TARGETOS test
make CC="$CC $CFLAGS" TARGET=$TARGETOS test

echo ========== Cross-compiler code generation tests ...
REMOTEHOST=noexecute; export REMOTEHOST

# echo make CC="$CC $CFLAGS" TARGET=alpha/osf test
# make CC="$CC $CFLAGS" TARGET=alpha/osf test -i

echo make CC="$CC $CFLAGS" TARGET=mips/irix test
make CC="$CC $CFLAGS" TARGET=mips/irix test -i

## No longer supported in lcc-4.1:
## echo make CC="$CC $CFLAGS" TARGET=mips/ultrix test
## make CC="$CC $CFLAGS" TARGET=mips/ultrix test -i

echo make CC="$CC $CFLAGS" TARGET=sparc/solaris test
make CC="$CC $CFLAGS" TARGET=sparc/solaris test -i

## No longer supported in lcc-4.1:
## echo make CC="$CC $CFLAGS" TARGET=sparc/sun test
## make CC="$CC $CFLAGS" TARGET=sparc/sun test -i

# The GNU/Linux cross-compilation test can only be run on GNU/Linux
# systems, because some lcc header files require certain GNU/Linux
# header files that do not exist on other systems.
if test "xx$os" = "xxLinux"
then
    echo make CC="$CC $CFLAGS" TARGET=x86/linux test
    make CC="$CC $CFLAGS" TARGET=x86/linux test -i
fi

echo ========== Installing user-callable programs ...
if test ! -d $INSTALLBINDIR
then
    mkdir -p $INSTALLBINDIR
fi

for f in lburg lcc bprint
do
    $RM $INSTALLBINDIR/$f
    $RM $INSTALLBINDIR/$f-$VERSIONNUMBER
    $CP $BUILDDIR/$f $INSTALLBINDIR/$f
    ln $INSTALLBINDIR/$f $INSTALLBINDIR/$f-$VERSIONNUMBER
    chmod 775 $INSTALLBINDIR/$f
    ls -l $INSTALLBINDIR/$f $INSTALLBINDIR/$f-$VERSIONNUMBER
done

echo ========== Installing auxiliary programs and run-time libraries ...
if test ! -d $INSTALLLIBDIR
then
    mkdir -p $INSTALLLIBDIR
fi

for f in rcc cpp
do
    $RM $INSTALLLIBDIR/$f
    $CP $BUILDDIR/$f $INSTALLLIBDIR/$f
    chmod 775 $INSTALLLIBDIR/$f
    ls -l $INSTALLLIBDIR/$f
done
for f in bbexit.o
do
    $RM $INSTALLLIBDIR/$f
    $CP $BUILDDIR/$f $INSTALLLIBDIR/$f
    chmod 664 $INSTALLLIBDIR/$f
    ls -l $INSTALLLIBDIR/$f
done
for f in liblcc.a librcc.a
do
    $RM $INSTALLLIBDIR/$f
    $CP $BUILDDIR/$f $INSTALLLIBDIR/$f
    chmod 664 $INSTALLLIBDIR/$f
    ranlib $INSTALLLIBDIR/$f || true
    ls -l $INSTALLLIBDIR/$f
done


if test "xx$os" = "xxLinux"
then
    echo ========== Installing gcc link ...
    TMPDIRNAME=/tmp/lcc.dirname.$$
    dirname `find /usr/lib/gcc-lib/*/[0-9]* /usr/local/lib/gcc-lib/*/[0-9]* -name cpp 2>/dev/null | head -1` >$TMPDIRNAME
    echo Using gcc release from `cat $TMPDIRNAME`
    if test -h $INSTALLLIBDIR/gcc
    then
	rm -f $INSTALLLIBDIR/gcc
    fi
    ln -s `cat $TMPDIRNAME` $INSTALLLIBDIR/gcc
    rm -f $TMPDIRNAME
fi

echo ========== Installing include files ...
mkdir -p $INSTALLINCDIR
$CP include/$TARGETOS/*.h $INSTALLINCDIR
ls -lR $INSTALLINCDIR/*

echo ========== Installing man pages ...

if test ! -d $INSTALLMANDIR
then
    mkdir -p $INSTALLMANDIR
fi

if test ! -d $INSTALLMANDIR/man1
then
    mkdir -p $INSTALLMANDIR/man1
fi

if test ! -d $INSTALLMANDIR/cat1
then
    mkdir -p $INSTALLMANDIR/cat1
    chmod 777 $INSTALLMANDIR/cat1
fi

for f in doc/*.1
do
    g=`basename $f`
    chmod 777 $INSTALLMANDIR/cat1/$g 2>/dev/null
    $RM $INSTALLMANDIR/cat1/$g
    chmod 777 $INSTALLMANDIR/man1/$g 2>/dev/null
    $RM $INSTALLMANDIR/man1/$g
    $CP $f $INSTALLMANDIR/man1/$g
    chmod 664 $INSTALLMANDIR/man1/$g
    /bin/ls -l $INSTALLMANDIR/man1/$g
done

echo "If the build and install was successful, you can now delete the temporary build tree by"
echo "	rm -rf $TMPDIR/$VERSION"
