/* log.c - xenix86-devsys libm: log(x) and log10(x). x = 2^k (1 + f),
 * sqrt(1/2) <= 1 + f < sqrt(2); with s = f/(2 + f),
 * log(1 + f) = log((1+s)/(1-s)) = 2s + s^3 L(s^2) (L fitted by
 * tools/mathcoef.py), arranged as f - (f^2/2 - s (f^2/2 + R)) to keep the
 * error small; then k ln2 in two parts. Measured within 0.7 ulp. log10, and
 * pow, use _logdd, the same in double-double (to about 2^-70), so that
 * log10 rounds correctly (log10 of a power of ten is exact). log(0) is a
 * SING error and log(x < 0) a DOMAIN error, both -HUGE. */
#include "mathint.h"

static double L[] = {
	0.6666666666666666,
	0.4000000000000088,
	0.2857142857080323,
	0.2222222239180047,
	0.18181795630884753,
	0.15386240205813478,
	0.13268759668560612,
	0.13086767009948175,
};

static double V[] = {			/* log tail: s^5 V(s^2) */
	0.40000000000000024,
	0.2857142857133851,
	0.2222222227110234,
	0.18181808247544543,
	0.15385574796462032,
	0.13285957435193352,
	0.1291361767218225,
};

static double ln2hi = 0.6931471803691238, ln2lo = 1.9082149292705877e-10;
static double sqrth = 0.7071067811865476;
static double tth = 0.6666666666666666, ttl = 3.700743415417188e-17;	/* 2/3 */
static double ivh = 0.4342944819032518, ivl = 1.098319650216765e-17;	/* 1/ln10 */

/* x > 0, finite: x = 2^k (1 + *fp) */
static int
reduce(x, fp)
double x, *fp;
{
	double m;
	int k;

	m = frexp(x, &k);
	if (m < sqrth) {
		m = m + m;
		k--;
	}
	*fp = m - 1.0;
	return k;
}

/* NaN, infinity, 0 and negatives: the value to return, or 0.0 with *ok set */
static double
special(name, x, ok)
char *name;
double x;
int *ok;
{
	union _dw u;

	u.d = x;
	*ok = 0;
	if (_ISNAN(u))
		return x;
	if (x <= 0.0)
		return _merr(x == 0.0 ? SING : DOMAIN, name, x, 0.0, -HUGE);
	if (_ISINF(u))
		return x;
	*ok = 1;
	return 0.0;
}

double
log(x)
double x;
{
	double f, s, z, R, hfsq, dk, v;
	int k, ok;

	v = special("log", x, &ok);
	if (!ok)
		return v;
	k = reduce(x, &f);
	s = f / (2.0 + f);
	z = s * s;
	R = z * (L[0] + z * (L[1] + z * (L[2] + z * (L[3] + z * (L[4] + z * (L[5] +
		z * (L[6] + z * L[7])))))));
	hfsq = 0.5 * f * f;
	dk = k;
	return dk * ln2hi + (f - (hfsq - (s * (hfsq + R) + dk * ln2lo)));
}

/* _logdd(x, lo): log(x) as a double-double, returning the high part; x > 0
 * and finite. log(1+f) = 2s + (2/3)s^3 + s^5 V(s^2), the first two terms
 * carried in double-double. */
double
_logdd(x, lo)
double x, *lo;
{
	double f, dh, dl, sh, sl, p, e, q, ql, c, cl, bh, bl, z, t, h, l, h2, l2, dk;
	int k;

	k = reduce(x, &f);
	_ddadd(2.0, f, &dh, &dl);		/* s = f / (2 + f) */
	sh = f / dh;
	_ddmul(sh, dh, &p, &e);
	sl = (((f - p) - e) - sh * dl) / dh;
	_ddmul(sh, sh, &q, &ql);		/* s^2 */
	ql += 2.0 * sh * sl;
	_ddmul(q, sh, &c, &cl);			/* s^3 */
	cl += q * sl + ql * sh;
	_ddmul(c, tth, &bh, &bl);		/* (2/3) s^3 */
	bl += c * ttl + cl * tth;
	z = q;
	t = sh * z * z * (V[0] + z * (V[1] + z * (V[2] + z * (V[3] + z * (V[4] +
		z * (V[5] + z * V[6]))))));
	_ddadd(2.0 * sh, bh, &h, &l);
	l += 2.0 * sl + bl + t;
	dk = k;
	_ddadd(dk * ln2hi, h, &h2, &l2);
	l2 += l + dk * ln2lo;
	_ddadd(h2, l2, &h, lo);
	return h;
}

double
log10(x)
double x;
{
	double h, l, p, e, v;
	int ok;

	v = special("log10", x, &ok);
	if (!ok)
		return v;
	h = _logdd(x, &l);
	_ddmul(h, ivh, &p, &e);
	return p + (e + (h * ivl + l * ivh));
}
