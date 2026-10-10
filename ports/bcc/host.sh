#!/bin/sh
# host.sh - build BCC's compiler pass (bcc-cc1) for the host, from the same
# patched Dev86 sources as the XENIX one, into build/ports/host. The project's
# cross-compiling uses it (bcc -B build/ports/host/) instead of the host's
# packaged bcc-cc1, which lacks the patches; ports/bcc/test.py uses it as the
# reference the XENIX tools are compared against.
set -e
cd "$(dirname "$0")/../.."
SRC=build/ports/dev86-0.16.17
OUT=build/ports/host
[ -d $SRC ] || sh ports/bcc/fetch.sh
mkdir -p $OUT
CC1="bcc-cc1 codefrag dbnode declare express exptree floatop function gencode genloads
	glogcode hardop input label loadexp longop output preproc preserve scan softop
	state table type assign hashcmd"
files=""
for f in $CC1; do files="$files $SRC/bcc/$f.c"; done
${HOSTCC:-cc} -std=gnu89 -O -w -o $OUT/bcc-cc1 $files
echo "host bcc-cc1 in $OUT"
