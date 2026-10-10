/* memcmp.c - xenix86-devsys libc (K&R C) */

int
memcmp(a, b, n)
register char *a, *b;
register unsigned n;
{
	while (n-- > 0)
		if (*a++ != *b++)
			return (a[-1] & 0377) - (b[-1] & 0377);
	return 0;
}
