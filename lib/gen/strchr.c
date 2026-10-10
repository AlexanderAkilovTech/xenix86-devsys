/* strchr.c - xenix86-devsys libc (K&R C) */

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
