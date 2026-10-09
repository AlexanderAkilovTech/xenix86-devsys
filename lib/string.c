/* string.c - xenix86-devsys libc: string routines (K&R C) */

int
strlen(s)
register char *s;
{
	register char *p = s;

	while (*p)
		p++;
	return p - s;
}

char *
strcpy(d, s)
register char *d, *s;
{
	char *r = d;

	while (*d++ = *s++)
		;
	return r;
}

char *
strcat(d, s)
register char *d, *s;
{
	char *r = d;

	while (*d)
		d++;
	while (*d++ = *s++)
		;
	return r;
}

int
strcmp(a, b)
register char *a, *b;
{
	while (*a == *b++)
		if (*a++ == 0)
			return 0;
	return *a - *--b;
}

int
strncmp(a, b, n)
register char *a, *b;
register int n;
{
	while (n > 0 && *a == *b) {
		if (*a == 0)
			return 0;
		a++; b++; n--;
	}
	return n > 0 ? *a - *b : 0;
}
