/* execlp.c - xenix86-devsys libc (K&R C) */

execlp(file, arg0)
char *file, *arg0;
{
	return execvp(file, &arg0);
}
