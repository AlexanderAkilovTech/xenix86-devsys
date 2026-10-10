/* setjmp/longjmp test: deep unwinding, return values, register variables,
   and escaping from a signal handler */
#include <stdio.h>
#include <setjmp.h>
#include <signal.h>

jmp_buf env, tmo;
int depth;

dive(n)
int n;
{
	char pad[50];

	pad[0] = n;
	depth = n;
	if (n == 40)
		longjmp(env, 42);
	dive(n + 1);
	return pad[0];
}

ontime(sig)
int sig;
{
	longjmp(tmo, sig);
}

main()
{
	register int keep = 777;
	int r, count = 0;

	printf("1. setjmp returns %d the first time\n", setjmp(env) ? -1 : 0);

	r = setjmp(env);
	count++;
	if (r == 0)
		dive(1);
	printf("2. longjmp from depth %d: setjmp returned %d, pass %d, register var %d\n",
		depth, r, count, keep);

	r = setjmp(env);
	if (r == 0)
		longjmp(env, 0);
	printf("3. longjmp(env, 0) makes setjmp return %d\n", r);

	r = setjmp(tmo);
	if (r == 0) {
		signal(SIGALRM, ontime);
		alarm(1);
		for (;;)
			pause();
	}
	printf("4. escaped from the SIGALRM handler, setjmp returned %d\n", r);
	printf("done\n");
	return 0;
}
