"""Tests for the math library (lib/m): runs tests/mtest.c on the simulator and
measures each result against mpmath at 60 digits, in ulps (units in the last
place of the correct result). Exact functions (sqrt, fmod, floor, ceil, fabs,
frexp, ldexp, modf) must be correctly rounded; the others must stay within the
bound given for each. Special cases (NaNs, infinities, zeros, the System V
errors and their errno values and messages) are checked exactly.

    python3 tests/unit/test_m.py [count]      (count: random cases per function)"""
import math, os, random, struct, subprocess, sys
from mpmath import mp, mpf
import mpmath

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
N = int(sys.argv[1]) if len(sys.argv) > 1 else 300
random.seed(int(os.environ.get('FP_SEED', 1986)))
mp.prec = 200

def d2b(x): return struct.unpack('<Q', struct.pack('<d', x))[0]
def b2d(b): return struct.unpack('<d', struct.pack('<Q', b))[0]
INF, NAN = float('inf'), float('nan')
EDOM, ERANGE = 33, 34

def ulp_of(v):
    """the spacing of doubles at the (mpf) value v"""
    a = abs(v)
    if a < mpf(2) ** -1022: return mpf(2) ** -1074
    e = int(mpmath.floor(mpmath.log(a, 2)))
    return mpf(2) ** (e - 52)

def rnd(lo, hi): return random.uniform(lo, hi)
def logrnd(lo_e, hi_e):     # random magnitudes over a range of binary exponents
    return random.choice([1, -1]) * 2.0 ** random.uniform(lo_e, hi_e)

# function: (reference, bound in ulps, input generator)
pi = math.pi
def near_halfpi():
    k = random.randint(1, 10**random.randint(1, 15))
    return float(k * mpmath.pi / 2) * (1 + random.choice([0, 1, -1]) * 2**-52)

F1 = {
    'sqrt':  (mpmath.sqrt, 0.5, lambda: abs(logrnd(-1074, 1023))),
    'exp':   (mpmath.exp, 1.0, lambda: random.choice([rnd(-745, 709.7), rnd(-1, 1), logrnd(-60, 3)])),
    'log':   (mpmath.log, 1.0, lambda: random.choice([abs(logrnd(-1074, 1023)), rnd(0.7, 1.4), 1 + logrnd(-50, -1)])),
    'log10': (mpmath.log10, 1.0, lambda: random.choice([abs(logrnd(-1074, 1023)), rnd(0.7, 1.4), 10.0 ** random.randint(-300, 300)])),
    'sin':   (mpmath.sin, 1.0, lambda: random.choice([rnd(-10, 10), logrnd(-30, 50), near_halfpi(), rnd(-1e6, 1e6)])),
    'cos':   (mpmath.cos, 1.0, lambda: random.choice([rnd(-10, 10), logrnd(-30, 50), near_halfpi(), rnd(-1e6, 1e6)])),
    'tan':   (mpmath.tan, 1.5, lambda: random.choice([rnd(-10, 10), logrnd(-30, 50), near_halfpi(), rnd(-1e6, 1e6)])),
    'asin':  (mpmath.asin, 1.0, lambda: random.choice([rnd(-1, 1), random.choice([1, -1]) * (1 - 2.0 ** -random.uniform(1, 52)), logrnd(-40, -1)])),
    'acos':  (mpmath.acos, 1.0, lambda: random.choice([rnd(-1, 1), random.choice([1, -1]) * (1 - 2.0 ** -random.uniform(1, 52)), logrnd(-40, -1)])),
    'atan':  (mpmath.atan, 1.0, lambda: random.choice([rnd(-3, 3), logrnd(-40, 80)])),
    'sinh':  (mpmath.sinh, 1.5, lambda: random.choice([rnd(-3, 3), rnd(-710, 710), logrnd(-40, 3)])),
    'cosh':  (mpmath.cosh, 1.5, lambda: random.choice([rnd(-3, 3), rnd(-710, 710), logrnd(-40, 3)])),
    'tanh':  (mpmath.tanh, 1.5, lambda: random.choice([rnd(-3, 3), rnd(-25, 25), logrnd(-40, 3)])),
    'fabs':  (abs, 0.5, lambda: logrnd(-1074, 1023)),
    'floor': (mpmath.floor, 0.5, lambda: random.choice([rnd(-1e6, 1e6), logrnd(-10, 60)])),
    'ceil':  (mpmath.ceil, 0.5, lambda: random.choice([rnd(-1e6, 1e6), logrnd(-10, 60)])),
}
def pow_args():
    k = random.random()
    if k < 0.3: return abs(logrnd(-30, 30)), rnd(-20, 20)
    if k < 0.5: return abs(logrnd(-5, 5)), float(random.randint(-200, 200))
    if k < 0.6: return -abs(logrnd(-5, 5)), float(random.randint(-100, 100))
    if k < 0.7: return float(random.randint(2, 20)), float(random.randint(-30, 30))
    if k < 0.85: return rnd(0.5, 2), rnd(-1000, 1000)
    return abs(logrnd(-1000, 1000)), rnd(-1.5, 1.5)
F2 = {
    'pow':   (lambda x, y: mpf(x) ** mpf(y) if x > 0 else (-1) ** int(y) * (-mpf(x)) ** mpf(y), 1.0, pow_args),
    'atan2': (mpmath.atan2, 1.5, lambda: (logrnd(-50, 50), logrnd(-50, 50))),
    'hypot': (mpmath.hypot, 1.0, lambda: random.choice([(logrnd(-1074, 1023), logrnd(-1074, 1023)),
                                                        (logrnd(-20, 20), logrnd(-20, 20)),
                                                        (logrnd(500, 1023), logrnd(500, 1023))])),
    'fmod':  (lambda x, y: mpf(math.fmod(x, y)), 0.5, lambda: (logrnd(-60, 300), logrnd(-60, 60))),
}

lines, checks = [], []     # checks: (kind, data...)

def add1(name, x):
    lines.append(f'{name} {d2b(x):016x}')
def add2(name, x, y):
    lines.append(f'{name} {d2b(x):016x} {d2b(y):016x}')

for name, (ref, bound, gen) in F1.items():
    for _ in range(N):
        x = gen()
        add1(name, x)
        checks.append(('ulp', name, bound, ref(mpf(x)), (x,)))
for name, (ref, bound, gen) in F2.items():
    for _ in range(N):
        x, y = gen()
        add2(name, x, y)
        checks.append(('ulp', name, bound, ref(x, y), (x, y)))

# exact: frexp, ldexp, modf (against Python's own, which are exact)
for _ in range(N):
    x = random.choice([logrnd(-1074, 1023), rnd(-100, 100)])
    lines.append(f'frexp {d2b(x):016x}')
    m, e = math.frexp(x)
    checks.append(('text', f'{d2b(m):016x} {e}'))
    n = random.randint(-1200, 1200)
    lines.append(f'ldexp {d2b(x):016x} {n}')
    try:
        r = math.ldexp(x, n)
        err = 34 if r == 0 else 0
    except OverflowError:
        r, err = math.copysign(INF, x), 34
    checks.append(('text', f'{d2b(r):016x} {err}'))
    lines.append(f'modf {d2b(x):016x}')
    f, i = math.modf(x)
    checks.append(('text', f'{d2b(f):016x} {d2b(i):016x}'))

# special cases: (line, expected result, errno, message or None)
S = [
    ('sqrt', (-1.0,), 0.0, EDOM, 'sqrt: DOMAIN error'), ('sqrt', (-0.0,), -0.0, 0, None),
    ('sqrt', (INF,), INF, 0, None), ('sqrt', (0.0,), 0.0, 0, None),
    ('log', (0.0,), -INF, EDOM, 'log: SING error'), ('log', (-1.0,), -INF, EDOM, 'log: DOMAIN error'),
    ('log', (INF,), INF, 0, None), ('log', (1.0,), 0.0, 0, None),
    ('log10', (0.0,), -INF, EDOM, 'log10: SING error'), ('log10', (1000.0,), 3.0, 0, None),
    ('log10', (1e22,), 22.0, 0, None), ('log10', (1e-300,), -300.0, 0, None),
    ('exp', (1000.0,), INF, ERANGE, None), ('exp', (-1000.0,), 0.0, ERANGE, None),
    ('exp', (INF,), INF, 0, None), ('exp', (-INF,), 0.0, 0, None), ('exp', (0.0,), 1.0, 0, None),
    ('sin', (1e17,), 0.0, ERANGE, 'sin: TLOSS error'), ('cos', (-1e17,), 0.0, ERANGE, 'cos: TLOSS error'),
    ('tan', (2e16,), 0.0, ERANGE, 'tan: TLOSS error'), ('sin', (-0.0,), -0.0, 0, None),
    ('cos', (0.0,), 1.0, 0, None), ('tan', (0.0,), 0.0, 0, None),
    ('asin', (1.5,), 0.0, EDOM, 'asin: DOMAIN error'), ('acos', (-2.0,), 0.0, EDOM, 'acos: DOMAIN error'),
    ('asin', (1.0,), pi / 2, 0, None), ('acos', (-1.0,), pi, 0, None), ('acos', (1.0,), 0.0, 0, None),
    ('atan', (INF,), pi / 2, 0, None), ('atan', (-0.0,), -0.0, 0, None),
    ('atan2', (0.0, 0.0), 0.0, EDOM, 'atan2: DOMAIN error'), ('atan2', (0.0, -1.0), pi, 0, None),
    ('atan2', (-0.0, -1.0), -pi, 0, None), ('atan2', (1.0, 0.0), pi / 2, 0, None),
    ('atan2', (INF, INF), pi / 4, 0, None), ('atan2', (-INF, -INF), -3 * pi / 4, 0, None),
    ('atan2', (1.0, INF), 0.0, 0, None), ('atan2', (1.0, -INF), pi, 0, None),
    ('pow', (0.0, 0.0), 0.0, EDOM, 'pow: DOMAIN error'), ('pow', (0.0, -1.0), 0.0, EDOM, 'pow: DOMAIN error'),
    ('pow', (-8.0, 1 / 3), 0.0, EDOM, 'pow: DOMAIN error'), ('pow', (2.0, 0.0), 1.0, 0, None),
    ('pow', (1.0, NAN), 1.0, 0, None), ('pow', (-0.0, 3.0), -0.0, 0, None), ('pow', (-2.0, 3.0), -8.0, 0, None),
    ('pow', (10.0, 22.0), 1e22, 0, None), ('pow', (2.0, -1074.0), 5e-324, 0, None),
    ('pow', (2.0, 1024.0), INF, ERANGE, None), ('pow', (10.0, -400.0), 0.0, ERANGE, None),
    ('pow', (-10.0, 309.0), -INF, ERANGE, None), ('pow', (4.0, 0.5), 2.0, 0, None),
    ('pow', (-1.0, INF), 1.0, 0, None), ('pow', (0.5, INF), 0.0, 0, None), ('pow', (2.0, -INF), 0.0, 0, None),
    ('pow', (-INF, 3.0), -INF, 0, None), ('pow', (-INF, -3.0), -0.0, 0, None), ('pow', (INF, -1.0), 0.0, 0, None),
    ('sinh', (1000.0,), INF, ERANGE, None), ('sinh', (-1000.0,), -INF, ERANGE, None),
    ('cosh', (-1000.0,), INF, ERANGE, None), ('tanh', (INF,), 1.0, 0, None), ('tanh', (-1000.0,), -1.0, 0, None),
    ('hypot', (INF, NAN), INF, 0, None), ('hypot', (3.0, 4.0), 5.0, 0, None),
    ('hypot', (1.5e308, 1.5e308), INF, ERANGE, None), ('hypot', (0.0, -0.0), 0.0, 0, None),
    ('fmod', (5.5, 0.0), 5.5, 0, None), ('fmod', (-7.0, 2.0), -1.0, 0, None), ('fmod', (6.0, 3.0), 0.0, 0, None),
    ('fmod', (-6.0, 3.0), -0.0, 0, None), ('fmod', (1.0, INF), 1.0, 0, None),
    ('floor', (-0.5,), -1.0, 0, None), ('floor', (-0.0,), -0.0, 0, None), ('ceil', (-0.5,), -0.0, 0, None),
    ('ceil', (0.5,), 1.0, 0, None), ('fabs', (-INF,), INF, 0, None),
]
messages = []
for name, args, want, err, msg in S:
    if len(args) == 1: add1(name, args[0])
    else: add2(name, *args)
    checks.append(('exact', name, args, want, err))
    if msg: messages.append(msg)
for name in ('sqrt', 'exp', 'log', 'sin', 'cos', 'tan', 'asin', 'atan', 'sinh', 'cosh', 'tanh', 'floor', 'fabs'):
    add1(name, NAN)
    checks.append(('nan', name))
for name in ('pow', 'atan2', 'hypot', 'fmod'):
    add2(name, NAN, 2.0)
    checks.append(('nan', name))

prog = f'{ROOT}/build/mtest'
if not os.path.exists(prog):
    subprocess.run([f'{ROOT}/build.sh', 'tests/mtest.c'], cwd=ROOT, check=True, stdout=subprocess.DEVNULL)
env = dict(os.environ, XSIM_LIMIT='100000000000')
out = subprocess.run([sys.executable, f'{ROOT}/tools/xsim.py', prog], input='\n'.join(lines) + '\n',
                     capture_output=True, text=True, env=env)
got = out.stdout.splitlines()
fails = 0
worst = {}

def fail(msg):
    global fails
    fails += 1
    if fails <= 15: print('FAIL', msg)

for i, (line, c) in enumerate(zip(lines, checks)):
    if i >= len(got):
        fail(f'no output for {line}')
        break
    g = got[i].split()
    if c[0] == 'text':
        if got[i] != c[1]: fail(f'{line}: want {c[1]}, got {got[i]}')
        continue
    r = b2d(int(g[0], 16))
    if c[0] == 'nan':
        if r == r: fail(f'{line}: want a NaN, got {r!r}')
        continue
    if c[0] == 'exact':
        _, name, args, want, err = c
        if d2b(r) != d2b(want) or int(g[1]) != err:
            fail(f'{name}{args}: want {want!r} errno {err}, got {r!r} errno {g[1]}')
        continue
    _, name, bound, exact, args = c
    if exact == 0 or mpmath.isinf(exact) or abs(exact) > mpf(2) ** 1024:
        ok = (r == float(exact)) if abs(exact) <= mpf(2) ** 1024 else math.isinf(r)
        e = 0.0 if ok else 99.0
    else:
        e = float(abs(mpf(r) - exact) / ulp_of(exact))
    if e > worst.get(name, (0,))[0]: worst[name] = (e, args)
    if e > bound:
        fail(f'{name}{tuple(float(a) for a in args)}: {e:.3f} ulp (bound {bound}): got {r!r}, exact {mpmath.nstr(exact, 20)}')

errs = [l for l in out.stderr.splitlines() if l.endswith(' error')]
if errs != messages:
    fail(f'messages on stderr: want {messages}, got {errs}')
for name in sorted(worst):
    print(f'  {name:6s} worst {worst[name][0]:.3f} ulp')
print(f'{len(lines)} cases, {fails} failures: ' + ('ALL PASS' if not fails else 'FAILED'))
# XDS_DUMP=dir: keep this run's input and its checked output, for running
# the same program on real XENIX and comparing (tests/m21)
if os.environ.get('XDS_DUMP') and not fails:
    name = os.path.basename(prog)
    with open(os.path.join(os.environ['XDS_DUMP'], name + '.in'), 'w') as f: f.write('\n'.join(lines) + '\n')
    with open(os.path.join(os.environ['XDS_DUMP'], name + '.ok'), 'w') as f: f.write(out.stdout)
sys.exit(1 if fails else 0)
