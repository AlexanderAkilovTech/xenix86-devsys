/* misc.c - xenix86-devsys libc: environment and process exit (K&R C) */

extern char **environ;
extern int _cleanup();

char *
getenv(name)
register char *name;
{
	register char **e, *p, *q;

	for (e = environ; *e; e++) {
		for (p = name, q = *e; *p && *p == *q; p++, q++)
			;
		if (*p == 0 && *q == '=')
			return q + 1;
	}
	return (char *)0;
}

exit(status)
int status;
{
	_cleanup();
	_exit(status);
}
