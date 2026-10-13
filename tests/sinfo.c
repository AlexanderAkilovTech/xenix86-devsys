/* sinfo.c - probe for xtop: the kernel's sysinfo counters (System V's, as
 * sar reads them: processor ticks idle, in programs and in the kernel;
 * ticks waiting for I/O and for swapping; blocks read and written; swaps;
 * switches and system calls; characters on the terminals), read twice
 * some seconds apart; and each process's controlling terminal from its u
 * area (u_ttyp at 1336, u_ttyd at 1338, counted from the measured u_utime
 * at 1296 and u_comm at 1388), beside the devices in /dev.
 *	sinfo seconds
 * xenix86-devsys test program. */
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

char	*want[] = { "_sysinfo", "_syswait", "_proc", "_v", 0 };
long	sym[4];
int	got[4];
char	*field[] = { "cpu idle", "cpu user", "cpu kernel", "wait io", "wait swap", "wait pio",
	"bread", "bwrite", "lread", "lwrite", "swapin", "swapout", "pswitch", "qswitch",
	"idle", "preempt", "syscall", "iget", "namei", "dirblk", "readch", "writech",
	"rcvint", "xmtint", "mdmint", "rawch", "canch", "outch", 0 };
int	kmem, mem;
extern long lseek();

long	get4(p) unsigned char *p; { return p[0] | p[1] << 8 | (long)p[2] << 16 | (long)p[3] << 24; }
unsigned get2(p) unsigned char *p; { return p[0] | p[1] << 8; }

/* the symbols, from /xenix's x.out symbol table (as xtop finds them) */
nlist(file)
char *file;
{
	int fd, i, k, n, left;
	unsigned char h[64], seg[32];
	static char buf[1100];
	char *p, *name, *e, *lim;
	long segpos, segsize, pos, end;

	if ((fd = open(file, 0)) < 0)
		return 0;
	read(fd, (char *)h, 64);
	segpos = get4(h + 52);
	segsize = get4(h + 56);
	for (left = 0; want[left]; left++)
		;
	for (i = 0; i < segsize && left; i += 32) {
		lseek(fd, segpos + i, 0);
		read(fd, (char *)seg, 32);
		if (get2(seg) != 3)
			continue;
		pos = get4(seg + 8);
		end = pos + get4(seg + 12);
		while (left && pos < end) {
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
				for (k = 0; want[k]; k++)
					if (!got[k] && strcmp(name, want[k]) == 0) {
						sym[k] = get4((unsigned char *)p + 4);
						got[k] = 1;
						left--;
					}
			}
			if (p == buf)
				break;
			pos += p - buf;
		}
	}
	close(fd);
	return 1;
}

kread(a, b, n)
long a;
char *b;
int n;
{
	lseek(kmem, a, 0);
	return read(kmem, b, n);
}

/* the devices in /dev that are terminals: name and number */
devs()
{
	int fd;
	struct { unsigned short ino; char name[14]; } d;
	struct stat st;
	char path[30];

	if ((fd = open("/dev", 0)) < 0)
		return;
	while (read(fd, (char *)&d, 16) == 16) {
		if (d.ino == 0)
			continue;
		sprintf(path, "/dev/%.14s", d.name);
		if (stat(path, &st) == 0 && (st.st_mode & S_IFMT) == S_IFCHR &&
		    (strncmp(d.name, "tty", 3) == 0 || strcmp(d.name, "console") == 0 || strcmp(d.name, "syscon") == 0 ||
		     strcmp(d.name, "systty") == 0))
			printf("  %-14.14s %04x\n", d.name, st.st_rdev & 0xffff);
	}
	close(fd);
}

main(argc, argv)
int argc;
char **argv;
{
	unsigned char a[112], b[112], v[64], slot[48], u[8];
	int i, n, secs = argc > 1 ? atoi(argv[1]) : 10;
	long x, y;
	unsigned addr;

	nlist("/xenix");
	for (i = 0; want[i]; i++)
		printf("%-10s %s %06lx\n", want[i], got[i] ? "at" : "not found", sym[i]);
	if ((kmem = open("/dev/kmem", 0)) < 0 || (mem = open("/dev/mem", 0)) < 0) {
		printf("cannot open /dev/kmem or /dev/mem: run as root\n");
		exit(1);
	}
	if (got[0]) {
		kread(sym[0], (char *)a, 112);
		printf("sysinfo, then %d seconds later:\n", secs);
		sleep(secs);
		kread(sym[0], (char *)b, 112);
		for (i = 0; field[i]; i++) {
			x = get4(a + i * 4);
			y = get4(b + i * 4);
			printf("  %3d %-10s %10ld %10ld %8ld\n", i * 4, field[i], x, y, y - x);
		}
	}
	if (got[1]) {
		kread(sym[1], (char *)v, 8);
		printf("syswait: %d %d %d\n", get2(v), get2(v + 2), get2(v + 4));
	}
	/* each process: its terminal, from its u area (those in memory) */
	kread(sym[3], (char *)v, 64);
	n = get2(v + 24);
	printf("processes (v_proc %d): pid, in memory, u_ttyp, u_ttyd, name\n", n);
	for (i = 0; i < n && i < 100; i++) {
		kread(sym[2] + i * 48L, (char *)slot, 48);
		if (slot[0] == 0)
			continue;
		addr = get2(slot + 22);
		if (!(slot[1] & 1)) {
			printf("  %5u  swapped out\n", get2(slot + 18));
			continue;
		}
		lseek(mem, addr * 16L + 1336, 0);
		read(mem, (char *)u, 4);
		{
			char comm[15];
			lseek(mem, addr * 16L + 1388, 0);
			read(mem, comm, 14);
			comm[14] = 0;
			printf("  %5u  %04x %04x  %s\n", get2(slot + 18), get2(u), get2(u + 2), comm);
		}
	}
	printf("terminals in /dev:\n");
	devs();
	exit(0);
}
