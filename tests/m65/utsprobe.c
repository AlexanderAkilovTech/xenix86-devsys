/* utsprobe - where XENIX 2.1.3's kernel keeps what uname prints (M65), so
 * that hostname can set the node name: the kernel's data symbols whose names
 * look like it, and where in its memory each of uname's answers is, by
 * offset and nearby symbol only. It never prints what uname printed, nor the
 * bytes it finds: one of uname's answers is the system's serial number.
 *	utsprobe file ...	each file one option's answer: u.s, u.r, ...
 * xenix86-devsys test program. */
#include <stdio.h>

#define	NSYM	1200
#define	POOL	20000

long	addr[NSYM];
int	nm[NSYM], nsym;
char	pool[POOL];
int	pused;
char	kd[4096];

long	get4(p) unsigned char *p; { return p[0] | p[1] << 8 | (long)p[2] << 16 | (long)p[3] << 24; }
unsigned get2(p) unsigned char *p; { return p[0] | p[1] << 8; }

/* the data and bss symbols of /xenix */
syms()
{
	unsigned char h[64], seg[32], r[8];
	char name[64];
	long segpos, segsize, pos, end;
	int fd, i, c, n, t;

	if ((fd = open("/xenix", 0)) < 0)
		return;
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
		lseek(fd, pos, 0);
		while (pos < end && read(fd, (char *)r, 8) == 8) {
			pos += 8;
			for (n = 0; read(fd, &name[n], 1) == 1 && name[n]; )
				if (n < 62)
					n++;
			name[n] = 0;
			pos += n + 1;
			t = get2(r) & 0x1f;
			if ((t == 3 || t == 4) && nsym < NSYM && pused + n + 1 < POOL) {
				addr[nsym] = get4(r + 4);
				nm[nsym] = pused;
				strcpy(pool + pused, name);
				pused += n + 1;
				nsym++;
			}
		}
	}
	close(fd);
}

/* the symbol at or below a, as name+offset */
char *
near(a)
long a;
{
	static char s[80];
	int i, best = -1;

	for (i = 0; i < nsym; i++)
		if (addr[i] <= a && (best < 0 || addr[i] > addr[best]))
			best = i;
	if (best < 0)
		return "?";
	sprintf(s, "%s+%ld", pool + nm[best], a - addr[best]);
	return s;
}

lower(c)
int c;
{
	return c >= 'A' && c <= 'Z' ? c + 32 : c;
}

/* does name have w in it, any case */
has(name, w)
char *name, *w;
{
	int i, j;

	for (i = 0; name[i]; i++) {
		for (j = 0; w[j] && lower(name[i + j]) == w[j]; j++)
			;
		if (!w[j])
			return 1;
	}
	return 0;
}

main(argc, argv)
int argc;
char **argv;
{
	char want[64];
	long top = 0, a;
	int i, k, n, len, hits, kmem, fd;

	syms();
	printf("%d data and bss symbols\n", nsym);
	for (i = 0; i < nsym; i++) {
		if (addr[i] > top)
			top = addr[i];
		if (has(pool + nm[i], "uts") || has(pool + nm[i], "node") || has(pool + nm[i], "sysname")
		    || has(pool + nm[i], "serial") || has(pool + nm[i], "uname"))
			printf("symbol %s at %ld (%lx)\n", pool + nm[i], addr[i], addr[i]);
	}
	printf("highest data symbol at %ld\n", top);
	kmem = open("/dev/kmem", 2);
	printf("/dev/kmem opens for writing: %s\n", kmem >= 0 ? "yes" : "no");
	if (kmem < 0 && (kmem = open("/dev/kmem", 0)) < 0) {
		printf("cannot read /dev/kmem\n");
		exit(1);
	}
	for (k = 1; k < argc; k++) {
		if ((fd = open(argv[k], 0)) < 0)
			continue;
		len = read(fd, want, sizeof want - 1);
		close(fd);
		if (len > 0 && want[len - 1] == '\n')
			len--;
		want[len] = 0;
		printf("%s: %d characters", argv[k], len);
		if (len == 0 || strcmp(want, "(empty)") == 0) {
			printf("%s\n", len ? ", says (empty)" : "");
			continue;
		}
		/* through the kernel's data, 4K at a time, overlapping by the length */
		hits = 0;
		for (a = 0; a < top + 4096 && hits < 4; a += sizeof kd - len) {
			lseek(kmem, a, 0);
			if ((n = read(kmem, kd, sizeof kd)) <= len)
				break;
			for (i = 0; i + len <= n && hits < 4; i++)
				if (memcmp(kd + i, want, len) == 0) {
					printf(", at %ld (%s)", a + i, near(a + i));
					hits++;
					i += len - 1;
				}
		}
		printf(hits ? "\n" : ", not found\n");
	}
	exit(0);
}
