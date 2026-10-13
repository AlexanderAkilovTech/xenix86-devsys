/* spin.c - keep the processor busy for a while, for top to show: in the
 * program itself (u), or in the kernel, reading the same block of a file
 * over and over (s).
 *	spin u|s [seconds]
 * xenix86-devsys test program. */
extern long time();

main(argc, argv)
int argc;
char **argv;
{
	long end;
	register int i;
	register long x = 0;
	char buf[512];
	int fd = -1;

	end = time((long *)0) + (argc > 2 ? atoi(argv[2]) : 30);
	if (argc > 1 && argv[1][0] == 's')
		fd = open("/etc/passwd", 0);
	while (time((long *)0) < end)
		if (fd >= 0)
			for (i = 0; i < 50; i++) {
				lseek(fd, 0L, 0);
				read(fd, buf, sizeof buf);
			}
		else
			for (i = 0; i < 2000; i++)
				x += i * i;
	exit((int)x & 1);
}
