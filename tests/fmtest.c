/* fmtest.c - for tests/unit/test_fmt.py: printf's floating point conversions
 * and strtod, scanf and the ecvt family. Input lines:
 *	p <16 hex digits> <format>	printf the double with the format
 *	s <text>			strtod: the double, where it stopped, errno
 *	f <text>			sscanf %lf and %f
 *	e <16 hex digits> <n>		ecvt and fcvt with n digits */
#include <stdio.h>
#include <errno.h>

double	strtod();
char	*ecvt(), *fcvt();

union dbl {
	double d;
	unsigned w[4];
};

char line[2100];

unsigned
hexw(p)
char *p;
{
	unsigned v = 0;
	int i;

	for (i = 0; i < 4; i++)
		v = (v << 4) | (p[i] <= '9' ? p[i] - '0' : p[i] - 'a' + 10);
	return v;
}

main()
{
	union dbl x;
	union {
		float f;
		unsigned w[2];
	} y;
	char *end, *s;
	int n, decpt, sign;

	while (fgets(line, sizeof line, stdin)) {
		line[strlen(line) - 1] = 0;
		switch (line[0]) {
		case 'p':
		case 'e':
			x.w[3] = hexw(line + 2);
			x.w[2] = hexw(line + 6);
			x.w[1] = hexw(line + 10);
			x.w[0] = hexw(line + 14);
			if (line[0] == 'p') {
				printf("[");
				n = printf(line + 19, x.d);
				printf("] %d\n", n);
			} else {
				n = atoi(line + 19);
				s = ecvt(x.d, n, &decpt, &sign);
				printf("%s %d %d ", s, decpt, sign);
				s = fcvt(x.d, n, &decpt, &sign);
				printf("%s %d %d\n", s, decpt, sign);
			}
			break;
		case 's':
			errno = 0;
			x.d = strtod(line + 2, &end);
			printf("%04x%04x%04x%04x %d %d\n", x.w[3], x.w[2], x.w[1], x.w[0],
				end - (line + 2), errno);
			break;
		case 'f':
			x.w[0] = x.w[1] = x.w[2] = x.w[3] = 0;
			y.w[0] = y.w[1] = 0;
			n = sscanf(line + 2, "%lf", &x.d);
			n += sscanf(line + 2, "%f", &y.f) * 10;
			printf("%04x%04x%04x%04x %04x%04x %d\n", x.w[3], x.w[2], x.w[1], x.w[0],
				y.w[1], y.w[0], n);
			break;
		}
	}
	exit(0);
}
