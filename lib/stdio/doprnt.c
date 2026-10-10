/* doprnt.c - xenix86-devsys libc: printf, fprintf, sprintf (K&R C)
 * Conversions: d i u o x X c s e E f g G %, with the l (long) modifier, flags
 * - 0 + space #, field width and precision (* for either). The floating point
 * conversions print the double's exact value correctly rounded (fltcvt.c),
 * and inf or nan for those. As in System V, each returns the number of
 * characters written (EOF after an output error). */
#include <stdio.h>

char	*_fltfmt();

static int nout;		/* characters written by the current _doprnt */
#define	PUT(c, f)	(putc(c, f), nout++)

static char *
cvt(v, base, upper, buf)
unsigned long v;
int base, upper;
char *buf;
{
	register char *p = buf + 12;
	register char *digits = upper ? "0123456789ABCDEF" : "0123456789abcdef";

	*--p = 0;
	do {
		*--p = digits[(int)(v % base)];
		v /= base;
	} while (v);
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
	char buf[13], *s, *pfx;

	nout = 0;
	for (; *fmt; fmt++) {
		if (*fmt != '%') {
			PUT(*fmt, f);
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
			len = strlen(s);
			if (prec >= 0)
				zero = 0;
			n = prec > len ? prec - len : 0;	/* precision zeros */
			width -= len + n + strlen(pfx);
			if (!left && !zero)
				while (width-- > 0) PUT(' ', f);
			while (*pfx) PUT(*pfx++, f);
			if (!left && zero)
				while (width-- > 0) PUT('0', f);
			while (n-- > 0) PUT('0', f);
			while (*s) PUT(*s++, f);
			while (width-- > 0) PUT(' ', f);
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
			if (!left && !zero)
				while (width-- > 0) PUT(' ', f);
			while (*pfx) PUT(*pfx++, f);
			if (!left && zero)
				while (width-- > 0) PUT('0', f);
			while (*s) PUT(*s++, f);
			while (width-- > 0) PUT(' ', f);
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
			for (len = 0; s[len] && (prec < 0 || len < prec); len++)
				;
		string:
			width -= len;
			if (!left)
				while (width-- > 0) PUT(' ', f);
			while (len-- > 0) PUT(*s++, f);
			while (width-- > 0) PUT(' ', f);
			break;
		case 0:
			return nout;
		default:
			PUT(*fmt, f);
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
