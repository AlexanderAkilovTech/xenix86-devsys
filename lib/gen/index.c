/* index.c - xenix86-devsys libc (K&R C) */

char *
index(s, c)
char *s;
int c;
{
	return strchr(s, c);
}
