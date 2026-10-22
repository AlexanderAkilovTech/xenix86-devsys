"""Tests for xtop (cmd/xtop.c) on the simulator, against a made-up kernel.

xtop reads /xenix's symbol table and the kernel's memory through /dev/kmem,
/dev/mem and /dev/swap, and the file systems in /etc/mnttab. Here those are
files made to the layouts measured on XENIX 2.1.3 (ABI.md): an x.out with
the symbols xtop looks up among 1500 others, a proc table of 48-byte slots,
the core and swap maps, u areas holding each process's ticks and name, in
memory or on swap, and two file systems' super blocks. The simulator opens
them in place of the real ones (XSIM_MAP); xtop -b prints its screen, and
on a terminal its output is played through a terminal emulator (pyte).

    python3 tests/unit/test_xtop.py"""
import os, struct, subprocess, sys, tempfile, shutil

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SIM = [sys.executable, f'{ROOT}/tools/xsim.py', f'{ROOT}/build/xtop']
count = fails = 0

def check(what, cond, info=''):
    global count, fails
    count += 1
    if not cond:
        fails += 1
        print(f'FAIL {what} {info}')

tmp = tempfile.mkdtemp()
SYMS = {'_proc': 0x5c3a, '_v': 0x16e2, '_coremap': 0x16a6, '_swapmap': 0x16ac, '_nswap': 0x165c,
        '_swplo': 0x1658, '_maxmem': 0x3dd4, '_bootime': 0xb0dc, '_time': 0x4df6, '_lbolt': 0xb940,
        '_u': 0x36da}

# /xenix: a header, a segment table with one symbol segment, the symbols:
# the ones xtop wants among 1500 others, some of them nearly the same, so
# that records cross the 1K pieces xtop reads them in
import random
random.seed(5)
others = [('_' if random.random() < 0.7 else '') + ''.join(random.choice('abcdefghijklmnopqrstuvwxyz_0123456789')
          for _ in range(random.randint(1, 30))) for _ in range(1500)]
others += ['_procs', '_v_', '_timer', '_proc_', '_maxmemory', '_lbolt2', '_tim']
random.shuffle(others)
names = [(n, 0x7777) for n in others if n not in SYMS]
for i, item in enumerate(SYMS.items()):
    names.insert(150 * i + 37, item)
recs = b''.join(struct.pack('<HHl', 0, 0, v) + n.encode() + b'\0' for n, v in names)
hdr = bytearray(64)
struct.pack_into('<H', hdr, 0, 0x206)
struct.pack_into('<l', hdr, 52, 64)          # the segment table, at 64
struct.pack_into('<l', hdr, 56, 32)          # one entry
seg = bytearray(32)
struct.pack_into('<H', seg, 0, 3)            # the symbols
struct.pack_into('<l', seg, 8, 96)
struct.pack_into('<l', seg, 12, len(recs))
open(f'{tmp}/xenix', 'wb').write(bytes(hdr) + bytes(seg) + recs)

kmem = bytearray(65536)
mem = bytearray(655360)
swap = bytearray(1600 * 1024)
def put(buf, at, fmt, *v): struct.pack_into(fmt, buf, at, *v)

put(kmem, SYMS['_v'] + 24, '<H', 40)                     # v_proc
put(kmem, SYMS['_maxmem'], '<H', 25536)                   # 399K for processes
put(kmem, SYMS['_nswap'], '<H', 1599)
put(kmem, SYMS['_swplo'], '<l', -1)
boot = 907000000
put(kmem, SYMS['_bootime'], '<l', boot)
put(kmem, SYMS['_time'], '<l', boot + 3 * 3600 + 25 * 60)  # up 3:25
LB = (3 * 3600 + 25 * 60) * 20                            # the ticks since the start, 20 a second
put(kmem, SYMS['_lbolt'], '<l', LB)
# the maps: free core 384 + 5888 clicks (98K), free swap 5 + 1316 blocks
for a, ents, at in ((SYMS['_coremap'], [(384, 16704), (5888, 19648)], 0xb00a),
                    (SYMS['_swapmap'], [(5, 1), (1316, 284)], 0x4a2e)):
    put(kmem, a, '<HHH', 0, 20, at)
    for i, (sz, ad) in enumerate(ents):
        put(kmem, at + i * 4, '<HH', sz, ad)
# the processes: (pid, ppid, uid, stat, in memory, addr, size in clicks,
# pri, utime, stime, name)
# and its terminal: none, the console (0000) or tty02 (0001), as on XENIX
procs = [(0, 0, 0, 1, 1, 0x29c0, 128, 0, 0, 11, '', None),
         (1, 0, 0, 1, 0, 0x7b, 960, 30, 2, 33, 'init', None),
         (29, 1, 0, 1, 1, 0x74c0, 1728, 30, 77, 80, 'sh', 0x0000),
         (21, 1, 14, 1, 0, 0x9a, 1664, 26, 5, 15, 'lpsched', None),
         (88, 29, 0, 3, 1, 0x3c40, 576, 61, 122, 312, 'kprobe', 0x0001),
         (90, 29, 3, 5, 0, 0, 0, 0, 0, 0, '', None),
         (1234, 29, 0, 1, 1, 0x7000, 16, 50, 100, 50, 'xtop', 0x0000)]   # xtop itself: the simulator's pid
for slot, (pid, ppid, uid, stat, inmem, addr, size, pri, ut, st, name, tty) in enumerate(procs):
    s = SYMS['_proc'] + slot * 48
    put(kmem, s, '<BBbBBb', stat, 1 if inmem else 0, pri, 0, 0, 20)
    put(kmem, s + 10, '<H', uid)
    put(kmem, s + 18, '<HHH', pid, ppid, addr)
    put(kmem, s + 44, '<H', size)
    u = addr * 16 if inmem else (addr - 1) * 1024
    target = mem if inmem else swap
    if stat != 5:
        put(target, u + 1296, '<ll', ut, st)
        target[u + 1388:u + 1388 + len(name)] = name.encode()
        if tty is not None:
            put(target, u + 1336, '<HH', 0x8a40, tty)     # u_ttyp (any non-zero), u_ttyd
# the kernel's _u: the running process's u area as it is now, which the
# copy in the process's memory catches up with only when another process
# runs: xtop's own times, 160 and 70 ticks to the 100 and 50 in memory
put(kmem, SYMS['_u'] + 1296, '<ll', 160, 70)
kmem[SYMS['_u'] + 1388:SYMS['_u'] + 1392] = b'xtop'
for n, b in (('kmem', kmem), ('mem', mem), ('swap', swap)):
    open(f'{tmp}/{n}', 'wb').write(bytes(b))
open(f'{tmp}/termcap', 'w').write(
    'ansi|a test terminal:co#80:li#25:cm=\\E[%i%d;%dH:cl=\\E[2J\\E[H:ce=\\E[K:so=\\E[7m:se=\\E[m:\n'
    'mono|a terminal without color:co#80:li#24:cm=\\E[%i%d;%dH:cl=\\E[2J\\E[H:ce=\\E[K:so=\\E[7m:se=\\E[m:\n'
    'dumb|no reverse video either:co#80:li#24:cm=\\E[%i%d;%dH:cl=\\E[2J\\E[H:ce=\\E[K:\n')
# two file systems: their super blocks (s_isize at 0, s_fsize at 2,
# s_tfree at 618, the magic number at 1016, the type at 1020: 2 for blocks
# of 1024 bytes) and /etc/mnttab's records of 150 bytes
def fsimage(path, fsize, tfree, btype):
    sb = bytearray(1024)
    struct.pack_into('<Hl', sb, 0, 100, fsize)
    struct.pack_into('<l', sb, 618, tfree)
    struct.pack_into('<ll', sb, 1016, 0x2b5544, btype)
    open(path, 'wb').write(bytes(1024) + bytes(sb))
fsimage(f'{tmp}/root', 18000, 11000, 2)     # 18000K, 11000K free: 6.8M of 18M in use
fsimage(f'{tmp}/u', 4000, 3500, 1)          # 512-byte blocks: 2000K, 1750K free
def mnt(dev, d):
    return dev.encode().ljust(15, b'\0') + d.encode().ljust(129, b'\0') + bytes(6)
open(f'{tmp}/mnttab', 'wb').write(mnt('root', '/') + mnt('u', '/u'))
open(f'{tmp}/xtoprc', 'w').write('')
open(f'{tmp}/passwd', 'w').write('root:x:0:0:Super:/:/bin/sh\nbin:x:3:3::/:\nlp:x:14:3::/:\n')
# /dev's terminals, as character devices (making them needs root), with
# XENIX's numbers: the console, tty02 (the second screen), tty1a (the first
# serial line) and tty11, its other name, and tty, which is no terminal of
# its own; a file named tty03 that is not a device
os.makedirs(f'{tmp}/dev')
try:
    for n, (ma, mi) in (('console', (0, 0)), ('tty', (3, 0)), ('tty02', (0, 1)), ('tty1a', (5, 0)), ('tty11', (5, 0))):
        os.mknod(f'{tmp}/dev/{n}', 0o20620, os.makedev(ma, mi))
    open(f'{tmp}/dev/tty03', 'w').write('')
    DEVS = True
except PermissionError:
    DEVS = False

env = dict(os.environ, XSIM_MAP=','.join(f'{a}={tmp}/{b}' for a, b in (
    ('/xenix', 'xenix'), ('/dev/kmem', 'kmem'), ('/dev/mem', 'mem'), ('/dev/swap', 'swap'),
    ('/etc/passwd', 'passwd'), ('/etc/termcap', 'termcap'), ('/etc/mnttab', 'mnttab'),
    ('/dev/root', 'root'), ('/dev/u', 'u'), ('/.xtoprc', 'xtoprc'), ('/dev', 'dev'), ('/dev/', 'dev/'),
    ('/usr/lib/xtop.syms', 'xtop.syms'))))
r = subprocess.run(SIM + ['-b', '-n', '1', '-d', '1'], env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
out = r.stdout.decode('latin1')
lines = out.split('\n')
check('xtop -b runs', r.returncode == 0, repr(out[-300:]))
check('the CPU meter', lines[0].startswith(' CPU[') and '0.0%]' in lines[0], repr(lines[0]))
check('the process count', 'Processes: 7, 1 running' in lines[0], repr(lines[0]))
# memory: of the 640K /dev/mem reaches, 25536 clicks for processes, so 15424
# for the kernel; 19264 clicks given out, 2432 of them to the processes in
# memory: 542K in use
check('the memory meter', lines[1] == ' Mem[|||||||||||||||||542K/640K]    Uptime: 03:25:00', repr(lines[1]))
# swap: 1599 - 1321 = 278K of 1599K; the simulator's x87 says 037F after fninit
check('the swap meter, the processes swapped out (no time since a last look)',
      lines[2] == ' Swp[||||             278K/1.6M]    Swapped out: 2', repr(lines[2]))
check('the root file system', lines[3] == ' Dsk[||||||||||        6.8M/18M]    /: 11M free', repr(lines[3]))
check('a file system of 512-byte blocks', lines[4] == ' Dsk[|||              250K/2.0M]    /u: 1.7M free', repr(lines[4]))
check('the FPU under the summaries', lines[5] == ' ' * 36 + 'FPU: x87', repr(lines[5]))
check('a blank line, then the header, the order at its end', lines[6] == '' and
      lines[7].startswith('  PID USER     TTY PRI  NI  SIZE   RES S  CPU%    TIME+ COMMAND') and
      lines[7].endswith('Sort: CPU ') and len(lines[7]) == 79, repr(lines[6:8]))
check('the keys', lines[24] == 'h:Help  P:CPU  M:Mem  T:Time  N:PID  k:Kill  d:Delay  c:Theme  q:Quit', repr(lines[24]))
rows = {l.split()[0]: l for l in lines[8:24] if l.strip() and l.split()[0].isdigit()}
CO, T2 = ('co', '02') if DEVS else ('?', '?')
check('a process in memory, on the console', rows.get('29', '').split() ==
      ['29', 'root', CO, '30', '0', '27K', '27K', 'S', '0.0', '0:07.85', 'sh'], repr(rows.get('29')))
check('a process on swap: its name and time from /dev/swap, RES 0, no terminal', rows.get('21', '').split() ==
      ['21', 'lp', '?', '26', '0', '26K', '0', 'S', '0.0', '0:01.00', 'lpsched'], repr(rows.get('21')))
check('a process on tty02', rows.get('88', '').split()[2] == T2, repr(rows.get('88')))
check('a running process', rows.get('88', '').split()[7:9] == ['R', '0.0'] and
      rows['88'].split()[-1] == 'kprobe' and rows['88'].split()[-2] == '0:21.70', repr(rows.get('88')))
check('the swapper', rows.get('0', '').split()[-1] == 'swapper', repr(rows.get('0')))
check("xtop's own times from the kernel's _u, not the older copy in memory", rows.get('1234', '').split()[-2:] ==
      ['0:11.50', 'xtop'], repr(rows.get('1234')))
with open(f'{tmp}/kmem', 'r+b') as f:      # a _u that is some other process's: the memory's copy
    f.seek(SYMS['_u'] + 1388)
    f.write(b'cron')
r = subprocess.run(SIM + ['-b', '-n', '1', '-d', '1'], env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
row = [l for l in r.stdout.decode('latin1').split('\n') if l.split()[:1] == ['1234']]
check("a _u not xtop's own is not taken for it", row and row[0].split()[-2:] == ['0:07.50', 'xtop'], repr(row))
with open(f'{tmp}/kmem', 'r+b') as f:
    f.seek(SYMS['_u'] + 1388)
    f.write(b'xtop')
# the terminals' devices looked up only as far as /dev's order needs: the
# console and tty02 for the processes here, not tty1a, tty11 or tty03; and
# a terminal not in /dev looked for through all of them, and shown as ?
def stats(extra={}):
    log = f'{tmp}/stat.log'
    open(log, 'w').close()
    r = subprocess.run(SIM + ['-b', '-n', '1', '-d', '1'], env=dict(env, XSIM_STATLOG=log, **extra),
                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    return r.stdout.decode('latin1'), [l for l in open(log).read().split() if l.startswith(('/dev/tty', '/dev/console'))]
if DEVS:
    o, looked = stats()
    check("terminals looked up only as far as needed", looked == ['/dev/console', '/dev/tty02'], repr(looked))
    with open(f'{tmp}/mem', 'r+b') as f:      # sh on a terminal /dev does not have
        f.seek(0x74c0 * 16 + 1338)
        f.write(struct.pack('<H', 0x0009))
    o, looked = stats()
    row = [l for l in o.split('\n') if l.split()[:1] == ['29']]
    check("a terminal not in /dev: all looked at once, and ?", sorted(looked) ==
          ['/dev/console', '/dev/tty02', '/dev/tty03', '/dev/tty11', '/dev/tty1a'] and row and row[0].split()[2] == '?',
          repr((looked, row)))
    with open(f'{tmp}/mem', 'r+b') as f:
        f.seek(0x74c0 * 16 + 1338)
        f.write(struct.pack('<H', 0x0000))
check('a zombie', rows.get('90', '').split()[7] == 'Z' and rows['90'].split()[-1] == '<defunct>',
      repr(rows.get('90')))
check('the owner by number when /etc/passwd has no name', rows.get('90', '').split()[1] == 'bin',
      repr(rows.get('90')))
# three screens: the lines kept from one screen to the next (xtop makes a
# process's line again only when it changes, and does not read the u area
# of a process still swapped out where it was) come out the same
r = subprocess.run(SIM + ['-b', '-n', '3', '-d', '1'], env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
out3 = r.stdout.decode('latin1').split('\n')
screens = [out3[i * 26 + 1:i * 26 + 25] for i in range(3)]     # 25 lines and a blank each, but the CPU line
check('three screens alike', screens[0] == screens[1] == screens[2] and 'lpsched' in screens[2][9], repr(screens))
# the console's display, when the program's terminal is the console
# (the simulator's console answers 0 to GIO_COLOR: a monochrome display,
# whose status port shows no retrace, or with XSIM_VSYNC one that comes and
# goes, as a Hercules card's)
r = subprocess.run(SIM + ['-b', '-n', '1', '-d', '1'], env=dict(env, XSIM_TTY='0'), stdout=subprocess.PIPE)
check('the display: an MDA', r.stdout.decode('latin1').split('\n')[5].endswith(
      'FPU: x87   Video: MDA 80x25'), repr(r.stdout[:300]))
r = subprocess.run(SIM + ['-b', '-n', '1', '-d', '1'], env=dict(env, XSIM_TTY='0', XSIM_VSYNC='1'), stdout=subprocess.PIPE)
check('the display: a Hercules card', r.stdout.decode('latin1').split('\n')[5].endswith(
      'FPU: x87   Video: Hercules 80x25'), repr(r.stdout[:300]))

# On the console (TERM=ansi), played through a terminal emulator. xtop
# draws only the characters that change, so what counts is the screen that
# results: its text, its colors, and how little is sent once it is up.
import pyte, threading, time, re

def play(data, cols=80, rows=25):
    sc = pyte.Screen(cols, rows)
    pyte.Stream(sc).feed(data)
    return sc

def text(sc):
    return [sc.display[i].rstrip() for i in range(sc.lines)]

def run(args, rc='', term='ansi', rows=25):
    open(f'{tmp}/xtoprc', 'w').write(rc)
    r = subprocess.run(SIM + args, env=dict(env, XSIM_ENV=f'TERM={term}'), stdin=subprocess.DEVNULL,
                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    o = r.stdout.decode('latin1')
    return r, o, play(o[:o.rfind('\033[2J')], rows=rows)    # (xtop clears the screen as it ends)

r, scr, sc = run(['-n', '1', '-d', '1'])
check('xtop on a terminal runs', r.returncode == 0, repr(scr[-300:]))
check('the screen cleared first, in white on black', scr.startswith('\033[0m\033[37m\033[40m\033[2J\033[H'),
      repr(scr[:30]))
batch = [l.rstrip() for l in lines[:25]]
check('Color: the screen as the batch output', text(sc) == batch, '\n'.join(text(sc)) + '\n----\n' + '\n'.join(batch))
def attrs(sc, row, c0, c1):
    return [(sc.buffer[row][c].fg, sc.buffer[row][c].bg, sc.buffer[row][c].bold, sc.buffer[row][c].reverse)
            for c in range(c0, c1)]
G, Y, B, R, W = ('green', 'default', True, False), ('brown', 'default', True, False), \
    ('blue', 'default', True, False), ('red', 'default', True, False), ('white', 'default', False, False)
def norm(a):            # pyte's black background is "black" or "default": the same here
    return [(f, 'default' if b in ('black', 'default') else b, bo, rv) for f, b, bo, rv in a]
check('the memory meter in color: processes, shared text, kernel',
      norm(attrs(sc, 1, 5, 31)) == [G] + [Y] * 11 + [B] * 10 + [W] * 4, repr(norm(attrs(sc, 1, 5, 31))))
check('the swap meter in color', norm(attrs(sc, 2, 5, 9)) == [R] * 4 and norm(attrs(sc, 2, 22, 31)) == [W] * 9,
      repr(norm(attrs(sc, 2, 5, 31))))
check('the disk meter in color', norm(attrs(sc, 3, 5, 15)) == [B] * 10 and sc.display[3][15:23] == ' ' * 8, repr(norm(attrs(sc, 3, 5, 31))))
check('the heading, black on green', norm(attrs(sc, 7, 0, 79)) == [('black', 'green', False, False)] * 79,
      repr(set(norm(attrs(sc, 7, 0, 79)))))
check("the keys' letters bright cyan, the words white", norm(attrs(sc, 24, 0, 1)) == [('cyan', 'default', True, False)]
      and norm(attrs(sc, 24, 1, 6)) == [W] * 5 and norm(attrs(sc, 24, 8, 9)) == [('cyan', 'default', True, False)],
      repr(norm(attrs(sc, 24, 0, 9))))
check('the cursor waits past the keys, not on them', (sc.cursor.y, sc.cursor.x) == (24, 71),
      repr((sc.cursor.y, sc.cursor.x)))
r3 = subprocess.run(SIM + ['-n', '3', '-d', '1'], env=env, stdin=subprocess.DEVNULL, stdout=subprocess.PIPE,
                    stderr=subprocess.STDOUT).stdout
check('nothing sent for a screen with nothing changed', len(r3) == len(r.stdout), f'{len(r3)} {len(r.stdout)}')

# kill from another terminal (M73): xtop catches SIGTERM, kill's default, and
# leaves as q does, the screen cleared in plain white; a signal it does not
# catch (16 here) ends it on the spot with the screen left as it was, which is
# what SIGTERM did before M73 (seen on XENIX: the last frame and the shell's
# "Terminated" over it, the tty still raw)
def killed(sig):
    k = subprocess.run(SIM + ['-n', '50', '-d', '1'], env=dict(env, XSIM_ENV='TERM=ansi', XSIM_KILL=str(sig)),
                       stdin=subprocess.DEVNULL, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    return k.returncode, k.stdout.decode('latin1')
kc, ko = killed(15)
check('kill (SIGTERM): xtop ends as with q', kc == 0, repr((kc, ko[-120:])))
check('kill (SIGTERM): the screen cleared, plain, at the end',
      ko.endswith('\033[2J\033[H') and ko.count('\033[2J') >= 2, repr(ko[-60:]))
kc, ko = killed(16)
check('a signal not caught still kills it, the screen left as it was',
      kc == 128 + 16 and ko.count('\033[2J') == 1, repr((kc, ko.count('\033[2J'))))

# Monochrome: chosen with -m, or kept in $HOME/.xtoprc, or the first choice
# on a monochrome display (an MDA or a Hercules card); no colors, the
# heading in reverse video, the meters and the keys' letters bright
for how, args, rc, ex in (('-m', ['-m'], '', {}), ('.xtoprc', [], 'theme mono\n', {}),
                          ('a monochrome display', [], '', {'XSIM_TTY': '0'})):
    open(f'{tmp}/xtoprc', 'w').write(rc)
    r = subprocess.run(SIM + args + ['-n', '1', '-d', '1'], env=dict(env, XSIM_ENV='TERM=ansi', **ex),
                       stdin=subprocess.DEVNULL, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    mo = r.stdout.decode('latin1')
    msc = play(mo[:mo.rfind('\033[2J')])
    check(f'Monochrome ({how}): no colors', not re.search('\033\\[[34][0-9]m', mo), repr(mo[:200]))
    want = [l.rstrip() for l in lines[:25]] if not ex else \
        [l.rstrip() for l in subprocess.run(SIM + ['-b', '-n', '1', '-d', '1'], env=dict(env, **ex),
                                            stdout=subprocess.PIPE).stdout.decode('latin1').split('\n')[:25]]
    check(f'Monochrome ({how}): the text as the batch output', text(msc) == want, '\n'.join(text(msc)))
    check(f'Monochrome ({how}): the heading in reverse video',
          all(a[3] for a in attrs(msc, 7, 0, 79)) and not any(a[3] for a in attrs(msc, 8, 0, 79)),
          repr(attrs(msc, 7, 0, 3)))
    check(f'Monochrome ({how}): the bars bright, plain |',
          all(a[2] for a in attrs(msc, 1, 5, 22)) and msc.display[1][5:22] == '|' * 17, repr(attrs(msc, 1, 5, 9)))
    check(f"Monochrome ({how}): the keys' letters bright, not reversed; the words plain",
          attrs(msc, 24, 0, 1)[0][2:] == (True, False) and not any(a[2] or a[3] for a in attrs(msc, 24, 1, 6)),
          repr(attrs(msc, 24, 0, 6)))
r, mo, msc = run(['-c', '-n', '1', '-d', '1'], rc='theme mono\n')
check('-c over .xtoprc', '\033[32m' in mo, repr(mo[:100]))

# a terminal other than the console: Monochrome only, 24 lines
r, mo, msc = run(['-n', '1', '-d', '1'], term='mono', rows=24)
check('a terminal without color: no colors, reverse video', not re.search('\033\\[[34][0-9]m', mo) and
      all(a[3] for a in attrs(msc, 7, 0, 79)), repr(mo[:200]))
check('a terminal without color: 24 lines, the keys at the bottom', text(msc)[:5] == [l.rstrip() for l in lines[:5]]
      and text(msc)[23] == lines[24], '\n'.join(text(msc)))
r, mo, msc = run(['-n', '1', '-d', '1'], term='dumb', rows=24)
check('a terminal without reverse video: no attributes', not re.search('\033\\[[0-9;]*m', mo) and
      text(msc)[23] == lines[24], '\n'.join(text(msc)))

# A kernel that changes between screens, the screens driven by keys: a
# process uses the processor and moves to the top, another changes its
# name, the zombie goes, the clock moves on two days; then c changes the
# theme to Monochrome, and .xtoprc keeps it.
kpath = dict(kv.split('=') for kv in env['XSIM_MAP'].split(','))
def poke(name, at, fmt, *v):
    with open(kpath[name], 'r+b') as f:
        f.seek(at)
        f.write(struct.pack(fmt, *v))
open(f'{tmp}/xtoprc', 'w').write('')
p = subprocess.Popen(SIM + ['-d', '1'], env=env, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                     stderr=subprocess.STDOUT)
got = []
threading.Thread(target=lambda: [got.append(b) for b in iter(lambda: p.stdout.read1(4096), b'')],
                 daemon=True).start()
def settle():
    n, t = -1, time.time()
    while time.time() - t < 60:
        time.sleep(1.5)
        if len(got) == n:
            return
        n = len(got)
def key(k):
    p.stdin.write(k)
    p.stdin.flush()
    settle()
settle()
first = sum(len(b) for b in got)
poke('/dev/kmem', SYMS['_lbolt'], '<l', LB + 60)              # three seconds on
poke('/dev/mem', 0x3c40 * 16 + 1296, '<ll', 122 + 45, 312 + 12)  # kprobe ran 57 ticks of them
poke('/dev/kmem', SYMS['_time'], '<l', boot + 3 * 3600 + 27 * 60)
# and lpsched came into memory, at 0x5000
poke('/dev/kmem', SYMS['_proc'] + 3 * 48 + 1, '<B', 1)
poke('/dev/kmem', SYMS['_proc'] + 3 * 48 + 22, '<H', 0x5000)
poke('/dev/mem', 0x5000 * 16 + 1296, '<ll', 5, 15)
poke('/dev/mem', 0x5000 * 16 + 1388, '14s', b'lpsched')
key(b' ')
second = sum(len(b) for b in got) - first
sc2 = play(b''.join(got).decode('latin1'))
check('beside the swap meter: init still out, lpsched back in, over 3 seconds', text(sc2)[2] ==
      ' Swp[||||             278K/1.6M]    Swapped out: 1   last 3s: 0 out, 1 in', repr(text(sc2)[2]))
check("the CPU meter from the processes' ticks: 57 of 60", text(sc2)[0].startswith(' CPU[') and
      '95.0%]' in text(sc2)[0], repr(text(sc2)[0]))
# 45 ticks in programs, 12 in the kernel, of 60: 19 and 24 of 26 cells, the
# last 5 the percentage
check('the CPU meter: programs green, kernel red',
      norm(attrs(sc2, 0, 5, 26)) == [G] * 19 + [R] * 2 and sc2.display[0][5:26] == '|' * 21,
      repr(norm(attrs(sc2, 0, 5, 31))))
check('lpsched in memory now', any(l.split()[:1] == ['21'] and l.split()[6] == '26K' for l in text(sc2)),
      '\n'.join(text(sc2)))
poke('/dev/mem', 0x74c0 * 16 + 1388, '14s', b'vi')                 # sh became vi
poke('/dev/kmem', SYMS['_proc'] + 5 * 48, '<B', 0)                 # the zombie is gone
poke('/dev/kmem', SYMS['_lbolt'], '<l', (2 * 86400 + 3 * 3600 + 27 * 60 + 5) * 20)
poke('/dev/kmem', SYMS['_time'], '<l', 1791975049)               # the clock set to 2026: no matter (M66)
key(b' ')
key(b' ')                                                          # once more: no change, CPU 0.0
mark = sum(len(b) for b in got)
key(b'c')                                                          # to Monochrome
key(b'q')
p.wait(timeout=60)
out = b''.join(got).decode('latin1')
sc = play(out[:mark])
final = subprocess.run(SIM + ['-b', '-n', '1', '-d', '1'], env=env, stdout=subprocess.PIPE).stdout.decode('latin1')
fl = [l.rstrip() for l in final.split('\n')[:25]]
check('after changes, the screen as the batch output', text(sc) == fl,
      '\n'.join(text(sc)) + '\n----\n' + '\n'.join(fl))
check('the uptime in days, from the ticks, whatever the clock says', fl[1].endswith('Uptime: 2 days, 03:27:05'), repr(fl[1]))
check('the busy process at the top while it ran', 'kprobe' in out[first:first + second], repr(out[first:first + second]))
check('only what changed sent: fewer bytes than a screen', 0 < second < first / 2, f'{second} of {first}')
check('the zombie gone, the name changed', not any('defunct' in l for l in fl) and any(l.endswith(' vi') for l in fl),
      '\n'.join(fl))
msc = play(out[:out.rfind('\033[2J')])
check('c: to Monochrome, the heading in reverse video, no colors after it',
      all(a[3] for a in attrs(msc, 7, 0, 79)) and not re.search('\033\\[[34][0-9]m', out[mark:]), repr(out[mark:mark + 200]))
check('c: Monochrome kept in .xtoprc', open(f'{tmp}/xtoprc').read() == 'theme mono\n', repr(open(f'{tmp}/xtoprc').read()))
# the kernel's symbols kept in /usr/lib/xtop.syms (M57). With no such file
# (all the runs above) nothing is kept; installed empty, the first run fills
# it, and later runs use it while /xenix is the same file
xs = f'{tmp}/xtop.syms'
open(xs, 'wb').close()
def once():
    return subprocess.run(SIM + ['-b', '-n', '1', '-d', '1'], env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
r1 = once()
kept = open(xs, 'rb').read()
st = os.stat(f'{tmp}/xenix')
dst = os.stat(f'{tmp}/dev')
check('the symbols kept: 190 bytes, xts2, /xenix as it is, the addresses',
      r1.returncode == 0 and len(kept) == 190 and kept[:4] == b'xts2'
      and struct.unpack_from('<ll', kept, 8) == (st.st_size, int(st.st_mtime))
      and list(struct.unpack_from('<11l', kept, 16)) == list(SYMS.values())
      and struct.unpack_from('<11h', kept, 60) == (1,) * 11, repr(kept) + repr(r1.stdout[-200:]))
# (M58) and the terminals' names, with /dev's time; and the size of memory,
# with the time the machine started
ttys = [struct.unpack_from('<h4s', kept, 88 + 6 * i) for i in range(struct.unpack_from('<h', kept, 86)[0])]
check("the terminals' names kept, with /dev's time", struct.unpack_from('<l', kept, 82)[0] == int(dst.st_mtime)
      and (not DEVS or ttys == [(0, b'co\0\0'), (1, b'02\0\0')]), repr(ttys))
check('the size of memory kept, with the start time', struct.unpack_from('<lH', kept, 184) == (boot, 640),
      repr(struct.unpack_from('<lH', kept, 184)))
xen = open(f'{tmp}/xenix', 'rb').read()
open(f'{tmp}/xenix', 'wb').write(xen.replace(b'_proc\0', b'_prXc\0'))
os.utime(f'{tmp}/xenix', (st.st_atime, st.st_mtime))
r2 = once()
check('the kept symbols used: /xenix not read again, though _proc is gone from it',
      r2.returncode == 0 and r2.stdout == r1.stdout, repr(r2.stdout[-300:]))
os.utime(f'{tmp}/xenix', (st.st_atime, st.st_mtime + 60))
r3 = once()
check('/xenix changed: the kept symbols not used (so _proc is missed)',
      r3.returncode == 1 and b'cannot read the kernel' in r3.stdout, repr(r3.stdout[-200:]))
open(f'{tmp}/xenix', 'wb').write(xen)
st = os.stat(f'{tmp}/xenix')
r4 = once()
check('kept again for the new /xenix', r4.returncode == 0 and r4.stdout == r1.stdout
      and struct.unpack_from('<ll', open(xs, 'rb').read(), 8) == (st.st_size, int(st.st_mtime)))
open(xs, 'wb').write(b'junk' + kept[4:])
r5 = once()
check('a file that is not xtop\'s own: looked past, and made over', r5.returncode == 0 and r5.stdout == r1.stdout
      and open(xs, 'rb').read()[:4] == b'xts2')
if DEVS:
    o, looked = stats()
    check('the terminals kept: none looked up', looked == [] and o.split('\n')[:25] == r1.stdout.decode('latin1').split('\n')[:25],
          repr(looked))
    os.utime(f'{tmp}/dev', (dst.st_atime, dst.st_mtime + 60))
    o, looked = stats()
    check('/dev changed: the terminals looked up again', looked == ['/dev/console', '/dev/tty02'], repr(looked))
    o, looked = stats()
    check('and kept again', looked == [], repr(looked))
k = bytearray(open(xs, 'rb').read())
struct.pack_into('<H', k, 188, 512)
open(xs, 'wb').write(k)
r7 = once()
check('the size of memory kept: used', b'/512K]' in r7.stdout, repr(r7.stdout[:200]))
with open(f'{tmp}/kmem', 'r+b') as f:      # started again
    f.seek(SYMS['_bootime'])
    f.write(struct.pack('<l', boot + 7))
r8 = once()
check('the machine started again: memory measured again, and kept',
      b'/640K]' in r8.stdout and struct.unpack_from('<lH', open(xs, 'rb').read(), 184) == (boot + 7, 640),
      repr(r8.stdout[:200]))
with open(f'{tmp}/kmem', 'r+b') as f:
    f.seek(SYMS['_bootime'])
    f.write(struct.pack('<l', boot))
r = subprocess.run(SIM + ['-x'], env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
check('a bad option', r.returncode == 2)
# xtfetch: the same program under that name, printed once
shutil.copy(f'{ROOT}/build/xtop', f'{tmp}/xtfetch')
open(f'{tmp}/systemid', 'w').write('xenix86\n')
fenv = dict(env, XSIM_MAP=env['XSIM_MAP'] + f',/etc/systemid={tmp}/systemid', TERM='ansi', SHELL='/bin/csh')
r = subprocess.run([sys.executable, f'{ROOT}/tools/xsim.py', f'{tmp}/xtfetch', '-m'], env=fenv,
                   stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
fo = r.stdout.decode('latin1')
check('xtfetch -m runs', r.returncode == 0, repr(fo[-300:]))
for want in ('@xenix86', 'OS: XENIX System V 2.1.3', 'CPU: Intel 8088', 'FPU: ', 'Memory: 542K used of 640K',
             'Disk (/): ', 'Uptime: ', 'Shell: /bin/', 'xenix86-devsys 0.12.0-alpha', "'--------------------------'"):
    check('xtfetch shows ' + want, want in fo, repr(fo))
check('xtfetch -m has no escapes', '\033' not in fo)
check('xtfetch has no serial', 'serial' not in fo.lower())
# in color, as on the console: printed once (M72d printed twice, its color
# bar's line overrunning its buffer), every escape one number, all 8 blocks,
# and gray again at the end (the console's 0m leaves the color)
import re
r = subprocess.run([sys.executable, f'{ROOT}/tools/xsim.py', f'{tmp}/xtfetch'],
                   env=dict(fenv, XSIM_TTY='0 1', XSIM_COLOR='1'), stdout=subprocess.PIPE)
co = r.stdout.decode('latin1')
check('xtfetch in color runs', r.returncode == 0)
plain = re.sub(r'\033\[\d+m', '', co)
check('xtfetch in color: printed once', plain.count('@xenix86') == 1 and plain.count('Dev system') == 1, repr(co[-400:]))
check('xtfetch in color: one number an escape', not re.search(r'\033\[\d+;', co))
check('xtfetch in color: 8 blocks', co.count('\333\333') == 8)
check('xtfetch in color: gray at the end', co.rstrip('\n').endswith('\033[0m\033[37m'))
r = subprocess.run([sys.executable, f'{ROOT}/tools/xsim.py', f'{tmp}/xtfetch', '-m'],
                   env=dict(fenv, XSIM_TTY='0 1', XSIM_COLOR='1'), stdout=subprocess.PIPE)
check('xtfetch -m on a color console: no escapes, once', '\033' not in r.stdout.decode('latin1')
      and r.stdout.decode('latin1').count('@xenix86') == 1)
r = subprocess.run([sys.executable, f'{ROOT}/tools/xsim.py', f'{tmp}/xtfetch', '-b'], env=fenv, stdout=subprocess.PIPE)
check('xtfetch: a bad option', r.returncode == 2)
if os.environ.get('XTOP_KEEP'):     # keep the made-up kernel, to run xtop on it by hand
    print(out)
    print('XSIM_MAP=' + env['XSIM_MAP'])
else:
    shutil.rmtree(tmp)
print(f'{count} checks, {fails} failures: ' + ('ALL PASS' if not fails else 'FAILED'))
sys.exit(1 if fails else 0)
