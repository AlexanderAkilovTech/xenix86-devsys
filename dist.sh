#!/bin/sh
# dist.sh - build the whole Development System and lay it out the way it
# installs on XENIX, under build/xenix/root:
#   /usr/bin/cc, make, as86, ld86, ar86,        commands
#            nm, size, strip, clear
#   /usr/lib/cc/bcc-cpp, bcc-cc1, aout2x        compiler passes
#   /usr/lib/cc/copt, rules.*                   peephole optimiser (cc -O)
#   /usr/lib/cc/crt0.o, libc.a, libm.a,         start-up code, C, math and
#               libtermcap.a                    termcap libraries
#   /usr/include                                headers
#   /usr/bin/man, /usr/man/cat1, cat3, whatis   the manual, formatted
set -e
cd "$(dirname "$0")"
./build.sh > /dev/null
sh ports/bcc/build.sh > /dev/null
./build.sh cmd/cc.c > /dev/null
./build.sh cmd/aout2x.c > /dev/null
./build.sh cmd/make.c > /dev/null
for p in nm size strip; do ./build.sh cmd/$p.c cmd/objfile.c > /dev/null; done
./build.sh cmd/clear.c > /dev/null
./build.sh cmd/man.c > /dev/null
R=build/xenix/root
rm -rf $R
mkdir -p $R/usr/bin $R/usr/lib/cc $R/usr/include
# the commands are installed without symbol tables, as SCO's are
for p in cc make nm size strip clear man; do python3 tools/aout2xout.py --strip build/$p.aout $R/usr/bin/$p > /dev/null; done
cp build/xenix/bin/as86 build/xenix/bin/ld86 build/xenix/bin/ar86 $R/usr/bin/
cp build/xenix/bin/bcc-cpp build/xenix/bin/bcc-cc1 $R/usr/lib/cc/
python3 tools/aout2xout.py --strip build/aout2x.aout $R/usr/lib/cc/aout2x > /dev/null
cp build/xenix/bin/copt build/xenix/bin/rules.* $R/usr/lib/cc/
cp build/crt0.o build/libc.a build/libm.a build/libtermcap.a $R/usr/lib/cc/
cp include/*.h $R/usr/include/
# the manual, formatted for XENIX, which has no nroff
python3 tools/man.py $R/usr/man man/man1/*.1 man/man3/*.3
# include/sys holds stand-ins for cross-compiling; on XENIX, /usr/include/sys
# is SCO's own, from the Operating System disk N05 (see tools/mkset.py)
echo "XENIX tree in $R:"
(cd $R && find . -type f | sort | sed 's|^\.||')

# the custom-installable set: soft.perms and the Development System disks
python3 tools/mkset.py

# for tests on the host: the tree plus the sys/ stand-ins that, on XENIX,
# SCO's disk N05 supplies
T=build/xenix/testroot
rm -rf $T
cp -r $R $T
mkdir -p $T/usr/include/sys
cp include/sys/*.h $T/usr/include/sys/
