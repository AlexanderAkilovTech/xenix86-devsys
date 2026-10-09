/* Milestone 4 test: fork, exec, wait, pipe, signals, time */
#include <stdio.h>
#include <signal.h>
#include <time.h>

int caught;

onalrm(sig)
int sig;
{
	caught = sig;
}

main()
{
	int pid, status, fd[2], n;
	char buf[64];
	long t0, t1;

	t0 = time((long *)0);
	printf("1. time() = %ld seconds since 1970\n", t0);
	printf("   my pid %d, parent %d\n", getpid(), getppid());
	fflush(stdout);

	printf("2. fork + exec /bin/echo\n");
	fflush(stdout);
	if ((pid = fork()) == 0) {
		execl("/bin/echo", "echo", "   (child) hello from /bin/echo, exec'd by our libc", (char *)0);
		_exit(99);
	}
	wait(&status);
	printf("   child %d finished, status %d (exit code %d)\n", pid, status, (status >> 8) & 0377);

	printf("3. fork + exit code\n");
	fflush(stdout);
	if ((pid = fork()) == 0)
		_exit(42);
	wait(&status);
	printf("   child exited with %d\n", (status >> 8) & 0377);

	printf("4. pipe between processes\n");
	fflush(stdout);
	pipe(fd);
	if ((pid = fork()) == 0) {
		close(fd[0]);
		write(fd[1], "message through a pipe", 22);
		_exit(0);
	}
	close(fd[1]);
	n = read(fd[0], buf, sizeof buf - 1);
	buf[n > 0 ? n : 0] = 0;
	close(fd[0]);
	wait(&status);
	printf("   parent read %d bytes: \"%s\"\n", n, buf);

	printf("5. signals: alarm(2) with a handler\n");
	fflush(stdout);
	signal(SIGALRM, onalrm);
	alarm(2);
	pause();
	printf("   handler ran, caught signal %d\n", caught);

	printf("6. sleep(1) and system()\n");
	fflush(stdout);
	sleep(1);
	status = system("ls -l /etc/passwd");
	printf("   system() returned %d\n", status);
	t1 = time((long *)0);
	printf("   %ld seconds elapsed\ndone\n", t1 - t0);
	return 0;
}
