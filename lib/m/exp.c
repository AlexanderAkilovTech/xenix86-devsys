/* exp.c - xenix86-devsys libm: exp(x). x = k ln2 + r, |r| <= ln2/2, with ln2
 * in two parts so that k ln2 is exact; then e^r from a rational form:
 * r coth(r/2) = 2 + r^2 P(r^2) (P fitted by tools/mathcoef.py), and
 * e^r = 1 + 2r / (2 - c), c = r - r^2 P(r^2); then 2^k. Measured within 0.8
 * ulp. Too large: OVERFLOW, HUGE; too small: UNDERFLOW, 0 (errno ERANGE). */
#include "mathint.h"

static double P[] = {
	0.16666666666666666,
	-0.0027777777777777545,
	6.613756613530131e-05,
	-1.6534390729514401e-06,
	4.1752220525280315e-08,
	-1.0472482723285645e-09,
};

static double ln2hi = 0.6931471803691238;	/* 32 bits: k ln2hi is exact */
static double ln2lo = 1.9082149292705877e-10;
static double invln2 = 1.4426950408889634;
static double xmax = 709.782712893384;		/* log(largest double) */
static double xmin = -745.1332191019411;	/* log(smallest denormal / 2) */
static double tiny = 5.551115123125783e-17;	/* 2^-54 */

/* _expdd(x, xl): e^(x + xl), xl a small correction to x (pow passes
 * the low part of a double-double) */
double
_expdd(x, xl)
double x, xl;
{
	double hi, lo, r, t, c, y;
	int k;

	k = x * invln2 + (x < 0.0 ? -0.5 : 0.5);
	hi = x - k * ln2hi;
	lo = k * ln2lo - xl;
	r = hi - lo;
	t = r * r;
	c = r - t * (P[0] + t * (P[1] + t * (P[2] + t * (P[3] + t * (P[4] + t * P[5])))));
	y = 1.0 - ((lo - (r * c) / (2.0 - c)) - hi);
	return k ? ldexp(y, k) : y;
}

double
exp(x)
double x;
{
	union _dw u;

	u.d = x;
	if (_ISNAN(u))
		return x;
	if (x > xmax) {
		if (_ISINF(u))
			return x;
		return _merr(OVERFLOW, "exp", x, 0.0, HUGE);
	}
	if (x < xmin) {
		if (_ISINF(u))
			return 0.0;
		return _merr(UNDERFLOW, "exp", x, 0.0, 0.0);
	}
	if (fabs(x) < tiny)
		return 1.0 + x;
	return _expdd(x, 0.0);
}
