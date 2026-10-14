/* utdump - the entries of a utmp file (/etc/utmp, /etc/wtmp), one a line,
 * for looking at what XENIX's date writes when it sets the clock (M63).
 *	utdump [-n count] file		the last count entries, or all of them
 * xenix86-devsys test program. */
#include <stdio.h>

#define	UTSIZE	36

extern long lseek();

long	get4(p) unsigned char *p; { return p[0] | p[1] << 8 | (long)p[2] << 16 | (long)p[3] << 24; }
unsigned get2(p) unsigned char *p; { return p[0] | p[1] << 8; }

main(argc, argv)
int argc;
char **argv;
{
	unsigned char u[UTSIZE];
	long size;
	int fd, n = 0;

	if (argc == 4 && strcmp(argv[1], "-n") == 0) {
		n = atoi(argv[2]);
		argv += 2;
		argc -= 2;
	}
	if (argc != 2 || (fd = open(argv[1], 0)) < 0) {
		printf("utdump: cannot read %s\n", argv[1]);
		exit(1);
	}
	size = lseek(fd, 0L, 2);
	printf("%s: %ld bytes, %ld entries\n", argv[1], size, size / UTSIZE);
	if (n > 0 && size / UTSIZE > n)
		lseek(fd, size - (long)n * UTSIZE, 0);
	else
		lseek(fd, 0L, 0);
	while (read(fd, (char *)u, UTSIZE) == UTSIZE)
		printf("type %u pid %u line '%.12s' id '%.4s' user '%.8s' exit %u %u time %ld\n",
			get2(u + 26), get2(u + 24), (char *)u + 12, (char *)u + 8, (char *)u,
			get2(u + 28), get2(u + 30), get4(u + 32));
	exit(0);
}
