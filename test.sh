#!/bin/sh
# test.sh - run the xenix86-devsys test suite on the host.
#  1. unit tests: 32-bit arithmetic and floating point helpers on an emulated 8086,
#     checked against Python
#  2. program tests: build test programs and run them under tools/xsim.py,
#     comparing output with expected results
# Real XENIX remains the final check; this catches regressions before a disk is made.
# Needs: bcc, as86, ld86, python3 with the "unicorn", "mpmath" and "numpy" packages.
set -e
cd "$(dirname "$0")"
pass=0; fail=0
ok()  { echo "PASS  $1"; pass=$((pass+1)); }
bad() { echo "FAIL  $1"; fail=$((fail+1)); }

python3 tests/unit/test_long.py >/tmp/xds_unit.out 2>&1 && ok "long arithmetic helpers ($(tail -1 /tmp/xds_unit.out))" || bad "long arithmetic helpers"

for t in m9 m11 m11b m12 xwc sigt ids jmp bigmem timet strt forkt sysprobe scant structt; do ./build.sh tests/$t.c >/dev/null; done

out=$(python3 tools/xsim.py build/m9 one two "three four") || true
echo "$out" | grep -q 'argv\[3\] = "three four"' && echo "$out" | grep -q 'octal 10, padded \[   42\]' \
	&& ok "m9: argv, environment, printf" || bad "m9"

out=$(python3 tools/xsim.py build/m11)
echo "$out" | grep -q 'a damaged 0, b damaged 0' && echo "$out" | grep -q "reused a's block" \
	&& echo "$out" | grep -q 'heap filled: 13 more 4K blocks' && ok "m11: sbrk/malloc/free" || bad "m11"

out=$(python3 tools/xsim.py build/m12 2>&1)
echo "$out" | grep -q '10! = 3628800' && echo "$out" | grep -q '17 words, 68 digits, hash 1219218365' \
	&& echo "$out" | grep -q 'at offset 168: the end.' && ok "m12: stdio, long math, sprintf" || bad "m12"

printf 'one two three\nfour five\n\nsix\n' > /tmp/xds_w1
want=$(wc < /tmp/xds_w1 | awk '{print $1, $2, $3}')
got=$(python3 tools/xsim.py build/xwc < /tmp/xds_w1 | awk '{print $1, $2, $3}')
[ "$want" = "$got" ] && ok "xwc matches host wc ($got)" || bad "xwc: got '$got', want '$want'"

python3 tools/xsim.py build/sigt >/tmp/xds_sig.out 2>&1 && grep -q 'caught 14, extra 1235, register var still 777' /tmp/xds_sig.out \
	&& ok "sigt: signal trampolines (simulated delivery)" || bad "sigt"

out=$(python3 tools/xsim.py build/ids)
echo "$out" | grep -q 'full 32 bits' && echo "$out" | grep -q 'pid 1234, ppid 1' && echo "$out" | grep -q 'euid 0, gid 0, egid 0' \
	&& ok "ids: time high word, getppid/geteuid/getegid from BX" || bad "ids"

out=$(python3 tools/xsim.py build/jmp)
echo "$out" | grep -q 'depth 40: setjmp returned 42, pass 2, register var 777' && echo "$out" | grep -q 'return 1$' \
	&& echo "$out" | grep -q 'handler, setjmp returned 14' && ok "jmp: setjmp/longjmp, including out of a signal handler" || bad "jmp"

python3 tools/xsim.py build/m11b > /tmp/xds_m11b.out && ok "m11b: realloc shrinks in place ($(head -1 /tmp/xds_m11b.out | cut -d'(' -f1| sed 's/ *$//'))" || bad "m11b"

python3 tools/xsim.py build/strt > /tmp/xds_strt.out && ok "strt: assembly string routines vs C ($(cat /tmp/xds_strt.out))" || bad "strt"

python3 tools/xsim.py build/forkt build/tiny > /tmp/xds_fork.out 2>&1 && grep -q 'exit code 7' /tmp/xds_fork.out \
	&& grep -q "exec'd build/tiny: exit code 3" /tmp/xds_fork.out && ok "forkt: fork, exec, wait, pipe (real host processes)" || bad "forkt"

python3 tools/xsim.py build/sysprobe > /tmp/xds_probe.out 2>&1 && ok "sysprobe: open flags and struct stat" || bad "sysprobe"

python3 tools/xsim.py build/scant > /tmp/xds_scan.out 2>&1 && ok "scant: scanf family, strerror, perror" || bad "scant"

python3 tools/xsim.py build/structt > /tmp/xds_struct.out 2>&1 && ok "structt: structs by value, assigned, returned (patch 05)" || bad "structt"

python3 tools/xsim.py build/bigmem > /tmp/xds_big.out && grep -q 'lies above it' /tmp/xds_big.out \
	&& ok "bigmem: malloc of 32K and more ($(cut -d';' -f1 /tmp/xds_big.out))" || bad "bigmem"

./build.sh tests/fptest.c tests/fptests.s > /dev/null
python3 tests/unit/test_fp.py 1000 > /tmp/xds_fp.out 2>&1 && ok "floating point vs Python ($(tail -1 /tmp/xds_fp.out))" \
	|| { bad "floating point"; head -20 /tmp/xds_fp.out; }

./build.sh tests/fmtest.c > /dev/null
python3 tests/unit/test_fmt.py 400 > /tmp/xds_fmt.out 2>&1 && ok "printf, strtod, scanf, ecvt with floats vs Python ($(tail -1 /tmp/xds_fmt.out))" \
	|| { bad "float conversions"; head -20 /tmp/xds_fmt.out; }
python3 tests/unit/test_fgets.py > /tmp/xds_fgets.out 2>&1 && ok "fgets vs a model of it ($(tail -1 /tmp/xds_fgets.out))" \
	|| { bad "fgets"; head -20 /tmp/xds_fgets.out; }
python3 tests/unit/test_printf.py > /tmp/xds_printf.out 2>&1 && ok "printf with integers and strings vs the host's ($(tail -1 /tmp/xds_printf.out))" \
	|| { bad "printf"; head -20 /tmp/xds_printf.out; }

./build.sh tests/mtest.c > /dev/null
python3 tests/unit/test_m.py 300 > /tmp/xds_m.out 2>&1 && ok "math library vs mpmath ($(tail -1 /tmp/xds_m.out))" \
	|| { bad "math library"; head -20 /tmp/xds_m.out; }

python3 tests/unit/test_nm.py > /tmp/xds_nm.out 2>&1 && ok "nm, size, strip vs Python ($(tail -1 /tmp/xds_nm.out))" \
	|| { bad "nm, size, strip"; head -20 /tmp/xds_nm.out; }

python3 tests/unit/test_termcap.py > /tmp/xds_tc.out 2>&1 && ok "termcap library vs Python ($(tail -1 /tmp/xds_tc.out))" \
	|| { bad "termcap library"; head -20 /tmp/xds_tc.out; }

./build.sh tests/curt.c > /dev/null
if python3 -c 'import pyte' 2>/dev/null; then
	python3 tests/unit/test_curses.py > /tmp/xds_curses.out 2>&1 && ok "curses library through a terminal emulator ($(tail -1 /tmp/xds_curses.out))" \
		|| { bad "curses library"; head -20 /tmp/xds_curses.out; }
else
	echo "SKIP  curses: needs the Python package pyte (pip install pyte)"
fi

./build.sh cmd/man.c > /dev/null
python3 tests/unit/test_man.py > /tmp/xds_man.out 2>&1 && ok "man and its page formatter ($(tail -1 /tmp/xds_man.out))" \
	|| { bad "man"; head -20 /tmp/xds_man.out; }

for p in df whereis ls; do ./build.sh cmd/$p.c > /dev/null; done
./build.sh cmd/xtop.c cmd/hwprobe.s > /dev/null
python3 tests/unit/test_tools.py > /tmp/xds_tools.out 2>&1 && ok "df, whereis, hostname, cursor, lastlogin and date ($(tail -1 /tmp/xds_tools.out))" \
	|| { bad "df, whereis, hostname, cursor, lastlogin and date"; head -20 /tmp/xds_tools.out; }
if [ "$(id -u)" = 0 ]; then
	python3 tests/unit/test_ls.py > /tmp/xds_ls.out 2>&1 && ok "ls against XENIX's ($(tail -1 /tmp/xds_ls.out))" \
		|| { bad "ls"; head -20 /tmp/xds_ls.out; }
else
	echo "SKIP  ls: rebuilding XENIX's test tree needs root (device files, owners)"
fi
if python3 -c "import pyte" 2>/dev/null; then
	python3 tests/unit/test_xtop.py > /tmp/xds_xtop.out 2>&1 && ok "xtop against a made-up kernel ($(tail -1 /tmp/xds_xtop.out))" \
		|| { bad "xtop"; head -20 /tmp/xds_xtop.out; }
else
	echo "SKIP  xtop: needs the Python package pyte (pip install pyte)"
fi

python3 tests/unit/test_time.py > /tmp/xds_time.out 2>&1 && ok "time conversions vs Python ($(tail -1 /tmp/xds_time.out))" || bad "time conversions"

for t in echo true false; do ./build.sh tests/bin/$t.c > /dev/null; done
./build.sh cmd/make.c > /dev/null
if [ -x build/xenix/testroot/usr/bin/cc ]; then
	python3 tests/unit/test_make.py > /tmp/xds_make.out 2>&1 && ok "make ($(tail -1 /tmp/xds_make.out))" || { bad "make"; cat /tmp/xds_make.out; }
else
	echo "SKIP  make: needs the native cc (run ./dist.sh)"
fi

echo "$pass passed, $fail failed"
[ $fail -eq 0 ]
