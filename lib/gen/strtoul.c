/* strtoul.c - xenix86-devsys libc (K&R C)
 * Base 0 means: 0x or 0X prefix for hex, leading 0 for octal, otherwise decimal. */
#include <ctype.h>

unsigned long
strtoul(s, endp, base)
char *s, **endp;
int base;
{
	register char *p = s;
	register int d;
	unsigned long n = 0;
	int neg = 0, any = 0;

	while (isspace(*p))
		p++;
	if (*p == '-' || *p == '+')
		neg = *p++ == '-';
	if ((base == 0 || base == 16) && p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
		p += 2;
		base = 16;
	} else if (base == 0)
		base = *p == '0' ? 8 : 10;
	for (;; p++) {
		if (isdigit(*p))
			d = *p - '0';
		else if (isalpha(*p))
			d = (*p | 040) - 'a' + 10;
		else
			break;
		if (d >= base)
			break;
		n = n * base + d;
		any = 1;
	}
	if (endp)
		*endp = any ? p : s;
	return neg ? -n : n;
}
