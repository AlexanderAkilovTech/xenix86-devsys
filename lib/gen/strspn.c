/* strspn.c - xenix86-devsys libc: strspn, strcspn, strpbrk (System V). */

/* the length of the start of s made of characters in set */
int
strspn(s, set)
char *s, *set;
{
	register char *p, *q;

	for (p = s; *p; p++) {
		for (q = set; *q && *q != *p; q++)
			;
		if (!*q)
			break;
	}
	return p - s;
}

/* the length of the start of s made of characters not in set */
int
strcspn(s, set)
char *s, *set;
{
	register char *p, *q;

	for (p = s; *p; p++) {
		for (q = set; *q && *q != *p; q++)
			;
		if (*q)
			break;
	}
	return p - s;
}

/* the first character of s that is in set, or 0 */
char *
strpbrk(s, set)
char *s, *set;
{
	s += strcspn(s, set);
	return *s ? s : (char *)0;
}
