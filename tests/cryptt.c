/* cryptt - crypt each line of the input, "salt password", for
 * tests/unit/test_crypt.py. xenix86-devsys test program. */
#include <stdio.h>

extern char *crypt();

main()
{
	char line[100];
	int n;

	while (fgets(line, sizeof line, stdin)) {
		n = strlen(line);
		if (n && line[n - 1] == '\n')
			line[--n] = 0;
		if (n < 2)
			continue;
		printf("%s\n", crypt(line + 2, line));
	}
	exit(0);
}
