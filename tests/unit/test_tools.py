"""Tests for the system tools: df (cmd/df.c) and whereis (cmd/whereis.c), on
the simulator.

df reads a file system image made here, with the numbers of the XENIX 2.1.3
system it was written for, and must print what XENIX's own df printed there,
to the character (custom reads the free blocks from it), but for -v, which
shows sizes in K and M where XENIX's counts blocks. whereis looks in a tree
made here.

    python3 tests/unit/test_tools.py"""
import os, shutil, struct, subprocess, sys, tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SIM = [sys.executable, f'{ROOT}/tools/xsim.py']
count = fails = 0

def check(what, want, got):
    global count, fails
    count += 1
    if want != got:
        fails += 1
        print(f'FAIL {what}\n  want {want!r}\n  got  {got!r}')

def run(prog, args, cwd):
    r = subprocess.run(SIM + [f'{ROOT}/build/{prog}'] + args, capture_output=True, cwd=cwd)
    return r.stdout.decode('latin1'), r.stderr.decode('latin1'), r.returncode

def image(path, isize, fsize, tfree, tinode, typ=2):
    """a XENIX file system: the super block at byte 1024, magic and type at its end"""
    sb = bytearray(1024)
    struct.pack_into('<H', sb, 0, isize)
    struct.pack_into('<l', sb, 2, fsize)
    struct.pack_into('<l', sb, 618, tfree)
    struct.pack_into('<H', sb, 622, tinode)
    struct.pack_into('<l', sb, 1016, 0x2b5544)
    struct.pack_into('<l', sb, 1020, typ)
    open(path, 'wb').write(bytes(1024) + sb + bytes(1024))

tmp = tempfile.mkdtemp()
# the root file system of the system this was written on: 18790 blocks of 1K,
# an i-list of 296, 11352 free, 3686 free i-nodes. Its name, devroot.i, is as
# long as /dev/root, which it stands for in what XENIX's df printed.
image(f'{tmp}/devroot.i', 296, 18790, 11352, 3686)
xenix = {   # XENIX's df, with the mount directory "/" blanked, as for an image
    '': '        (/dev/root ):    22704 blocks    3686 i-nodes\n',
    '-t': '        (/dev/root ):    22704 blocks    3686 i-nodes\n'
          '                     (   37580 total blocks,  592 for i-nodes)\n',
    '-f': '        (/dev/root ):    22704 blocks\n',
    '-i': 'Mount Dir   Filesystem  iused\tifree\t%iused\n'
          '            /dev/root \t 1018\t 3686\t   22%\n',
}
for opt, want in xenix.items():
    out, err, rc = run('df', ([opt] if opt else []) + ['devroot.i'], tmp)
    check(f'df {opt}: as XENIX prints it', [want, '', 0], [out.replace('devroot.i', '/dev/root'), err, rc])
image(f'{tmp}/devroot.i', 296, 18790, 11414, 3686)      # when XENIX's df -v was run
out, err, rc = run('df', ['-v', 'devroot.i'], tmp)
check("df -v: XENIX's table, in sizes (what custom's disk usage shows)",
      'Mount Dir   Filesystem    size\t  used\t  free\t% used\n'
      '            /dev/root      18M\t  7.2M\t   11M\t   39%\n', out.replace('devroot.i', '/dev/root'))
out, err, rc = run('df', ['-h', 'devroot.i'], tmp)
check('df -h', 'Filesystem     Size   Used  Avail Capacity  Mounted on\n'
               'devroot.i       18M   7.2M    11M      39%  \n', out)

# human sizes at the edges of the units, a 512-byte-block file system, and errors
for kb, want in ((0, '0K'), (1023, '1023K'), (1024, '1.0M'), (10239, '10M'), (10240, '10M'),
                 (1048576, '1.0G'), (20 * 1048576, '20G')):
    image(f'{tmp}/h.i', 10, kb, 0, 0)       # fsize in 1K blocks: Size is kb
    out, err, rc = run('df', ['-h', 'h.i'], tmp)
    check(f'df -h of {kb}K', want, out.split('\n')[1].split()[1] if out.count('\n') > 1 else out)
image(f'{tmp}/s.i', 100, 1000, 300, 50, typ=1)
out, err, rc = run('df', ['-t', 's.i'], tmp)
check('df of 512-byte blocks', '        (s.i       ):      300 blocks      50 i-nodes\n'
      '                     (    1000 total blocks,  100 for i-nodes)\n', out)
open(f'{tmp}/junk', 'wb').write(bytes(4096))
out, err, rc = run('df', ['nosuch'], tmp)
check('df of a missing name', ['df: nosuch: not found\n', 1], [err, rc])
out, err, rc = run('df', ['-q'], tmp)
check('df with a bad option', 2, rc)

# whereis, in a tree of its own
t = f'{tmp}/w'
for d in ('bin', 'ubin', 'man/cat1', 'man/cat3', 'man/man1', 'src/cmd/ls'):
    os.makedirs(f'{t}/{d}')
for f in ('bin/ls', 'ubin/ls', 'bin/cc'):
    open(f'{t}/{f}', 'w').write('x'); os.chmod(f'{t}/{f}', 0o755)
open(f'{t}/bin/data', 'w').write('x')                      # not executable
for f in ('man/cat1/ls.1', 'man/man1/ls.1', 'man/cat3/string.3', 'man/cat1/cc.1', 'src/cmd/cc.c'):
    open(f'{t}/{f}', 'w').write('x')
open(f'{t}/man/whatis', 'w').write('cc(1)\tcc - C compiler\n'
                                   'string(3)\tstrcat, strcpy, strlen - string operations\n')
dirs = ['-B', f'{t}/bin', f'{t}/ubin', '-M', f'{t}/man', '-S', f'{t}/src/cmd', '-f']
out, err, rc = run('whereis', dirs + ['ls', 'strcpy', '/usr/src/cc.c', 'data', 's.cc'], tmp)
check('whereis: programs, manuals, sources, names through whatis',
      f'ls: {t}/bin/ls {t}/ubin/ls {t}/man/cat1/ls.1 {t}/man/man1/ls.1 {t}/src/cmd/ls\n'
      f'strcpy: {t}/man/cat3/string.3\n'
      f'cc: {t}/bin/cc {t}/man/cat1/cc.1 {t}/src/cmd/cc.c\n'
      f'data:\n'
      f'cc: {t}/bin/cc {t}/man/cat1/cc.1 {t}/src/cmd/cc.c\n', out)
out, err, rc = run('whereis', ['-b'] + dirs + ['ls'], tmp)
check('whereis -b', f'ls: {t}/bin/ls {t}/ubin/ls\n', out)
out, err, rc = run('whereis', ['-m'] + dirs + ['ls'], tmp)
check('whereis -m', f'ls: {t}/man/cat1/ls.1 {t}/man/man1/ls.1\n', out)
out, err, rc = run('whereis', ['-u'] + dirs + ['ls', 'cc', 'strcpy'], tmp)
check('whereis -u: only names without one of each',
      f'ls: {t}/bin/ls {t}/ubin/ls {t}/man/cat1/ls.1 {t}/man/man1/ls.1 {t}/src/cmd/ls\n'
      f'strcpy: {t}/man/cat3/string.3\n', out)
out, err, rc = run('whereis', [], tmp)
check('whereis with no name', 2, rc)

shutil.rmtree(tmp)
print(f'{count} checks, {fails} failures: ' + ('ALL PASS' if not fails else 'FAILED'))
sys.exit(1 if fails else 0)
