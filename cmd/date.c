/* date - print the date and time, or set the clock.
 *	date [ +format ]
 *	date mmddhhmm[[cc]yy]
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
 * argument after the first. -u, with which XENIX's date uses a time-of-year
 * clock, finds none, as XENIX's does on an XT: this one cannot read one.
 * xenix86-devsys system tools. */
#include <stdio.h>
#include <time.h>

#define	UTMP	"/etc/utmp"
#define	WTMP	"/etc/wtmp"
#define	UTSIZE	36		/* a System V utmp entry: user 8, id 4, line 12,
				 * pid 2, type 2, exit 4, time 4 */
#define	OLD_TIME 3		/* ut_type: the clock before it was set */
#define	NEW_TIME 4		/* and after */

extern long time(), lseek();
extern struct tm *localtime();

int	mdays[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

leap(y)
int y;
{
	return y % 4 == 0 && (y % 100 != 0 || y % 400 == 0);
}

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

/* an "old time" or "new time" entry: in place of the last of its kind in
 * /etc/utmp, or at its end; and at the end of /etc/wtmp */
record(type, msg, t)
int type;
char *msg;
long t;
{
	unsigned char u[UTSIZE], e[UTSIZE];
	long off, at = -1L;
	int fd;

	memset((char *)u, 0, UTSIZE);
	strcpy((char *)u + 12, msg);
	u[26] = type;
	u[32] = t;
	u[33] = t >> 8;
	u[34] = t >> 16;
	u[35] = t >> 24;
	if ((fd = open(UTMP, 2)) >= 0) {
		for (off = 0L; read(fd, (char *)e, UTSIZE) == UTSIZE; off += UTSIZE)
			if (e[26] == type && e[27] == 0)
				at = off;
		if (at >= 0L)
			lseek(fd, at, 0);
		else
			lseek(fd, off, 0);
		write(fd, (char *)u, UTSIZE);
		close(fd);
	}
	if ((fd = open(WTMP, 1)) >= 0) {
		lseek(fd, 0L, 2);
		write(fd, (char *)u, UTSIZE);
		close(fd);
	}
}

/* set the clock from mmddhhmm, mmddhhmmyy or mmddhhmmccyy, as local time */
set(s)
register char *s;
{
	int n, mon, day, hour, min, year, y;
	long old, d, x;
	unsigned long t;

	for (n = 0; s[n]; n++)
		if (s[n] < '0' || s[n] > '9')
			bad();
	if (n != 8 && n != 10 && n != 12)
		bad();
	mon = num(s);
	day = num(s + 2);
	hour = num(s + 4);
	min = num(s + 6);
	time(&old);
	if (n == 8)
		year = localtime(&old)->tm_year + 1900;
	else if (n == 10)
		year = num(s + 8) + (num(s + 8) < 70 ? 2000 : 1900);
	else
		year = num(s + 8) * 100 + num(s + 10);
	if (mon < 1 || mon > 12 || day < 1 || day > mdays[mon - 1] + (mon == 2 && leap(year)) ||
	    hour > 23 || min > 59 || year < 1970 || year > 2106)
		bad();
	d = 0L;
	for (y = 1970; y < year; y++)
		d += leap(y) ? 366 : 365;
	for (y = 0; y < mon - 1; y++)
		d += mdays[y] + (y == 1 && leap(year));
	d += day - 1;
	if (d > 49710L)			/* past 7 Feb 2106, the last day there is */
		bad();
	/* the seconds, as standard time; an hour less if that is daylight time */
	t = (unsigned long)d * 86400L;
	x = hour * 3600L + min * 60 + timezone;
	if (x >= 0L ? t + x < t : t < (unsigned long)-x)
		bad();
	t += x;
	if (localtime((long *)&t)->tm_isdst) {
		if (t < 3600L)
			bad();
		t -= 3600L;
	}
	if (stime((long *)&t) < 0) {		/* XENIX's then prints the time */
		fprintf(stderr, "date: no permission\n");	/* it did not set */
		exit(2);
	}
	record(OLD_TIME, "old time", old);
	record(NEW_TIME, "new time", (long)t);
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
	if (argc > 1 && argv[1][0] != '+')
		set(argv[1]);
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
