/* strrchr.c - xenix86-devsys libc (K&R C) */

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
