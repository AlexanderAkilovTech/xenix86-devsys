/* uptime - how long the machine has been up, and how many are logged in, as
 * 4.2BSD's uptime and w print it:
 *	 5:02 PM  up 2 days,  3:04,  2 users
 * The time of day is as /etc/default/date says, 12- or 24-hour (clock.c).
 * The time up is times()'s count of the clock's ticks since the machine
 * started, 20 a second, which goes on whatever the clock is set to; the
 * users are the logged-in entries of /etc/utmp. XENIX's kernel keeps no load
 * average, so there is none to print. xenix86-devsys system tools. */
#include <stdio.h>
#include <sys/types.h>
#include <sys/times.h>
#include <time.h>

#define	UTMP	"/etc/utmp"
#define	UTSIZE	36
#define	USER_PROCESS 7

extern long times();
extern char *clockhm();

char *
plural(n)
long n;
{
	return n == 1 ? "" : "s";
}

main(argc, argv)
int argc;
char **argv;
{
	struct tms tb;
	unsigned char u[UTSIZE];
	unsigned long up;
	long now, days, hrs, mins;
	int fd, users = 0;

	if (argc > 1) {
		fprintf(stderr, "usage: uptime\n");
		exit(2);
	}
	clockdefs();
	up = (unsigned long)times(&tb) / 20;
	time(&now);
	if ((fd = open(UTMP, 0)) >= 0) {
		while (read(fd, (char *)u, UTSIZE) == UTSIZE)
			if (u[26] == USER_PROCESS && u[27] == 0 && u[0])
				users++;
		close(fd);
	}
	days = up / 86400L;
	hrs = up % 86400L / 3600;
	mins = up % 3600 / 60;
	printf(" %s  up ", clockhm(localtime(&now)));
	if (days > 0)
		printf("%ld day%s, ", days, plural(days));
	if (hrs > 0 && mins > 0)
		printf("%2ld:%02ld, ", hrs, mins);
	else if (hrs > 0)
		printf("%ld hr%s, ", hrs, plural(hrs));
	else if (mins > 0)
		printf("%ld min%s, ", mins, plural(mins));
	else if (days == 0)
		printf("%ld sec%s, ", (long)up, plural((long)up));
	printf(" %d user%s\n", users, plural((long)users));
	exit(0);
}
