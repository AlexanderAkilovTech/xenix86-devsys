"""Tests for the software floating point routines (lib/fp): runs tests/fptest.c
(built by test.sh, or here) on the simulator, feeding it operands, and checks
every result bit for bit against a Python model of the same rules: IEEE double
and float arithmetic, rounded to nearest even (Python's own floats), with the
8087's choices where IEEE leaves room: the default NaN FFF8 0000 0000 0000 for
invalid operations, NaN operands made quiet and the one with the larger
fraction passed on, and the "integer indefinite" for conversions out of range.

    python3 tests/unit/test_fp.py [count]       (count: lines per kind, default 400)"""
import math, os, random, struct, subprocess, sys, warnings
import numpy as np

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
N = int(sys.argv[1]) if len(sys.argv) > 1 else 400
random.seed(int(os.environ.get("FP_SEED", 1986)))
warnings.simplefilter('ignore')

SIGN, FRAC, QUIET = 1 << 63, (1 << 52) - 1, 1 << 51
DNAN = 0xFFF8000000000000
ONE = 0x3FF0000000000000

def b2d(b): return struct.unpack('<d', struct.pack('<Q', b))[0]
def d2b(x): return struct.unpack('<Q', struct.pack('<d', x))[0]
def isnan(b): return (b >> 52) & 0x7FF == 0x7FF and b & FRAC != 0
def isinf(b): return b & ~SIGN == 0x7FF0000000000000

def nanres(a, b):
    a = a | QUIET if isnan(a) else a
    b = b | QUIET if isnan(b) else b
    if isnan(a) and isnan(b):
        return a if a & FRAC >= b & FRAC else b
    return a if isnan(a) else b

def add(a, b):
    if isnan(a) or isnan(b): return nanres(a, b)
    if isinf(a) and isinf(b) and (a ^ b) & SIGN: return DNAN
    return d2b(b2d(a) + b2d(b))

def sub(a, b):
    return add(a, b) if isnan(b) else add(a, b ^ SIGN)

def mul(a, b):
    if isnan(a) or isnan(b): return nanres(a, b)
    x, y = b2d(a), b2d(b)
    if (isinf(a) and y == 0) or (isinf(b) and x == 0): return DNAN
    return d2b(x * y)

def div(a, b):
    if isnan(a) or isnan(b): return nanres(a, b)
    x, y = b2d(a), b2d(b)
    if (isinf(a) and isinf(b)) or (x == 0 and y == 0): return DNAN
    if y == 0: return 0x7FF0000000000000 | ((a ^ b) & SIGN)
    return d2b(x / y)

def cmp(a, b):
    """-1, 0, 1; a NaN compares as greater"""
    if isnan(a) or isnan(b): return 1
    x, y = b2d(a), b2d(b)
    return (x > y) - (x < y)

def rels(a, b):
    c = cmp(a, b)
    return ''.join(str(int(v)) for v in (c < 0, c <= 0, c == 0, c != 0, c > 0, c >= 0))

def tst(a): return cmp(a, 0)

# floats
FSIGN, FFRAC = 1 << 31, (1 << 23) - 1
def fisnan(f): return (f >> 23) & 0xFF == 0xFF and f & FFRAC != 0

def dtof(a):
    if isnan(a):
        return ((a >> 32) & FSIGN) | 0x7FC00000 | ((a & FRAC) >> 29)
    return int(np.array([b2d(a)], dtype=np.float64).astype(np.float32).view(np.uint32)[0])

def fext(f):
    """float to double, as Fpushf does it (a NaN made quiet)"""
    if fisnan(f):
        return ((f & FSIGN) << 32) | 0x7FF0000000000000 | ((f & FFRAC) << 29) | QUIET
    return d2b(float(np.array([f], dtype=np.uint32).view(np.float32)[0]))

def trunc(a):
    """the integer part, or None for a NaN or infinity"""
    if isnan(a) or isinf(a): return None
    return math.trunc(b2d(a))

def dtoi(a):
    t = trunc(a)
    return 0x8000 if t is None or abs(t) >= 0x8000 else t & 0xFFFF

def dtol(a):
    t = trunc(a)
    return 0x80000000 if t is None or abs(t) >= 1 << 31 else t & 0xFFFFFFFF

def dtoul(a):
    t = trunc(a)
    return 0 if t is None or abs(t) >= 1 << 63 else t & 0xFFFFFFFF

def i2d(v): return d2b(float(v))

# operands
SPECIAL = [0, SIGN, ONE, ONE | SIGN, 0x7FF0000000000000, 0xFFF0000000000000,
           0x7FF8000000000000, DNAN, 0x7FF0000000000001, 0x7FF4000000000000, 0xFFF7FFFFFFFFFFFF,
           1, FRAC, 1 << 52, 0x7FEFFFFFFFFFFFFF, 0x0010000000000001, 0x000FFFFFFFFFFFFF | SIGN,
           0x4000000000000000, 0x3FE0000000000000, 0x3FEFFFFFFFFFFFFF, 0x3FF0000000000001,
           0x7FE0000000000000, 0x0020000000000000, 0x3CA0000000000000, 0x4340000000000000,
           d2b(0.1), d2b(1/3), d2b(-2.5), d2b(32767.5), d2b(-32768.0), d2b(-32768.9),
           d2b(65535.9), d2b(2147483647.9), d2b(-2147483648.0), d2b(4294967295.5),
           d2b(4294967296.0), d2b(9.2233720368547758e18), d2b(-9.2233720368547758e18),
           d2b(1e300), d2b(1e-300), d2b(3.4028235677973366e38), d2b(1.401298464324817e-45)]

def operand():
    k = random.random()
    if k < 0.2: return random.choice(SPECIAL)
    if k < 0.4: return random.getrandbits(64)
    if k < 0.6: return d2b(random.choice([1, -1]) * random.randint(0, 100000) / random.choice([1, 2, 3, 10, 7, 1024]))
    if k < 0.8:    # moderate exponents, random fractions
        return (random.getrandbits(1) << 63) | (random.randint(1023 - 70, 1023 + 70) << 52) | random.getrandbits(52)
    e = random.choice([0, 1, 2, 3, 50, 51, 52, 53, 54, 960, 1000, 2045, 2046])   # near the edges
    return (random.getrandbits(1) << 63) | (e << 52) | random.getrandbits(52)

def edge_pair():
    """products and quotients that land in the denormal range or near overflow"""
    ea = random.randint(1, 2046)
    t = random.choice([random.randint(-60, 2), random.randint(2040, 2050)])  # the result's exponent
    if random.random() < 0.5:
        eb = t - ea + 1023          # a * b
    else:
        eb = ea - t + 1023          # a / b
    eb = min(2046, max(1, eb))
    frac = lambda: random.choice([0, random.getrandbits(52), FRAC, 1 << 51, random.getrandbits(8)])
    return ((random.getrandbits(1) << 63) | (ea << 52) | frac(),
            (random.getrandbits(1) << 63) | (eb << 52) | frac())

def pair():
    if random.random() < 0.15: return edge_pair()
    a = operand()
    k = random.random()
    if k < 0.15: b = a ^ SIGN                         # x and -x
    elif k < 0.3: b = a ^ random.getrandbits(random.randint(1, 10))   # close to x
    elif k < 0.4: b = (a & ~(0x7FF << 52)) | (min(2046, max(1, ((a >> 52) & 0x7FF) + random.randint(-60, 60))) << 52)
    else: b = operand()
    return (a, b) if random.random() < 0.5 else (b, a)

def foperand():
    k = random.random()
    if k < 0.3: return dtof(operand())
    if k < 0.5: return random.getrandbits(32)
    return random.choice([0, FSIGN, 0x3F800000, 0x7F800000, 0xFF800000, 0x7FC00000, 0x7F800001,
                          1, 0x807FFFFF, 0x7F7FFFFF, 0x00800000, random.getrandbits(32)])

h16 = lambda v: f'{v:016x}'
h8 = lambda v: f'{v:08x}'
lines, expect = [], []

for _ in range(N * 3):
    a, b = pair()
    lines.append(f'a {h16(a)} {h16(b)}')
    r = [add(a, b), sub(a, b), mul(a, b), div(a, b)]
    expect.append('a ' + ' '.join(h16(v) for v in r + r))
for _ in range(N * 2):
    a, b = pair()
    lines.append(f'c {h16(a)} {h16(b)}')
    t = tst(a)
    expect.append(f'c {rels(a, b)} {rels(a, b)} {int(t == 0)}{int(t != 0)}{int(t == 0)}')
for _ in range(N):
    a = operand()
    lines.append(f'd {h16(a)}')
    i = dtoi(a)
    expect.append(f'd {h16(a ^ SIGN)} {h16(a ^ SIGN)} {h8(dtof(a))} {i:04x} {dtol(a) & 0xFFFF:04x} '
                  f'{h8(dtol(a))} {h8(dtoul(a))} {i & 0xFF:02x} {i & 0xFF:02x}')
for _ in range(N):
    p, q = foperand(), foperand()
    if random.random() < 0.2: q = p ^ FSIGN
    lines.append(f'f {h8(p)} {h8(q)}')
    P, Q = fext(p), fext(q)
    r = [dtof(add(P, Q)), dtof(sub(P, Q)), dtof(mul(P, Q)), dtof(div(P, Q))]
    t = tst(P)
    expect.append('f ' + ' '.join(h8(v) for v in r) + f' {h16(P)} {h8(dtof(P ^ SIGN))} {rels(P, Q)} '
                  f'{int(t == 0)}{int(t != 0)} {dtoi(P):04x} {dtol(P) & 0xFFFF:04x} {h8(dtol(P))} '
                  f'{h8(dtoul(P))} {h16(add(P, Q))} {h16(mul(P, Q))}')
for _ in range(N):
    v = random.choice([random.getrandbits(32), random.getrandbits(16), random.getrandbits(8),
                       0, 0xFFFFFFFF, 0x80000000, 0x7FFFFFFF, 0x8000, 0x7FFF, 0xFFFF, 0x80, 0xFF])
    lines.append(f'i {h8(v)}')
    s16 = (v & 0xFFFF) - ((v & 0x8000) << 1)
    s32 = v - ((v & 0x80000000) << 1)
    s8 = (v & 0xFF) - ((v & 0x80) << 1)
    expect.append('i ' + ' '.join(h16(i2d(x)) for x in (s16, v & 0xFFFF, s32, v, v & 0xFF, s8, v & 0xFF))
                  + ' ' + h8(dtof(i2d(s32))))
for _ in range(N):
    x = operand()
    lines.append(f'p {h16(x)}')
    f = dtof(x)
    finc = dtof(add(fext(f), ONE))
    fdec = dtof(sub(fext(f), ONE))
    r = [x, add(x, ONE), add(x, ONE), x, sub(x, ONE), sub(x, ONE), fext(f)]
    expect.append('p ' + ' '.join(h16(v) for v in r) + f' {h8(finc)} {h16(fext(fdec))} {h8(fdec)}')
for _ in range(N):
    x, p = operand(), foperand()
    lines.append(f'h {h16(x)} {h8(p)}')
    P = fext(p)
    expect.append('h ' + ' '.join(h16(v) for v in (add(x, P), sub(x, P), mul(x, P), div(x, P)))
                  + f' {cmp(x, P)} {tst(x)} {h16(x ^ SIGN)}')

prog = f'{ROOT}/build/fptest'
if not os.path.exists(prog):
    subprocess.run([f'{ROOT}/build.sh', 'tests/fptest.c', 'tests/fptests.s'], cwd=ROOT, check=True,
                   stdout=subprocess.DEVNULL)
env = dict(os.environ, XSIM_LIMIT='4000000000')
out = subprocess.run([sys.executable, f'{ROOT}/tools/xsim.py', prog], input='\n'.join(lines) + '\n',
                     capture_output=True, text=True, env=env)
got = out.stdout.splitlines()
fails = 0
for i, (line, want) in enumerate(zip(lines, expect)):
    g = got[i] if i < len(got) else '(no output)'
    if g != want:
        fails += 1
        if fails <= 12:
            print(f'FAIL {line}\n  want {want}\n  got  {g}')
if len(got) < len(lines):
    print(f'only {len(got)} of {len(lines)} lines of output; {out.stderr.strip()[-300:]}')
    fails += 1
print(f'{len(lines)} lines, {fails} mismatches: ' + ('ALL PASS' if not fails else 'FAILED'))
# XDS_DUMP=dir: keep this run's input and its checked output, for running
# the same program on real XENIX and comparing (tests/m21)
if os.environ.get('XDS_DUMP') and not fails:
    name = os.path.basename(prog)
    with open(os.path.join(os.environ['XDS_DUMP'], name + '.in'), 'w') as f: f.write('\n'.join(lines) + '\n')
    with open(os.path.join(os.environ['XDS_DUMP'], name + '.ok'), 'w') as f: f.write(out.stdout)
sys.exit(1 if fails else 0)
