/* execv.c - xenix86-devsys libc (K&R C) */

extern char **environ;

execv(path, argv)
char *path, **argv;
{
	return execve(path, argv, environ);
}
