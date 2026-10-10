/* sinh.c - xenix86-devsys libm: sinh, cosh and tanh. Polynomials near 0
 * (tools/mathcoef.py), exp beyond, and exp of half the argument, squared,
 * just short of overflow. Measured within 1.5 ulp. Overflow is an OVERFLOW error
 * giving +-HUGE (errno ERANGE). */
#include "mathint.h"

static double SH[] = {
	0.16666666666666666,
	0.008333333333333333,
	0.00019841269841269773,
	2.7557319224040365e-06,
	2.5052108364049466e-08,
	1.6059048397722392e-10,
	7.646625253572627e-13,
	2.8445297426378895e-15,
};
static double CH[] = {
	0.041666666666666664,
	0.001388888888888889,
	2.4801587301587268e-05,
	2.7557319224013105e-07,
	2.087675697718064e-09,
	1.1470747876437706e-11,
	4.779208277073027e-14,
	1.578448430728322e-16,
};
static double TH[] = {
	-0.3333333333333333,
	0.1333333333333331,
	-0.05396825396822026,
	0.02186948853406759,
	-0.00886323546296844,
	0.003592126767036039,
	-0.0014558190673236184,
	0.000589905102351157,
	-0.00023847191162361473,
	9.454922606070154e-05,
	-3.3717730644899484e-05,
	7.872343957526295e-06,
};

static double xmax = 709.782712893384;		/* log(largest double) */
static double ymax = 710.4758600739439;		/* largest with cosh(x) finite */
static double eps28 = 3.725290298461914e-09;	/* 2^-28 */

double
sinh(x)
double x;
{
	union _dw u;
	double ax, t, z, w;

	u.d = x;
	if (_EXPF(u) == 0x7FF)
		return x;
	ax = fabs(x);
	if (ax < eps28)
		return x;
	if (ax <= 1.0) {
		z = x * x;
		return x + x * z * (SH[0] + z * (SH[1] + z * (SH[2] + z * (SH[3] + z * (SH[4] +
			z * (SH[5] + z * (SH[6] + z * SH[7])))))));
	}
	if (ax < 22.0) {
		t = exp(ax);
		t = 0.5 * t - 0.5 / t;
	} else if (ax <= xmax)
		t = 0.5 * exp(ax);
	else if (ax <= ymax) {
		w = exp(0.5 * ax);
		t = (0.5 * w) * w;
	} else
		return _merr(OVERFLOW, "sinh", x, 0.0, x > 0.0 ? HUGE : -HUGE);
	return x < 0.0 ? -t : t;
}

double
cosh(x)
double x;
{
	union _dw u;
	double ax, t, z, w;

	u.d = x;
	if (_ISNAN(u))
		return x;
	ax = fabs(x);
	if (_ISINF(u))
		return ax;
	if (ax <= 1.0) {
		z = x * x;
		return 1.0 + (0.5 * z + z * z * (CH[0] + z * (CH[1] + z * (CH[2] + z * (CH[3] +
			z * (CH[4] + z * (CH[5] + z * (CH[6] + z * CH[7]))))))));
	}
	if (ax < 22.0) {
		t = exp(ax);
		return 0.5 * t + 0.5 / t;
	}
	if (ax <= xmax)
		return 0.5 * exp(ax);
	if (ax <= ymax) {
		w = exp(0.5 * ax);
		return (0.5 * w) * w;
	}
	return _merr(OVERFLOW, "cosh", x, 0.0, HUGE);
}

double
tanh(x)
double x;
{
	union _dw u;
	double ax, t, z;

	u.d = x;
	if (_ISNAN(u))
		return x;
	ax = fabs(x);
	if (ax < eps28)
		return x;
	if (ax <= 0.55) {
		z = x * x;
		return x + x * z * (TH[0] + z * (TH[1] + z * (TH[2] + z * (TH[3] + z * (TH[4] +
			z * (TH[5] + z * (TH[6] + z * (TH[7] + z * (TH[8] + z * (TH[9] +
			z * (TH[10] + z * TH[11])))))))))));
	}
	if (ax < 22.0)
		t = 1.0 - 2.0 / (exp(2.0 * ax) + 1.0);
	else
		t = 1.0;
	return x < 0.0 ? -t : t;
}
