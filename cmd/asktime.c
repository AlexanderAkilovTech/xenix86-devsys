/* asktime - the time prompt when XENIX starts, in place of XENIX's own.
 *	asktime [ -a ]
 * /etc/rc runs it at every start, as root, with -a when the machine starts
 * by itself (autoboot), when it waits 30 seconds for an answer and then
 * goes on. XENIX on an XT has no clock that runs while it is off: its clock
 * starts from the date in the root file system's super-block, the time it
 * last wrote to it, so after a night off it is a night behind.
 * XENIX's asktime takes only [yymmdd]hhmm, and its /lib/cvtdate turns a
 * year past 1999 into 1970 (M65). This one takes the time as people write
 * it, as date does (3:36 PM, 10/14 9:05 AM, 2026-10-14 15:36; clock.c),
 * and XENIX's forms as well: hhmm, ddhhmm, mmddhhmm, yymmddhhmm. Return
 * keeps the time as it is.
 * As XENIX's does, with a real-time clock (/dev/clock) it sets the clock
 * from it first, through /etc/setclock, and sets it after from the clock.
 * It also gives the kernel the machine's name from /etc/systemid, which
 * uname -n prints and which is lost at every start (hostname): XENIX's
 * start-up has nowhere else to do it. xenix86-devsys system tools. */
#include <stdio.h>
#include <signal.h>
#include <time.h>

extern long time();
extern struct tm *localtime();
extern char *asctime(), *clockhm();
extern int clock12, dmy;

/* -a: no answer in 30 seconds; the time stays */
onalarm()
{
	printf("\n");
	exit(0);
}

/* the clock as XENIX's date prints it, without its newline */
char *
now()
{
	static char s[40];
	long t;
	struct tm *tm;
	char *a;

	time(&t);
	tm = localtime(&t);
	a = asctime(tm);
	sprintf(s, "%.20s%s%.5s, %s", a, tzname[tm->tm_isdst], a + 19, clockhm(tm));
	return s;
}

/* XENIX's [yymmdd]hhmm, all digits: 4, 6, 8 or 10 of them, into *tp; 0,
 * or -1 if it is not that or not a time */
xenixform(s, old, tp)
char *s;
long old;
unsigned long *tp;
{
	struct tm *t = localtime(&old);
	int n, d[5], year = t->tm_year + 1900, mon = t->tm_mon + 1, day = t->tm_mday;

	for (n = 0; s[n] >= '0' && s[n] <= '9'; n++)
		;
	if (s[n] || n != 6 && n != 8 && n != 10)
		return -1;
	for (n = 0; s[2 * n]; n++)
		d[n] = (s[2 * n] - '0') * 10 + s[2 * n + 1] - '0';
	/* from the end: minute, hour, day, month, year */
	if (n == 5)
		year = d[0] + (d[0] < 70 ? 2000 : 1900);
	if (n >= 4)
		mon = d[n - 4];
	day = d[n - 3];
	return toclock(year, mon, day, d[n - 2], d[n - 1], 0, tp);
}

main(argc, argv)
int argc;
char **argv;
{
	char line[80], *p;
	long old;
	unsigned long t;
	int rtc, fd;

	if (argc > 1 && strcmp(argv[1], "-a") == 0) {
		signal(SIGALRM, onalarm);
		alarm(30);
	}
	clockdefs();
	if ((fd = open("/dev/clock", 0)) >= 0) {
		close(fd);
		rtc = 1;
		system("date `/etc/setclock` > /dev/null 2>&1");
	} else
		rtc = 0;
	if (access("/usr/bin/hostname", 1) == 0)
		system("/usr/bin/hostname `/usr/bin/hostname` > /dev/null 2>&1");
	printf("\nCurrent System Time is %s\n", now());
	printf("Enter the time as %s or %s %s, or Return to keep it: ",
		clock12 ? "3:36 PM" : "15:36", dmy ? "14/10" : "10/14", clock12 ? "9:05 AM" : "09:05");
	for (;;) {
		fflush(stdout);
		if (fgets(line, sizeof line, stdin) == 0)
			break;
		if ((p = (char *)strchr(line, '\n')) != 0)
			*p = 0;
		for (p = line; *p == ' ' || *p == '\t'; p++)
			;
		if (*p == 0)
			break;
		time(&old);
		if (xenixform(p, old, &t) == 0 || parsewhen(p, old, &t) == 0) {
			if (setclock(t, old) < 0) {
				printf("Only root can set the clock.\n");
				break;
			}
			printf("The time is now %s\n", now());
			break;
		}
		printf("That is not a time; try again, or Return to keep it: ");
	}
	alarm(0);
	if (rtc)
		system("/etc/setclock `date +%m%d%H%M%y` > /dev/null 2>&1");
	exit(0);
}
