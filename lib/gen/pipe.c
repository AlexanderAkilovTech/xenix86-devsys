/* pipe.c - xenix86-devsys libc (K&R C) */

extern long _pipe();

pipe(fd)
int *fd;
{
	long r = _pipe();

	if (r == -1L)
		return -1;
	fd[0] = (int)r;
	fd[1] = (int)(r >> 16);
	return 0;
}
