#!/bin/sh
# build.sh - build BCC (Dev86 0.16.17) to run on XENIX 2.1.3 (8086):
#   bcc-cpp  preprocessor     bcc-cc1  compiler     as86  assembler     ld86  linker
#   copt     peephole optimizer (cc -O), and its rules files
#   ar86     archiver (GNU ar, as Dev86 ships it)
# Results go to build/xenix/bin. Each must fit the 8086's 64K code segment;
# bcc-cc1 is the tight one. Needs the host BCC tools and the fetched sources.
set -e
cd "$(dirname "$0")/../.."
ROOT=$(pwd)
SRC=build/ports/dev86-0.16.17
OBJ=build/ports/obj
OUT=build/xenix/bin
[ -d $SRC ] || sh ports/bcc/fetch.sh
sh ports/bcc/host.sh > /dev/null	# the patched compiler pass and copt, for the host
./build.sh > /dev/null			# our crt0.o and libc.a
mkdir -p $OUT
# Only our headers (-I alone drops BCC's own); K&R mode has no const;
# M_XENIX is the name SCO's compiler predefined, and patch 03 keys on it.
CC="bcc -B$ROOT/build/ports/host/ -0 -I -I$ROOT/include -Dconst= -DM_XENIX"

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
	ld86 -0 -i -s -o $OBJ/$prog.aout build/crt0.o $objs build/libc.a	# installed stripped
	python3 tools/aout2xout.py $OBJ/$prog.aout $OUT/$prog $stack
}

for d in bcc as ld; do echo '#define VERSION "0.16.17"' > $SRC/$d/version.h; done
cp $SRC/libc/include/ar.h $SRC/ld/ar.h
cp $SRC/libc/include/ar.h $SRC/ld/rel_aout.h $SRC/ar/

CPP="main cpp hash token1 token2"
compile cpp "" $CPP
link bcc-cpp 8192 cpp $CPP

# bcc-cc1 only fits in 64K as callersaves code (cc1 -c: a function keeps no
# registers for its caller), through copt with patch 11's rules and rules.mc,
# and assembled with as86 -O (the shortest jumps). With floating point: the
# soft float routines come from libc.a, and NO_ATOF leaves out atof, which
# does not fit; cc has bcc-cpp write floating constants in hexadecimal (-H),
# which bcc-cc1 reads itself (patch 13).
CC1="bcc-cc1 codefrag dbnode declare express exptree floatop function gencode genloads
	glogcode hardop input label loadexp longop output preproc preserve scan softop
	state table type assign hashcmd"
mkdir -p $OBJ/bcc
for f in $CC1; do
	o=$OBJ/bcc/$f
	/usr/lib/bcc/bcc-cpp $SRC/bcc/$f.c -o $o.i -D__ELKS__ -D__unix__ -I$ROOT/include \
		-Dconst= -DM_XENIX -DNO_ATOF -D__BCC_VERSION__=0x001011L -0
	build/ports/host/bcc-cc1 $o.i -o $o.cc1 -0 -O -c
	build/ports/host/copt $o.cc1 -o $o.s -c! -d$SRC/copt rules.start rules.86 rules.mc rules.end
	as86 -0 -O -u -w -o $o.o $o.s
	rm -f $o.i $o.cc1 $o.s
done
link bcc-cc1 8192 bcc $CC1

COPT=copt
compile copt "" $COPT
link copt 4096 copt $COPT
for r in rules.start rules.86 rules.mc rules.end; do cp $SRC/copt/$r $OUT/$r; done

AS="as assemble errors express genbin genlist genobj gensym keywords macro mops pops
	readsrc scan table typeconv alloc"
# Read sources a line at a time (a 64K data segment cannot hold a large file
# whole) and keep symbols in one fixed heap (patch 03).
compile as "-DBIGBUFFER=0 -DUSE_FIXED_HEAP" $AS
link as86 8192 as $AS

LD="dumps io ld readobj table typeconv linksyms mkar writex86 writebin writeemu"
compile ld "-DREL_OUTPUT" $LD
# ld86 takes all the memory it can get for its tables and needs little stack
# (under 300 bytes linking bcc-cc1): a smaller stack leaves it more room
link ld86 4096 ld $LD

# GNU ar wants alloca; its own malloc-based one is the portable choice
compile ar "" ar
mkdir -p $OBJ/ar
$CC -D__TINYC__ -c -o $OBJ/ar/alloca.o $SRC/ar/alloca.c
link ar86 4096 ar ar alloca

echo "XENIX binaries in $OUT"
