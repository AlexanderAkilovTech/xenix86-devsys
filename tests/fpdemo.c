/* fpdemo.c - floating point at a glance, for real XENIX: each line shows a
 * result and whether it is right (the wanted text came from Python's
 * printf-style formatting on the host). The math functions other than sqrt
 * are within 1 ulp but not always correctly rounded, so they are shown to 14
 * digits, which an error that small does not change here; the arithmetic,
 * sqrt and the conversions are exact, and shown in full. */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int bad;

check(what, got, want)
char *what, *got, *want;
{
	int ok = strcmp(got, want) == 0;

	if (!ok)
		bad++;
	printf("%-20s %-24s %s\n", what, got, ok ? "ok" : "WRONG");
	if (!ok)
		printf("%-20s %-24s (wanted)\n", "", want);
}

main()
{
	char buf[64];

	sprintf(buf, "%.17g", sqrt(2.0));
	check("sqrt(2.0)", buf, "1.4142135623730951");
	sprintf(buf, "%.14g", 4.0 * atan(1.0));
	check("4.0 * atan(1.0)", buf, "3.1415926535898");
	sprintf(buf, "%.14g", exp(1.0));
	check("exp(1.0)", buf, "2.718281828459");
	sprintf(buf, "%.14g", log(10.0));
	check("log(10.0)", buf, "2.302585092994");
	sprintf(buf, "%.14g", sin(1.0));
	check("sin(1.0)", buf, "0.8414709848079");
	sprintf(buf, "%.14g", cos(1.0));
	check("cos(1.0)", buf, "0.54030230586814");
	sprintf(buf, "%.14g", tan(0.5));
	check("tan(0.5)", buf, "0.54630248984379");
	sprintf(buf, "%.14g", pow(2.0, 0.5));
	check("pow(2.0, 0.5)", buf, "1.4142135623731");
	sprintf(buf, "%.17g", 1.0 / 3.0);
	check("1.0 / 3.0", buf, "0.33333333333333331");
	sprintf(buf, "%.17g", 0.1 + 0.2);
	check("0.1 + 0.2", buf, "0.30000000000000004");
	sprintf(buf, "%g", 1e300 * 1e10);
	check("1e300 * 1e10", buf, "inf");
	sprintf(buf, "%.3f", 123456.789);
	check("123456.789", buf, "123456.789");
	sprintf(buf, "%e", 6.02214076e23);
	check("6.02214076e23", buf, "6.022141e+23");
	sprintf(buf, "%g", -0.000123456);
	check("-0.000123456", buf, "-0.000123456");
	sprintf(buf, "%.17g", (double)(float)0.1);
	check("(double)(float)0.1", buf, "0.10000000149011612");
	sprintf(buf, "%.17g", atof("3.14159"));
	check("atof(\"3.14159\")", buf, "3.1415899999999999");
	sprintf(buf, "%g", floor(-2.5));
	check("floor(-2.5)", buf, "-3");
	sprintf(buf, "%g", fmod(10.0, 3.0));
	check("fmod(10.0, 3.0)", buf, "1");
	printf(bad ? "%d WRONG\n" : "all correct\n", bad);
	exit(bad != 0);
}
