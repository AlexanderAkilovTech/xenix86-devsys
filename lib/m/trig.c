/* trig.c - xenix86-devsys libm: sin, cos, tan.
 *
 * x is reduced to n pi/2 + r, |r| <= pi/4, r carried as a double-double:
 * below 2^19 pi/2 with pi/2 in three parts (Cody and Waite), and where that
 * leaves too few bits (x very near a multiple of pi/2), or for larger x,
 * exactly, from 192 bits of 2/pi with big integers. Then polynomials on r
 * (tools/mathcoef.py): measured within 0.75 ulp for sin and cos, 1.5 for tan.
 * Beyond 1.4e16 System V reports TLOSS (total loss of significance) and
 * returns 0; an infinity or NaN gives a NaN. */
#include "mathint.h"

static double S[] = {
	-0.16666666666666666,
	0.008333333333333331,
	-0.00019841269841265063,
	2.7557319219337312e-06,
	-2.5052106231802837e-08,
	1.6058531516797758e-10,
	-7.586691094197958e-13,
};

static double C[] = {
	0.041666666666666664,
	-0.0013888888888887398,
	2.480158729876456e-05,
	-2.7557317271145144e-07,
	2.087614614655861e-09,
	-1.1382623647474604e-11,
};

static double T[] = {
	0.3333333333333333,
	0.13333333333333378,
	0.05396825396819062,
	0.02186948853970431,
	0.008863235426386609,
	0.0035921298401657823,
	0.0014558140975088557,
	0.0005901816934304232,
	0.00023831666653127416,
	9.990458319455929e-05,
	3.1660039521151534e-05,
	2.8962817088266603e-05,
	-7.532297785368857e-06,
	1.050162560079821e-05,
};

static double pio4h = 0.7853981633974483, pio4l = 3.061616997868383e-17;
static double pio2h = 1.5707963267948966, pio2l = 6.123233995736766e-17;
static double pio2_1 = 1.5707963267341256;	/* pi/2 in parts of 33, 33, 53 bits */
static double pio2_2 = 6.077100506303966e-11;
static double pio2_3 = 2.0222662487959506e-21;
static double twobypi = 0.6366197723675814;
static double medium = 823550.0;		/* 2^19 pi/2 */
static double tloss = 1.414847550405688e+16;
static double small = 9.313225746154785e-10;	/* 2^-30 */
static double eps27 = 7.450580596923828e-09;	/* 2^-27 */

/* 2/pi * 2^192, the lowest word first */
static unsigned G[12] = {
	0x9041, 0x3C43, 0x9599, 0xDB62, 0xDDC0, 0xF534,
	0x57D1, 0xFC27, 0x1529, 0x4E44, 0x836E, 0xA2F9,
};

#define	NP	18
#define	BIT(a, k)	(((a)[(k) >> 4] >> ((k) & 15)) & 1)

static int
blen(a, n)		/* the bit length of a big number of n words */
unsigned *a;
int n;
{
	unsigned v;
	int b;

	while (n > 0 && a[n - 1] == 0)
		n--;
	if (n == 0)
		return 0;
	for (b = (n - 1) * 16, v = a[n - 1]; v; v >>= 1)
		b++;
	return b;
}

/* exact reduction of ax (>= pi/4, < 2^54): ax * 2/pi = M G 2^(e - 192) */
static long
bigrem(ax, y, yy)
double ax, *y, *yy;
{
	union _dw u;
	unsigned m[4], P[NP], F[NP];
	int e, s, i, L, neg, nf;
	long n;
	double h, l, t, d, p, q;

	u.d = ax;
	e = _EXPF(u) - 1075;
	m[0] = u.w[0];
	m[1] = u.w[1];
	m[2] = u.w[2];
	m[3] = (u.w[3] & 0x000F) | 0x0010;
	for (i = 0; i < NP; i++)
		P[i] = F[i] = 0;
	for (i = 0; i < 4; i++)
		_bnmac(P + i, G, 12, m[i]);
	s = 192 - e;				/* the binary point: bit s */
	n = BIT(P, s) + 2 * BIT(P, s + 1);	/* n mod 4 */
	for (i = 0; i < (s >> 4); i++)		/* the fraction */
		F[i] = P[i];
	F[s >> 4] = P[s >> 4] & ((1 << (s & 15)) - 1);
	neg = BIT(P, s - 1);
	if (neg) {				/* f >= 1/2: n + 1, f - 1 */
		n++;
		for (i = 0; i < NP; i++)
			P[i] = 0;
		P[s >> 4] = 1 << (s & 15);
		nf = blen(F, NP);
		_bnsub(P, (s >> 4) + 1, F, (nf + 15) >> 4);
		for (i = 0; i < NP; i++)
			F[i] = P[i];
	}
	L = blen(F, NP);
	if (L == 0) {
		*y = *yy = 0.0;
		return n;
	}
	nf = (L + 15) >> 4;
	if (L > 128)				/* the top 128 bits, at 0..127 */
		for (i = L - 128; i > 0; i--)
			nf = _bnshr1(F, nf);
	else
		nf = _bnshl(F, nf, 128 - L);
	h = l = 0.0;
	for (i = 7; i >= 0; i--) {
		_ddadd(h * 65536.0, (double)F[i], &h, &d);
		l = l * 65536.0 + d;
	}
	_ddadd(h, l, &h, &l);
	h = ldexp(h, L - 128 - s);
	l = ldexp(l, L - 128 - s);
	_ddmul(h, pio2h, &p, &q);		/* r = f pi/2 */
	q += h * pio2l + l * pio2h;
	_ddadd(p, q, y, yy);
	if (neg) {
		*y = -*y;
		*yy = -*yy;
	}
	return n;
}

/* the quadrant n mod 4, and *y + *yy = x - n pi/2, |*y| <= pi/4 about */
static int
rem(x, y, yy)
double x, *y, *yy;
{
	double ax, fn, r1, h, e, w;
	long n;

	ax = fabs(x);
	if (ax <= pio4h) {
		*y = x;
		*yy = 0.0;
		return 0;
	}
	n = -1;
	if (ax < medium) {
		n = ax * twobypi + 0.5;
		fn = n;
		r1 = ax - fn * pio2_1;		/* exact */
		_ddadd(r1, -(fn * pio2_2), &h, &e);
		w = fn * pio2_3;
		_ddadd(h, e - w, y, yy);
		if (fabs(*y) < small)		/* too close to n pi/2 */
			n = -1;
	}
	if (n < 0)
		n = bigrem(ax, y, yy);
	if (x < 0.0) {
		*y = -*y;
		*yy = -*yy;
		n = -n;
	}
	return (int)n & 3;
}

/* sin and cos of x + y, |x| <= pi/4, y a small correction */
static double
ksin(x, y)
double x, y;
{
	double z;

	z = x * x;
	return x + (x * z * (S[0] + z * (S[1] + z * (S[2] + z * (S[3] + z * (S[4] +
		z * (S[5] + z * S[6])))))) + y * (1.0 - 0.5 * z));
}

static double
kcos(x, y)
double x, y;
{
	double z, hz, w;

	z = x * x;
	hz = 0.5 * z;
	w = 1.0 - hz;
	return w + (((1.0 - w) - hz) + (z * z * (C[0] + z * (C[1] + z * (C[2] + z * (C[3] +
		z * (C[4] + z * C[5]))))) - x * y));
}

/* tan of x + y, or -1/tan for odd */
static double
ktan(x, y, odd)
double x, y;
int odd;
{
	double z, r, d, t, q, p, e;
	int big = 0, neg = 0;

	if (fabs(x) > 0.6744) {			/* tan(pi/4 - w) = (1 - tan w)/(1 + tan w) */
		if (x < 0.0) {
			x = -x;
			y = -y;
			neg = 1;
		}
		x = (pio4h - x) + (pio4l - y);
		y = 0.0;
		big = 1;
	}
	z = x * x;
	r = x * z * (T[0] + z * (T[1] + z * (T[2] + z * (T[3] + z * (T[4] + z * (T[5] +
		z * (T[6] + z * (T[7] + z * (T[8] + z * (T[9] + z * (T[10] + z * (T[11] +
		z * (T[12] + z * T[13])))))))))))));
	d = r + y * (1.0 + z);
	t = x + d;
	if (big)
		t = odd ? -(1.0 + t) / (1.0 - t) : (1.0 - t) / (1.0 + t);
	else if (odd) {				/* -1/t, with t's rounding error */
		d -= t - x;
		q = -1.0 / t;
		_ddmul(q, t, &p, &e);
		e = ((1.0 + p) + e) + q * d;
		t = q + q * e;
	}
	return neg ? -t : t;
}

/* NaN and infinities give a NaN, very large values TLOSS: *ok = 0 */
static double
check(name, x, ok)
char *name;
double x;
int *ok;
{
	union _dw u;

	u.d = x;
	*ok = 0;
	if (_EXPF(u) == 0x7FF)
		return x - x;
	if (fabs(x) > tloss)
		return _merr(TLOSS, name, x, 0.0, 0.0);
	*ok = 1;
	return x;
}

double
sin(x)
double x;
{
	double y, yy;
	int ok;

	x = check("sin", x, &ok);
	if (!ok || fabs(x) < eps27)
		return x;
	switch (rem(x, &y, &yy)) {
	case 0:
		return ksin(y, yy);
	case 1:
		return kcos(y, yy);
	case 2:
		return -ksin(y, yy);
	}
	return -kcos(y, yy);
}

double
cos(x)
double x;
{
	double y, yy;
	int ok;

	x = check("cos", x, &ok);
	if (!ok)
		return x;
	if (fabs(x) < eps27)
		return 1.0;
	switch (rem(x, &y, &yy)) {
	case 0:
		return kcos(y, yy);
	case 1:
		return -ksin(y, yy);
	case 2:
		return -kcos(y, yy);
	}
	return ksin(y, yy);
}

double
tan(x)
double x;
{
	double y, yy;
	int ok, n;

	x = check("tan", x, &ok);
	if (!ok || fabs(x) < eps27)
		return x;
	n = rem(x, &y, &yy);
	return ktan(y, yy, n & 1);
}
