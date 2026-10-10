/* time.h - xenix86-devsys */
#ifndef _TIME_H
#define _TIME_H

#ifndef _TIME_T
#define _TIME_T
typedef	long	time_t;
#endif

struct	tm {
	int	tm_sec;
	int	tm_min;
	int	tm_hour;
	int	tm_mday;
	int	tm_mon;
	int	tm_year;
	int	tm_wday;
	int	tm_yday;
	int	tm_isdst;
};

extern	long timezone;
extern	int daylight;
extern	char *tzname[];

struct tm *gmtime();
struct tm *localtime();
char	*asctime();
char	*ctime();

/* In 16-bit C an undeclared function returns int, which would cut the
   32-bit time in half. Declare it before use. */
long	time();

#endif
