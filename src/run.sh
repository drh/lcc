#!/bin/sh
# $Id$
# ../run target-os test [ remotehost ]

# set -x
case "$1" in
symbolic-irix)	target=$1; include=mips/irix ;;
symbolic-osf)	target=$1; include=alpha/osf ;;
*)		target=$1;
		include=`expr "$1" : '\(.*\)-'`/`expr "$1" : '.*-\(.*\)'` ;;
esac
remotehost=${3-$REMOTEHOST} LCC=${LCC-lcc}

cd tst
if [ ! -r ../../../include/$include ]; then
	echo 2>&1 $0: unknown combination '"'$target'"'
	exit 1
fi

BUILDDIR=${BUILDDIR-../}
echo ${BUILDDIR}rcc -target=$target $2: 1>&2
$LCC -N -S -B${BUILDDIR} -I../../../include/$include -Ualpha -Usun -Uvax -Umips -Ux86 \
	 -Wf-errout=$2.2 -D`expr "$1" : '\(.*\)-'` -Wf-g0 -Wf-target=$target ../../../tst/$2.c
if [ $? != 0 ]; then remotehost=noexecute; fi
sed 's|\.\./\.\./\.\./tst/||g' <$2.2 >$2.tmp; mv $2.tmp $2.2
diff ../../../tst/$2.2 $2.2
if [ ! -r $2.sbk ]; then
	mv $2.s $2.sbk
	cp $2.sbk $2.s
else
	if diff $2.sbk $2.s; then exit 0; fi
fi

case "$remotehost" in
noexecute)	exit 0 ;;
""|"-")	echo "   executing" $2: 1>&2
	$LCC -o $2 $2.s -lm; ./$2 <../../../tst/$2.0 >$2.1 ;;
*)	echo "   executing" $2 on $remotehost: 1>&2
	rcp $2.s $remotehost:
	if expr "$remotehost" : '.*@' >/dev/null ; then
		remotehost="`expr $remotehost : '.*@\(.*\)'` -l `expr $remotehost : '\(.*\)@'`"
	fi
	rsh $remotehost "cc -o $2 $2.s -lm;./$2;rm -f $2 $2.[so]" <../../../tst/$2.0 >$2.1
	;;
esac
if [ ! -r $2.1bk ]; then
	mv $2.1 $2.1bk
	cp $2.1bk $2.1
	exit 0
fi
diff $2.1bk $2.1
exit $?
