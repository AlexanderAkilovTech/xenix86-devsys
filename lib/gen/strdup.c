/* strdup.c - xenix86-devsys libc (K&R C) */

extern char *malloc();

char *
strdup(s)
char *s;
{
	register char *p, *d;

	for (p = s; *p; p++)
		;
	if ((d = malloc(p - s + 1)) == 0)
		return 0;
	for (p = d; *p++ = *s++; )
		;
	return d;
}
