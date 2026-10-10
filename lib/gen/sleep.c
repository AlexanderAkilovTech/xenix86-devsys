/* sleep.c - xenix86-devsys libc (K&R C) */

#include <signal.h>

static
sleepcatch()
{
}

unsigned
sleep(n)
unsigned n;
{
	int (*old)();

	if (n == 0)
		return 0;
	old = signal(SIGALRM, sleepcatch);
	alarm(n);
	pause();
	signal(SIGALRM, old);
	return 0;
}
