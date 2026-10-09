/* printf.c - xenix86-devsys libc: buffered stdout and a small printf (K&R C)
 * Supports %d %u %o %x %c %s %% with optional field width, '-' and '0' flags. */

#define OBUFSZ	256

static char obuf[OBUFSZ];
static int ocnt;

_flushout()
{
	if (ocnt > 0)
		write(1, obuf, ocnt);
	ocnt = 0;
}

putchar(c)
int c;
{
	obuf[ocnt++] = c;
	if (ocnt >= OBUFSZ || c == '\n')
		_flushout();
	return c & 0377;
}

puts(s)
register char *s;
{
	while (*s)
		putchar(*s++);
	putchar('\n');
	return 0;
}

static
pad(n, c)
register int n, c;
{
	while (n-- > 0)
		putchar(c);
}

static
outnum(v, base, sign, width, left, zero)
unsigned v;
int base, sign, width, left, zero;
{
	char buf[8];
	register char *p = buf + sizeof buf;
	register int len;
	int neg = 0;

	if (sign && (int)v < 0) {
		neg = 1;
		v = -(int)v;
	}
	*--p = 0;
	do {
		*--p = "0123456789abcdef"[v % base];
		v /= base;
	} while (v);
	len = buf + sizeof buf - 1 - p + neg;
	if (!left && !zero)
		pad(width - len, ' ');
	if (neg)
		putchar('-');
	if (!left && zero)
		pad(width - len, '0');
	while (*p)
		putchar(*p++);
	if (left)
		pad(width - len, ' ');
}

printf(fmt, args)
register char *fmt;
int args;
{
	register int *ap = &args;
	int width, left, zero, slen;
	char *s;

	for (; *fmt; fmt++) {
		if (*fmt != '%') {
			putchar(*fmt);
			continue;
		}
		fmt++;
		left = zero = width = 0;
		if (*fmt == '-') {
			left = 1;
			fmt++;
		}
		if (*fmt == '0') {
			zero = 1;
			fmt++;
		}
		while (*fmt >= '0' && *fmt <= '9')
			width = width * 10 + *fmt++ - '0';
		switch (*fmt) {
		case 'd':
			outnum(*ap++, 10, 1, width, left, zero);
			break;
		case 'u':
			outnum(*ap++, 10, 0, width, left, zero);
			break;
		case 'o':
			outnum(*ap++, 8, 0, width, left, zero);
			break;
		case 'x':
			outnum(*ap++, 16, 0, width, left, zero);
			break;
		case 'c':
			putchar(*ap++);
			break;
		case 's':
			s = (char *)*ap++;
			slen = strlen(s);
			if (!left)
				pad(width - slen, ' ');
			while (*s)
				putchar(*s++);
			if (left)
				pad(width - slen, ' ');
			break;
		case 0:
			return 0;
		default:
			putchar(*fmt);
		}
	}
	return 0;
}
