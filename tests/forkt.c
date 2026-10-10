/* fork, exec, wait and pipe, for the simulator's real fork */
#include <stdio.h>

main(argc, argv)
int argc;
char **argv;
{
	int fd[2], pid, status, n;
	char buf[40];

	pipe(fd);
	if ((pid = fork()) == 0) {
		close(fd[0]);
		write(fd[1], "through the pipe", 16);
		exit(7);
	}
	close(fd[1]);
	n = read(fd[0], buf, sizeof buf);
	buf[n > 0 ? n : 0] = 0;
	printf("parent read \"%s\"; ", buf);
	n = wait(&status);	/* before the printf: BCC evaluates arguments right to left */
	printf("wait gave pid %s, exit code %d\n", n == pid ? "right" : "WRONG", status >> 8);
	fflush(stdout);
	if (argc > 1 && fork() == 0) {
		execl(argv[1], argv[1], (char *)0);
		exit(99);
	}
	wait(&status);
	printf("exec'd %s: exit code %d\n", argc > 1 ? argv[1] : "nothing", status >> 8);
	return 0;
}
