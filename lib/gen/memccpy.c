/* memccpy.c - xenix86-devsys libc (K&R C): copy up to and including c, at most n bytes */

char *
memccpy(d, s, c, n)
register char *d, *s;
register int c;
unsigned n;
{
	for (c &= 0377; n > 0; n--)
		if (((*d++ = *s++) & 0377) == c)
			return d;
	return 0;
}
