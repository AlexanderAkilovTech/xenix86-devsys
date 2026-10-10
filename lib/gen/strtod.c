/* strtod.c - xenix86-devsys libc: strtod, decimal text to double (K&R C).
 *
 * The result is the double nearest the decimal value (ties to even), exactly
 * as IEEE asks, however many digits there are: the digits become a big
 * integer, which is scaled by the power of ten and divided out with big
 * integers (bignum.s), and the bits are rounded once. Only the first 800
 * significant digits are kept; any others count only as "something more",
 * which is all the rounding needs (a halfway case between two doubles has
 * fewer). Too large a value gives an infinity, too small a nonzero one 0,
 * both setting errno to ERANGE. No floating point arithmetic is used. */
#include <ctype.h>
#include <errno.h>

#define	NW	250		/* words in a big number: 4000 bits */
#define	MAXDIG	800

static unsigned N[NW], D[NW];
static char dg[MAXDIG];
static int nn, nd2;

static int
bits(a, n)
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
mul10k(a, n, k)
unsigned *a;
int n, k;
{
	for (; k >= 4; k -= 4)
		n = _bnmul(a, n, 10000, 0);
	while (k-- > 0)
		n = _bnmul(a, n, 10, 0);
	return n;
}

/* the top 64 bits of N (with its bit length L) in m[0..3], and whether any
 * bit below them is set */
static int
top64(m, L)
unsigned *m;
int L;
{
	register int i, k, sticky = 0;

	if (L > 64) {
		k = (L - 64) >> 4;		/* whole words first */
		for (i = 0; i < k; i++)
			sticky |= N[i];
		for (i = k; i < nn; i++)
			N[i - k] = N[i];
		nn -= k;
		for (i = (L - 64) & 15; i > 0; i--) {
			sticky |= N[0] & 1;
			nn = _bnshr1(N, nn);
		}
		sticky = sticky != 0;
	} else
		nn = _bnshl(N, nn, 64 - L);
	for (i = 0; i < 4; i++)
		m[i] = i < nn ? N[i] : 0;
	return sticky;
}

/* the double with mantissa m[0..3] (top bit set), value m / 2^63 x 2^(e -
 * 1023), and sticky set if the true value is a little more, rounded */
static
pack(m, sticky, e, w)
register unsigned *m;
int sticky, e;
unsigned *w;
{
	int sh, half, i;

	if (e > 2046) {
		w[3] = 0x7FF0;		/* overflow: infinity */
		w[2] = w[1] = w[0] = 0;
		errno = ERANGE;
		return;
	}
	sh = 11;			/* 53 bits kept, more lost for a denormal */
	if (e < 1)
		sh += 1 - e;
	if (sh > 66)
		sh = 66;
	for (i = 1; i < sh; i++) {
		sticky |= m[0] & 1;
		_bnshr1(m, 4);
	}
	half = m[0] & 1;
	_bnshr1(m, 4);
	if (half && (sticky || (m[0] & 1))) {
		for (i = 0; i < 4 && ++m[i] == 0; i++)
			;
	}
	/* add the exponent less one: the leading 1, if any, adds the one */
	m[3] += (e < 1 ? 0 : e - 1) << 4;
	for (i = 0; i < 4; i++)
		w[i] = m[i];
	if ((w[3] & 0x7FF0) == 0x7FF0 || (w[3] | w[2] | w[1] | w[0]) == 0)
		errno = ERANGE;
}

double
strtod(str, endp)
char *str, **endp;
{
	union {
		double d;
		unsigned w[4];
	} u;
	unsigned m[4], q[5];
	register char *s = str;
	int neg = 0, nd = 0, dexp = 0, tail = 0, any = 0, ex, esign, i, c, sh, t, L, sticky;
	char *p;

	while (isspace(*s))
		s++;
	if (*s == '-' || *s == '+')
		neg = *s++ == '-';
	while (*s == '0') {
		s++;
		any = 1;
	}
	for (; isdigit(*s); s++) {
		any = 1;
		if (nd < MAXDIG)
			dg[nd++] = *s;
		else {
			if (*s != '0')
				tail = 1;
			if (dexp < 20000)
				dexp++;
		}
	}
	if (*s == '.') {
		s++;
		if (nd == 0)
			for (; *s == '0'; s++) {
				any = 1;
				if (dexp > -20000)
					dexp--;
			}
		for (; isdigit(*s); s++) {
			any = 1;
			if (nd < MAXDIG) {
				dg[nd++] = *s;
				dexp--;
			} else if (*s != '0')
				tail = 1;
		}
	}
	if (!any) {			/* no number at all */
		if (endp)
			*endp = str;
		return 0.0;
	}
	if (*s == 'e' || *s == 'E') {
		p = s + 1;
		esign = 0;
		if (*p == '-' || *p == '+')
			esign = *p++ == '-';
		if (isdigit(*p)) {
			for (ex = 0; isdigit(*p); p++)
				if (ex < 1000)
					ex = ex * 10 + *p - '0';
			dexp += esign ? -ex : ex;	/* within +-31000: no overflow */
			s = p;
		}
	}
	if (endp)
		*endp = s;

	while (nd > 0 && dg[nd - 1] == '0') {	/* trailing zeros: exponent */
		nd--;
		dexp++;
	}
	u.w[3] = u.w[2] = u.w[1] = u.w[0] = 0;
	if (nd == 0)
		goto done;			/* zero */
	if (nd + dexp > 310) {			/* over 1e310: too large */
		u.w[3] = 0x7FF0;
		errno = ERANGE;
		goto done;
	}
	if (nd + dexp < -326) {			/* under 1e-326: too small */
		errno = ERANGE;
		goto done;
	}

	nn = 0;					/* N = the digits, 4 at a time */
	for (i = 0; i < nd; ) {
		for (c = 0, t = 1; c < 4 && i < nd; c++, i++)
			t *= 10;
		for (ex = 0, c = i - c; c < i; c++)
			ex = ex * 10 + dg[c] - '0';
		nn = _bnmul(N, nn, t, ex);
	}
	if (dexp >= 0) {			/* an integer: N x 10^dexp */
		nn = mul10k(N, nn, dexp);
		L = bits(N, nn);
		sticky = top64(m, L) | tail;
		pack(m, sticky, L - 1 + 1023, u.w);
	} else {
		/* N / 10^-dexp: scale one or the other by a power of 2 so the
		 * quotient has 66 bits, then divide bit by bit */
		D[0] = 1;
		nd2 = mul10k(D, 1, -dexp);
		sh = 66 + bits(D, nd2) - bits(N, nn);
		if (sh > 0)
			nn = _bnshl(N, nn, sh);
		else if (sh < 0)
			nd2 = _bnshl(D, nd2, -sh);
		t = bits(N, nn) - bits(D, nd2);
		nd2 = _bnshl(D, nd2, t);
		for (i = 0; i < 5; i++)
			q[i] = 0;
		for (i = t; i >= 0; i--) {
			_bnmul(q, 5, 2, 0);
			if (_bncmp(N, nn, D, nd2) >= 0) {
				nn = _bnsub(N, nn, D, nd2);
				q[0] |= 1;
			}
			nd2 = _bnshr1(D, nd2);
		}
		sticky = nn != 0 || tail;
		/* q has L = 66 or 67 bits: keep the top 64 */
		L = bits(q, 5);
		for (i = L; i > 64; i--) {
			sticky |= q[0] & 1;
			_bnshr1(q, 5);
		}
		for (i = 0; i < 4; i++)
			m[i] = q[i];
		pack(m, sticky, L - 1 - sh + 1023, u.w);
	}
done:
	if (neg)
		u.w[3] |= 0x8000;
	return u.d;
}
