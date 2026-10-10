/* memmove.c - xenix86-devsys libc (K&R C): copy n bytes; the areas may overlap */

char *
memmove(d, s, n)
char *d, *s;
register unsigned n;
{
	register char *p = d, *q = s;

	if (p < q || p >= q + n)
		while (n-- > 0)
			*p++ = *q++;
	else
		for (p += n, q += n; n-- > 0; )
			*--p = *--q;
	return d;
}
