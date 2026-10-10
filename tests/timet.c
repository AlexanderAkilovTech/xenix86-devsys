/* time conversion test: for each number on stdin, print gmtime fields and ctime */
#include <stdio.h>
#include <time.h>

extern long atol();

main()
{
	char line[40];
	long t;
	struct tm g;
	char *c;

	while (fgets(line, sizeof line, stdin)) {
		t = atol(line);
		g = *gmtime(&t);	/* copy: ctime reuses the same static buffer */
		c = ctime(&t);
		printf("%ld %d %d %d %d %d %d %d %d | %s", t, g.tm_year, g.tm_mon, g.tm_mday,
			g.tm_hour, g.tm_min, g.tm_sec, g.tm_wday, g.tm_yday, c);
	}
	return 0;
}
