"""Tests for the curses library (lib/curses): tests/curt.c runs lists of
operations on the simulator, then dumps what curses believes is on the
screen (curscr) and what is in stdscr. Three checks for each list:

  - stdscr against this file's own model of the operations;
  - curscr against stdscr (after the last refresh they must agree);
  - the terminal output, played through the pyte terminal emulator,
    against curscr: what curses sent must make the screen it thinks it made.

The terminals are tests/curses.tc's: one VT100-like with everything, one
with only cursor motion and clearing, one without automatic margins. Then
function keys through keypad.

    python3 tests/unit/test_curses.py [lists-per-terminal]"""
import os, random, subprocess, sys
import pyte

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SIM = [sys.executable, f'{ROOT}/tools/xsim.py', f'{ROOT}/build/curt']
TC = f'{ROOT}/tests/curses.tc'
L, C = 24, 80
A_STANDOUT, A_UNDERLINE, A_REVERSE, A_BOLD = 0x100, 0x200, 0x400, 0x2000
count = fails = 0

def check(what, want, got):
    global count, fails
    count += 1
    if want != got:
        fails += 1
        if isinstance(want, list) and len(want) == L and isinstance(want[0], list):
            diff = [(y, x, hex(want[y][x]), hex(got[y][x])) for y in range(L) for x in range(C)
                    if want[y][x] != got[y][x]]
            print(f'FAIL {what}: {len(diff)} cells differ (y, x, want, got): {diff[:6]}')
        else:
            print(f'FAIL {what}\n  want {want!r}\n  got  {got!r}')
        return False
    return True

class Model:
    """stdscr, as the operations should leave it"""
    def __init__(self):
        self.s = [[0x20] * C for _ in range(L)]
        self.y = self.x = 0
    def newline(self):
        self.x = 0
        if self.y == L - 1:
            self.s.pop(0); self.s.append([0x20] * C)
        else:
            self.y += 1
    def put(self, v):
        self.s[self.y][self.x] = v
        self.x += 1
        if self.x >= C:
            self.newline()
    def addch(self, v):
        c, a = v & 0xFF, v & 0xFF00
        if c == 10:
            self.clrtoeol(); self.newline()
        elif c == 13:
            self.x = 0
        elif c == 8:
            self.x = max(0, self.x - 1)
        elif c == 9:
            while True:
                self.put(0x20 | a)
                if self.x % 8 == 0 or self.x == C - 1:
                    break
        elif c < 32 or c == 127:
            for ch in '^' + ('?' if c == 127 else chr(c + 64)):
                self.put(ord(ch) | a)
        else:
            self.put(v)
    def clrtoeol(self):
        for x in range(self.x, C):
            self.s[self.y][x] = 0x20
    def clrtobot(self):
        self.clrtoeol()
        for y in range(self.y + 1, L):
            self.s[y] = [0x20] * C
    def op(self, line):
        f = line.split(' ')
        k = f[0]
        if k == 'm':
            y, x = int(f[1]), int(f[2])
            if 0 <= y < L and 0 <= x < C:
                self.y, self.x = y, x
        elif k == 's':
            a = int(f[1], 16) & 0xFF00
            for ch in line.split(' ', 2)[2]:
                self.addch(ord(ch) | a)
        elif k == 'c':
            self.addch(int(f[2]) | int(f[1], 16))
        elif k == 'e': self.clrtoeol()
        elif k == 'b': self.clrtobot()
        elif k == 'i':
            row = self.s[self.y]
            row.insert(self.x, int(f[1])); row.pop()
        elif k == 'd':
            row = self.s[self.y]
            row.pop(self.x); row.append(0x20)
        elif k == 'I':
            self.s.insert(self.y, [0x20] * C); self.s.pop()
        elif k == 'D':
            self.s.pop(self.y); self.s.append([0x20] * C)
        elif k == 'E':
            self.s = [[0x20] * C for _ in range(L)]; self.y = self.x = 0
        elif k == 'S':
            n = int(f[1])
            for _ in range(abs(n)):
                if n > 0:
                    self.s.pop(0); self.s.append([0x20] * C)
                else:
                    self.s.pop(); self.s.insert(0, [0x20] * C)
        elif k == 'p':
            y, x = int(f[1]), int(f[2])
            if 0 <= y < L and 0 <= x < C:
                self.y, self.x = y, x
                for ch in f'<{int(f[3])}>':
                    self.addch(ord(ch))

def run(term, ops, keys=b''):
    path = f'/tmp/curt_{os.getpid()}.ops'
    open(path, 'w').write(''.join(o + '\n' for o in ops))
    env = dict(os.environ, XSIM_ENV=f'TERM={term} TERMCAP={TC}')
    r = subprocess.run(SIM + [path], input=keys, capture_output=True, env=env)
    os.remove(path)
    err = r.stderr.decode('latin1').split('\n')
    dumps, i = {}, 0
    while i < len(err):
        if err[i] in ('curscr', 'stdscr'):
            dumps[err[i]] = [[int(row[j:j + 4], 16) for j in range(0, C * 4, 4)] for row in err[i + 1:i + 1 + L]]
            i += 1 + L
        else:
            i += 1
    return r.stdout, dumps, [l for l in err if l.startswith('key ')]

def screen(out):
    s = pyte.Screen(C, L)
    s.set_mode(pyte.modes.LNM)          # \n is a return and a line feed, as with ONLCR
    pyte.ByteStream(s).feed(out)
    return s

def shown(v, term):
    """the character and (reverse, underline, bold) the terminal should show"""
    a = v & 0xFF00
    if term == 'cvt':
        rev = bool(a & (A_STANDOUT | A_REVERSE))
        return chr(v & 0xFF), (rev, bool(a & A_UNDERLINE), bool(a & A_BOLD))
    if term == 'cnoam':
        return chr(v & 0xFF), (bool(a & (A_STANDOUT | A_REVERSE | A_BOLD)), False, False)
    return chr(v & 0xFF), (False, False, False)

def randops(rnd):
    ops = []
    for _ in range(rnd.randint(20, 60)):
        k = rnd.random()
        y, x = rnd.randrange(L), rnd.randrange(C)
        if k < 0.25:
            ops.append(f'm {y} {x}')
        elif k < 0.5:
            a = rnd.choice([0, 0, 0, A_STANDOUT, A_UNDERLINE, A_BOLD, A_REVERSE, A_BOLD | A_UNDERLINE])
            text = ''.join(rnd.choice('abcdefghijklmnopqrstuvwxyz ABC0123456789.,-=+') for _ in range(rnd.randint(1, 100)))
            ops.append(f's {a:x} {text}')
        elif k < 0.55:
            ops.append(f'c {rnd.choice([0, A_STANDOUT]):x} {rnd.choice([9, 10, 13, 8, 1, 27, 127, 65])}')
        elif k < 0.6: ops.append('e')
        elif k < 0.62: ops.append('b')
        elif k < 0.66: ops.append(f'i {rnd.choice([42, 64]) | rnd.choice([0, A_STANDOUT])}')
        elif k < 0.70: ops.append('d')
        elif k < 0.73: ops.append('I')
        elif k < 0.76: ops.append('D')
        elif k < 0.77: ops.append('E')
        elif k < 0.80: ops.append(f'S {rnd.choice([1, 2, -1, 3])}')
        elif k < 0.83: ops.append(f'p {y} {x} {rnd.randint(-500, 32767)}')
        elif k < 0.87: ops.append(f'w {rnd.randrange(L - 6)} {rnd.randrange(C - 22)} {rnd.randint(3, 6)} {rnd.randint(8, 22)}')
        else: ops.append('r')
    return ops

nlists = int(sys.argv[1]) if len(sys.argv) > 1 else 12
rnd = random.Random(1986)
for term in ('cvt', 'cmin', 'cnoam'):
    for n in range(nlists):
        ops = randops(rnd)
        out, dumps, _ = run(term, ops)
        if not check(f'{term} #{n}: curt ran and dumped', ['curscr', 'stdscr'], sorted(dumps)):
            continue
        m = Model()
        for o in ops:
            m.op(o)
        if not check(f'{term} #{n}: stdscr matches the model', m.s, dumps['stdscr']):
            print('   ops:', ops)
            continue
        check(f'{term} #{n}: curscr matches stdscr after the refresh', dumps['stdscr'], dumps['curscr'])
        s = screen(out)
        bad = []
        for y in range(L):
            for x in range(C):
                want = shown(dumps['curscr'][y][x], term)
                cell = s.buffer[y][x]
                got = (cell.data, (cell.reverse, cell.underscore, cell.bold) if term != 'cmin' else (False, False, False))
                if want != got:
                    bad.append((y, x, want, got))
        if not check(f'{term} #{n}: the terminal shows curscr', [], bad[:5]):
            print('   ops:', ops)

# function keys: arrows, F1, a letter, a lone escape, then q
out, dumps, keys = run('cvt', ['k'], b'\033[A\033[Bx\033OP\033q')
check('keypad: arrows, a letter, F1, escape, q',
      ['key 259', 'key 258', 'key 120', 'key 265', 'key 27', 'key 113'], keys)

print(f'{count} checks, {fails} failures: ' + ('ALL PASS' if not fails else 'FAILED'))
sys.exit(1 if fails else 0)
