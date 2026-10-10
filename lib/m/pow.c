/* pow.c - xenix86-devsys libm: pow(x, y) = x^y.
 *
 * For an integer y up to 2^15 in size, by repeated squaring in
 * double-double, so that results which are exact (2^10, 10^22, 3^20) come
 * out exact and the rest are correctly rounded almost always. Otherwise
 * e^(y log x) with log x in double-double (_logdd) and y log x carried into
 * exp's reduction (_expdd): measured within 0.8 ulp.
 *
 * System V's errors: pow(0, y <= 0) and pow(x < 0, y not an integer) are
 * DOMAIN errors returning 0; overflow gives +-HUGE and underflow 0
 * (errno ERANGE). IEEE's rules otherwise: pow(x, 0) = 1, pow(1, y) = 1,
 * infinities and NaNs as C99 has them. */
#include "mathint.h"

static double xmax = 709.782712893384;
static double xmin = -745.1332191019411;
static double big = 1.0715086071862673e+301;	/* 2^1000 */
static double small = 1.0e-280;

/* what kind of number y is: 0 not an integer, 1 an odd integer, 2 even */
static int
kind(y)
double y;
{
	union _dw u;
	int e, bit;

	u.d = y;
	e = _EXPF(u) - 1023;			/* the unbiased exponent */
	if (e < 0)
		return (u.w[3] & 0x7FFF) | u.w[2] | u.w[1] | u.w[0] ? 0 : 2;
	if (e >= 53)
		return 2;			/* (and infinities) */
	if (e == 0)				/* 1 <= |y| < 2: the units bit */
		return (u.w[3] & 0xF) | u.w[2] | u.w[1] | u.w[0] ? 0 : 1;	/* is the hidden 1 */
	bit = 52 - e;				/* the units bit */
	/* the bits below it must be 0 */
	if (bit >= 16 && (u.w[0] || (bit >= 32 && u.w[1]) || (bit >= 48 && u.w[2])))
		return 0;
	if (u.w[bit >> 4] & ((1 << (bit & 15)) - 1))
		return 0;
	return u.w[bit >> 4] & (1 << (bit & 15)) ? 1 : 2;
}

/* x^n for an integer n, 0 < n < 2^15, in double-double; 0 if it leaves the
 * double range on the way (the caller then uses logarithms) */
static int
ipow(x, n, rh, rl)
double x;
int n;
double *rh, *rl;
{
	double h = 1.0, l = 0.0, bh = x, bl = 0.0, p, e;

	for (;;) {
		if (n & 1) {			/* r = r * b */
			_ddmul(h, bh, &p, &e);
			e += h * bl + l * bh;
			_ddadd(p, e, &h, &l);
		}
		if ((n >>= 1) == 0)
			break;
		_ddmul(bh, bh, &p, &e);		/* b = b * b */
		e += 2.0 * bh * bl;
		_ddadd(p, e, &bh, &bl);
		if (bh > big || bh < small)
			return 0;
	}
	if (h > big || h < small)
		return 0;
	*rh = h;
	*rl = l;
	return 1;
}

double
pow(x, y)
double x, y;
{
	union _dw ux, uy;
	double ax, h, l, th, tl, p, e, q, r;
	int k, neg, n;

	ux.d = x;
	uy.d = y;
	ax = fabs(x);
	if (x == 0.0 && !_ISNAN(uy) && y <= 0.0)
		return _merr(DOMAIN, "pow", x, y, 0.0);
	if (y == 0.0 || x == 1.0)
		return 1.0;
	if (_ISNAN(ux) || _ISNAN(uy))
		return x + y;
	k = kind(y);
	neg = (ux.w[3] & 0x8000) && k == 1;	/* an odd power of a negative */
	if (_ISINF(uy)) {
		if (ax == 1.0)
			return 1.0;
		return (ax > 1.0) == (y > 0.0) ? y * y : 0.0;	/* inf or 0 */
	}
	if (_ISINF(ux) || x == 0.0) {		/* 0 or inf, by the sign of y */
		r = (y > 0.0) == (x == 0.0) ? 0.0 : ax;
		return neg ? -r : r;
	}
	if (x < 0.0 && k == 0)
		return _merr(DOMAIN, "pow", x, y, 0.0);

	if (k && fabs(y) < 32768.0) {		/* an integer power */
		n = fabs(y);
		if (ipow(ax, n, &h, &l)) {
			if (y < 0.0) {		/* 1 / (h + l) */
				q = 1.0 / h;
				_ddmul(q, h, &p, &e);
				r = ((1.0 - p) - e) - q * l;
				h = q + q * r;
			}
			return neg ? -h : h;
		}
	}
	if (y == 0.5)
		return sqrt(x);			/* exact when it can be */
	h = _logdd(ax, &l);			/* y log |x| */
	_ddmul(y, h, &th, &tl);
	tl += y * l;
	_ddadd(th, tl, &th, &tl);
	if (th > xmax + 1.0)
		return _merr(OVERFLOW, "pow", x, y, neg ? -HUGE : HUGE);
	if (th < xmin - 1.0)
		return _merr(UNDERFLOW, "pow", x, y, neg ? -0.0 : 0.0);
	r = _expdd(th, tl);
	ux.d = r;
	if (_ISINF(ux))
		return _merr(OVERFLOW, "pow", x, y, neg ? -HUGE : HUGE);
	if (r == 0.0)
		return _merr(UNDERFLOW, "pow", x, y, neg ? -0.0 : 0.0);
	return neg ? -r : r;
}
