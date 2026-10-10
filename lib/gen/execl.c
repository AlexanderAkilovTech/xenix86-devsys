/* execl.c - xenix86-devsys libc (K&R C) */

extern char **environ;

execl(path, arg0)
char *path, *arg0;
{
	return execve(path, &arg0, environ);
}
