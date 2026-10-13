/* tct.c - look up a terminal's entry in /etc/termcap, n times, for timing
 * tgetent (with tests/tm.c); says whether it was found and how long.
 *	tct name [n]
 * xenix86-devsys test program. */
#include <stdio.h>

char	tbuf[1024];

main(argc, argv)
int argc;
char **argv;
{
	int i, n = argc > 2 ? atoi(argv[2]) : 1, r = 0;

	for (i = 0; i < n; i++)
		r = tgetent(tbuf, argv[1]);
	printf("%s: %s, %d bytes\n", argv[1], r > 0 ? "found" : r == 0 ? "not found" : "no termcap file",
		r > 0 ? strlen(tbuf) : 0);
	exit(r > 0 ? 0 : 1);
}
