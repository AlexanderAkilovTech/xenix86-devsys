#!/bin/sh
# build.sh - build BCC (Dev86 0.16.17) to run on XENIX 2.1.3 (8086):
#   bcc-cpp  preprocessor     bcc-cc1  compiler     as86  assembler     ld86  linker
# Results go to build/xenix/bin. Each must fit the 8086's 64K code segment;
# bcc-cc1 is the tight one. Needs the host BCC tools and the fetched sources.
set -e
cd "$(dirname "$0")/../.."
ROOT=$(pwd)
SRC=build/ports/dev86-0.16.17
OBJ=build/ports/obj
OUT=build/xenix/bin
[ -d $SRC ] || sh ports/bcc/fetch.sh
./build.sh > /dev/null			# our crt0.o and libc.a
mkdir -p $OUT
# Only our headers (-I alone drops BCC's own); K&R mode has no const;
# M_XENIX is the name SCO's compiler predefined, and patch 03 keys on it.
CC="bcc -0 -I -I$ROOT/include -Dconst= -DM_XENIX"

compile() {	# compile <dir> <extra flags> <files...>
	d=$1; flags=$2; shift 2
	mkdir -p $OBJ/$d
	for f in "$@"; do
		$CC $flags -c -o $OBJ/$d/$f.o $SRC/$d/$f.c
	done
}

link() {	# link <program> <stack> <dir> <files...>
	prog=$1; stack=$2; d=$3; shift 3
	objs=""
	for f in "$@"; do objs="$objs $OBJ/$d/$f.o"; done
	ld86 -0 -i -o $OBJ/$prog.aout build/crt0.o $objs build/libc.a
	python3 tools/aout2xout.py $OBJ/$prog.aout $OUT/$prog $stack
}

for d in bcc as ld; do echo '#define VERSION "0.16.17"' > $SRC/$d/version.h; done
cp $SRC/libc/include/ar.h $SRC/ld/ar.h

CPP="main cpp hash token1 token2"
compile cpp "" $CPP
link bcc-cpp 8192 cpp $CPP

CC1="bcc-cc1 codefrag dbnode declare express exptree floatop function gencode genloads
	glogcode hardop input label loadexp longop output preproc preserve scan softop
	state table type assign hashcmd"
compile bcc "" $CC1
link bcc-cc1 8192 bcc $CC1

AS="as assemble errors express genbin genlist genobj gensym keywords macro mops pops
	readsrc scan table typeconv alloc"
# Read sources a line at a time (a 64K data segment cannot hold a large file
# whole) and keep symbols in one fixed heap (patch 03).
compile as "-DBIGBUFFER=0 -DUSE_FIXED_HEAP" $AS
link as86 8192 as $AS

LD="dumps io ld readobj table typeconv linksyms mkar writex86 writebin writeemu"
compile ld "-DREL_OUTPUT" $LD
link ld86 8192 ld $LD

echo "XENIX binaries in $OUT"
