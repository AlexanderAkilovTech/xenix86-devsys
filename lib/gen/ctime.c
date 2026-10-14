/* ctime.c - xenix86-devsys libc: gmtime, localtime, asctime, ctime, tzset (K&R C)
 *
 * The time zone comes from TZ in the System V form, e.g. TZ=PST8PDT: a standard
 * zone name, hours west of Greenwich, and an optional daylight zone name.
 * Daylight time follows the United States' rules for the year: from 2 a.m. on
 * the last Sunday in April to 2 a.m. on the last Sunday in October until 1986
 * (from January 6 in 1974 and February 23 in 1975); from the first Sunday in
 * April from 1987; and from the second Sunday in March to the first Sunday in
 * November from 2007. Without TZ, or with TZ empty, the zone is XENIX's own
 * default, PST8PDT, as its date showed it (M63).
 *
 * A time is taken as an unsigned count of seconds since 1970, which runs to
 * February 2106; taken as signed, it would turn negative in January 2038 and
 * read as 1901. Nothing on XENIX is older than 1970 to need the sign. */
#include <time.h>

extern char *getenv();

long	timezone = 8 * 3600L;	/* seconds west of Greenwich */
int	daylight = 1;		/* nonzero if a daylight zone was named */
char	*tzname[2] = { "PST", "PDT" };

static char tzstd[4], tzdst[4];
static int tzdone;
static int mdays[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
static struct tm tmbuf;
static char cbuf[26];

tzset()
{
	register char *p;
	register int i, h;

	tzdone = 1;
	if ((p = getenv("TZ")) == 0 || *p == 0)
		return;
	for (i = 0; i < 3 && *p && !(*p >= '0' && *p <= '9') && *p != '-'; i++)
		tzstd[i] = *p++;
	tzstd[i] = 0;
	tzname[0] = tzstd;
	i = *p == '-';
	if (i)
		p++;
	for (h = 0; *p >= '0' && *p <= '9'; p++)
		h = h * 10 + *p - '0';
	timezone = h * 3600L;
	if (i)
		timezone = -timezone;
	for (i = 0; i < 3 && *p; i++)
		tzdst[i] = *p++;
	tzdst[i] = 0;
	daylight = i > 0;
	if (daylight)
		tzname[1] = tzdst;
}

static
leap(y)
register int y;
{
	return y % 4 == 0 && (y % 100 != 0 || y % 400 == 0);
}

/* the time *tp, as unsigned seconds since 1970, moved by off seconds (east
 * of Greenwich is positive), into the date and time */
static struct tm *
conv(tp, off)
long *tp;
long off;
{
	register struct tm *t = &tmbuf;
	register int y, m, n;
	unsigned long u = *tp;
	long days, s;

	days = u / 86400L;
	s = u % 86400L + off;
	while (s < 0) {
		s += 86400L;
		days--;
	}
	while (s >= 86400L) {
		s -= 86400L;
		days++;
	}
	t->tm_hour = s / 3600;
	n = s % 3600;
	t->tm_min = n / 60;
	t->tm_sec = n % 60;
	t->tm_wday = (int)((days + 4) % 7);	/* 1 Jan 1970 was a Thursday */
	if (t->tm_wday < 0)
		t->tm_wday += 7;
	for (y = 1970; days < 0; )		/* 31 Dec 1969, west of Greenwich */
		days += leap(--y) ? 366 : 365;
	for (; ; y++) {
		n = leap(y) ? 366 : 365;
		if (days < n)
			break;
		days -= n;
	}
	t->tm_year = y - 1900;
	t->tm_yday = (int)days;
	n = (int)days;
	for (m = 0; ; m++) {
		y = mdays[m] + (m == 1 && leap(t->tm_year + 1900));
		if (n < y)
			break;
		n -= y;
	}
	t->tm_mon = m;
	t->tm_mday = n + 1;
	t->tm_isdst = 0;
	return t;
}

struct tm *
gmtime(tp)
long *tp;
{
	return conv(tp, 0L);
}

/* day of the year (0-based) of a month's first (n=1), second (n=2) or last
 * (n=-1) Sunday, in t's year */
static
sunday(t, mon, n)
register struct tm *t;
int mon, n;
{
	register int d, i, wd;

	d = 0;
	for (i = 0; i < mon; i++)
		d += mdays[i] + (i == 1 && leap(t->tm_year + 1900));
	/* weekday of the first of that month, from this date's known weekday */
	wd = ((t->tm_wday - (t->tm_yday - d)) % 7 + 7) % 7;
	if (n > 0)
		return d + (7 - wd) % 7 + 7 * (n - 1);
	i = mdays[mon] + (mon == 1 && leap(t->tm_year + 1900)) - 1;	/* last day */
	return d + i - ((wd + i) % 7);
}

struct tm *
localtime(tp)
long *tp;
{
	register struct tm *t;
	register int start, end, y;

	if (!tzdone)
		tzset();
	t = conv(tp, -timezone);
	if (!daylight)
		return t;
	y = t->tm_year + 1900;
	if (y >= 2007) {
		start = sunday(t, 2, 2);	/* the second Sunday in March */
		end = sunday(t, 10, 1);		/* the first in November */
	} else {
		start = y >= 1987 ? sunday(t, 3, 1) :	/* April */
			y == 1974 ? 5 : y == 1975 ? 53 : sunday(t, 3, -1);
		end = sunday(t, 9, -1);			/* the last in October */
	}
	/* from 2 a.m. standard time to 2 a.m. daylight time, 1 a.m. standard */
	if ((t->tm_yday > start || (t->tm_yday == start && t->tm_hour >= 2)) &&
	    (t->tm_yday < end || (t->tm_yday == end && t->tm_hour < 1))) {
		t = conv(tp, 3600L - timezone);
		t->tm_isdst = 1;
	}
	return t;
}

static char *
put2(p, n)
register char *p;
register int n;
{
	*p++ = n / 10 + '0';
	*p++ = n % 10 + '0';
	return p;
}

char *
asctime(t)
register struct tm *t;
{
	register char *p = cbuf;
	register char *s;
	int y;

	s = &"SunMonTueWedThuFriSat"[t->tm_wday * 3];
	*p++ = *s++; *p++ = *s++; *p++ = *s;
	*p++ = ' ';
	s = &"JanFebMarAprMayJunJulAugSepOctNovDec"[t->tm_mon * 3];
	*p++ = *s++; *p++ = *s++; *p++ = *s;
	*p++ = ' ';
	*p++ = t->tm_mday >= 10 ? t->tm_mday / 10 + '0' : ' ';
	*p++ = t->tm_mday % 10 + '0';
	*p++ = ' ';
	p = put2(p, t->tm_hour);
	*p++ = ':';
	p = put2(p, t->tm_min);
	*p++ = ':';
	p = put2(p, t->tm_sec);
	*p++ = ' ';
	y = t->tm_year + 1900;
	p = put2(p, y / 100);
	p = put2(p, y % 100);
	*p++ = '\n';
	*p = 0;
	return cbuf;
}

char *
ctime(tp)
long *tp;
{
	return asctime(localtime(tp));
}
