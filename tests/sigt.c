/* signal trampoline test (runs under tools/xsim.py and on XENIX) */
#include <stdio.h>
#include <signal.h>

int caught, extra = 1234;

h1(sig)
int sig;
{
	caught = sig;
	extra++;
}

h2(sig)
int sig;
{
}

main()
{
	int (*old)();
	register int keep = 777;

	old = signal(SIGALRM, h1);
	printf("first signal() returned %s\n", old == SIG_DFL ? "SIG_DFL" : "something else");
	old = signal(SIGALRM, h2);
	printf("second returned %s\n", old == h1 ? "h1" : "something else");
	signal(SIGALRM, h1);
	alarm(1);
	pause();
	printf("caught %d, extra %d, register var still %d\n", caught, extra, keep);
	return caught == 14 && extra == 1235 && keep == 777 ? 0 : 1;
}
