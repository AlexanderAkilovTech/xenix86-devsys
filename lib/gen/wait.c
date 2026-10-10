/* wait.c - xenix86-devsys libc (K&R C) */

extern long _wait();

wait(status)
int *status;
{
	long r = _wait();

	if ((int)r == -1)
		return -1;
	if (status)
		*status = (int)(r >> 16);
	return (int)r;
}
