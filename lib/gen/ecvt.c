/* ecvt.c - xenix86-devsys libc: ecvt, fcvt and gcvt (System V), from the
 * exact digits fltcvt.c makes. ecvt and fcvt return a static string, which
 * the next call overwrites. */

char	*_fltfmt();

static char ebuf[530];

static char *
cvt(value, n, decpt, sign, mode)
double value;
int n, *decpt, *sign;
int mode;
{
	register unsigned *w = (unsigned *)&value;

	*sign = (w[3] & 0x8000) != 0;
	if (n < 0)
		n = 0;
	if (_fltdig(w, mode, n, ebuf, decpt) < 0) {
		*decpt = 0;
		return (w[3] & 0xF) | w[2] | w[1] | w[0] ? "nan" : "inf";
	}
	return ebuf;
}

/* ecvt: the first ndigit digits of value, rounded; *decpt is the position
 * of the decimal point from their start, *sign nonzero if value < 0 */
char *
ecvt(value, ndigit, decpt, sign)
double value;
int ndigit, *decpt, *sign;
{
	return cvt(value, ndigit, decpt, sign, 'e');
}

/* fcvt: the digits of value rounded to ndigit places after the point */
char *
fcvt(value, ndigit, decpt, sign)
double value;
int ndigit, *decpt, *sign;
{
	return cvt(value, ndigit, decpt, sign, 'f');
}

/* gcvt: value as printf's %.<ndigit>g would print it, in buf */
char *
gcvt(value, ndigit, buf)
double value;
int ndigit;
char *buf;
{
	register unsigned *w = (unsigned *)&value;
	register char *p = buf;

	if (w[3] & 0x8000)
		*p++ = '-';
	strcpy(p, _fltfmt(w, 'g', ndigit, 0));
	return buf;
}
