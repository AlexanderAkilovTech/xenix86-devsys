/* atan.c - xenix86-devsys libm: atan(x) and atan2(y, x).
 * |x| is reduced to |t| < 7/16 with atan x = atan c + atan t for c = 0,
 * 1/2, 1, 3/2 or infinity (the atan c in double-double), then
 * atan t = t - t^3 A(t^2) (A from tools/mathcoef.py). Measured within 0.75
 * ulp; atan2 within 1.1. atan2(0, 0) is a DOMAIN error returning 0 (System V). */
#include "mathint.h"

static double A[] = {
	0.3333333333333333,
	-0.1999999999999941,
	0.14285714285566672,
	-0.1111111109663336,
	0.09090908355081977,
	-0.07692285541401948,
	0.06666241706297325,
	-0.05876944220330461,
	0.052166649790788854,
	-0.044921989571273566,
	0.03312674367384146,
	-0.014771803541259421,
};

static double hi[] = { 0.4636476090008061, 0.7853981633974483, 0.982793723247329, 1.5707963267948966 };
static double lo[] = { 2.2698777452961687e-17, 3.061616997868383e-17, 1.3903311031230998e-17, 6.123233995736766e-17 };
static double pih = 3.141592653589793, pil = 1.2246467991473532e-16;
static double eps29 = 1.862645149230957e-09;	/* 2^-29 */
static double big = 7.378697629483821e+19;	/* 2^66 */

double
atan(x)
double x;
{
	union _dw u;
	double ax, t, z, w;
	int id;

	u.d = x;
	if (_ISNAN(u))
		return x;
	ax = fabs(x);
	if (ax >= big)
		return x > 0.0 ? hi[3] + lo[3] : -hi[3] - lo[3];
	if (ax < 0.4375) {
		if (ax < eps29)
			return x;
		id = -1;
		t = x;
	} else if (ax < 0.6875) {
		id = 0;
		t = (2.0 * ax - 1.0) / (2.0 + ax);
	} else if (ax < 1.1875) {
		id = 1;
		t = (ax - 1.0) / (ax + 1.0);
	} else if (ax < 2.4375) {
		id = 2;
		t = (ax - 1.5) / (1.0 + 1.5 * ax);
	} else {
		id = 3;
		t = -1.0 / ax;
	}
	z = t * t;
	w = t * z * (A[0] + z * (A[1] + z * (A[2] + z * (A[3] + z * (A[4] + z * (A[5] +
		z * (A[6] + z * (A[7] + z * (A[8] + z * (A[9] + z * (A[10] + z * A[11])))))))))));
	if (id < 0)
		return t - w;
	z = hi[id] - ((w - lo[id]) - t);
	return x < 0.0 ? -z : z;
}

double
atan2(y, x)
double y, x;
{
	union _dw ux, uy;
	double z;
	int ix, iy, sy, sx;

	ux.d = x;
	uy.d = y;
	if (_ISNAN(ux) || _ISNAN(uy))
		return x + y;
	sy = (uy.w[3] & 0x8000) != 0;
	sx = (ux.w[3] & 0x8000) != 0;
	ix = _ISINF(ux);
	iy = _ISINF(uy);
	if (y == 0.0) {
		if (x == 0.0)
			return _merr(DOMAIN, "atan2", y, x, 0.0);
		z = sx ? pih : 0.0;		/* (+-0, x) */
	} else if (x == 0.0)
		z = hi[3] + lo[3];		/* (y, +-0) */
	else if (ix) {
		if (iy)
			z = sx ? 3.0 * hi[1] : hi[1];	/* (inf, inf) */
		else
			z = sx ? pih : 0.0;
	} else if (iy)
		z = hi[3] + lo[3];
	else {
		z = fabs(y / x);
		if (z > big)
			z = hi[3] + lo[3];
		else if (sx && z < 1.0 / big)
			z = 0.0;
		else
			z = atan(z);
		if (sx)				/* pi - z */
			z = pih - (z - pil);
	}
	return sy ? -z : z;
}
