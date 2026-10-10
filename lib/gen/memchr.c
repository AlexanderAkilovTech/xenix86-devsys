/* memchr.c - xenix86-devsys libc (K&R C) */

char *
memchr(s, c, n)
register char *s;
register int c;
register unsigned n;
{
	for (c &= 0377; n > 0; n--, s++)
		if ((*s & 0377) == c)
			return s;
	return 0;
}
