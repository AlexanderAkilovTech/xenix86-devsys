"""Tests for the system tools: df (cmd/df.c), whereis (cmd/whereis.c),
hostname, cursor, lastlogin and date, on the simulator.

df reads a file system image made here, with the numbers of the XENIX 2.1.3
system it was written for, and must print what XENIX's own df printed there,
to the character (custom reads the free blocks from it), but for -v, which
shows sizes in K and M where XENIX's counts blocks. whereis looks in a tree
made here. hostname reads and writes a /etc/systemid made here; cursor's
writes to the display adapter's ports are logged (XSIM_PORTLOG) and the
console's answer to GIO_COLOR given (XSIM_COLOR); lastlogin reads a
/etc/utmp made here and keeps a /usr/adm/lastlog. date prints at a time
given (XSIM_NOW), sets the simulator's clock and writes a utmp and wtmp made
here, and is refused for a user other than root (XSIM_UID).

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

# hostname, cursor and lastlogin (M61): with XENIX's files made here, and
# the simulator's getppid() of 1 and getuid() of 0
def runenv(prog, args, env):
    r = subprocess.run(SIM + [f'{ROOT}/build/{prog}'] + args, capture_output=True, cwd=tmp,
                       env=dict(os.environ, **env))
    return r.stdout.decode('latin1'), r.stderr.decode('latin1'), r.returncode

sid = f'{tmp}/systemid'
hm = {'XSIM_MAP': f'/etc/systemid={sid}'}
open(sid, 'w').write('xenix86\n')
check('hostname: the first line of /etc/systemid', ('xenix86\n', 0), runenv('hostname', [], hm)[0::2])
open(sid, 'w').write('xenix86\nsite micnet\n')
out, err, rc = runenv('hostname', ['wyse'], hm)
check('hostname wyse: the first line set, the others kept', (0, 'wyse\nsite micnet\n'), (rc, open(sid).read()))
check('hostname reads it back', 'wyse\n', runenv('hostname', [], hm)[0])
for bad in ('toolongname', 'a.b', ''):
    out, err, rc = runenv('hostname', [bad], hm)
    check(f'hostname {bad!r}: not a name, nothing written', (1, 'wyse\nsite micnet\n'), (rc, open(sid).read()))
check('hostname -x: usage', 2, runenv('hostname', ['-x'], hm)[2])
os.remove(sid)
out, err, rc = runenv('hostname', [], hm)
check('hostname with no /etc/systemid', (1, 'hostname: cannot read /etc/systemid\n'), (rc, err))

plog = f'{tmp}/ports'
for color, arg, want in (('1', 'block', '3d4 a 3d5 0 3d4 b 3d5 7'), ('1', 'line', '3d4 a 3d5 6 3d4 b 3d5 7'),
                         ('0', 'block', '3b4 a 3b5 0 3b4 b 3b5 c'), ('0', 'line', '3b4 a 3b5 b 3b4 b 3b5 c')):
    open(plog, 'w').close()
    out, err, rc = runenv('cursor', [arg], {'XSIM_TTY': '0 1', 'XSIM_COLOR': color, 'XSIM_PORTLOG': plog})
    check(f'cursor {arg} on a {"color" if color == "1" else "monochrome"} adapter: the 6845\'s registers 10 and 11',
          (0, want), (rc, ' '.join(open(plog).read().split())))
open(plog, 'w').close()
out, err, rc = runenv('cursor', ['block'], {'XSIM_PORTLOG': plog})
check('cursor not on the console: nothing written', (1, 'cursor: only on the console\n', ''), (rc, err, open(plog).read()))
check('cursor with no argument: usage', 2, runenv('cursor', [], {})[2])

utmp, llog = f'{tmp}/utmp', f'{tmp}/lastlog'
lm = {'XSIM_MAP': f'/etc/utmp={utmp},/usr/adm/lastlog={llog}', 'XSIM_ENV': 'TZ=PST8PDT'}
def ut(user, line, pid, typ, t):
    return (user.encode().ljust(8, b'\0') + b'co\0\0' + line.encode().ljust(12, b'\0') +
            struct.pack('<hhhhl', pid, typ, 0, 0, t))
def utmpfile(*recs):
    open(utmp, 'wb').write(b''.join(recs))
t1, t2 = 908143812, 908230212        # Oct 11 and 12 1998 at 15:10:12 in PST8PDT: 22:10:12 GMT
open(llog, 'wb').close()
utmpfile(ut('LOGIN', 'tty03', 31, 6, t1), ut('alex', 'tty02', 1, 7, t1))
out, err, rc = runenv('lastlogin', [], lm)
check('lastlogin, the first login: nothing to say, and this one kept',
      ('', 0, 24, (0, b'\0' * 8, t1, b'tty02\0\0\0')), (out, rc, os.path.getsize(llog), struct.unpack('<l8sl8s', open(llog, 'rb').read())))
out, err, rc = runenv('lastlogin', [], lm)
check('lastlogin again in the same login: still nothing', '', out)
utmpfile(ut('alex', 'console', 1, 7, t2))
out, err, rc = runenv('lastlogin', [], lm)
check('lastlogin at the next login: the one before, as 4.2BSD said it',
      'Last login: Sun Oct 11 15:10:12 on tty02\n', out)
out, err, rc = runenv('lastlogin', [], lm)
check('and again in the same login: the same', 'Last login: Sun Oct 11 15:10:12 on tty02\n', out)
utmpfile(ut('alex', 'tty04', 77, 7, t2 + 600))
out, err, rc = runenv('lastlogin', [], lm)
check('no login shell above it: it only prints', ('Last login: Sun Oct 11 15:10:12 on tty02\n', t2),
      (out, struct.unpack('<l8sl8s', open(llog, 'rb').read())[2]))
os.remove(llog)
out, err, rc = runenv('lastlogin', [], lm)
check('no /usr/adm/lastlog: nothing printed, nothing made', ('', 0, False), (out, rc, os.path.exists(llog)))

# date (M63): printing at a set time (XSIM_NOW), and setting the clock: the
# simulator's stime keeps the time it is given, as the clock; utmp and wtmp
# are files made here
import datetime
from zoneinfo import ZoneInfo
la = ZoneInfo('America/Los_Angeles')
def secs(y, mo, d, h, mi):
    return int(datetime.datetime(y, mo, d, h, mi, tzinfo=la).timestamp())
wtmp = f'{tmp}/wtmp'
dm = {'XSIM_ENV': 'TZ=PST8PDT', 'XSIM_MAP': f'/etc/utmp={utmp},/etc/wtmp={wtmp}'}
def date(args, now, **env):
    return runenv('date', args, dict(dm, XSIM_NOW=str(now), **env))
t98 = 908143221                                  # Sun Oct 11 15:00:21 PDT 1998
check('date: as XENIX prints it', ('Sun Oct 11 15:00:21 PDT 1998\n', '', 0), date([], t98))
check('date +format: every letter',
      ('98 10 11 15:00:21 Sun Oct 284 0 10/11/98 15:00:21 03:00:21 PM 1998\n\t|\n', 0),
      date(['+%y %m %d %H:%M:%S %a %h %j %w %D %T %r %Y%n%t|'], t98)[0::2])
check('date in 2026: the day, %y and %j right, %Y', ('26 2026 10/13/26 286 Tue\n', 0),
      date(['+%y %Y %D %j %a'], secs(2026, 10, 13, 12, 0))[0::2])
check('date in 2106, the last second there is', 'Sat Feb  6 22:28:15 PST 2106\n', date([], 2**32 - 1)[0])
check('date +%q: not a format', ('', 'date: bad format character - q\n', 2), date(['+%q'], t98))
check('date +a%%b: a percent sign', 'a%b\n', date(['+a%%b'], t98)[0])
check('date +%y more: what follows the first argument is ignored, as by XENIX\'s', '98\n',
      date(['+%y', 'more'], t98)[0])
check('date -u: no time-of-year clock, as XENIX\'s says on an XT', ('', 'date: no TOY clock\n', 2), date(['-u'], t98))
check('date without TZ: XENIX\'s own zone, PST8PDT', 'Sun Oct 11 15:00:21 PDT 1998\n',
      runenv('date', [], {'XSIM_NOW': str(t98), 'XSIM_ENV': 'TZ='})[0])
for when, args in (((2026, 10, 13, 12, 0), ['101312002026']), ((2026, 10, 13, 12, 0), ['1013120026']),
                   ((1998, 10, 13, 12, 0), ['1013120098']), ((1998, 10, 13, 12, 0), ['10131200']),
                   ((2000, 2, 29, 12, 0), ['022912002000']), ((2069, 12, 31, 23, 59), ['1231235969']),
                   ((1970, 1, 1, 0, 0), ['0101000070']), ((2026, 3, 8, 3, 0), ['030803002026']),
                   ((2026, 1, 15, 8, 30), ['011508302026'])):
    open(utmp, 'wb').write(ut('alex', 'tty02', 77, 7, t98))
    open(wtmp, 'wb').close()
    t = secs(*when)
    out, err, rc = date(args, t98)
    rec = lambda typ, msg, x: bytes(12) + msg.ljust(12, b'\0') + struct.pack('<hhhhL', 0, typ, 0, 0, x)
    check(f'date {args[0]}: the clock set to {t}, printed, and "old time" and "new time" kept',
          (datetime.datetime.fromtimestamp(t, la).strftime('%a %b %e %H:%M:%S %Z %Y\n'), 0,
           ut('alex', 'tty02', 77, 7, t98) + rec(3, b'old time', t98) + rec(4, b'new time', t),
           rec(3, b'old time', t98) + rec(4, b'new time', t)),
          (out, rc, open(utmp, 'rb').read(), open(wtmp, 'rb').read()))
out, err, rc = date(['101312002026'], t98)
check('date again: utmp\'s entries replaced, wtmp\'s added to', (108, 144),
      (os.path.getsize(utmp), os.path.getsize(wtmp)))
for bad in ('1301120098', '0230120098', '0229120001', '1013250098', '1013126098', '101312', '1013120098x',
            '10131200981', '010100002107', '0208000021066', '020622292106', '123123591969', 'a'):
    open(wtmp, 'wb').close()
    check(f'date {bad}: not a time it can set, nothing written',
          ('', 'date: bad conversion\n', 2, 0), date([bad], t98)[0:3] + (os.path.getsize(wtmp),))
check('date 020622282106: the last minute there is', (0, 'Sat Feb  6 22:28:00 PST 2106\n'),
      date(['020622282106'], t98)[2::-2])
open(wtmp, 'wb').close()
out, err, rc = date(['1013120098'], t98, XSIM_UID='200')
check('date, not the super-user', ('', 'date: no permission\n', 2, 0), (out, err, rc, os.path.getsize(wtmp)))

shutil.rmtree(tmp)
print(f'{count} checks, {fails} failures: ' + ('ALL PASS' if not fails else 'FAILED'))
sys.exit(1 if fails else 0)
