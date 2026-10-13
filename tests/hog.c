/* hog.c - take about 56K of memory and keep it for a while, so that XENIX
 * swaps other processes out (for kprobe to find them on /dev/swap).
 *	hog [seconds]
 * xenix86-devsys test program. */
extern char *malloc();

main(argc, argv)
int argc;
char **argv;
{
	register char *p;
	register int i, n;

	for (n = 0; n < 7; n++)
		if ((p = malloc(8000)) != 0)
			for (i = 0; i < 8000; i++)
				p[i] = i;
	sleep(argc > 1 ? atoi(argv[1]) : 30);
	exit(0);
}
