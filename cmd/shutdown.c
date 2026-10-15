/* shutdown - warn whoever is logged in, stop the processes, and halt the
 * machine, so that it can be turned off.
 *	shutdown [ -h ] now
 *	shutdown [ -h ] +minutes
 * -h, halt, is the only thing an XT can do, so it is the default; 4.2BSD's
 * shutdown and Linux's take it, and it is taken here the same. With
 * +minutes, the users are warned then, at each minute of the last five,
 * and at the end. At the end: SIGTERM to every process (shutdown itself
 * ignores it), a few seconds for them to finish, every file system but
 * the root unmounted (/etc/mnttab), sync, and XENIX's own /etc/haltsys,
 * which halts it. Only root may; others can use sudo(1). XENIX's own
 * shutdown is still /etc/shutdown. xenix86-devsys system tools. */
#include <stdio.h>
#include <signal.h>

#define	UTMP	"/etc/utmp"
#define	HALTSYS	"/etc/haltsys"

char	by[16] = "root";

/* the message to every terminal someone is logged in on (USER_PROCESS
 * entries in /etc/utmp: the line at 12, the type at 26) */
warn(msg)
char *msg;
{
	FILE *u;
	unsigned char e[36];
	char tty[24], b[200];
	int fd;

	sprintf(b, "\r\n\007*** %s (shutdown by %s) ***\r\n", msg, by);
	if ((u = fopen(UTMP, "r")) == NULL)
		return;
	while (fread((char *)e, 36, 1, u) == 1) {
		if (e[26] != 7 || e[12] == 0)
			continue;
		sprintf(tty, "/dev/%.12s", (char *)e + 12);
		if ((fd = open(tty, 1)) >= 0) {
			write(fd, b, strlen(b));
			close(fd);
		}
	}
	fclose(u);
	printf("%s\n", msg);
	fflush(stdout);
}

/* every mounted file system but the root, unmounted: /etc/mnttab's
 * records are 150 bytes, the device's name in /dev first (15 bytes) */
unmountall()
{
	FILE *m;
	unsigned char rec[150];
	char cmd[60];

	if ((m = fopen("/etc/mnttab", "r")) == NULL)
		return;
	while (fread((char *)rec, 150, 1, m) == 1) {
		if (rec[0] == 0 || strcmp((char *)rec + 15, "/") == 0)
			continue;
		sprintf(cmd, "/etc/umount /dev/%.14s > /dev/null 2>&1", (char *)rec);
		system(cmd);
	}
	fclose(m);
}

usage()
{
	fprintf(stderr, "usage: shutdown [-h] now, or shutdown [-h] +minutes\n");
	exit(2);
}

main(argc, argv)
int argc;
char **argv;
{
	int i, mins = -1;
	char b[80], *s;

	for (i = 1; i < argc; i++)
		if (strcmp(argv[i], "-h") == 0)
			;
		else if (strcmp(argv[i], "now") == 0)
			mins = 0;
		else if (argv[i][0] == '+') {
			mins = 0;
			for (s = argv[i] + 1; *s >= '0' && *s <= '9'; s++)
				mins = mins * 10 + *s - '0';
			if (*s || s == argv[i] + 1 || mins > 1440)
				usage();
		} else
			usage();
	if (mins < 0)
		usage();
	if (getuid() != 0) {
		fprintf(stderr, "shutdown: only root may halt the system (sudo shutdown -h now)\n");
		exit(1);
	}
	signal(SIGTERM, SIG_IGN);
	signal(SIGHUP, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	while (mins > 0) {
		sprintf(b, "The system is going down in %d minute%s", mins, mins == 1 ? "" : "s");
		if (mins <= 5 || mins % 10 == 0)
			warn(b);
		sleep(60);
		mins--;
	}
	warn("The system is going down NOW");
	sync();
	kill(-1, SIGTERM);		/* everyone but init and the kernel */
	sleep(5);
	unmountall();
	sync();
	sync();
	execl(HALTSYS, "haltsys", (char *)0);
	fprintf(stderr, "shutdown: cannot run %s\n", HALTSYS);
	exit(1);
}
