/* clock.c - what date, asktime and uptime share: the settings in
 * /etc/default/date, a date and time as people write them, a local date and
 * time into the clock's seconds, setting the clock as XENIX's date does, and
 * the time of day as people read it.
 * xenix86-devsys system tools. */
#include <stdio.h>
#include <time.h>

#define	DEFAULTS "/etc/default/date"
#define	UTMP	"/etc/utmp"
#define	WTMP	"/etc/wtmp"
#define	UTSIZE	36		/* a System V utmp entry: user 8, id 4, line 12,
				 * pid 2, type 2, exit 4, time 4 */
#define	OLD_TIME 3		/* ut_type: the clock before it was set */
#define	NEW_TIME 4		/* and after */

extern long lseek();

int	clock12 = 1;		/* CLOCK=12, the default, or CLOCK=24 */
int	dmy;			/* DATE=dmy: 10/12 is the 10th of December */

static int cmdays[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
static char *at;		/* where parsewhen is in its string */

cleap(y)
int y;
{
	return y % 4 == 0 && (y % 100 != 0 || y % 400 == 0);
}

/* the settings: a line CLOCK=12 or CLOCK=24, and DATE=mdy or DATE=dmy */
clockdefs()
{
	FILE *f;
	char line[80];

	if ((f = fopen(DEFAULTS, "r")) == 0)
		return;
	while (fgets(line, sizeof line, f))
		if (strncmp(line, "CLOCK=", 6) == 0)
			clock12 = atoi(line + 6) != 24;
		else if (strncmp(line, "DATE=", 5) == 0)
			dmy = strncmp(line + 5, "dmy", 3) == 0;
	fclose(f);
}

/* a local date and time into seconds since 1970, unsigned, in *tp: 0, or -1
 * if there is no such time on the clock (1970 to 7 February 2106) */
toclock(year, mon, day, hour, min, sec, tp)
int year, mon, day, hour, min, sec;
unsigned long *tp;
{
	long d, x;
	unsigned long t;
	int y;

	if (mon < 1 || mon > 12 || day < 1 || day > cmdays[mon - 1] + (mon == 2 && cleap(year)) ||
	    hour < 0 || hour > 23 || min < 0 || min > 59 || sec < 0 || sec > 59 ||
	    year < 1970 || year > 2106)
		return -1;
	d = 0L;
	for (y = 1970; y < year; y++)
		d += cleap(y) ? 366 : 365;
	for (y = 0; y < mon - 1; y++)
		d += cmdays[y] + (y == 1 && cleap(year));
	d += day - 1;
	if (d > 49710L)			/* past 7 February 2106, the last day */
		return -1;
	/* the seconds, as standard time; an hour less if that is daylight time */
	tzset();
	t = (unsigned long)d * 86400L;
	x = hour * 3600L + min * 60 + sec + timezone;
	if (x >= 0L ? t + x < t : t < (unsigned long)-x)
		return -1;
	t += x;
	if (localtime((long *)&t)->tm_isdst) {
		if (t < 3600L)
			return -1;
		t -= 3600L;
	}
	*tp = t;
	return 0;
}

/* digits at the parse: their value, and how many (5 at most) */
static
digits(v)
int *v;
{
	int n = 0;

	*v = 0;
	while (*at >= '0' && *at <= '9' && n < 5) {
		*v = *v * 10 + *at++ - '0';
		n++;
	}
	return n;
}

/* am or pm, after any blanks: AM, am, a, a.m., any case; 1 for am, 2 for
 * pm, 0 if there is none (the parse then stays where it was) */
static
meridiem()
{
	char *q = at;
	int r;

	while (*q == ' ' || *q == '\t')
		q++;
	if (*q == 'a' || *q == 'A')
		r = 1;
	else if (*q == 'p' || *q == 'P')
		r = 2;
	else
		return 0;
	if (*++q == '.')
		q++;
	if (*q == 'm' || *q == 'M')
		if (*++q == '.')
			q++;
	if (*q && *q != ' ' && *q != '\t')
		return 0;
	at = q;
	return r;
}

/* a date and time as people write them, in local time, into *tp: 0, or -1
 * if it is not one. A date, a time, or both, in either order:
 *	10/14  10/14/26  10/14/2026  (14/10 with DATE=dmy)  2026-10-14
 *	9:05  9:05:30  9:05 AM  3:36pm  3 PM  0905  15:36
 * A time with no AM or PM is on the 24-hour clock. What is not given is
 * now's: the date, or the time of day with its seconds; a time given has
 * second 0 unless it says otherwise. */
parsewhen(s, now, tp)
char *s;
long now;
unsigned long *tp;
{
	struct tm *t = localtime(&now);
	int year = t->tm_year + 1900, mon = t->tm_mon + 1, day = t->tm_mday;
	int hour = t->tm_hour, min = t->tm_min, sec = t->tm_sec;
	int gotdate = 0, gottime = 0, ap = 0, a, b, c, na, nb, nc;

	at = s;
	for (;;) {
		while (*at == ' ' || *at == '\t')
			at++;
		if (*at == 0)
			break;
		if ((na = digits(&a)) == 0)
			return -1;
		if (*at == '/') {		/* month/day[/year] */
			if (gotdate || na > 2)
				return -1;
			at++;
			if ((nb = digits(&b)) == 0 || nb > 2)
				return -1;
			if (*at == '/') {
				at++;
				nc = digits(&c);
				if (nc == 2)
					year = c + (c < 70 ? 2000 : 1900);
				else if (nc == 4)
					year = c;
				else
					return -1;
			}
			mon = dmy ? b : a;
			day = dmy ? a : b;
			gotdate = 1;
		} else if (*at == '-') {	/* year-month-day */
			if (gotdate || na != 4)
				return -1;
			at++;
			if ((nb = digits(&b)) == 0 || nb > 2 || *at != '-')
				return -1;
			at++;
			if ((nc = digits(&c)) == 0 || nc > 2)
				return -1;
			year = a;
			mon = b;
			day = c;
			gotdate = 1;
		} else if (*at == ':') {	/* hour:minute[:second] */
			if (gottime || na > 2)
				return -1;
			at++;
			if (digits(&b) != 2)
				return -1;
			hour = a;
			min = b;
			sec = 0;
			if (*at == ':') {
				at++;
				if (digits(&c) != 2)
					return -1;
				sec = c;
			}
			gottime = 1;
			ap = meridiem();
		} else if (na == 4 && !gottime) {	/* hhmm */
			hour = a / 100;
			min = a % 100;
			sec = 0;
			gottime = 1;
			ap = meridiem();
		} else if (na <= 2 && !gottime) {	/* an hour, with AM or PM */
			hour = a;
			min = sec = 0;
			gottime = 1;
			if ((ap = meridiem()) == 0)
				return -1;
		} else
			return -1;
		if (*at && *at != ' ' && *at != '\t')
			return -1;
	}
	if (!gotdate && !gottime)
		return -1;
	if (ap) {
		if (hour < 1 || hour > 12)
			return -1;
		hour = hour % 12 + (ap == 2 ? 12 : 0);
	}
	return toclock(year, mon, day, hour, min, sec, tp);
}

/* the time of day as people read it: 3:39 PM, or 15:39 with CLOCK=24 */
char *
clockhm(t)
struct tm *t;
{
	static char s[12];
	int h = t->tm_hour;

	if (clock12)
		sprintf(s, "%d:%02d %s", h % 12 ? h % 12 : 12, t->tm_min, h < 12 ? "AM" : "PM");
	else
		sprintf(s, "%02d:%02d", h, t->tm_min);
	return s;
}

/* an "old time" or "new time" entry: in place of the last of its kind in
 * /etc/utmp, or at its end; and at the end of /etc/wtmp */
static
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

/* the clock to t, with the time before, old, and after in utmp and wtmp, as
 * XENIX's date does: 0, or -1 for one not the super-user */
setclock(t, old)
unsigned long t;
long old;
{
	if (stime((long *)&t) < 0)
		return -1;
	record(OLD_TIME, "old time", old);
	record(NEW_TIME, "new time", (long)t);
	return 0;
}
