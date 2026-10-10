/* bzero.c - xenix86-devsys libc (K&R C) */

extern char *memset();

bzero(d, n)
char *d;
unsigned n;
{
	memset(d, 0, n);
}
