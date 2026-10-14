/* hostname - print or set the name of this machine. XENIX 2.1.3 keeps it
 * as the first line of /etc/systemid, the file Micnet and UUCP read; the
 * kernel's own node name (uname -n) is empty, and nothing sets it (M60).
 *	hostname		print the name
 *	hostname name		set it (root), keeping the file's other lines
 * xenix86-devsys system tools. */
#include <stdio.h>
#include <ctype.h>

#define	SYSID	"/etc/systemid"
#define	MAXN	8

extern char *strchr();

main(argc, argv)
int argc;
char **argv;
{
	char buf[1024], *rest, *p;
	int fd, n;

	if (argc > 2 || argc == 2 && argv[1][0] == '-') {
		fprintf(stderr, "usage: hostname [name]\n");
		exit(2);
	}
	n = 0;
	if ((fd = open(SYSID, 0)) >= 0) {
		n = read(fd, buf, sizeof buf - 1);
		close(fd);
	}
	if (n < 0)
		n = 0;
	buf[n] = 0;
	rest = (p = strchr(buf, '\n')) != 0 ? p + 1 : buf + n;
	if (argc == 1) {
		if (fd < 0) {
			fprintf(stderr, "hostname: cannot read %s\n", SYSID);
			exit(1);
		}
		if (p)
			*p = 0;
		printf("%s\n", buf);
		exit(0);
	}
	/* the new name: letters, digits and -, at most MAXN */
	p = argv[1];
	n = strlen(p);
	if (n == 0 || n > MAXN) {
		fprintf(stderr, "hostname: a name is 1 to %d letters, digits and -\n", MAXN);
		exit(1);
	}
	for (; *p; p++)
		if (!isalnum(*p) && *p != '-') {
			fprintf(stderr, "hostname: a name is 1 to %d letters, digits and -\n", MAXN);
			exit(1);
		}
	/* written over the old, the lines after the first as they were */
	if ((fd = creat(SYSID, 0644)) < 0) {
		fprintf(stderr, "hostname: cannot write %s%s\n", SYSID,
			getuid() ? " (only root can)" : "");
		exit(1);
	}
	if (write(fd, argv[1], strlen(argv[1])) < 0 || write(fd, "\n", 1) != 1
	    || *rest && write(fd, rest, strlen(rest)) != strlen(rest)) {
		fprintf(stderr, "hostname: cannot write %s\n", SYSID);
		exit(1);
	}
	close(fd);
	exit(0);
}
