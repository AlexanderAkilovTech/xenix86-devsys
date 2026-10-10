/* asin.c - xenix86-devsys libm: asin(x) and acos(x).
 * |x| <= 1/2: asin x = x + x^3 R(x^2) (R from tools/mathcoef.py).
 * Above: asin x = pi/2 - 2 asin(s), s = sqrt((1 - x)/2), with s and the sum
 * carried in double-double; acos likewise. Measured within 0.6 ulp.
 * |x| > 1 is a DOMAIN error (returning 0, as System V does). */
#include "mathint.h"

static double R[] = {
	0.16666666666666666,
	0.07500000000000119,
	0.04464285714255019,
	0.030381944475693704,
	0.022372157435623815,
	0.017352816768546473,
	0.013963775787848137,
	0.011566511821890942,
	0.009621402747344831,
	0.009322101501082774,
	0.0030350071727267636,
	0.019579201619744684,
	-0.019277545928584553,
	0.029635026263548546,
};

static double pio2h = 1.5707963267948966, pio2l = 6.123233995736766e-17;
static double pih = 3.141592653589793, pil = 1.2246467991473532e-16;
static double eps27 = 7.450580596923828e-09;

static double
poly(z)
double z;
{
	return z * (R[0] + z * (R[1] + z * (R[2] + z * (R[3] + z * (R[4] + z * (R[5] +
		z * (R[6] + z * (R[7] + z * (R[8] + z * (R[9] + z * (R[10] + z * (R[11] +
		z * (R[12] + z * R[13])))))))))))));
}

/* asin(s) for s = sqrt(z), z = (1 - |x|)/2 <= 1/4, as *h + *l */
static
half(z, h, l)
double z, *h, *l;
{
	double sh, sl, p, e;

	if (z == 0.0) {				/* |x| = 1 */
		*h = *l = 0.0;
		return;
	}
	sh = sqrt(z);
	_ddmul(sh, sh, &p, &e);
	sl = ((z - p) - e) / (sh + sh);
	_ddadd(sh, sl + sh * poly(z), h, l);
}

static int
bad(name, x, v)
char *name;
double x, *v;
{
	union _dw u;

	u.d = x;
	if (_ISNAN(u)) {
		*v = x;
		return 1;
	}
	if (fabs(x) > 1.0) {
		*v = _merr(DOMAIN, name, x, 0.0, 0.0);
		return 1;
	}
	return 0;
}

double
asin(x)
double x;
{
	double ax, h, l, a, b, v;

	if (bad("asin", x, &v))
		return v;
	ax = fabs(x);
	if (ax < eps27)
		return x;
	if (ax <= 0.5)
		return x + x * poly(x * x);
	half(0.5 * (1.0 - ax), &h, &l);		/* pi/2 - 2 asin(s) */
	_ddadd(pio2h, -2.0 * h, &a, &b);
	v = a + (b + (pio2l - 2.0 * l));
	return x < 0.0 ? -v : v;
}

double
acos(x)
double x;
{
	double ax, h, l, a, b, v;

	if (bad("acos", x, &v))
		return v;
	ax = fabs(x);
	if (ax <= 0.5) {			/* pi/2 - asin x */
		_ddadd(pio2h, -x, &a, &b);
		return a + (b + (pio2l - x * poly(x * x)));
	}
	half(0.5 * (1.0 - ax), &h, &l);
	if (x > 0.0)				/* 2 asin(s) */
		return 2.0 * h + 2.0 * l;
	_ddadd(pih, -2.0 * h, &a, &b);		/* pi - 2 asin(s) */
	return a + (b + (pil - 2.0 * l));
}
