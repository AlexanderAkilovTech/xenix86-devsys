/* calloc.c - xenix86-devsys libc (K&R C) */

extern char *malloc();

char *
calloc(n, size)
unsigned n, size;
{
	register char *p, *q;
	register unsigned len = n * size;

	if ((p = malloc(len)) != 0)
		for (q = p; len-- > 0; )
			*q++ = 0;
	return p;
}
