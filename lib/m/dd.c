/* dd.c - xenix86-devsys libm: exact products and sums of doubles, as pairs
 * (Dekker's and Knuth's algorithms, which need only correctly rounded
 * arithmetic, as both the software routines and the 8087 give). pow, log10,
 * hypot and the trigonometric reduction use them for extra precision. */

/* a = *hi + *lo, each with at most 26 significant bits (|a| < 2^995) */
_ddsplit(a, hi, lo)
double a, *hi, *lo;
{
	double c;

	c = 134217729.0 * a;		/* 2^27 + 1 */
	*hi = c - (c - a);
	*lo = a - *hi;
}

/* a * b = *p + *e exactly */
_ddmul(a, b, p, e)
double a, b, *p, *e;
{
	double ah, al, bh, bl;

	*p = a * b;
	_ddsplit(a, &ah, &al);
	_ddsplit(b, &bh, &bl);
	*e = ((ah * bh - *p) + ah * bl + al * bh) + al * bl;
}

/* a + b = *s + *e exactly */
_ddadd(a, b, s, e)
double a, b, *s, *e;
{
	double v;

	*s = a + b;
	v = *s - a;
	*e = (a - (*s - v)) + (b - v);
}
