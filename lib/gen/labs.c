/* labs.c - xenix86-devsys libc: labs(l), |l| for a long. */

long
labs(l)
long l;
{
	return l < 0 ? -l : l;
}
