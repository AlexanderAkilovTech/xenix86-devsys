#!/bin/sh
# build.sh - build the xenix86-devsys C library (build/libc.a) and, optionally, a program
# for XENIX 2.1.3 (8086).
# Usage: ./build.sh                                   ->  build/crt0.o and build/libc.a
#        ./build.sh prog.c [more.c | more.s ...]      ->  build/<prog> (x.out executable)
# Needs: bcc, as86, ld86, ar86 (dev86 / Debian-Ubuntu "bcc" + "bin86" packages), python3.
set -e
cd "$(dirname "$0")"
# Compile with BCC's compiler pass built from this project's patched sources
# (ports/bcc), not the host's packaged one: see ports/bcc/patches.
[ -x build/ports/host/bcc-cc1 ] || sh ports/bcc/host.sh >&2
BCC="bcc -Bbuild/ports/host/"
mkdir -p build/lib
rm -f build/lib/*.o build/sys/*.s build/libc.a
python3 tools/gensys.py build/sys
as86 -0 -o build/crt0.o lib/csu/crt0.s
for f in lib/crt/*.s lib/sys/*.s lib/gen/*.s lib/fp/*.s build/sys/*.s; do
	o=build/lib/$(basename "$f" .s).o
	[ -f "$o" ] && { echo "build.sh: two library modules named $o" >&2; exit 1; }
	as86 -0 -o "$o" "$f"
done
for f in lib/gen/*.c lib/stdio/*.c; do
	o=build/lib/$(basename "$f" .c).o
	[ -f "$o" ] && { echo "build.sh: two library modules named $o" >&2; exit 1; }
	$BCC -0 -I -Iinclude -c -o "$o" "$f"
done
# printf's and scanf's floating point conversions win over their stubs when
# linked (class 1; see tools/objclass.py)
python3 tools/objclass.py build/lib/fltcvt.o 1
python3 tools/objclass.py build/lib/fltscan.o 1
ar86 rc build/libc.a build/lib/*.o
# the math library, libm.a (cc ... -lm)
mkdir -p build/libm
rm -f build/libm/*.o build/libm.a
for f in lib/m/*.c; do
	$BCC -0 -I -Iinclude -c -o build/libm/$(basename "$f" .c).o "$f"
done
python3 tools/objclass.py build/libm/matherr.o 1	# a program's own matherr wins
ar86 rc build/libm.a build/libm/*.o
[ $# -gt 0 ] || { echo "libc.a built ($(ls build/lib/*.o | wc -l) modules), libm.a ($(ls build/libm/*.o | wc -l))"; exit 0; }
OBJS=""
for f in "$@"; do
	case "$f" in
	*.s)	o=build/$(basename "$f" .s)_s.o; as86 -0 -o "$o" "$f" ;;
	*)	o=build/$(basename "$f" .c).o; $BCC -0 -I -Iinclude -c -o "$o" "$f" ;;
	esac
	OBJS="$OBJS $o"
done
prog=build/$(basename "$1" .c)
# crt0.o must come first: it supplies the text reserved block and the DS:0 header.
# ld86 takes only the library modules the program actually uses.
ld86 -0 -i -o "$prog.aout" build/crt0.o $OBJS build/libm.a build/libc.a
python3 tools/aout2xout.py "$prog.aout" "$prog"
