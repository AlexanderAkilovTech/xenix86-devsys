#!/bin/sh
# dist.sh - build the whole Development System and lay it out the way it
# installs on XENIX, under build/xenix/root:
#   /usr/bin/cc, make, as86, ld86, ar86,        commands
#            nm, size, strip, clear
#   /usr/lib/cc/bcc-cpp, bcc-cc1, aout2x        compiler passes
#   /usr/lib/cc/copt, rules.*                   peephole optimizer (cc -O)
#   /usr/lib/cc/crt0.o, libc.a, libm.a,         start-up code, C, math,
#               libtermcap.a, libcurses.a       termcap and curses libraries
#   /usr/include                                headers
#   /usr/bin/man, /usr/man/cat1, cat3, whatis   the manual, formatted
# and the system tools, some of them in place of XENIX's own:
#   /bin/ls (l, lc, lf, lr, lx), /bin/df        XENIX's, with -h
#   /bin/date                                   XENIX's, past 1999
#   /usr/bin/whereis, xtop (top)                new
#   /usr/bin/hostname, cursor, lastlogin,       new
#            uptime
#   /etc/asktime                                XENIX's, the boot time prompt
#   /etc/default/date                           how dates are written: 12- or
#                                               24-hour, month/day or day/month
#   /usr/adm/lastlog                            lastlogin's records
#   /usr/lib/xtop.syms                          xtop's kept kernel symbols
#   /usr/lib/cdprompt.csh, setprompt.csh,
#   /usr/lib/mkuser/*                           shell prompts with the directory
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
for p in ls df whereis hostname lastlogin; do ./build.sh cmd/$p.c > /dev/null; done
for p in date uptime asktime; do ./build.sh cmd/$p.c cmd/clock.c > /dev/null; done
for p in xtop cursor; do ./build.sh cmd/$p.c cmd/hwprobe.s > /dev/null; done
R=build/xenix/root
rm -rf $R
mkdir -p $R/bin $R/usr/bin $R/usr/lib/cc $R/usr/lib/mkuser $R/usr/include $R/usr/adm
# the commands are installed without symbol tables, as SCO's are
for p in cc make nm size strip clear man; do python3 tools/aout2xout.py --strip build/$p.aout $R/usr/bin/$p > /dev/null; done
cp build/xenix/bin/as86 build/xenix/bin/ld86 build/xenix/bin/ar86 $R/usr/bin/
cp build/xenix/bin/bcc-cpp build/xenix/bin/bcc-cc1 $R/usr/lib/cc/
python3 tools/aout2xout.py --strip build/aout2x.aout $R/usr/lib/cc/aout2x > /dev/null
for p in ls df date; do python3 tools/aout2xout.py --strip build/$p.aout $R/bin/$p > /dev/null; done
for p in l lc lf lr lx; do ln $R/bin/ls $R/bin/$p; done
for p in whereis xtop hostname cursor lastlogin uptime; do python3 tools/aout2xout.py --strip build/$p.aout $R/usr/bin/$p > /dev/null; done
mkdir -p $R/etc/default
cp skel/date.default $R/etc/default/date
python3 tools/aout2xout.py --strip build/asktime.aout $R/etc/asktime > /dev/null
ln $R/usr/bin/xtop $R/usr/bin/top
cp skel/cdprompt.csh skel/setprompt.csh $R/usr/lib/
: > $R/usr/adm/lastlog		# lastlogin's records: empty until the first login
: > $R/usr/lib/xtop.syms		# xtop's kept symbols: empty until it first runs
for f in cshrc login prof; do cp skel/mkuser.$f $R/usr/lib/mkuser/; done
cp build/xenix/bin/copt build/xenix/bin/rules.* $R/usr/lib/cc/
cp build/crt0.o build/libc.a build/libm.a build/libtermcap.a build/libcurses.a $R/usr/lib/cc/
cp include/*.h $R/usr/include/
# the manual, formatted for XENIX, which has no nroff
python3 tools/man.py $R/usr/man man/man1/*.1 man/man3/*.3
# include/sys holds stand-ins for cross-compiling; on XENIX, /usr/include/sys
# is SCO's own, from the Operating System disk N05 (see tools/mkset.py)
echo "XENIX tree in $R:"
(cd $R && find . -type f | sort | sed 's|^\.||')
# (the permissions, owners and links that custom gives them are in tools/mkset.py)

# the custom-installable set: soft.perms and the Development System disks
python3 tools/mkset.py

# for tests on the host: the tree plus the sys/ stand-ins that, on XENIX,
# SCO's disk N05 supplies
T=build/xenix/testroot
rm -rf $T
cp -r $R $T
mkdir -p $T/usr/include/sys
cp include/sys/*.h $T/usr/include/sys/
