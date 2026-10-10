"""Tests for the decimal conversions: printf's e, f, g, strtod, sscanf's %f and
%lf, ecvt and fcvt (lib/stdio/fltcvt.c, lib/gen/strtod.c). Runs tests/fmtest.c
on the simulator and compares with Python, whose float formatting and float()
are correctly rounded too, so every digit must agree.

    python3 tests/unit/test_fmt.py [count]     (count: cases per kind, default 300)"""
import os, random, re, struct, subprocess, sys
from fractions import Fraction

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
N = int(sys.argv[1]) if len(sys.argv) > 1 else 300
random.seed(int(os.environ.get('FP_SEED', 1986)))

def b2d(b): return struct.unpack('<d', struct.pack('<Q', b))[0]
def d2b(x): return struct.unpack('<Q', struct.pack('<d', x))[0]
def isnan(b): return (b >> 52) & 0x7FF == 0x7FF and b & ((1 << 52) - 1)

def operand():
    k = random.random()
    if k < 0.1:
        return random.choice([0, 1 << 63, 0x7FF0000000000000, 0xFFF0000000000000, 0x7FF8000000000000,
                              0xFFF8000000000000, 1, 0x000FFFFFFFFFFFFF, 0x0010000000000000,
                              0x7FEFFFFFFFFFFFFF, d2b(0.5), d2b(9.5), d2b(0.125), d2b(1e22), d2b(1e23),
                              d2b(999999.5), d2b(0.0001), d2b(0.00001), d2b(123456.0), d2b(5e-324)])
    if k < 0.35: return random.getrandbits(64)
    if k < 0.6: return d2b(random.choice([1, -1]) * random.randint(0, 10**random.randint(1, 9)) / 10**random.randint(0, 9))
    return (random.getrandbits(1) << 63) | (random.randint(1023 - 80, 1023 + 80) << 52) | random.getrandbits(52)

def pyfmt(fmt, b):
    if (b >> 52) & 0x7FF == 0x7FF:
        # C pads infinities and NaNs with spaces, never zeros, and prints a
        # NaN's sign; Python zero-pads them and drops the sign of a NaN
        while re.match(r'%[-+ #]*0', fmt):
            fmt = re.sub(r'^(%[-+ #]*)0', r'\1', fmt)
        s = fmt % (float('-inf') if b >> 63 else float('inf'))
        return s.replace('inf', 'nan').replace('INF', 'NAN') if isnan(b) else s
    return fmt % b2d(b)

lines, expect = [], []
for _ in range(N * 4):
    b = operand()
    conv = random.choice('feEgG')
    flags = ''.join(random.sample('-+ #0', random.randint(0, 2)))
    width = random.choice(['', '', str(random.randint(1, 30))])
    prec = random.choice(['', '.' + str(random.randint(0, 20)), '.' + str(random.randint(0, 60)), '.0', '.1'])
    if conv == 'f' and prec and int(prec[1:]) > 40 and abs(b2d(b)) > 1e30:
        prec = '.3'          # keep the lines short
    fmt = f'%{flags}{width}{prec}{conv}'
    lines.append(f'p {b:016x} {fmt}')
    s = pyfmt(fmt, b)
    expect.append(f'[{s}] {len(s)}')

def errno_for(text, val):
    """34 (ERANGE) for an infinity from finite text, or 0 from a nonzero value"""
    m = re.match(r'\s*[+-]?(\d*\.?\d*(?:[eE][+-]?\d+)?)', text)
    digits = re.sub(r'[eE].*', '', m.group(1))
    nonzero = any(c in '123456789' for c in digits)
    return 34 if (val in (float('inf'), float('-inf'))) or (val == 0 and nonzero) else 0

def strtod_case():
    k = random.random()
    if k < 0.2:
        x = b2d(operand() & ~(1 << 63) & 0x7FEFFFFFFFFFFFFF)
        return random.choice(['%.17g', '%.16g', '%.15g', '%.20e', '%.3g', '%.25f']) % x
    if k < 0.4:     # exactly halfway between two doubles, and just either side
        b = random.choice([random.randint(1, 1 << 20), random.randint(1, 0x7FEFFFFFFFFFFFFE),
                           (random.randint(1000, 1100) << 52) | random.getrandbits(52)])
        lo, hi = Fraction(b2d(b)), Fraction(b2d(b + 1))
        mid = (lo + hi) / 2
        s = exact(mid)
        if random.random() < 0.3:
            s = s + random.choice(['0000', '00001', '9'])
        return s
    if k < 0.6:
        digits = ''.join(random.choice('0123456789') for _ in range(random.randint(1, 40)))
        return f"{random.choice(['', '-', '+'])}{digits[:random.randint(0, len(digits))]}.{digits}e{random.randint(-340, 320)}"
    if k < 0.7:
        digits = ''.join(random.choice('0123456789') for _ in range(random.randint(100, 900)))
        return '0.' + digits + 'e' + str(random.randint(-30, 30))
    return random.choice(['0', '-0', '.5', '5.', '1e', '1e+', '  42abc', '-.e5', 'x', '', '1e400', '-1e400',
                          '1e-400', '00012.50e-1', '+.000001E+06', '1.7976931348623157e308',
                          '1.7976931348623158e308', '2.4703282292062327e-324', '4.9e-324',
                          '0.' + '0' * 1500 + '1', '1' + '0' * 330 + 'e-20'])

def exact(fr):
    """the exact decimal text of a fraction whose denominator is a power of 2"""
    n, d = fr.numerator, fr.denominator
    k = d.bit_length() - 1
    assert d == 1 << k
    t = str(n * 5**k).rjust(k + 1, '0')
    return t[:-k] + '.' + t[-k:] if k else t

def endlen(text):
    m = re.match(r'\s*[+-]?(\d+\.?\d*|\.\d+)([eE][+-]?\d+)?', text)
    return m.end() if m else 0

for _ in range(N * 2):
    t = strtod_case()
    lines.append(f's {t}')
    n = endlen(t)
    v = float(t[:n].strip()) if n else 0.0
    expect.append(f'{d2b(v):016x} {n} {errno_for(t, v) if n else 0}')

import numpy as np, warnings
warnings.simplefilter('ignore')
for _ in range(N // 2):
    t = strtod_case()
    if len(t) > 800 or not endlen(t): t = '3.25'
    lines.append(f'f {t}')
    n = endlen(t)
    v = float(t[:n].strip())
    f = int(np.array([v]).astype(np.float32).view(np.uint32)[0])
    expect.append(f'{d2b(v):016x} {f:08x} 11')

def digits_e(x, n):
    if x == 0: return '0' * n, 0
    s = '%.*e' % (n - 1, abs(x))
    m, e = s.split('e')
    return m.replace('.', ''), int(e) + 1

def digits_f(x, n):
    if x == 0: return '0' * n, 0
    s = '%.*f' % (n, abs(x))
    ip, _, fp = s.partition('.')
    if ip != '0': return ip + fp, len(ip)
    t = fp.lstrip('0')
    return (t, -(len(fp) - len(t))) if t else ('', -n)

for _ in range(N // 2):
    b = operand() & 0x7FEFFFFFFFFFFFFF | (random.getrandbits(1) << 63)
    n = random.randint(1, 18)
    lines.append(f'e {b:016x} {n}')
    x = b2d(b)
    de, ee = digits_e(x, n)
    df, ef = digits_f(x, n)
    sg = b >> 63
    expect.append(f'{de} {ee} {sg} {df} {ef} {sg}')

prog = f'{ROOT}/build/fmtest'
if not os.path.exists(prog):
    subprocess.run([f'{ROOT}/build.sh', 'tests/fmtest.c'], cwd=ROOT, check=True, stdout=subprocess.DEVNULL)
env = dict(os.environ, XSIM_LIMIT='40000000000')
out = subprocess.run([sys.executable, f'{ROOT}/tools/xsim.py', prog], input='\n'.join(lines) + '\n',
                     capture_output=True, text=True, env=env)
got = out.stdout.splitlines()
fails = 0
for i, (line, want) in enumerate(zip(lines, expect)):
    g = got[i] if i < len(got) else '(no output)'
    if g != want:
        fails += 1
        if fails <= 12:
            print(f'FAIL {line[:150]}\n  want {want[:150]}\n  got  {g[:150]}')
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
