/* date - print the date and time, or set the clock.
 *	date [ +format ]
 *	date mmddhhmm[[cc]yy]
 *	date [ month/day[/year] | year-month-day ] [ hour:minute [ AM | PM ] ]
 * As XENIX's date (System V's), with the years past 1999. XENIX's takes a
 * two-digit year, and from February 29, 2000 on prints the date a day ahead,
 * for it takes 2000 for a common year (the 29th shows as March 1, December 31
 * as January 1, 2001), and %y as ":0" for 2000 and "<6" for 2026 (M62, M63).
 * This one takes the century as well, or reads 00 to 69 as 2000 to 2069; it
 * prints any date to February 2106, with this libc's ctime, which has the
 * daylight time rules of 2007 on as well; and %Y gives the year in four
 * digits.
 * Setting the clock also writes the time before and after as "old time" and
 * "new time" entries in /etc/utmp (in place of the last ones, or at its
 * end) and at the end of /etc/wtmp, as XENIX's does (M63).
 * XENIX's messages, statuses and formats, checked against it letter by
 * letter (M63), but where it takes what is not a time: XENIX's sets the
 * clock from February 30 (to March 2) and from 1013120098x or 10131200981
 * (ignoring the end); this one refuses them. Like XENIX's, it ignores any
 * argument after a format or a time in digits. -u, with which XENIX's date
 * uses a time-of-year clock, finds none, as XENIX's does on an XT: this one
 * cannot read one.
 * It also sets the clock from a date and time as people write them, in its
 * arguments together: 3:36 PM, 10/14 9:05 AM, 2026-10-14 15:36 (clock.c),
 * month/day or day/month as /etc/default/date says.
 * xenix86-devsys system tools. */
#include <stdio.h>
#include <time.h>

extern long time(), lseek();
extern struct tm *localtime();

two(n)
int n;
{
	putchar(n / 10 + '0');
	putchar(n % 10 + '0');
}

three(s)
char *s;
{
	putchar(s[0]);
	putchar(s[1]);
	putchar(s[2]);
}

hms(t)
struct tm *t;
{
	two(t->tm_hour);
	putchar(':');
	two(t->tm_min);
	putchar(':');
	two(t->tm_sec);
}

format(f, t)
register char *f;
register struct tm *t;
{
	register int c;

	while ((c = *f++) != 0) {
		if (c != '%') {
			putchar(c);
			continue;
		}
		switch (c = *f++) {
		case '%':	putchar('%'); break;
		case 'n':	putchar('\n'); break;
		case 't':	putchar('\t'); break;
		case 'm':	two(t->tm_mon + 1); break;
		case 'd':	two(t->tm_mday); break;
		case 'y':	two(t->tm_year % 100); break;
		case 'Y':	two((t->tm_year + 1900) / 100); two(t->tm_year % 100); break;
		case 'D':	two(t->tm_mon + 1); putchar('/'); two(t->tm_mday);
				putchar('/'); two(t->tm_year % 100); break;
		case 'H':	two(t->tm_hour); break;
		case 'M':	two(t->tm_min); break;
		case 'S':	two(t->tm_sec); break;
		case 'T':	hms(t); break;
		case 'j':	putchar((t->tm_yday + 1) / 100 + '0'); two((t->tm_yday + 1) % 100); break;
		case 'w':	putchar(t->tm_wday + '0'); break;
		case 'a':	three(&"SunMonTueWedThuFriSat"[t->tm_wday * 3]); break;
		case 'h':	three(&"JanFebMarAprMayJunJulAugSepOctNovDec"[t->tm_mon * 3]); break;
		case 'r':	two(t->tm_hour % 12 ? t->tm_hour % 12 : 12); putchar(':');
				two(t->tm_min); putchar(':'); two(t->tm_sec);
				fputs(t->tm_hour < 12 ? " AM" : " PM", stdout); break;
		default:
			fflush(stdout);
			fprintf(stderr, "date: bad format character - %c\n", c);
			exit(2);
		}
	}
	putchar('\n');
}

bad()
{
	fprintf(stderr, "date: bad conversion\n");
	exit(2);
}

num(s)
char *s;
{
	return (s[0] - '0') * 10 + s[1] - '0';
}

/* set the clock from mmddhhmm, mmddhhmmyy or mmddhhmmccyy, as local time,
 * as XENIX's date does (with the century), or from a date and time as
 * people write them, all the arguments together (clock.c's parsewhen):
 * 3:36 PM, 10/14 9:05 AM, 2026-10-14 15:36 */
set(argc, argv)
int argc;
char **argv;
{
	register char *s = argv[1];
	char buf[80];
	int n, year, i;
	long old;
	unsigned long t;

	time(&old);
	for (n = 0; s[n] >= '0' && s[n] <= '9'; n++)
		;
	if (s[n] == 0 && (n == 8 || n == 10 || n == 12)) {
		if (n == 8)
			year = localtime(&old)->tm_year + 1900;
		else if (n == 10)
			year = num(s + 8) + (num(s + 8) < 70 ? 2000 : 1900);
		else
			year = num(s + 8) * 100 + num(s + 10);
		if (toclock(year, num(s), num(s + 2), num(s + 4), num(s + 6), 0, &t) < 0)
			bad();
	} else {
		buf[0] = 0;
		for (i = 1; i < argc; i++) {
			if (strlen(buf) + strlen(argv[i]) + 2 > sizeof buf)
				bad();
			if (i > 1)
				strcat(buf, " ");
			strcat(buf, argv[i]);
		}
		if (parsewhen(buf, old, &t) < 0)
			bad();
	}
	if (setclock(t, old) < 0) {	/* XENIX's then prints the time it did */
		fprintf(stderr, "date: no permission\n");	/* not set */
		exit(2);
	}
}

main(argc, argv)
int argc;
char **argv;
{
	long now;
	register struct tm *t;
	register char *s;

	tzset();
	if (argc > 1 && strcmp(argv[1], "-u") == 0) {
		fprintf(stderr, "date: no TOY clock\n");
		exit(2);
	}
	if (argc > 1 && argv[1][0] != '+') {
		clockdefs();
		set(argc, argv);
	}
	time(&now);
	t = localtime(&now);
	if (argc > 1 && argv[1][0] == '+')
		format(argv[1] + 1, t);
	else {
		s = asctime(t);
		printf("%.20s%s%s", s, tzname[t->tm_isdst], s + 19);
	}
	exit(0);
}
