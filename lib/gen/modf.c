/* modf.c - xenix86-devsys libc: modf(value, iptr): the integer part of value
 * (toward zero) in *iptr, and the fraction returned; both have value's
 * sign. An infinity has fraction 0; a NaN gives NaNs. */

double
modf(value, iptr)
double value, *iptr;
{
	union {
		double d;
		unsigned w[4];
	} u;
	register int e, bits, i;
	unsigned lost;

	u.d = value;
	e = ((u.w[3] >> 4) & 0x7FF) - 1023;	/* the unbiased exponent */
	if (e < 0) {				/* |value| < 1 */
		u.w[3] &= 0x8000;
		u.w[2] = u.w[1] = u.w[0] = 0;
		*iptr = u.d;
		return value;
	}
	if (e >= 52) {				/* an integer, an infinity, a NaN */
		*iptr = value;
		if (e == 1024 && ((u.w[3] & 0x000F) | u.w[2] | u.w[1] | u.w[0]))
			return value;
		u.w[3] &= 0x8000;
		u.w[2] = u.w[1] = u.w[0] = 0;
		return u.d;
	}
	lost = 0;				/* clear the 52 - e fraction bits */
	for (bits = 52 - e, i = 0; bits > 0; i++, bits -= 16)
		if (bits >= 16) {
			lost |= u.w[i];
			u.w[i] = 0;
		} else {
			lost |= u.w[i] & ((1 << bits) - 1);
			u.w[i] &= ~((1 << bits) - 1);
		}
	*iptr = u.d;
	if (lost == 0) {			/* an integer: fraction 0, signed */
		u.w[3] &= 0x8000;
		u.w[2] = u.w[1] = u.w[0] = 0;
		return u.d;
	}
	return value - u.d;			/* exact */
}
