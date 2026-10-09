/* signal.h - xenix86-devsys (XENIX / System V signal numbers) */
#ifndef _SIGNAL_H
#define _SIGNAL_H
#define SIGHUP	1
#define SIGINT	2
#define SIGQUIT	3
#define SIGILL	4
#define SIGTRAP	5
#define SIGIOT	6
#define SIGEMT	7
#define SIGFPE	8
#define SIGKILL	9
#define SIGBUS	10
#define SIGSEGV	11
#define SIGSYS	12
#define SIGPIPE	13
#define SIGALRM	14
#define SIGTERM	15
#define SIGUSR1	16
#define SIGUSR2	17
#define SIGCLD	18
#define SIGPWR	19
#define NSIG	20

#define SIG_DFL	(int (*)())0
#define SIG_IGN	(int (*)())1

extern int (*signal())();
#endif
