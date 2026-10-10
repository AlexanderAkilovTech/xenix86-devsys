/* fpu87.c - what XENIX 2.1.3 does with an 8087.
 *
 * 1. Finds out whether there is an 8087, the safe way (works without one).
 * 2. Has it compute 1/3, sqrt(2) and pi, printed as the bits of a double.
 * 3. Runs two processes at once, each keeping its own values and control
 *    word in the 8087, and counts how often a process finds the other's
 *    there instead: that shows whether the kernel saves and restores the
 *    8087 when it switches processes.
 * The same program is built twice, as fpu87 and fpu87h; fpu87h has the
 * x.out header bit that says the program needs floating point hardware. */
#include <stdio.h>

long time();
unsigned fpgetcw();

unsigned r[4];

show(what, expect)
char *what, *expect;
{
	char got[20];

	sprintf(got, "%04x%04x%04x%04x", r[3], r[2], r[1], r[0]);
	printf("%-8s %s  %s\n", what, got, strcmp(got, expect) ? "WRONG" : "ok");
}

/* keep val and val + 1 on the 8087 stack and control word cw in place, for
 * secs seconds; count the checks and the times something else was there */
swaptest(who, val, cw, secs)
char *who;
unsigned cw;
{
	long t0, n, bad, badcw;
	int k;

	fpinit();
	fpsetcw(cw);
	fpld(val);
	fpld(val + 1);
	n = bad = badcw = 0;
	t0 = time((long *)0);
	while (time((long *)0) - t0 < secs) {
		for (k = 0; k < 50; k++) {
			n++;
			if (fpst0() != val + 1 || fpst1() != val)
				bad++;
			if (fpgetcw() != cw)
				badcw++;
		}
	}
	printf("%s: %ld checks, %ld with wrong values, %ld with wrong control word\n",
		who, n, bad, badcw);
	fflush(stdout);
}

main(argc, argv)
char **argv;
{
	unsigned cw, sw;
	int pid, status, secs;

	secs = argc > 1 ? atoi(argv[1]) : 10;
	fpprobe(&cw, &sw);
	printf("after fninit: control word %04x, status word %04x\n", cw, sw);
	if (cw == 0x5a5a) {
		printf("no 8087\n");
		exit(0);
	}
	printf("8087 present\n");
	fpdiv3(r);
	show("1/3", "3fd5555555555555");
	fpsqrt2(r);
	show("sqrt(2)", "3ff6a09e667f3bcd");
	fppi(r);
	show("pi", "400921fb54442d18");
	printf("two processes for %d seconds...\n", secs);
	fflush(stdout);
	if ((pid = fork()) == 0) {
		/* round toward zero, so the child's control word differs */
		swaptest("child ", 2222, cw | 0x0c00, secs);
		exit(0);
	}
	swaptest("parent", 1111, cw, secs);
	while (wait(&status) != pid)
		;
	exit(0);
}
