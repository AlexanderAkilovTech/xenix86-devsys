/* system.c - xenix86-devsys libc (K&R C) */

#include <signal.h>

system(cmd)
char *cmd;
{
	register int pid, w;
	int status;
	int (*oint)(), (*oquit)();

	if ((pid = fork()) == 0) {
		execl("/bin/sh", "sh", "-c", cmd, (char *)0);
		_exit(127);
	}
	if (pid == -1)
		return -1;
	oint = signal(SIGINT, SIG_IGN);
	oquit = signal(SIGQUIT, SIG_IGN);
	while ((w = wait(&status)) != pid && w != -1)
		;
	signal(SIGINT, oint);
	signal(SIGQUIT, oquit);
	return w == -1 ? -1 : status;
}
