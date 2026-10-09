/* proc.c - xenix86-devsys libc: processes, pipes, time (K&R C) */
#include <signal.h>

extern char **environ;
extern long _wait(), _time(), _pipe();
extern int errno;

wait(status)
int *status;
{
	long r = _wait();

	if ((int)r == -1)
		return -1;
	if (status)
		*status = (int)(r >> 16);
	return (int)r;
}

pipe(fd)
int *fd;
{
	long r = _pipe();

	if (r == -1L)
		return -1;
	fd[0] = (int)r;
	fd[1] = (int)(r >> 16);
	return 0;
}

long
time(tp)
long *tp;
{
	long t = _time();

	if (tp)
		*tp = t;
	return t;
}

execv(path, argv)
char *path, **argv;
{
	return execve(path, argv, environ);
}

execl(path, arg0)
char *path, *arg0;
{
	return execve(path, &arg0, environ);
}

execle(path, arg0)
char *path, *arg0;
{
	register char **p = &arg0;

	while (*p++)
		;
	return execve(path, &arg0, *(char ***)p);
}

/* execvp: search $PATH when the name has no slash */
execvp(file, argv)
char *file, **argv;
{
	extern char *getenv(), *strchr();
	register char *path, *d;
	char buf[128];
	register int n;

	if (strchr(file, '/'))
		return execv(file, argv);
	if ((path = getenv("PATH")) == 0)
		path = ":/bin:/usr/bin";
	for (;;) {
		d = buf;
		while (*path && *path != ':')
			*d++ = *path++;
		if (d != buf)
			*d++ = '/';
		for (n = 0; file[n]; n++)
			*d++ = file[n];
		*d = 0;
		execv(buf, argv);
		if (errno == 8) {	/* ENOEXEC: hand it to the shell */
			argv[-1] = "sh";
			argv[0] = buf;
			return execv("/bin/sh", argv - 1);
		}
		if (*path == 0)
			return -1;
		path++;
	}
}

execlp(file, arg0)
char *file, *arg0;
{
	return execvp(file, &arg0);
}

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

static
sleepcatch()
{
}

unsigned
sleep(n)
unsigned n;
{
	int (*old)();

	if (n == 0)
		return 0;
	old = signal(SIGALRM, sleepcatch);
	alarm(n);
	pause();
	signal(SIGALRM, old);
	return 0;
}
