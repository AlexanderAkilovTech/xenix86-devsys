/* frexp.c - xenix86-devsys libc: frexp(value, eptr): value = m * 2^*eptr
 * with 0.5 <= |m| < 1; m is returned. Zero, infinities and NaNs come back
 * as they are, with *eptr 0. Exact: only the bits are moved. */

double
frexp(value, eptr)
double value;
int *eptr;
{
	union {
		double d;
		unsigned w[4];
	} u;
	register int e;
	unsigned sign;

	u.d = value;
	*eptr = 0;
	e = (u.w[3] >> 4) & 0x7FF;
	if (e == 0x7FF)
		return value;
	sign = u.w[3] & 0x8000;
	if (e == 0) {				/* zero, or a denormal */
		u.w[3] &= 0x000F;
		if ((u.w[3] | u.w[2] | u.w[1] | u.w[0]) == 0)
			return value;
		for (e = 1; !(u.w[3] & 0x0010); e--) {
			u.w[3] = (u.w[3] << 1) | (u.w[2] >> 15);
			u.w[2] = (u.w[2] << 1) | (u.w[1] >> 15);
			u.w[1] = (u.w[1] << 1) | (u.w[0] >> 15);
			u.w[0] <<= 1;
		}
	}
	*eptr = e - 1022;
	u.w[3] = (u.w[3] & 0x000F) | sign | (1022 << 4);
	return u.d;
}
