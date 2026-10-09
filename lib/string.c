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

char *
strchr(s, c)
register char *s;
register int c;
{
	do {
		if (*s == c)
			return s;
	} while (*s++);
	return 0;
}

char *
strrchr(s, c)
register char *s;
register int c;
{
	char *r = 0;

	do {
		if (*s == c)
			r = s;
	} while (*s++);
	return r;
}

char *
index(s, c)
char *s;
int c;
{
	return strchr(s, c);
}

char *
rindex(s, c)
char *s;
int c;
{
	return strrchr(s, c);
}

char *
memcpy(d, s, n)
register char *d, *s;
register unsigned n;
{
	char *r = d;

	while (n-- > 0)
		*d++ = *s++;
	return r;
}

char *
memset(d, c, n)
register char *d;
register int c;
register unsigned n;
{
	char *r = d;

	while (n-- > 0)
		*d++ = c;
	return r;
}

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
