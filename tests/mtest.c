/* mtest.c - for tests/unit/test_m.py: runs the math library. Each input line
 * is a function name and its arguments (doubles as 16 hex digits, ldexp's
 * int in decimal); each output line is the result in hex and errno. */
#include <stdio.h>
#include <math.h>
#include <errno.h>

union dbl {
	double d;
	unsigned w[4];
};

char *names[] = {
	"sqrt", "exp", "log", "log10", "sin", "cos", "tan", "asin", "acos", "atan",
	"sinh", "cosh", "tanh", "fabs", "floor", "ceil", 0
};
double (*fns[])() = {
	sqrt, exp, log, log10, sin, cos, tan, asin, acos, atan,
	sinh, cosh, tanh, fabs, floor, ceil
};

char line[200], name[20];

double
hexd(p)
char *p;
{
	union dbl u;
	int i, j;
	unsigned v;

	for (i = 3; i >= 0; i--) {
		for (v = 0, j = 0; j < 4; j++, p++)
			v = (v << 4) | (*p <= '9' ? *p - '0' : *p - 'a' + 10);
		u.w[i] = v;
	}
	return u.d;
}

put(d)
double d;
{
	union dbl u;

	u.d = d;
	printf("%04x%04x%04x%04x", u.w[3], u.w[2], u.w[1], u.w[0]);
}

main()
{
	double x, y, r, i;
	int k, e;
	char *a, *b;

	while (fgets(line, sizeof line, stdin)) {
		sscanf(line, "%s", name);
		a = line + strlen(name) + 1;
		b = a + 17;
		x = hexd(a);
		errno = 0;
		for (k = 0; names[k] && strcmp(names[k], name); k++)
			;
		if (names[k])
			r = (*fns[k])(x);
		else if (!strcmp(name, "pow"))
			r = pow(x, hexd(b));
		else if (!strcmp(name, "atan2"))
			r = atan2(x, hexd(b));
		else if (!strcmp(name, "fmod"))
			r = fmod(x, hexd(b));
		else if (!strcmp(name, "hypot"))
			r = hypot(x, hexd(b));
		else if (!strcmp(name, "ldexp"))
			r = ldexp(x, atoi(b));
		else if (!strcmp(name, "frexp")) {
			r = frexp(x, &e);
			put(r);
			printf(" %d\n", e);
			continue;
		} else if (!strcmp(name, "modf")) {
			r = modf(x, &i);
			put(r);
			printf(" ");
			put(i);
			printf("\n");
			continue;
		}
		e = errno;
		put(r);
		printf(" %d\n", e);
	}
	exit(0);
}
