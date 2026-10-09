#!/bin/sh
# build.sh - build the xenix86-devsys runtime and a C program for XENIX 2.1.3 (8086).
# Usage: ./build.sh prog.c [more.c ...]   ->  build/<prog> (x.out executable)
# Needs: bcc, as86, ld86 (dev86 / Debian-Ubuntu "bcc" + "bin86" packages), python3.
set -e
cd "$(dirname "$0")"
mkdir -p build
python3 tools/gensys.py
as86 -0 -o build/crt0.o lib/crt0.s
as86 -0 -o build/sys.o lib/sys.s
as86 -0 -o build/helpers.o lib/helpers.s
LIBOBJS=""
for f in lib/*.c; do
	o=build/$(basename "$f" .c).o
	bcc -0 -c -o "$o" "$f"
	LIBOBJS="$LIBOBJS $o"
done
[ $# -gt 0 ] || { echo "runtime built"; exit 0; }
OBJS=""
for f in "$@"; do
	o=build/$(basename "$f" .c).o
	bcc -0 -c -o "$o" "$f"
	OBJS="$OBJS $o"
done
prog=build/$(basename "$1" .c)
# crt0.o must come first: it supplies the text reserved block and the DS:0 header
ld86 -0 -i -o "$prog.aout" build/crt0.o $OBJS $LIBOBJS build/sys.o build/helpers.o
python3 tools/aout2xout.py "$prog.aout" "$prog"
