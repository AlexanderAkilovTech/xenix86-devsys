/* xsyms.c - every symbol in an x.out's symbol table (/xenix's, by default):
 * its type, its value and its name, one to a line, to look for the
 * kernel's counters among them.
 *	xsyms [file]
 * xenix86-devsys test program. */
#include <stdio.h>

extern long lseek();
long	get4(p) unsigned char *p; { return p[0] | p[1] << 8 | (long)p[2] << 16 | (long)p[3] << 24; }
unsigned get2(p) unsigned char *p; { return p[0] | p[1] << 8; }

main(argc, argv)
int argc;
char **argv;
{
	int fd, i, n;
	unsigned char h[64], seg[32];
	static char buf[1100];
	char *p, *name, *e, *lim;
	long segpos, segsize, pos, end, count = 0;

	if ((fd = open(argc > 1 ? argv[1] : "/xenix", 0)) < 0) {
		printf("cannot open\n");
		exit(1);
	}
	read(fd, (char *)h, 64);
	segpos = get4(h + 52);
	segsize = get4(h + 56);
	for (i = 0; i < segsize; i += 32) {
		lseek(fd, segpos + i, 0);
		read(fd, (char *)seg, 32);
		if (get2(seg) != 3)
			continue;
		pos = get4(seg + 8);
		end = pos + get4(seg + 12);
		while (pos < end) {
			lseek(fd, pos, 0);
			if ((n = read(fd, buf, 1024)) <= 8)
				break;
			if (end - pos < n)
				n = (int)(end - pos);
			buf[n] = 0;
			lim = buf + n;
			for (p = buf; p + 8 < lim; p = e + 1) {
				name = p + 8;
				if ((e = name + strlen(name)) >= lim)
					break;
				printf("%04x %04x %06lx %s\n", get2((unsigned char *)p), get2((unsigned char *)p + 2),
					get4((unsigned char *)p + 4), name);
				count++;
			}
			if (p == buf)
				break;
			pos += p - buf;
		}
	}
	printf("%ld symbols\n", count);
	exit(0);
}
