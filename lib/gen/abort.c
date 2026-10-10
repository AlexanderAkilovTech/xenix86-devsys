/* abort.c - xenix86-devsys libc (K&R C): end the program with SIGIOT (a core dump) */
#include <signal.h>

abort()
{
	signal(SIGIOT, SIG_DFL);
	kill(getpid(), SIGIOT);
	_exit(127);
}
