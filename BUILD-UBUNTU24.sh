#!/usr/bin/env bash
set -e

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# Ensure the project belongs to the current user.
sudo chown -R "$USER:$USER" "$ROOT"
set -o pipefail

ROOT=$(cd "$(dirname "$0")" && pwd)
LCC="$ROOT"
WORK="$ROOT/build-ubuntu24"
NASM_TARBALL="$WORK/nasm-0.98.39.tar.bz2"
NASM_URL="https://ftp2.osuosl.org/pub/blfs/conglomeration/nasm/nasm-0.98.39.tar.bz2"

if [[ $EUID -eq 0 ]]; then SUDO=""; else SUDO="sudo"; fi

$SUDO apt-get update
$SUDO apt-get install -y build-essential bison nasm gcc-multilib libc6-dev-i386 gdb wget tar bzip2

mkdir -p "$WORK"
if [[ ! -f "$NASM_TARBALL" ]]; then
  wget -O "$NASM_TARBALL" "$NASM_URL"
fi

rm -rf "$WORK/nasm-src"
mkdir -p "$WORK/nasm-src"
tar -xjf "$NASM_TARBALL" -C "$WORK/nasm-src"
NASMDIR=$(find "$WORK/nasm-src" -maxdepth 2 -type d -name lcc -print -quit)
if [[ -z "$NASMDIR" ]]; then
  echo "ERROR: NASM source archive did not contain the historical lcc/ port directory."
  exit 2
fi

# The historical port explicitly states that it was tested with LCC 3.6.
# It uses NASM as the x86 backend and provides Linux ELF driver code.
cp "$NASMDIR/x86nasm.md" "$LCC/src/x86nasm.md"
cp "$NASMDIR/lin-elf.c" "$LCC/etc/lin-elf.c"
cp "$NASMDIR/bind.c" "$LCC/src/bind.c"

# Ubuntu installs NASM as /usr/bin/nasm, while the historical LCC Linux
# driver hard-codes /usr/local/bin/nasm.  Use the NASM executable actually
# available on this system instead of relying on the historical path.
NASM_BIN="$(command -v nasm || true)"
if [[ -z "$NASM_BIN" ]]; then
  echo "ERROR: nasm was installed but could not be found in PATH."
  exit 3
fi
python3 - "$LCC/etc/lin-elf.c" "$NASM_BIN" <<'PY'
from pathlib import Path
import sys
p = Path(sys.argv[1])
nasm = sys.argv[2]
s = p.read_text()
s = s.replace("/usr/local/bin/nasm", nasm)
p.write_text(s)
print(f"LCC Linux driver NASM path: {nasm}")
PY

# The historical lin-elf.c invokes /usr/bin/ld directly and hard-codes
# 1990s Linux CRT objects such as /usr/lib/crt1.o and crtbegin.o.
# Ubuntu 24.04 keeps these startup objects under multiarch/GCC directories.
# Use the GCC 32-bit driver for the final link instead; it selects the
# correct Ubuntu 24.04 CRT objects and dynamic loader automatically.
python3 - "$LCC/etc/lin-elf.c" <<'PY'
from pathlib import Path
import re, sys
p = Path(sys.argv[1])
s = p.read_text()
pat = re.compile(r'char \*ld\[\]\s*=\s*\{.*?\};', re.S)
replacement = 'char *ld[] = { "/usr/bin/gcc", "-m32", "-o", "$3", "$2", "$1", 0 };'
s2, n = pat.subn(replacement, s, count=1)
if n != 1:
    raise SystemExit("ERROR: could not locate ld[] in lin-elf.c")
p.write_text(s2)
print("LCC Linux driver linker: /usr/bin/gcc -m32")
PY

# The historical README asks for x86nasm.o in the RCC object list and rules.
python3 - "$LCC/makefile" <<'PY'
from pathlib import Path
p=Path(__import__('sys').argv[1])
s=p.read_text()
if '$(BUILDDIR)/x86nasm.o' not in s:
    s=s.replace('\t$(BUILDDIR)/x86.o\n', '\t$(BUILDDIR)/x86.o \\\n\t$(BUILDDIR)/x86nasm.o\n')
if '$(BUILDDIR)/x86nasm.o:' not in s:
    marker='$(BUILDDIR)/x86.o:\t$(BUILDDIR)/x86.c;\t$(CC) -c $(CFLAGS) -Isrc -o $@ $(BUILDDIR)/x86.c\n'
    repl=marker+'$(BUILDDIR)/x86nasm.o:\t$(BUILDDIR)/x86nasm.c;\t$(CC) -c $(CFLAGS) -Isrc -o $@ $(BUILDDIR)/x86nasm.c\n'
    s=s.replace(marker,repl)
if '$(BUILDDIR)/x86nasm.c:' not in s:
    marker='$(BUILDDIR)/x86.c:\t$(BUILDDIR)/lburg src/x86.md;\t$(BUILDDIR)/lburg <src/x86.md   >$@\n'
    repl=marker+'$(BUILDDIR)/x86nasm.c:\t$(BUILDDIR)/lburg src/x86nasm.md;\t$(BUILDDIR)/lburg <src/x86nasm.md >$@\n'
    s=s.replace(marker,repl)
p.write_text(s)
PY

# Ubuntu 24.04 ships a modern Bison/Yacc. LCC 3.6's lburg grammar uses
# the token name TERM, which collides with lburg's old Kind enum, and the
# grammar action references yylineno before its old-style definition below.
# Keep the grammar semantics unchanged, but make those two names portable.
python3 - "$LCC/lburg/lburg.h" "$LCC/lburg/lburg.c" "$LCC/lburg/gram.y" <<'PY'
from pathlib import Path
import sys
h, c, y = map(Path, sys.argv[1:])
s=h.read_text()
s=s.replace('typedef enum { TERM=1, NONTERM } Kind;', 'typedef enum { BURG_TERM=1, BURG_NONTERM } Kind;')
s=s.replace('/* TERM */', '/* BURG_TERM */').replace('/* NONTERM */', '/* BURG_NONTERM */')
h.write_text(s)
s=c.read_text()
s=s.replace('== TERM', '== BURG_TERM').replace('= TERM', '= BURG_TERM')
s=s.replace('== NONTERM', '== BURG_NONTERM').replace('= NONTERM', '= BURG_NONTERM')
c.write_text(s)
s=y.read_text()
if 'static int yylineno = 0;' not in s.split('%}')[0]:
    s=s.replace('static char rcsid[] = "$Id$";', 'static char rcsid[] = "$Id$";\nstatic int yylineno = 0;')
s=s.replace('static int yylineno = 0;\nstatic int ppercent', 'static int ppercent')
y.write_text(s)
PY

# Build the compiler itself.  The historical Linux driver is used as HOSTFILE.
rm -rf "$WORK/lcc-build"
mkdir -p "$WORK/lcc-build/include"
cp -a "$LCC/include/x86/dos/." "$WORK/lcc-build/include/" 2>/dev/null || true
# The compiler can be bootstrapped independently of the final Linux C library headers.
# The historical lin-elf.c defaults LCCDIR to /usr/local/lib/lcc/.
# For this self-contained build, point LCCDIR at the actual bootstrap output
# directory so lcc can find the cpp/rcc binaries without an installation step.
LCCBUILD="$WORK/lcc-build"
CFLAGS="-O0 -g -D_POSIX_SOURCE -DLCCDIR=\\\"$LCCBUILD/\\\""
mkdir -p "$LCCBUILD/include"

make -C "$LCC" BUILDDIR="$LCCBUILD" HOSTFILE=etc/lin-elf.c YACC=yacc CC="gcc -m32" CFLAGS="$CFLAGS" all

echo
printf '%s\n' 'LCC 3.6 bootstrap build completed.'
printf 'rcc: %s\n' "$WORK/lcc-build/rcc"
printf 'lcc: %s\n' "$WORK/lcc-build/lcc"
