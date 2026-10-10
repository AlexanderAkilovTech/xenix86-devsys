"""mathcoef.py - derive the polynomial coefficients used by the math library
(lib/m), and check each kernel in double arithmetic against mpmath.

    python3 tools/mathcoef.py          prints the C tables and the error checks

Each approximation is a Chebyshev fit (near minimax) of a smooth function on
the reduced interval the library uses; the coefficients are rounded to
double, and the kernel is then evaluated in doubles, the way the C code does
it, at many points against 60-digit references. Needs mpmath."""
import struct
from mpmath import mp, mpf, chebyfit, sqrt, log, exp, sin, cos, tan, atan, asin, sinh, cosh, tanh, pi, coth

mp.dps = 60

def ulp(x):
    x = abs(float(x))
    if x == 0: return 2.0 ** -1074
    b = struct.unpack('<Q', struct.pack('<d', x))[0]
    return struct.unpack('<d', struct.pack('<Q', b + 1))[0] - x

def fit(f, a, b, n):
    """n coefficients, lowest power first, as doubles"""
    c, err = chebyfit(f, [mpf(a), mpf(b)], n, error=True)
    return [float(v) for v in reversed(c)], err

def poly(c, t):
    r = 0.0
    for v in reversed(c):
        r = r * t + v
    return r

def show(name, c, err, comment):
    print(f'/* {comment}; fit error {float(err):.2e} */')
    print(f'static double {name}[] = {{')
    for v in c:
        print(f'\t{v!r},')
    print('};\n')

def check(name, f, ref, pts):
    worst = max(abs(float(mpf(f(x)) - ref(mpf(x)))) / ulp(ref(mpf(x))) for x in pts if ref(mpf(x)) != 0)
    print(f'/* {name}: largest error {worst:.3f} ulp over {len(pts)} points */\n')

def grid(a, b, n=4000):
    return [a + (b - a) * i / (n - 1) for i in range(n)]

# exp: r * coth(r/2) = 2 + R(r^2), R(t) = t * P(t), on |r| <= ln2/2
T_EXP = float((log(2) / 2) ** 2)
P_exp, e = fit(lambda t: (sqrt(t) * coth(sqrt(t) / 2) - 2) / t if t else mpf(1) / 6, 0, T_EXP, 6)
show('P', P_exp, e, 'exp: r coth(r/2) - 2 = r^2 P(r^2), |r| <= ln2/2')
def kexp(r):
    t = r * r
    c = r - t * poly(P_exp, t)
    return 1.0 - ((r * c) / (c - 2.0) - r)
check('exp kernel', kexp, exp, grid(-0.3466, 0.3466))

# log: log(1+f) = 2s + s R(z), s = f/(2+f), z = s^2, R(z) = z * L(z)
S_MAX = float((sqrt(2) - 1) / (sqrt(2) + 1))
L_log, e = fit(lambda z: (log((1 + sqrt(z)) / (1 - sqrt(z))) - 2 * sqrt(z)) / (sqrt(z) * z) if z else mpf(2) / 3,
               0, S_MAX ** 2 * 1.0001, 8)
show('L', L_log, e, 'log: log((1+s)/(1-s)) - 2s = s z L(z), z = s^2, s <= (sqrt2-1)/(sqrt2+1)')
def klog(f):
    s = f / (2.0 + f)
    z = s * s
    R = z * poly(L_log, z)
    hfsq = 0.5 * f * f
    return f - (hfsq - s * (hfsq + R))
check('log kernel', klog, lambda f: log(1 + f), grid(0.7072 - 1, 1.4142 - 1))

# sin, cos on |r| <= pi/4
Z_PI4 = float((pi / 4) ** 2) * 1.0001
S_sin, e = fit(lambda z: (sin(sqrt(z)) / sqrt(z) - 1) / z if z else mpf(-1) / 6, 0, Z_PI4, 7)
show('S', S_sin, e, 'sin: sin r = r + r^3 S(r^2), |r| <= pi/4')
C_cos, e = fit(lambda z: (cos(sqrt(z)) - 1 + z / 2) / (z * z) if z else mpf(1) / 24, 0, Z_PI4, 6)
show('C', C_cos, e, 'cos: cos r = 1 - r^2/2 + r^4 C(r^2), |r| <= pi/4')
def ksin(x):
    z = x * x
    return x + x * z * poly(S_sin, z)
def kcos(x):
    z = x * x
    hz = 0.5 * z
    w = 1.0 - hz
    return w + (((1.0 - w) - hz) + z * z * poly(C_cos, z))
check('sin kernel', ksin, sin, grid(-0.7854, 0.7854))
check('cos kernel', kcos, cos, grid(-0.7854, 0.7854))

# tan on |r| <= 0.6744 (beyond, tan(pi/4 - r) = (1 - tan r)/(1 + tan r) is used)
Z_TAN = 0.6745 ** 2
T_tan, e = fit(lambda z: (tan(sqrt(z)) / sqrt(z) - 1) / z if z else mpf(1) / 3, 0, Z_TAN, 14)
show('T', T_tan, e, 'tan: tan r = r + r^3 T(r^2), |r| <= 0.6744')
def ktan(x):
    z = x * x
    return x + x * z * poly(T_tan, z)
check('tan kernel', ktan, tan, grid(-0.6744, 0.6744))

# atan on |t| <= 7/16: atan t = t - t z A(z), z = t^2
A_atan, e = fit(lambda z: (sqrt(z) - atan(sqrt(z))) / (sqrt(z) * z) if z else mpf(1) / 3, 0, (7 / 16) ** 2 * 1.0001, 12)
show('A', A_atan, e, 'atan: atan t = t - t^3 A(t^2), |t| <= 7/16')
def katan(t):
    z = t * t
    return t - t * z * poly(A_atan, z)
check('atan kernel', katan, atan, grid(-0.4375, 0.4375))

# asin on |x| <= 0.5: asin x = x + x z R(z), z = x^2 (and z <= 0.25 for the other half)
R_asin, e = fit(lambda z: (asin(sqrt(z)) / sqrt(z) - 1) / z if z else mpf(1) / 6, 0, 0.2501, 14)
show('R', R_asin, e, 'asin: asin x = x + x^3 R(x^2), x^2 <= 1/4')
def kasin(x):
    z = x * x
    return x + x * z * poly(R_asin, z)
check('asin kernel', kasin, asin, grid(-0.5, 0.5))

# sinh, cosh, tanh near 0
H_sinh, e = fit(lambda z: (sinh(sqrt(z)) / sqrt(z) - 1) / z if z else mpf(1) / 6, 0, 1.0001, 8)
show('SH', H_sinh, e, 'sinh: sinh x = x + x^3 SH(x^2), |x| <= 1')
K_cosh, e = fit(lambda z: (cosh(sqrt(z)) - 1 - z / 2) / (z * z) if z else mpf(1) / 24, 0, 1.0001, 8)
show('CH', K_cosh, e, 'cosh: cosh x = 1 + x^2/2 + x^4 CH(x^2), |x| <= 1')
N_tanh, e = fit(lambda z: (tanh(sqrt(z)) / sqrt(z) - 1) / z if z else mpf(-1) / 3, 0, 0.5501 ** 2, 12)
show('TH', N_tanh, e, 'tanh: tanh x = x + x^3 TH(x^2), |x| <= 0.55')
check('sinh kernel', lambda x: x + x * x * x * poly(H_sinh, x * x), sinh, grid(-1, 1))
check('cosh kernel', lambda x: 1.0 + (0.5 * x * x + x * x * x * x * poly(K_cosh, x * x)), cosh, grid(-1, 1))
check('tanh kernel', lambda x: x + x * x * x * poly(N_tanh, x * x), tanh, grid(-0.55, 0.55))

# log in double-double (pow, log10): log((1+s)/(1-s)) = 2s + (2/3)s^3 + s^5 V(s^2)
V_log, e = fit(lambda z: (log((1 + sqrt(z)) / (1 - sqrt(z))) - 2 * sqrt(z) - 2 * sqrt(z) ** 3 / 3) / sqrt(z) ** 5
               if z else mpf(2) / 5, 0, S_MAX ** 2 * 1.0001, 7)
show('V', V_log, e, 'log tail: log((1+s)/(1-s)) - 2s - 2s^3/3 = s^5 V(s^2)')

# constants as double-double pairs (high, the rest rounded)
def dd(name, v):
    h = float(v)
    print(f'/* {name} */\t{h!r}, {float(v - mpf(h))!r}')
dd('pi', pi); dd('pi/2', pi / 2); dd('pi/4', pi / 4); dd('2/3', mpf(2) / 3); dd('1/ln10', 1 / log(10))
dd('atan(1/2)', atan(mpf(1) / 2)); dd('atan(1)', atan(1)); dd('atan(3/2)', atan(mpf(3) / 2)); dd('atan(inf)', pi / 2)
