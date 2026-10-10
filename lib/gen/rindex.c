/* rindex.c - xenix86-devsys libc (K&R C) */

char *
rindex(s, c)
char *s;
int c;
{
	return strrchr(s, c);
}
