/* sqrt.c - xenix86-devsys libm: sqrt(x), correctly rounded (to nearest,
 * ties to even) as IEEE requires: an integer square root of the mantissa,
 * bit by bit, with big integers. sqrt(-0) is -0; for x < 0 System V reports
 * a DOMAIN error and returns 0. */
#include "mathint.h"

#define	NW	8

static int
addbit(a, n, p)			/* a += 2^p; returns the new length */
unsigned *a;
int n, p;
{
	register int i = p >> 4;
	unsigned b = 1 << (p & 15);

	if ((a[i] += b) < b)
		while (++a[++i] == 0)
			;
	return i + 1 > n ? i + 1 : n;
}

double
sqrt(x)
double x;
{
	union _dw u;
	unsigned n[NW], res[NW], t[NW];
	int e, nn, nr, nt, p, i, sticky, half;

	u.d = x;
	if (_ISNAN(u))
		return x;
	if (((u.w[3] & 0x7FFF) | u.w[2] | u.w[1] | u.w[0]) == 0)
		return x;				/* +0, -0 */
	if (u.w[3] & 0x8000)
		return _merr(DOMAIN, "sqrt", x, 0.0, 0.0);
	if (_ISINF(u))
		return x;

	/* x = m * 2^e, 2^52 <= m < 2^53 */
	e = _EXPF(u);
	for (i = 0; i < NW; i++)
		n[i] = res[i] = 0;
	n[0] = u.w[0];
	n[1] = u.w[1];
	n[2] = u.w[2];
	n[3] = u.w[3] & 0x000F;
	if (e) {
		n[3] |= 0x0010;
		e -= 1075;
	} else {				/* a denormal: normalize */
		for (e = -1074; !(n[3] & 0x0010); e--)
			_bnshl(n, 4, 1);
	}
	/* make e even, then N = m * 2^56 (or 2^57): its root has 55 bits */
	p = 56;
	if (e & 1) {
		p++;
		e--;
	}
	nn = _bnshl(n, 4, p);
	e = (e - 56) / 2;			/* the root's exponent */

	/* res = isqrt(N): bit = the largest power of 4 <= N */
	for (p = (nn - 1) * 16, i = n[nn - 1]; i; i = (unsigned)i >> 1)
		p++;
	p = (p - 1) & ~1;
	nr = 0;
	for (; p >= 0; p -= 2) {
		/* t = res + bit (res is a multiple of 2 bit: no carry) */
		for (i = 0; i < NW; i++)
			t[i] = res[i];
		t[p >> 4] |= 1 << (p & 15);
		nt = (p >> 4) + 1 > nr ? (p >> 4) + 1 : nr;
		if (_bncmp(n, nn, t, nt) >= 0) {	/* res = res/2 + bit */
			nn = _bnsub(n, nn, t, nt);
			nr = addbit(res, _bnshr1(res, nr), p);
		} else
			nr = _bnshr1(res, nr);
	}
	sticky = nn != 0;

	/* res has 55 bits: round off two */
	sticky |= res[0] & 1;
	_bnshr1(res, 4);
	half = res[0] & 1;
	_bnshr1(res, 4);
	e += 2;
	if (half && (sticky || (res[0] & 1))) {
		for (i = 0; i < 4 && ++res[i] == 0; i++)
			;
		if (res[3] & 0x0020) {		/* carried to 2^53 */
			_bnshr1(res, 4);
			e++;
		}
	}
	/* the result: res * 2^e, res with its leading 1 at bit 52 */
	u.w[0] = res[0];
	u.w[1] = res[1];
	u.w[2] = res[2];
	u.w[3] = (res[3] & 0x000F) | ((e + 1075) << 4);
	return u.d;
}
