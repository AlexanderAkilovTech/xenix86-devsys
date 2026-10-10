/* abs.c - xenix86-devsys libc (K&R C) */

abs(n)
int n;
{
	return n < 0 ? -n : n;
}
