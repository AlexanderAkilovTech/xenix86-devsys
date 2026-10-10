/* strncpy.c - xenix86-devsys libc (K&R C) */

char *
strncpy(d, s, n)
register char *d, *s;
register int n;
{
	char *r = d;

	while (n > 0 && (*d++ = *s++))
		n--;
	while (n-- > 1)
		*d++ = 0;
	return r;
}
