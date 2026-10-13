"""xprof.py - the instructions a program ran, by function, from a profile
written by the simulator (XSIM_PROFILE=file) and the program's symbols.

    XSIM_PROFILE=/tmp/p python3 tools/xsim.py build/top -b -n 1
    python3 tools/xprof.py build/top.aout /tmp/p [lines]"""
import bisect, subprocess, sys

aout, prof = sys.argv[1], sys.argv[2]
n = int(sys.argv[3]) if len(sys.argv) > 3 else 25
syms = []
for line in subprocess.run(['nm86', aout], stdout=subprocess.PIPE, text=True).stdout.split('\n'):
    f = line.split()
    if len(f) == 3 and f[1] in 'Tt':
        syms.append((int(f[0], 16), f[2]))
syms.sort()
addrs = [a for a, _ in syms]
by = {}
total = 0
for line in open(prof):
    a, c = line.split()
    a, c = int(a, 16), int(c)
    i = bisect.bisect_right(addrs, a) - 1
    name = syms[i][1] if i >= 0 else '?'
    by[name] = by.get(name, 0) + c
    total += c
print(f'{total} instructions')
for name, c in sorted(by.items(), key=lambda x: -x[1])[:n]:
    print(f'{c:10d} {100 * c / total:5.1f}%  {name}')
