/* strchr.c - xenix86-devsys libc (K&R C) */

char *
strchr(s, c)
register char *s;
register int c;
{
	c &= 0377;		/* as a char: strchr(s, 0351) finds a byte 0351 */
	do {
		if ((*s & 0377) == c)
			return s;
	} while (*s++);
	return 0;
}
