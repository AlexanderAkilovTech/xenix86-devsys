/* kprobe.c - find out how a running XENIX 2.1.3 (8086) kernel keeps its
 * process table, memory map and swap map, for top. Run as root:
 *
 *	kprobe > kprobe.txt
 *
 * It takes the kernel's symbols from /xenix (the x.out symbol table, as
 * nm reads it), reads the variables through /dev/kmem, decodes the proc
 * table by the layout we expect, and reads each process's u area, from
 * /dev/mem if it is in memory and /dev/swap if not, to find the command
 * name and times. Everything is also printed raw, so a wrong guess shows.
 * xenix86-devsys test program. */
#include <stdio.h>

char	*want[] = { "_proc", "_v", "_coremap", "_swapmap", "_nswap", "_swplo",
	"_maxmem", "_physmem", "_bootime", "_time", "_lbolt", "_swapdev",
	"_rootdev", "_utsname", "_runin", "_runout", "_text", "_kmemstart",
	"_environ", 0 };	/* _environ: only in programs, for testing on one */
long	val[32];
int	got[32];
int	kmem, mem, swap;

long	get4(p) unsigned char *p; { return p[0] | p[1] << 8 | (long)p[2] << 16 | (long)p[3] << 24; }
unsigned get2(p) unsigned char *p; { return p[0] | p[1] << 8; }

nlist(file)
char *file;
{
	int fd, i, k, n;
	unsigned char h[64], seg[32], rec[8];
	char name[64];
	long segpos, segsize, pos, end;

	if ((fd = open(file, 0)) < 0) {
		printf("cannot open %s\n", file);
		return;
	}
	read(fd, h, 64);
	segpos = get4(h + 52);
	segsize = get4(h + 56);
	printf("x.out: magic %04x, segment table at %ld, %ld bytes\n", get2(h), segpos, segsize);
	for (i = 0; i < segsize; i += 32) {
		lseek(fd, segpos + i, 0);
		read(fd, seg, 32);
		if (get2(seg) != 3)
			continue;
		pos = get4(seg + 8);
		end = pos + get4(seg + 12);
		while (pos < end) {
			lseek(fd, pos, 0);
			if (read(fd, rec, 8) != 8)
				break;
			n = read(fd, name, 63);
			name[n < 0 ? 0 : n] = 0;
			for (k = 0; want[k]; k++)
				if (strcmp(name, want[k]) == 0) {
					val[k] = get4(rec + 4);
					got[k] = 1;
				}
			pos += 8 + strlen(name) + 1;
		}
	}
	close(fd);
	for (k = 0; want[k]; k++)
		if (got[k])
			printf("%-12s %06lx\n", want[k], val[k]);
		else
			printf("%-12s not found\n", want[k]);
}

long
sym(name)
char *name;
{
	int k;

	for (k = 0; want[k]; k++)
		if (strcmp(want[k], name) == 0)
			return got[k] ? val[k] : -1L;
	return -1L;
}

/* n bytes from fd at off; print them in hex too if label is given */
rd(fd, off, buf, n, label)
int fd, n;
long off;
unsigned char *buf;
char *label;
{
	int i, r;

	lseek(fd, off, 0);
	r = read(fd, buf, n);
	if (label) {
		printf("%s @%06lx (%d bytes):", label, off, r);
		for (i = 0; i < r && i < n; i++)
			printf("%s%02x", i % 16 ? " " : "\n  ", buf[i]);
		printf("\n");
	}
	return r;
}

/* a swapped-out process: its image is on /dev/swap at its addr in 1024-byte
 * blocks, less one if swplo (-1) is added as System V adds it (a map
 * cannot hold 0, so addresses start at 1). M37 found nothing at addr: dump
 * the first 2K at addr - 1 and at addr, and say where the proc slot's
 * address and the name appear */
unsigned char img[2048];

dump(p, n)
unsigned char *p;
int n;
{
	int j;

	for (j = 0; j < n; j++)
		printf("%s%02x", j % 32 ? " " : "\n    ", p[j]);
	printf("\n");
}

findu(b, slot)
unsigned char *b;
int slot;
{
	unsigned procp = sym("_proc") + slot * 48;
	int n, j, k;
	long a;

	for (k = 1; k >= 0; k--) {
		a = (get2(b + 22) - k) * 1024L;
		n = rd(swap, a, img, 2048, 0);
		printf("  swap at %ld (addr - %d), %d bytes:", a, k, n);
		for (j = 0; j + 1 < n; j++)
			if ((img[j] | img[j + 1] << 8) == procp)
				printf(" procp at %d", j);
		for (j = 0; j + 3 < n; j++)
			if (img[j] >= 'a' && img[j] <= 'z' && img[j + 1] >= 'a' && img[j + 1] <= 'z' &&
			    img[j + 2] >= 'a' && img[j + 2] <= 'z' && (j == 0 || img[j - 1] == 0))
				printf(" text \"%.10s\" at %d", img + j, j);
		dump(img, n);
	}
}

/* the end of /dev/mem: the memory XENIX has, found by reading at the
 * 64K boundaries until a read fails */
memsize()
{
	long k;
	char c;

	for (k = 0; k < 1024; k += 16) {
		lseek(mem, k * 1024L, 0);
		if (read(mem, &c, 1) != 1)
			break;
	}
	printf("/dev/mem reads stop at %ldK\n", k);
}

main(argc, argv)
int argc;
char **argv;
{
	unsigned char b[2048], *p;
	long a, procs, nproc, eproc;
	int i, n, j;

	if (argc > 1) {			/* kprobe prog: only its symbols */
		nlist(argv[1]);
		exit(0);
	}
	nlist("/xenix");
	if ((kmem = open("/dev/kmem", 0)) < 0 || (mem = open("/dev/mem", 0)) < 0) {
		printf("cannot open /dev/kmem or /dev/mem: run as root\n");
		exit(1);
	}
	swap = open("/dev/swap", 0);

	/* the tunable variables: v_proc and ve_proc check the proc layout */
	rd(kmem, sym("_v"), b, 64, "v");
	nproc = get2(b + 24);
	eproc = get2(b + 26);
	printf("v_proc %ld, ve_proc %04lx, _proc %04lx: %ld bytes a slot\n",
		nproc, eproc, sym("_proc"), nproc ? (eproc - sym("_proc")) / nproc : 0L);
	for (i = 0; want[i]; i++)
		if (got[i] && strcmp(want[i], "_proc") && strcmp(want[i], "_v"))
			rd(kmem, val[i], b, 16, want[i]);

	/* the maps: struct map { nwaiting, ncells, mpent } and the entries */
	for (j = 0; j < 2; j++) {
		a = sym(j ? "_swapmap" : "_coremap");
		rd(kmem, a, b, 6, 0);
		printf("%s: nwaiting %u, ncells %u, entries at %04x\n", j ? "swapmap" : "coremap",
			get2(b), get2(b + 2), get2(b + 4));
		n = get2(b + 2);
		if (n > 100)
			n = 100;
		rd(kmem, (long)get2(b + 4), b, n * 4, 0);
		for (i = 0; i < n && get2(b + i * 4); i++)
			printf("  size %u at %u\n", get2(b + i * 4), get2(b + i * 4 + 2));
	}

	/* the proc table, raw and decoded */
	procs = sym("_proc");
	if (nproc > 60)
		nproc = 60;
	for (i = 0; i < nproc; i++) {
		rd(kmem, procs + i * 48L, b, 48, 0);
		if (b[0] == 0)
			continue;
		printf("slot %d:", i);
		for (j = 0; j < 48; j++)
			printf(" %02x", b[j]);
		printf("\n  stat %d flag %o pri %d cpu %d nice %d uid %u pid %d ppid %d addr %u usize %u size %u wchan %04x\n",
			b[0], b[1], (char)b[2], b[4], (char)b[5], get2(b + 10), get2(b + 18), get2(b + 20),
			get2(b + 22), get2(b + 30), get2(b + 44), get2(b + 34));
		/* the u area: in memory at addr clicks; on swap, try addr as 1024-
		 * and as 512-byte blocks and see which holds the right name */
		if (b[1] & 01) {
			n = rd(mem, get2(b + 22) * 16L, b + 64, 1500, 0);
			if (n >= 1500) {
				p = b + 64;
				printf("  in memory: comm \"%.14s\" procp %04x utime %ld stime %ld\n",
					p + 1388, get2(p + 1070), get4(p + 1296), get4(p + 1300));
				if (strcmp((char *)p + 1388, "kprobe") == 0) {	/* the layout, to match */
					printf("  this process's u page at %ld:", get2(b + 22) * 16L);
					n = rd(mem, get2(b + 22) * 16L, img, 2048, 0);
					dump(img, n);
				}
			}
		} else if (swap >= 0)
			findu(b, i);
	}
	/* for top and the system summary: the sizes of memory and swap */
	rd(kmem, sym("_maxmem"), b, 4, "maxmem");
	rd(kmem, sym("_physmem"), b, 4, "physmem");
	rd(kmem, sym("_nswap"), b, 2, "nswap");
	rd(kmem, sym("_swplo"), b, 4, "swplo");
	rd(kmem, sym("_utsname"), b, 54, "utsname");
	rd(kmem, sym("_bootime"), b, 4, "bootime");
	rd(kmem, sym("_time"), b, 4, "time");
	memsize();
	exit(0);
}
