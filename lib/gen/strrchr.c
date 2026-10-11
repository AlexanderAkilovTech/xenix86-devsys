/* strrchr.c - xenix86-devsys libc (K&R C) */

char *
strrchr(s, c)
register char *s;
register int c;
{
	char *r = 0;

	c &= 0377;		/* as a char: strchr(s, 0351) finds a byte 0351 */
	do {
		if ((*s & 0377) == c)
			r = s;
	} while (*s++);
	return r;
}
