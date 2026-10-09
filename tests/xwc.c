/* xwc - count lines, words and characters (a small wc, built with xenix86-devsys) */
#include <stdio.h>
#include <ctype.h>

long tl, tw, tc;

count(f, name)
FILE *f;
char *name;
{
	register int c, inword = 0;
	long l = 0, w = 0, ch = 0;

	while ((c = getc(f)) != EOF) {
		ch++;
		if (c == '\n')
			l++;
		if (isspace(c))
			inword = 0;
		else if (!inword) {
			inword = 1;
			w++;
		}
	}
	printf("%7ld %7ld %7ld %s\n", l, w, ch, name);
	tl += l; tw += w; tc += ch;
}

main(argc, argv)
int argc;
char **argv;
{
	register int i, status = 0;
	FILE *f;

	if (argc < 2) {
		count(stdin, "");
		return 0;
	}
	for (i = 1; i < argc; i++) {
		if ((f = fopen(argv[i], "r")) == NULL) {
			fprintf(stderr, "xwc: cannot open %s\n", argv[i]);
			status = 2;
			continue;
		}
		count(f, argv[i]);
		fclose(f);
	}
	if (argc > 2)
		printf("%7ld %7ld %7ld total\n", tl, tw, tc);
	return status;
}
