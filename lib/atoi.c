/* atoi.c - xenix86-devsys libc: number conversion */
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

atoi(s)
char *s;
{
	return (int)atol(s);
}

abs(n)
int n;
{
	return n < 0 ? -n : n;
}
