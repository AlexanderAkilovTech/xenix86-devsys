/* bcmp.c - xenix86-devsys libc (K&R C): zero if the n bytes are equal */

bcmp(a, b, n)
register char *a, *b;
register unsigned n;
{
	while (n-- > 0)
		if (*a++ != *b++)
			return 1;
	return 0;
}
