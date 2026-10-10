/* hypot.c - xenix86-devsys libm: hypot(x, y) = sqrt(x^2 + y^2) without
 * overflow or underflow on the way, the sum carried in double-double and
 * the root corrected once: measured within 0.5 ulp. An infinity wins over a
 * NaN. Overflow is an OVERFLOW error giving HUGE. cabs(z) is hypot of a
 * struct { double x, y; }. */
#include "mathint.h"

struct complex {
	double	x, y;
};

static double big = 3.273390607896142e+150;	/* 2^500: squares stay finite */
static double small = 3.4395525670743494e-136;	/* 2^-450: and well above denormals */
static double far = 1.152921504606847e+18;	/* 2^60 */

double
hypot(x, y)
double x, y;
{
	union _dw ux, uy;
	double a, b, t, ah, al, bh, bl, sh, sl, h, p, e;
	int k;

	ux.d = x;
	uy.d = y;
	if (_ISINF(ux))
		return fabs(x);
	if (_ISINF(uy))
		return fabs(y);
	if (_ISNAN(ux) || _ISNAN(uy))
		return x + y;
	a = fabs(x);
	b = fabs(y);
	if (a < b) {
		t = a;
		a = b;
		b = t;
	}
	if (b == 0.0 || a > far * b)
		return a + b;
	k = 0;
	if (a > big) {
		a = ldexp(a, -600);
		b = ldexp(b, -600);
		k = 600;
	} else if (b < small) {
		a = ldexp(a, 600);
		b = ldexp(b, 600);
		k = -600;
	}
	_ddmul(a, a, &ah, &al);
	_ddmul(b, b, &bh, &bl);
	_ddadd(ah, bh, &sh, &sl);
	sl += al + bl;
	h = sqrt(sh);
	_ddmul(h, h, &p, &e);
	h += (((sh - p) - e) + sl) / (h + h);
	if (k)
		h = ldexp(h, k);
	ux.d = h;
	if (_ISINF(ux))
		return _merr(OVERFLOW, "hypot", x, y, HUGE);
	return h;
}

double
cabs(z)
struct complex z;
{
	return hypot(z.x, z.y);
}
