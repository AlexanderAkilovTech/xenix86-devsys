#!/bin/sh
# build.sh - build the xenix86-devsys runtime and a C program for XENIX 2.1.3 (8086).
# Usage: ./build.sh prog.c [more.c | more.s ...]   ->  build/<prog> (x.out executable)
# Needs: bcc, as86, ld86 (dev86 / Debian-Ubuntu "bcc" + "bin86" packages), python3.
set -e
cd "$(dirname "$0")"
mkdir -p build
python3 tools/gensys.py
as86 -0 -o build/crt0.o lib/crt0.s
LIBOBJS=""
for f in lib/*.s; do
	[ "$f" = lib/crt0.s ] && continue
	o=build/$(basename "$f" .s).o
	as86 -0 -o "$o" "$f"
	LIBOBJS="$LIBOBJS $o"
done
for f in lib/*.c; do
	o=build/$(basename "$f" .c).o
	bcc -0 -Iinclude -c -o "$o" "$f"
	LIBOBJS="$LIBOBJS $o"
done
[ $# -gt 0 ] || { echo "runtime built"; exit 0; }
OBJS=""
for f in "$@"; do
	case "$f" in
	*.s)	o=build/$(basename "$f" .s)_s.o; as86 -0 -o "$o" "$f" ;;
	*)	o=build/$(basename "$f" .c).o; bcc -0 -Iinclude -c -o "$o" "$f" ;;
	esac
	OBJS="$OBJS $o"
done
prog=build/$(basename "$1" .c)
# crt0.o must come first: it supplies the text reserved block and the DS:0 header
ld86 -0 -i -o "$prog.aout" build/crt0.o $OBJS $LIBOBJS
python3 tools/aout2xout.py "$prog.aout" "$prog"
