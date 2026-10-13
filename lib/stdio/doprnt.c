/* doprnt.c - xenix86-devsys libc: printf, fprintf, sprintf (K&R C)
 * Conversions: d i u o x X c s e E f g G %, with the l (long) modifier, flags
 * - 0 + space #, field width and precision (* for either). The floating point
 * conversions print the double's exact value correctly rounded (fltcvt.c),
 * and inf or nan for those. As in System V, each returns the number of
 * characters written (EOF after an output error).
 *
 * Speed matters on a 4.77 MHz 8088: the text between conversions, the
 * converted strings and the padding go into the stream's buffer whole, with
 * memcpy and memset, when the buffer has room for them, rather than a putc
 * for each character; and numbers that fit in 16 bits are converted with the
 * 8086's own division rather than the 32-bit routines. */
#include <stdio.h>

char	*_fltfmt(), *memchr();

static int nout;		/* characters written by the current _doprnt */

/* n characters of s, or n copies of *s when rep, into f */
static
put(s, n, rep, f)
register char *s;
register int n;
int rep;
register FILE *f;
{
	if (n <= 0)
		return;
	nout += n;
	/* room in the buffer, and no newline for a line-buffered stream
	 * (whose _cnt is the room left only after _flsbuf has set it) */
	if (f->_cnt >= n && (!(f->_flag & _IOLBF) || !rep && memchr(s, '\n', n) == NULL)) {
		if (rep)
			memset(f->_ptr, *s, n);
		else
			memcpy(f->_ptr, s, n);
		f->_ptr += n;
		f->_cnt -= n;
		return;
	}
	while (n-- > 0) {
		putc(*s, f);
		if (!rep)
			s++;
	}
}

/* most pieces are empty (no padding, no prefix): no call for those */
#define	PUT(s, n, rep, f)	if ((n) > 0) put(s, n, rep, f)

static char *
cvt(v, base, upper, buf)
unsigned long v;
int base, upper;
char *buf;
{
	register char *p = buf + 12;
	register char *digits = upper ? "0123456789ABCDEF" : "0123456789abcdef";
	register unsigned u;

	*--p = 0;
	while (v > 65535L) {
		*--p = digits[(int)(v % base)];
		v /= base;
	}
	u = v;
	do {
		*--p = digits[u % base];
		u /= base;
	} while (u);
	return p;
}

_doprnt(fmt, ap, f)
register char *fmt;
register int *ap;
register FILE *f;
{
	int left, zero, plus, space, alt, width, prec, islong, neg, len, n;
	unsigned long uv;
	long sv;
	char buf[13], *s, *pfx, *e;

	nout = 0;
	for (; *fmt; fmt++) {
		if (*fmt != '%') {		/* the text up to the next % */
			s = fmt;
			while (fmt[1] && fmt[1] != '%')
				fmt++;
			PUT(s, fmt - s + 1, 0, f);
			continue;
		}
		left = zero = plus = space = alt = islong = 0;
		width = 0;
		prec = -1;
		for (;;) {
			switch (*++fmt) {
			case '-': left = 1; continue;
			case '0': zero = 1; continue;
			case '+': plus = 1; continue;
			case ' ': space = 1; continue;
			case '#': alt = 1; continue;
			}
			break;
		}
		if (*fmt == '*') {
			width = *ap++;
			if (width < 0) { left = 1; width = -width; }
			fmt++;
		} else
			while (*fmt >= '0' && *fmt <= '9')
				width = width * 10 + *fmt++ - '0';
		if (*fmt == '.') {
			prec = 0;
			if (*++fmt == '*') {
				prec = *ap++;
				fmt++;
			} else
				while (*fmt >= '0' && *fmt <= '9')
					prec = prec * 10 + *fmt++ - '0';
		}
		if (*fmt == 'l') {
			islong = 1;
			fmt++;
		}
		pfx = "";
		neg = 0;
		switch (*fmt) {
		case 'd':
		case 'i':
			if (islong) {
				sv = *(long *)ap;
				ap += 2;
			} else
				sv = *ap++;
			if (sv < 0) {
				neg = 1;
				uv = -sv;
			} else
				uv = sv;
			s = cvt(uv, 10, 0, buf);
			pfx = neg ? "-" : plus ? "+" : space ? " " : "";
			goto number;
		case 'u':
		case 'o':
		case 'x':
		case 'X':
			if (islong) {
				uv = *(unsigned long *)ap;
				ap += 2;
			} else
				uv = (unsigned)*ap++;
			n = *fmt == 'u' ? 10 : *fmt == 'o' ? 8 : 16;
			s = cvt(uv, n, *fmt == 'X', buf);
			if (alt && uv)
				pfx = *fmt == 'o' ? "0" : *fmt == 'x' ? "0x" : *fmt == 'X' ? "0X" : "";
		number:
			if (prec == 0 && uv == 0)	/* zero at precision 0: no digits */
				s = "";
			len = strlen(s);
			if (prec >= 0)
				zero = 0;
			n = prec > len ? prec - len : 0;	/* precision zeros */
			if (*fmt == 'o' && alt)		/* # makes the first digit 0 */
				pfx = n > 0 || *s == '0' ? "" : "0";
			width -= len + n + strlen(pfx);
			if (!left && !zero) {
				PUT(" ", width, 1, f);
				width = 0;
			}
			if (*pfx)
				put(pfx, strlen(pfx), 0, f);
			if (!left && zero) {
				PUT("0", width, 1, f);
				width = 0;
			}
			PUT("0", n, 1, f);
			PUT(s, len, 0, f);
			PUT(" ", width, 1, f);
			break;
		case 'e':
		case 'E':
		case 'f':
		case 'g':
		case 'G':
			s = _fltfmt(ap, *fmt, prec, alt);
			neg = ap[3] & 0x8000;
			ap += 4;
			pfx = neg ? "-" : plus ? "+" : space ? " " : "";
			if (*s > '9')		/* inf or nan: no zero padding */
				zero = 0;
			len = strlen(s);
			width -= len + strlen(pfx);
			if (!left && !zero) {
				PUT(" ", width, 1, f);
				width = 0;
			}
			if (*pfx)
				put(pfx, strlen(pfx), 0, f);
			if (!left && zero) {
				PUT("0", width, 1, f);
				width = 0;
			}
			PUT(s, len, 0, f);
			PUT(" ", width, 1, f);
			break;
		case 'c':
			buf[0] = *ap++;
			buf[1] = 0;
			s = buf;
			len = 1;
			goto string;
		case 's':
			s = (char *)*ap++;
			if (s == NULL)
				s = "(null)";
			if (prec < 0)
				len = strlen(s);
			else if ((e = memchr(s, 0, prec)) != NULL)
				len = e - s;
			else
				len = prec;
		string:
			width -= len;
			if (!left) {
				PUT(" ", width, 1, f);
				width = 0;
			}
			PUT(s, len, 0, f);
			PUT(" ", width, 1, f);
			break;
		case 0:
			return nout;
		default:
			PUT(fmt, 1, 0, f);
		}
	}
	return nout;
}

printf(fmt, args)
char *fmt;
int args;
{
	register int n = _doprnt(fmt, &args, stdout);

	return ferror(stdout) ? EOF : n;
}

fprintf(f, fmt, args)
FILE *f;
char *fmt;
int args;
{
	register int n = _doprnt(fmt, &args, f);

	return ferror(f) ? EOF : n;
}

sprintf(s, fmt, args)
char *s, *fmt;
int args;
{
	FILE strf;
	register int n;

	strf._flag = _IOWRT | _IOSTRG;
	strf._ptr = strf._base = s;
	strf._cnt = 32767;
	strf._file = -1;
	n = _doprnt(fmt, &args, &strf);

	*strf._ptr = 0;
	return n;
}
