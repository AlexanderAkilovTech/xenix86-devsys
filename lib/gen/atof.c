/* atof.c - xenix86-devsys libc: atof(s), strtod without the end pointer. */
double	strtod();

double
atof(s)
char *s;
{
	return strtod(s, (char **)0);
}
