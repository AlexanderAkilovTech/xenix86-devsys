"""Tests for ls (cmd/ls.c) on the simulator, against what XENIX's own ls
printed.

The tree is the one tests/m32/run.sh and tests/m33/run.sh made on XENIX
2.1.3, rebuilt here file for file: names, sizes, modes, owners, links, devices, and times
(TZ=PST8PDT, the clock at Saturday 10 October 1998, 4:35 a.m.). The
simulator is told the i-numbers and change times XENIX gave each file
(XSIM_STATMAP), reports directories 16 bytes an entry, as XENIX does, and
has ls read a passwd and a group file that name only root, as XENIX's named
neither 2 nor 999. tests/unit/ls_xenix.txt holds what XENIX's ls printed.

    python3 tests/unit/test_ls.py [-v]"""
import os, shutil, subprocess, sys, tempfile, datetime
from zoneinfo import ZoneInfo

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SIM = [sys.executable, f'{ROOT}/tools/xsim.py', f'{ROOT}/build/ls']
verbose = '-v' in sys.argv
LA = ZoneInfo('America/Los_Angeles')

def t(mo, d, h, mi, y=1998):
    return int(datetime.datetime(y, mo, d, h, mi, tzinfo=LA).timestamp())

tmp = tempfile.mkdtemp()
top = f'{tmp}/m32'
os.makedirs(f'{top}/t/sub/deep')
for d in ('empty', 'tdir', '.hdir'):
    os.mkdir(f'{top}/t/{d}')
# the run's own files beside t: /tmp/m32 had ten entries (192 bytes)
for f in ('ls', 'l', 'lc', 'lf', 'lr', 'lx', 'run.sh', 'cgatest', 'report'):
    open(f'{top}/{f}', 'w').write('x')
os.chdir(f'{top}/t')
open('a', 'w').write('hello\n')
os.link('a', 'link2')
open('medium', 'wb').write(bytes(36790))
open('huge', 'wb').write(bytes(440835))
open('prog', 'wb').write(bytes(8993))
for f in ('suid', 'suidnox', 'sgid', 'sgidnox', 'sticky', 'stickynox', 'old', 'future',
          'm5', 'm6', 'm7', 'abcdefghijklmn', '.hidden', 'sp ace', 'b\007l', 'sub/f', 'sub/deep/g'):
    open(f, 'w').write('x\n')
for f, m in (('prog', 0o755), ('suid', 0o4755), ('suidnox', 0o4644), ('sgid', 0o2755),
             ('sgidnox', 0o2644), ('sticky', 0o1755), ('stickynox', 0o1644), ('tdir', 0o1777)):
    os.chmod(f, m)
os.mkfifo('fifo')
os.mknod('cdev', 0o20644, os.makedev(3, 5))
os.mknod('bdev', 0o60644, os.makedev(1, 40))
os.chown('medium', 2, 2)
os.chown('huge', 999, 999)
# modification, access and change times, and XENIX's i-numbers
times = {   # name: (mtime, atime, ctime, i-number)
    'a': (t(10, 10, 0, 10), None, t(10, 10, 4, 27), 896),
    'medium': (t(10, 10, 0, 11), None, t(10, 10, 4, 27), 852),
    'huge': (t(10, 10, 0, 12), None, t(10, 10, 4, 27), 894),
    'prog': (t(10, 10, 0, 13), None, t(10, 10, 4, 27), 893),
    'suid': (t(10, 10, 0, 14), None, t(10, 10, 4, 27), 892),
    'suidnox': (t(10, 10, 0, 15), None, t(10, 10, 4, 27), 889),
    'sgid': (t(10, 10, 0, 16), None, t(10, 10, 4, 27), 888),
    'sgidnox': (t(10, 10, 0, 17), None, t(10, 10, 4, 27), 887),
    'sticky': (t(10, 10, 0, 18), None, t(10, 10, 4, 27), 885),
    'stickynox': (t(10, 10, 0, 19), None, t(10, 10, 4, 27), 890),
    'abcdefghijklmn': (t(10, 10, 0, 20), None, t(10, 10, 4, 27), 881),
    '.hidden': (t(10, 10, 0, 21), None, t(10, 10, 4, 27), 880),
    'sp ace': (t(10, 10, 0, 22), None, t(10, 10, 4, 27), 879),
    'b\007l': (t(10, 10, 0, 23), None, t(10, 10, 4, 27), 878),
    'fifo': (t(10, 10, 0, 24), None, t(10, 10, 4, 27), 870),
    'cdev': (t(10, 10, 0, 25), None, t(10, 10, 4, 27), 869),
    'bdev': (t(10, 10, 0, 26), None, t(10, 10, 4, 27), 850),
    'sub/f': (t(10, 10, 0, 27), None, t(10, 10, 4, 27), 872),
    'sub/deep/g': (t(10, 10, 0, 28), None, t(10, 10, 4, 27), 871),
    'sub/deep': (t(10, 10, 0, 29), None, t(10, 10, 4, 28), 876),
    'sub': (t(10, 10, 0, 30), t(10, 10, 4, 31) + 30, t(10, 10, 4, 28), 877),
    'empty': (t(10, 10, 0, 31), t(10, 10, 4, 31), t(10, 10, 4, 28), 875),
    'tdir': (t(10, 10, 0, 32), None, t(10, 10, 4, 28), 874),
    '.hdir': (t(10, 10, 0, 33), None, t(10, 10, 4, 28), 873),
    'old': (t(1, 1, 12, 0, 1985), None, t(10, 10, 4, 28), 886),
    'future': (t(1, 1, 12, 0, 1999), None, t(10, 10, 4, 28), 891),
    'm5': (t(5, 10, 12, 0), None, t(10, 10, 4, 28), 884),
    'm6': (t(4, 10, 12, 0), None, t(10, 10, 4, 28), 883),
    'm7': (t(3, 10, 12, 0), None, t(10, 10, 4, 28), 882),
    '.': (t(10, 10, 0, 5), None, t(10, 10, 4, 28), 868),
    '..': (t(10, 10, 4, 26), None, t(10, 10, 4, 26), 867),
}
smap = f'{tmp}/statmap'
with open(smap, 'w') as f:
    for name, (mt, at, ct, ino) in times.items():
        f.write(f'{os.stat(name).st_ino} {ino} {ct}\n')

def settimes():
    """reading a directory on the host can move its access time: set them all"""
    for name, (mt, at, ct, ino) in times.items():
        os.utime(f'{top}/t/{name}', (at or mt, mt))

open(f'{tmp}/passwd', 'w').write('root:x:0:0:Superuser:/:\n')
open(f'{tmp}/group', 'w').write('root::0:root\n')
env = dict(os.environ, XSIM_ENV='TZ=PST8PDT', XSIM_NOW=str(t(10, 10, 4, 35)), XSIM_STATMAP=smap,
           XSIM_MAP=f'/etc/passwd={tmp}/passwd,/etc/group={tmp}/group')
os.chdir(top)

# the cases XENIX printed
cases, cur = [], None
for line in open(f'{ROOT}/tests/unit/ls_xenix.txt', encoding='latin1').read().split('\n'):
    if line.startswith('# '):
        continue
    if line.startswith('=== '):
        cur = [line[4:], None, []]
        cases.append(cur)
    elif cur[1] is None:
        cur[1] = int(line.split()[1])
    else:
        cur[2].append(line)
count = fails = 0
for cmd, status, want in cases:
    while want and want[-1] == '':
        want.pop()
    settimes()
    name, argv = cmd.split()[0], cmd.split()[1:]
    prog = SIM
    if name != 'ls':                    # l, lc, lf, lr, lx: ls under another name
        os.makedirs(f'{tmp}/bin', exist_ok=True)
        shutil.copy(f'{ROOT}/build/ls', f'{tmp}/bin/{name}')
        prog = SIM[:2] + [f'{tmp}/bin/{name}']
    r = subprocess.run(prog + argv, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    got = r.stdout.decode('latin1').split('\n')
    while got and got[-1] == '':
        got.pop()
    count += 1
    if got != want or r.returncode != status:
        fails += 1
        print(f'FAIL {cmd} (status {r.returncode}, XENIX {status})')
        if verbose:
            import difflib
            for l in difflib.unified_diff(want, got, 'XENIX', 'ours', lineterm='', n=1):
                print('   ', repr(l)[1:-1])
# -h, which XENIX's ls has not: sizes in 5 characters, rounded
settimes()
r = subprocess.run(SIM + ['-lh', 't'], env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
got = r.stdout.decode('latin1').split('\n')
for want in ('total 503K', '-rw-r--r--   2 root     root          6B Oct 10 00:10 a',
             '-rw-r--r--   1 999      999         431K Oct 10 00:12 huge',
             '-rw-r--r--   1 2        2            36K Oct 10 00:11 medium',
             '-rwxr-xr-x   1 root     root        8.8K Oct 10 00:13 prog',
             'brw-r--r--   1 root     root       1, 40 Oct 10 00:26 bdev'):
    count += 1
    if want not in got:
        fails += 1
        print(f'FAIL ls -lh: no line {want!r}')
r = subprocess.run(SIM + ['-sh', 't/huge', 't/a'], env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
count += 1
if r.stdout.decode() != ' 1.0K t/a\n 436K t/huge\n':
    fails += 1
    print(f'FAIL ls -sh: {r.stdout.decode()!r}')
shutil.rmtree(tmp)
print(f'{count} checks, {fails} failures: ' + ('ALL PASS' if not fails else 'FAILED'))
sys.exit(1 if fails else 0)
