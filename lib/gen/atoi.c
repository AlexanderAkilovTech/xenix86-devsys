/* atoi.c - xenix86-devsys libc (K&R C) */

atoi(s)
char *s;
{
	return (int)atol(s);
}
