/* execle.c - xenix86-devsys libc (K&R C) */

execle(path, arg0)
char *path, *arg0;
{
	register char **p = &arg0;

	while (*p++)
		;
	return execve(path, &arg0, *(char ***)p);
}
