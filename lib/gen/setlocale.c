/* setlocale.c - xenix86-devsys libc: XENIX has only the "C" locale (K&R C) */

char *
setlocale(cat, name)
int cat;
char *name;
{
	if (name && *name && !(name[0] == 'C' && name[1] == 0))
		return (char *)0;
	return "C";
}
