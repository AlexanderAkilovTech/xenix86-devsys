/* tm.c - run a command and say how much of the processor it used, in its
 * own code and in the kernel for it, and how long it took (to 1/20 s since
 * M58). On standard error, so that the command's output can go where it would.
 *	tm command [arguments]
 * (XENIX 2.1.3 has no time command but csh's.) xenix86-devsys test program. */
#include <sys/types.h>
#include <sys/times.h>
#include <stdio.h>

extern long time(), times();

char *
secs(t)
long t;
{
	static char b[3][20];		/* three in one printf (M58 had two, and lost one) */
	static int w;
	char *p = b[w++ % 3];

	sprintf(p, "%ld.%02ld", t / 20, t % 20 * 5);	/* ticks of 1/20 s */
	return p;
}

main(argc, argv)
int argc;
char **argv;
{
	struct tms t;
	int pid, st;
	long start, end;

	if (argc < 2) {
		fprintf(stderr, "usage: tm command [arguments]\n");
		exit(2);
	}
	start = times(&t);		/* the clock, in ticks (ABI.md, M56) */
	if ((pid = fork()) == 0) {
		execv(argv[1], argv + 1);
		fprintf(stderr, "tm: cannot run %s\n", argv[1]);
		_exit(127);
	}
	while (wait(&st) != pid)
		;
	end = times(&t);
	fprintf(stderr, "%s: %s s in itself, %s s in the kernel, %s s in all\n", argv[1],
		secs(t.tms_cutime), secs(t.tms_cstime), secs(end - start));
	exit(0);
}
