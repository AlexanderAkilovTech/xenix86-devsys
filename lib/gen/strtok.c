/* strtok.c - xenix86-devsys libc: strtok(s, sep) (System V): the next token
 * of s, separated by characters in sep; s is 0 after the first call. */

char *
strtok(s, sep)
char *s, *sep;
{
	static char *next;
	register char *t;

	if (s == (char *)0)
		s = next;
	if (s == (char *)0)
		return (char *)0;
	s += strspn(s, sep);
	if (*s == 0) {
		next = (char *)0;
		return (char *)0;
	}
	t = s + strcspn(s, sep);
	if (*t)
		*t++ = 0;
	else
		t = (char *)0;
	next = t;
	return s;
}
