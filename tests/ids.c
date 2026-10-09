/* process ids and time: the calls that return a second word in BX */
#include <stdio.h>
#include <time.h>

main()
{
	long t;

	t = time((long *)0);
	printf("time() = %ld (%s)\n", t, t > 65535L ? "full 32 bits" : "HIGH WORD LOST");
	printf("pid %d, ppid %d\n", getpid(), getppid());
	printf("uid %d, euid %d, gid %d, egid %d\n", getuid(), geteuid(), getgid(), getegid());
	return 0;
}
