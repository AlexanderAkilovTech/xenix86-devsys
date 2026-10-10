/* fltcvt.c - xenix86-devsys libc: the decimal digits of a double, for
 * printf's e, f and g conversions and for ecvt, fcvt and gcvt (K&R C).
 *
 * The double is taken apart as bits and turned into digits with big
 * integers, so the digits are those of its exact value, rounded once (to
 * nearest, ties to even), and no floating point arithmetic is used: a
 * program that only prints doubles does not pull in the floating point
 * routines. */

/* BCC refers to __xfpcvt in a program that passes a double to a function
 * whose name ends in printf, which pulls this module in; otherwise printf
 * gets the stub _fltfmt in fltnone.c instead. build.sh makes this module
 * class 1 so that its _fltfmt wins over the stub's (tools/objclass.py). */
int	__xfpcvt;

#define	NW	80		/* words in a big number: room for any double */
#define	MAXDIG	520		/* digits produced at most */

static unsigned R[NW], S[NW], T[NW];
static int nr, ns;
static char dig[MAXDIG + 2];
static char text[850];		/* _fltfmt's result: 309 + 1 + 519 at most */

static int
bits(a, n)			/* the bit length of a big number */
unsigned *a;
int n;
{
	register unsigned v;
	register int b;

	if (n == 0)
		return 0;
	b = (n - 1) * 16;
	for (v = a[n - 1]; v; v >>= 1)
		b++;
	return b;
}

static int
mul10k(a, n, k)			/* a * 10^k */
unsigned *a;
int n, k;
{
	for (; k >= 4; k -= 4)
		n = _bnmul(a, n, 10000, 0);
	while (k-- > 0)
		n = _bnmul(a, n, 10, 0);
	return n;
}

static int
copy(d, s, n)
register unsigned *d, *s;
int n;
{
	register int i;

	for (i = 0; i < n; i++)
		d[i] = s[i];
	return n;
}

/* _fltdig(w, mode, n, buf, decpt): the decimal digits of the double whose
 * words (lowest first) are w[0..3], whatever its sign: n significant digits
 * for mode 'e', or as many as reach n places after the decimal point for
 * mode 'f'. The digits and a 0 go in buf (room for MAXDIG + 1); *decpt is
 * where the decimal point goes, the value being 0.digits x 10^*decpt.
 * Returns the number of digits, or -1 for an infinity or NaN. Zero gives
 * zeros with *decpt 0; in mode 'f' a value that rounds to 0 gives no
 * digits and *decpt = -n. */
int
_fltdig(w, mode, n, buf, decpt)
unsigned *w;
int mode, n;
char *buf;
int *decpt;
{
	int e, k, i, d, c, nd, nt;
	long t;

	e = (w[3] >> 4) & 0x7FF;
	if (e == 0x7FF)
		return -1;
	R[0] = w[0];
	R[1] = w[1];
	R[2] = w[2];
	R[3] = w[3] & 0x000F;
	if (e)
		R[3] |= 0x0010;		/* the leading 1 */
	else
		e = 1;			/* a denormal */
	e -= 1075;			/* the value is R * 2^e */
	for (nr = 4; nr > 0 && R[nr - 1] == 0; nr--)
		;
	if (n > MAXDIG)
		n = MAXDIG;
	if (nr == 0) {
		for (i = 0; i < n; i++)
			buf[i] = '0';
		buf[i] = 0;
		*decpt = 0;
		return i;
	}
	S[0] = 1;
	ns = 1;
	if (e >= 0)
		nr = _bnshl(R, nr, e);
	else
		ns = _bnshl(S, ns, -e);

	/* k = floor(log10(R/S)): estimate it from the bit lengths (within one
	 * either way), scale, and correct */
	t = (long)(bits(R, nr) - bits(S, ns) - 1) * 1233;
	k = t >= 0 ? (int)(t >> 12) : -(int)((-t + 4095) >> 12);
	if (k >= 0)
		ns = mul10k(S, ns, k);
	else
		nr = mul10k(R, nr, -k);
	for (;;) {			/* until 1 <= R/S < 10 */
		nt = copy(T, S, ns);
		nt = _bnmul(T, nt, 10, 0);
		if (_bncmp(R, nr, T, nt) >= 0) {
			ns = copy(S, T, nt);
			k++;
		} else if (_bncmp(R, nr, S, ns) < 0) {
			nr = _bnmul(R, nr, 10, 0);
			k--;
		} else
			break;
	}

	nd = mode == 'f' ? k + 1 + n : n;
	if (nd > MAXDIG)
		nd = MAXDIG;
	if (nd <= 0) {
		/* rounding at or above the first digit: 0, or 10^-n if this is
		 * more than half of that */
		nt = copy(T, S, ns);
		nt = _bnmul(T, nt, 5, 0);
		if (nd == 0 && _bncmp(R, nr, T, nt) > 0) {
			buf[0] = '1';
			buf[1] = 0;
			*decpt = 1 - n;
			return 1;
		}
		buf[0] = 0;
		*decpt = -n;
		return 0;
	}
	for (i = 0; i < nd; i++) {
		if (nr == 0) {		/* exact: the rest are zeros */
			buf[i] = '0';
			continue;
		}
		if (i)
			nr = _bnmul(R, nr, 10, 0);
		for (d = 0; _bncmp(R, nr, S, ns) >= 0; d++)
			nr = _bnsub(R, nr, S, ns);
		buf[i] = '0' + d;
	}
	/* round on what is left, R/S, against 1/2 */
	nr = _bnmul(R, nr, 2, 0);
	c = _bncmp(R, nr, S, ns);
	if (c > 0 || (c == 0 && (buf[nd - 1] & 1))) {
		for (i = nd - 1; i >= 0 && buf[i] == '9'; i--)
			buf[i] = '0';
		if (i >= 0)
			buf[i]++;
		else {			/* 99...9 became 100...0 */
			buf[0] = '1';
			k++;
			if (mode == 'f' && nd < MAXDIG)
				buf[nd++] = '0';
		}
	}
	buf[nd] = 0;
	*decpt = k + 1;
	return nd;
}

/* the digit at position i of 0.dig (0 outside the digits) */
#define	DIG(i)	((i) >= 0 && (i) < nd ? dig[i] : '0')

static char *
fixed(p, nd, decpt, frac, alt)	/* ddd.ddd */
register char *p;
int nd, decpt, frac, alt;
{
	register int i;

	if (decpt <= 0)
		*p++ = '0';
	for (i = 0; i < decpt; i++)
		*p++ = DIG(i);
	if (frac > 0 || alt)
		*p++ = '.';
	for (i = 0; i < frac; i++)
		*p++ = DIG(decpt + i);
	return p;
}

static char *
expo(p, nd, x, frac, alt, upper)	/* d.ddde+xx */
register char *p;
int nd, x, frac, alt, upper;
{
	register int i;

	*p++ = DIG(0);
	if (frac > 0 || alt)
		*p++ = '.';
	for (i = 1; i <= frac; i++)
		*p++ = DIG(i);
	*p++ = upper ? 'E' : 'e';
	if (x < 0) {
		*p++ = '-';
		x = -x;
	} else
		*p++ = '+';
	if (x >= 100) {
		*p++ = '0' + x / 100;
		x %= 100;
	}
	*p++ = '0' + x / 10;
	*p++ = '0' + x % 10;
	return p;
}

/* drop trailing zeros after a decimal point, and the point if nothing is
 * left after it, in the number ending at p (for %g) */
static char *
trim(start, p)
char *start;
register char *p;
{
	register char *q;

	for (q = start; q < p && *q != '.'; q++)
		;
	if (q == p)
		return p;
	while (p[-1] == '0')
		p--;
	if (p[-1] == '.')
		p--;
	return p;
}

/* _fltfmt(w, conv, prec, alt): the text of the double w[0..3] (lowest word
 * first) for printf conversion conv (e E f g G), with precision prec (-1:
 * the default) and the # flag alt, without its sign, in a static buffer
 * that the next call overwrites. */
char *
_fltfmt(w, conv, prec, alt)
unsigned *w;
int conv, prec, alt;
{
	register char *buf = text, *p = text;
	int nd, decpt, x, upper;

	upper = conv == 'E' || conv == 'G';
	if (prec < 0)
		prec = 6;
	if (prec > MAXDIG - 1)
		prec = MAXDIG - 1;
	if (((w[3] >> 4) & 0x7FF) == 0x7FF) {
		if ((w[3] & 0xF) | w[2] | w[1] | w[0])
			strcpy(buf, upper ? "NAN" : "nan");
		else
			strcpy(buf, upper ? "INF" : "inf");
		return buf;
	}
	switch (conv) {
	case 'f':
		nd = _fltdig(w, 'f', prec, dig, &decpt);
		p = fixed(p, nd, decpt, prec, alt);
		break;
	case 'e':
	case 'E':
		nd = _fltdig(w, 'e', prec + 1, dig, &decpt);
		x = dig[0] == '0' ? 0 : decpt - 1;
		p = expo(p, nd, x, prec, alt, upper);
		break;
	default:		/* g, G */
		if (prec == 0)
			prec = 1;
		nd = _fltdig(w, 'e', prec, dig, &decpt);
		x = dig[0] == '0' ? 0 : decpt - 1;
		if (x < -4 || x >= prec) {
			p = expo(p, nd, x, prec - 1, alt, upper);
			if (!alt) {
				char *e = p;

				while (e[-1] != 'e' && e[-1] != 'E')
					e--;
				e--;		/* the e */
				x = p - e;	/* the exponent's length */
				p = trim(buf, e);
				strncpy(p, e, x);
				p += x;
			}
		} else {
			if (dig[0] == '0')
				decpt = 1;
			p = fixed(p, nd, decpt, prec - 1 - x, alt);
			if (!alt)
				p = trim(buf, p);
		}
	}
	*p = 0;
	return buf;
}
