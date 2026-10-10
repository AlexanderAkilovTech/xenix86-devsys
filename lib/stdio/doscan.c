/* doscan.c - xenix86-devsys libc: scanf, fscanf, sscanf (K&R C)
 * Conversions: d i u o x X e E f g G c s [ ] n %, with * (no assignment), a
 * field width, and the l (long, or double) and h (short) sizes. Each returns
 * the number of fields assigned, or EOF if the input ended before the first. */
#include <stdio.h>
#include <ctype.h>


static int nread;		/* characters taken so far, for %n */
#define	GET(f)		(nread++, getc(f))
#define	UNGET(c, f)	(nread--, ungetc(c, f))

/* skip white space; return the next character (not taken) */
static
skipws(f)
register FILE *f;
{
	register int c;

	while ((c = GET(f)) != EOF && isspace(c))
		;
	if (c != EOF)
		UNGET(c, f);
	return c;
}

static
digit(c, base)
register int c, base;
{
	register int d;

	if (c >= '0' && c <= '9')
		d = c - '0';
	else if (c >= 'a' && c <= 'f')
		d = c - 'a' + 10;
	else if (c >= 'A' && c <= 'F')
		d = c - 'A' + 10;
	else
		return -1;
	return d < base ? d : -1;
}

_doscan(f, fmt, ap)
register FILE *f;
register char *fmt;
char **ap;
{
	int nassign = 0, width, suppress, size, base, neg, any, c, i, not;
	unsigned long v;
	char set[256], *s;

	nread = 0;
	for (;;) {
		c = *fmt++;
		if (c == 0)
			return nassign;
		if (isspace(c)) {
			skipws(f);
			continue;
		}
		if (c != '%' || *fmt == '%') {	/* ordinary character: must match */
			if (c == '%')
				fmt++;
			if ((i = GET(f)) != c) {
				if (i == EOF)
					return nassign ? nassign : EOF;
				UNGET(i, f);
				return nassign;
			}
			continue;
		}
		suppress = *fmt == '*';
		if (suppress)
			fmt++;
		for (width = 0; isdigit(*fmt); fmt++)
			width = width * 10 + *fmt - '0';
		if (width == 0)
			width = 32767;
		size = 0;
		if (*fmt == 'l' || *fmt == 'h')
			size = *fmt++;
		switch (c = *fmt++) {
		case 'n':
			if (!suppress)
				**(int **)ap++ = nread;
			continue;
		case 'd': case 'i': case 'u': case 'o': case 'x': case 'X':
			base = c == 'o' ? 8 : (c == 'x' || c == 'X') ? 16 : c == 'i' ? 0 : 10;
			if (skipws(f) == EOF)
				return nassign ? nassign : EOF;
			neg = any = 0;
			v = 0;
			i = GET(f);
			if ((i == '-' || i == '+') && width > 0) {
				neg = i == '-';
				width--;
				i = GET(f);
			}
			if (i == '0' && width > 0 && (base == 0 || base == 16)) {
				any = 1;		/* the 0 is a digit in any case */
				width--;
				i = GET(f);
				if ((i == 'x' || i == 'X') && width > 0) {
					base = 16;
					width--;
					i = GET(f);
				} else if (base == 0)
					base = 8;
			}
			if (base == 0)
				base = 10;
			while (width > 0 && digit(i, base) >= 0) {
				v = v * base + digit(i, base);
				any = 1;
				width--;
				i = GET(f);
			}
			if (i != EOF)
				UNGET(i, f);
			if (!any)
				return nassign;
			if (neg)
				v = -v;
			if (!suppress) {
				if (size == 'l')
					**(long **)ap++ = v;
				else if (size == 'h')
					**(short **)ap++ = v;
				else
					**(int **)ap++ = v;
				nassign++;
			}
			continue;
		case 'c':
			if (width == 32767)
				width = 1;
			s = suppress ? 0 : *ap++;
			for (i = 0; i < width; i++) {
				if ((c = GET(f)) == EOF)
					return nassign ? nassign : EOF;
				if (s)
					*s++ = c;
			}
			if (!suppress)
				nassign++;
			continue;
		case 's':
		case '[':
			if (c == 's') {
				if (skipws(f) == EOF)
					return nassign ? nassign : EOF;
				for (i = 0; i < 256; i++)
					set[i] = !isspace(i);
				set[0] = 0;
			} else {
				not = *fmt == '^';
				if (not)
					fmt++;
				for (i = 0; i < 256; i++)
					set[i] = not;
				if (*fmt == ']')	/* a ] first is part of the set */
					set[*fmt++ & 0377] = !not;
				while (*fmt && *fmt != ']')
					set[*fmt++ & 0377] = !not;
				if (*fmt)
					fmt++;
				set[0] = 0;
			}
			s = suppress ? 0 : *ap++;
			any = 0;
			while (width > 0 && (c = GET(f)) != EOF) {
				if (!set[c & 0377]) {
					UNGET(c, f);
					break;
				}
				if (s)
					*s++ = c;
				any = 1;
				width--;
			}
			if (!any)
				return nassign ? nassign : (c == EOF ? EOF : 0);
			if (s) {
				*s = 0;
				nassign++;
			}
			continue;
		case 'e':
		case 'E':
		case 'f':
		case 'g':
		case 'G':			/* fltscan.c */
			if (skipws(f) == EOF)
				return nassign ? nassign : EOF;
			i = _fltscan(f, width, suppress ? (char *)0 : *ap, size, &nread);
			if (!suppress)
				ap++;
			if (i == 0)
				return nassign;
			if (!suppress)
				nassign++;
			continue;
		default:
			return nassign;
		}
	}
}

scanf(fmt, args)
char *fmt;
int args;
{
	return _doscan(stdin, fmt, (char **)&args);
}

fscanf(f, fmt, args)
FILE *f;
char *fmt;
int args;
{
	return _doscan(f, fmt, (char **)&args);
}

sscanf(s, fmt, args)
char *s, *fmt;
int args;
{
	FILE strf;
	register char *p;

	for (p = s; *p; p++)
		;
	strf._flag = _IOREAD | _IOSTRG;
	strf._ptr = strf._base = s;
	strf._cnt = p - s;
	strf._file = -1;
	strf._bufsiz = p - s;
	return _doscan(&strf, fmt, (char **)&args);
}
