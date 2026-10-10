/* ldexp.c - xenix86-devsys libc: ldexp(value, n) = value * 2^n, rounded once
 * if it lands among the denormals. Too large gives an infinity, a nonzero
 * value too small for any double gives 0; both set errno to ERANGE. */
#include <errno.h>

double	frexp();

static unsigned two_m1022[4] = { 0, 0, 0, 0x0010 };	/* 2^-1022 */

double
ldexp(value, n)
double value;
int n;
{
	union {
		double d;
		unsigned w[4];
	} u;
	int k;
	long e;

	u.d = value;
	if (((u.w[3] >> 4) & 0x7FF) == 0x7FF)
		return value;			/* infinity, NaN */
	if (((u.w[3] & 0x7FFF) | u.w[2] | u.w[1] | u.w[0]) == 0)
		return value;			/* zero */
	u.d = frexp(value, &k);			/* m * 2^k, 0.5 <= |m| < 1 */
	e = (long)k + n + 1022;			/* the biased exponent of m * 2^(k + n) */
	if (e > 2046) {
		errno = ERANGE;
		u.w[3] = (u.w[3] & 0x8000) | 0x7FF0;
		u.w[2] = u.w[1] = u.w[0] = 0;
		return u.d;
	}
	if (e >= 1) {
		u.w[3] = (u.w[3] & 0x800F) | ((unsigned)e << 4);
		return u.d;
	}
	/* a denormal: make it m * 2^(k + n + 1022), still normal, then one
	 * multiplication by 2^-1022 rounds it */
	if (e + 1022 < 1) {
		errno = ERANGE;
		u.w[3] &= 0x8000;
		u.w[2] = u.w[1] = u.w[0] = 0;
		return u.d;
	}
	u.w[3] = (u.w[3] & 0x800F) | ((unsigned)(e + 1022) << 4);
	u.d = u.d * *(double *)two_m1022;
	if (((u.w[3] & 0x7FFF) | u.w[2] | u.w[1] | u.w[0]) == 0)
		errno = ERANGE;
	return u.d;
}
