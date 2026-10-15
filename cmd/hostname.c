/* hostname - print or set the name of this machine. XENIX 2.1.3 keeps it
 * as the first line of /etc/systemid, the file Micnet and UUCP read; the
 * kernel has a node name of its own, which uname -n prints, but nothing on
 * XENIX sets it (M60). Setting the name sets both: the file, and the
 * kernel's, through /dev/kmem, in its utsname and in uts3name, the copy for
 * XENIX 3 programs, 9 bytes into each (M65). The kernel's is gone when the
 * machine is started again, so asktime, at every start, gives the kernel
 * the file's name again with hostname `hostname`.
 *	hostname		print the name
 *	hostname name		set it (root), keeping the file's other lines
 * xenix86-devsys system tools. */
#include <stdio.h>
#include <ctype.h>

#define	SYSID	"/etc/systemid"
#define	MAXN	8		/* the kernel's field: 8 characters and a NUL */

extern char *strchr(), *strncpy();
extern long lseek();

long	get4(p) unsigned char *p; { return p[0] | p[1] << 8 | (long)p[2] << 16 | (long)p[3] << 24; }
unsigned get2(p) unsigned char *p; { return p[0] | p[1] << 8; }

/* the kernel's node name: 0, or -1 if it could not be set */
setnode(name)
char *name;
{
	static char *want[2] = { "_utsname", "_uts3name" };
	unsigned char h[64], seg[32], r[8];
	char sym[16], field[MAXN + 1];
	long addr[2], segpos, segsize, pos, end;
	int i, n, c, k, got = 0, fd;
	FILE *f;

	addr[0] = addr[1] = -1L;
	if ((f = fopen("/xenix", "r")) == 0 || fread((char *)h, 1, 64, f) != 64) {
		if (f)
			fclose(f);
		return -1;
	}
	segpos = get4(h + 52);
	segsize = get4(h + 56);
	for (i = 0; i < segsize && got < 2; i += 32) {
		fseek(f, segpos + i, 0);
		if (fread((char *)seg, 1, 32, f) != 32 || get2(seg) != 3)
			continue;
		pos = get4(seg + 8);
		end = pos + get4(seg + 12);
		fseek(f, pos, 0);
		/* each symbol: 8 bytes, the value at 4, then its name and a NUL */
		while (pos < end && got < 2 && fread((char *)r, 1, 8, f) == 8) {
			for (n = 0; (c = getc(f)) != EOF && c; n++)
				if (n < 15)
					sym[n] = c;
			sym[n < 15 ? n : 15] = 0;
			pos += 9 + n;
			for (k = 0; k < 2; k++)
				if (addr[k] < 0 && strcmp(sym, want[k]) == 0) {
					addr[k] = get4(r + 4);
					got++;
				}
		}
	}
	fclose(f);
	if (addr[0] < 0 || (fd = open("/dev/kmem", 2)) < 0)
		return -1;
	memset(field, 0, sizeof field);
	strncpy(field, name, MAXN);
	for (k = 0; k < 2; k++)
		if (addr[k] >= 0 && (lseek(fd, addr[k] + 9, 0) != addr[k] + 9
		    || write(fd, field, sizeof field) != sizeof field)) {
			close(fd);
			return -1;
		}
	close(fd);
	return 0;
}

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
	if (setnode(argv[1]) < 0) {
		fprintf(stderr, "hostname: cannot set the kernel's node name\n");
		exit(1);
	}
	exit(0);
}
