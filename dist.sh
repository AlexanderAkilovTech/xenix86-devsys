#!/bin/sh
# dist.sh - build the whole Development System and lay it out the way it
# installs on XENIX, under build/xenix/root:
#   /usr/bin/cc, make, as86, ld86, ar86         commands
#   /usr/lib/cc/bcc-cpp, bcc-cc1, aout2x        compiler passes
#   /usr/lib/cc/crt0.o, libc.a                  start-up code and C library
#   /usr/include                                headers
set -e
cd "$(dirname "$0")"
./build.sh > /dev/null
sh ports/bcc/build.sh > /dev/null
./build.sh cmd/cc.c > /dev/null
./build.sh cmd/aout2x.c > /dev/null
./build.sh cmd/make.c > /dev/null
R=build/xenix/root
rm -rf $R
mkdir -p $R/usr/bin $R/usr/lib/cc $R/usr/include/sys
cp build/cc build/make $R/usr/bin/
cp build/xenix/bin/as86 build/xenix/bin/ld86 build/xenix/bin/ar86 $R/usr/bin/
cp build/xenix/bin/bcc-cpp build/xenix/bin/bcc-cc1 build/aout2x $R/usr/lib/cc/
cp build/crt0.o build/libc.a $R/usr/lib/cc/
cp include/*.h $R/usr/include/
cp include/sys/*.h $R/usr/include/sys/
echo "XENIX tree in $R:"
(cd $R && find . -type f | sort | sed 's|^\.||')
