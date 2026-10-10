/* time.c - xenix86-devsys libc (K&R C) */

extern long _time();

long
time(tp)
long *tp;
{
	long t = _time();

	if (tp)
		*tp = t;
	return t;
}
