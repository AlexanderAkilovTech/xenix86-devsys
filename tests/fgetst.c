/* fgetst.c - fgets on a file, with buffer sizes taken in turn from the
 * arguments: each line it returns printed as its length and its bytes
 * (tests/unit/test_fgets.py checks them against Python).
 *	fgetst file size ...
 * xenix86-devsys test program. */
#include <stdio.h>

main(argc, argv)
int argc;
char **argv;
{
	FILE *f;
	char buf[700];
	int i = 0, n;

	if ((f = fopen(argv[1], "r")) == NULL)
		exit(1);
	for (;;) {
		n = atoi(argv[2 + i++ % (argc - 2)]);
		memset(buf, 'Z', sizeof buf);
		if (fgets(buf, n, f) == NULL)
			break;
		printf("%d:", strlen(buf));
		fwrite(buf, 1, strlen(buf), stdout);
		printf("|%c\n", buf[strlen(buf) + 1]);	/* nothing written past the 0 */
	}
	printf("EOF\n");
	exit(0);
}
