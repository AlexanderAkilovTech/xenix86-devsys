/* scanf family: conversions, widths, suppression, sets, %n, return values */
#include <stdio.h>
#include <string.h>
#include <errno.h>

extern char *strerror();
int bad;

check(name, cond)
char *name;
int cond;
{
	if (!cond) {
		printf("FAIL %s\n", name);
		bad++;
	}
}

main()
{
	int a, b, c, n, r;
	long l;
	short h;
	char s1[40], s2[40], ch[4];
	FILE *f;

	r = sscanf("  42 -17 0x1F", "%d %d %x", &a, &b, &c);
	check("d d x", r == 3 && a == 42 && b == -17 && c == 31);
	r = sscanf("077 0x10 12", "%i %i %i", &a, &b, &c);
	check("i (octal, hex, decimal)", r == 3 && a == 63 && b == 16 && c == 12);
	r = sscanf("123456789", "%ld", &l);
	check("ld", r == 1 && l == 123456789L);
	r = sscanf("-70000", "%ld", &l);
	check("negative ld", r == 1 && l == -70000L);
	r = sscanf("12345", "%3d%d", &a, &b);
	check("widths", r == 2 && a == 123 && b == 45);
	r = sscanf("7 8 9", "%d %*d %d", &a, &b);
	check("suppression", r == 2 && a == 7 && b == 9);
	r = sscanf("hello world", "%s %s", s1, s2);
	check("s s", r == 2 && !strcmp(s1, "hello") && !strcmp(s2, "world"));
	r = sscanf("abc123def", "%[a-c]%n", s1, &n);
	check("set and n", r == 1 && !strcmp(s1, "a") && n == 1);
	r = sscanf("abc123def", "%[abc]%[^d]", s1, s2);
	check("set, negated set", r == 2 && !strcmp(s1, "abc") && !strcmp(s2, "123"));
	r = sscanf("x=5;y=6", "x=%d;y=%d", &a, &b);
	check("literals", r == 2 && a == 5 && b == 6);
	r = sscanf("x=5;z=6", "x=%d;y=%d", &a, &b);
	check("literal mismatch stops", r == 1 && a == 5);
	r = sscanf("", "%d", &a);
	check("empty input is EOF", r == EOF);
	r = sscanf("abc", "%d", &a);
	check("no number is 0", r == 0);
	r = sscanf("XYZ", "%2c", ch);
	check("c with width", r == 1 && ch[0] == 'X' && ch[1] == 'Y');
	r = sscanf("100% sure", "%d%% %s", &a, s1);
	check("%%", r == 2 && a == 100 && !strcmp(s1, "sure"));
	r = sscanf("40000", "%hd", &h);
	check("hd", r == 1 && h == (short)40000);
	r = sscanf("  -0x2a", "%x", &a);
	check("signed hex", r == 1 && a == -42);

	f = fopen("/tmp/scant.tst", "w");
	fprintf(f, "3 apples\n17 pears\n");
	fclose(f);
	f = fopen("/tmp/scant.tst", "r");
	r = fscanf(f, "%d %s", &a, s1);
	n = fscanf(f, "%d %s", &b, s2);
	c = fscanf(f, "%d %s", &b, s2);
	fclose(f);
	unlink("/tmp/scant.tst");
	check("fscanf", r == 2 && n == 2 && a == 3 && !strcmp(s1, "apples") && b == 17 && !strcmp(s2, "pears") && c == EOF);

	check("strerror", !strcmp(strerror(ENOENT), "No such file or directory"));
	printf("%d scanf checks failed\n", bad);
	errno = ENOENT;
	perror("perror says");
	return bad != 0;
}
