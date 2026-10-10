/* atol.c - xenix86-devsys libc (K&R C) */
#include <ctype.h>

long
atol(s)
register char *s;
{
	register int neg = 0;
	long n = 0;

	while (isspace(*s))
		s++;
	if (*s == '-' || *s == '+')
		neg = *s++ == '-';
	while (isdigit(*s))
		n = n * 10 + (*s++ - '0');
	return neg ? -n : n;
}
