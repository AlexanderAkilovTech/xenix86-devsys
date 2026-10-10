/* bcopy.c - xenix86-devsys libc (K&R C): the BSD name, source first; overlap allowed */

extern char *memmove();

bcopy(s, d, n)
char *s, *d;
unsigned n;
{
	memmove(d, s, n);
}
