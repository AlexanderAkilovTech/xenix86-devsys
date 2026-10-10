/* strncat.c - xenix86-devsys libc (K&R C) */

char *
strncat(d, s, n)
register char *d, *s;
register int n;
{
	char *r = d;

	while (*d)
		d++;
	while (n-- > 0 && (*d = *s++))
		d++;
	*d = 0;
	return r;
}
