/* strtol.c - xenix86-devsys libc (K&R C) */

extern unsigned long strtoul();

long
strtol(s, endp, base)
char *s, **endp;
int base;
{
	return (long)strtoul(s, endp, base);
}
