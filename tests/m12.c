/* Milestone 3 test: stdio, long arithmetic, ctype, string routines */
#include <stdio.h>
#include <ctype.h>
#include <string.h>

long
fact(n)
int n;
{
	return n <= 1 ? 1L : n * fact(n - 1);
}

main(argc, argv)
int argc;
char **argv;
{
	FILE *f;
	char line[80], buf[64];
	register int i, c, words, digits;
	long sum, pos;
	unsigned long h;

	printf("1. long arithmetic\n");
	for (i = 1; i <= 12; i += 3)
		printf("   %2d! = %ld\n", i, fact(i));
	printf("   2000000000 / 7 = %ld, remainder %ld\n", 2000000000L / 7, 2000000000L % 7);
	printf("   -123456789 / 1000 = %ld, %% = %ld\n", -123456789L / 1000, -123456789L % 1000);
	printf("   hex %lx, octal %lo, unsigned %lu\n", 0xDEADBEEFL, 01234567L, 4000000000L);

	printf("2. writing /tmp/stdio.tst\n");
	if ((f = fopen("/tmp/stdio.tst", "w")) == NULL) {
		printf("   cannot create\n");
		return 1;
	}
	for (i = 1; i <= 5; i++)
		fprintf(f, "line %d: %ld squared is %ld\n", i, (long)i * 3000, (long)i * 3000 * i * 3000);
	fputs("the end.\n", f);
	fclose(f);

	printf("3. reading it back\n");
	f = fopen("/tmp/stdio.tst", "r");
	sum = 0;
	while (fgets(line, sizeof line, f) != NULL) {
		printf("   | %s", line);
		sum += strlen(line);
	}
	printf("   %ld bytes read\n", sum);

	printf("4. getc/ungetc/fseek/ftell\n");
	rewind(f);
	words = digits = 0;
	h = 5381;
	while ((c = getc(f)) != EOF) {
		h = h * 33 + c;
		if (isdigit(c))
			digits++;
		if (isalpha(c)) {
			words++;
			while ((c = getc(f)) != EOF && isalpha(c))
				h = h * 33 + c;
			ungetc(c, f);
		}
	}
	printf("   %d words, %d digits, hash %lu\n", words, digits, h);
	fseek(f, -9L, 2);
	pos = ftell(f);
	fgets(line, sizeof line, f);
	printf("   at offset %ld: %s", pos, line);
	fclose(f);
	unlink("/tmp/stdio.tst");

	printf("5. sprintf and strings\n");
	sprintf(buf, "[%-6s|%6s|%06d|%+d|%x|%c]", "ab", "cd", 42, 7, 255, 'Z');
	printf("   %s (%d chars)\n", buf, strlen(buf));
	strcpy(line, "Xenix");
	strcat(line, " 2.1.3");
	printf("   %s, strchr '.' -> \"%s\", toupper -> %c%c\n", line, strchr(line, '.'), toupper('x'), toupper('e'));
	fflush(stdout);
	fprintf(stderr, "6. this line went to stderr\n");
	printf("done\n");
	return 0;
}
