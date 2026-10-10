/* fmod.c - xenix86-devsys libm: fmod(x, y) = x - n y, n the integer part of
 * x / y, with the sign of x; exact (it always fits), by long division on
 * the mantissas. fmod(x, 0) is x, as System V has it; an infinite x or a
 * NaN gives a NaN. */
#include "mathint.h"

/* |x| = m * 2^e, m in m[0..3] (lowest word first); returns e */
static int
parts(x, m)
double x;
unsigned *m;
{
	union _dw u;
	int e;

	u.d = x;
	e = _EXPF(u);
	m[0] = u.w[0];
	m[1] = u.w[1];
	m[2] = u.w[2];
	m[3] = u.w[3] & 0x000F;
	m[4] = 0;
	if (e) {
		m[3] |= 0x0010;
		return e - 1075;
	}
	return -1074;
}

static int
len(m, n)
unsigned *m;
int n;
{
	while (n > 0 && m[n - 1] == 0)
		n--;
	return n;
}

double
fmod(x, y)
double x, y;
{
	union _dw ux, uy, r;
	unsigned mx[6], my[6];
	int ex, ey, nx, ny, k, L;
	unsigned v;

	ux.d = x;
	uy.d = y;
	if (_ISNAN(ux) || _ISNAN(uy) || _EXPF(ux) == 0x7FF)
		return (x - x) / (x - x);	/* NaN */
	if (((uy.w[3] & 0x7FFF) | uy.w[2] | uy.w[1] | uy.w[0]) == 0)
		return x;			/* y = 0 */
	if (_ISINF(uy) || fabs(x) < fabs(y))
		return x;
	ex = parts(x, mx);
	ey = parts(y, my);
	nx = len(mx, 5);
	ny = len(my, 5);
	for (k = ex - ey; ; k--) {
		if (_bncmp(mx, nx, my, ny) >= 0)
			nx = _bnsub(mx, nx, my, ny);
		if (k == 0)
			break;
		nx = _bnshl(mx, nx, 1);
	}
	/* the remainder: mx * 2^ey, less than |y|, so it fits exactly */
	r.w[3] = r.w[2] = r.w[1] = r.w[0] = 0;
	if (nx) {
		for (L = (nx - 1) * 16, v = mx[nx - 1]; v; v >>= 1)
			L++;			/* its bit length */
		k = L - 1 + ey + 1023;		/* the biased exponent */
		if (k >= 1)
			_bnshl(mx, nx, 53 - L);	/* the leading 1 to bit 52 */
		else {
			_bnshl(mx, nx, ey + 1074);	/* a denormal */
			k = 0;
		}
		r.w[0] = mx[0];
		r.w[1] = mx[1];
		r.w[2] = mx[2];
		r.w[3] = (mx[3] & 0x000F) | (k << 4);
	}
	r.w[3] |= ux.w[3] & 0x8000;
	return r.d;
}
