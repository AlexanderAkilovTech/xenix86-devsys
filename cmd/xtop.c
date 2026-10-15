/* xtop - XENIX top: the processes using the machine, its memory, swap and
 * disks, on a screen kept up to date
 *
 * usage: xtop [-b] [-c | -m] [-d seconds] [-n times]
 *
 * At the top of the screen, meters: the processor (time in programs and in
 * the kernel), the memory in use (the processes', the programs' shared
 * text, the kernel's, of all the machine has), the swap area in use, and
 * each mounted file system's space in use; beside them the number of
 * processes, the time since the system started, the 8087 if there is one
 * and the display, and each file system's free space. Below, a line for
 * each process: its id, owner, priority and nice value, its size, how much
 * of it is in memory (0 when swapped out), its state, its share of the
 * processor since the last screen, its time, its name.
 *
 * Two themes: Color, the meters' parts and the headings in the console's
 * colors, and Monochrome, white on black with the heading and the keys in
 * reverse video and the meters' parts told apart by their characters (| #
 * *). Color is the first choice on a terminal of type ansi, which has the
 * colors; the key c changes the theme, and the choice is kept in
 * $HOME/.xtoprc ("theme color" or "theme mono").
 *
 * Keys: q quits; P, M, T and N sort by processor, memory, time or id;
 * k asks for a process and a signal to send it; d asks for the seconds
 * between screens; c changes the theme; h shows the keys; anything else
 * redraws now.
 *	-b	print the screens one after another, without moving about
 *		the terminal (for a file or a pipe)
 *	-c, -m	Color or Monochrome, this time
 *	-d s	s seconds between screens (3; at most 25)
 *	-n n	stop after n screens
 *
 * xtop reads the kernel: its symbols from /xenix, its tables through
 * /dev/kmem, and each process's u area from /dev/mem when it is in memory,
 * from /dev/swap when it is not, as ps does (see ABI.md for the layouts,
 * measured with tests/kprobe.c); and the super blocks of the file systems
 * in /etc/mnttab, every 30 seconds, as df does. It runs as sysinfo, who
 * may read them, and gives that up once they are open.
 * xenix86-devsys, K&R C, with cmd/fpprobe.s. */
#include <stdio.h>
#include <signal.h>
#include <termio.h>
#include <sys/types.h>
#include <sys/stat.h>

#define	MAXP	100		/* process slots read, at most */
#define	PSLOT	48		/* bytes in a proc slot */
#define	HZ	20		/* XENIX's clock ticks a second */
#define	UTIME	1296		/* in a u area: user and system ticks, */
#define	UCOMM	1388		/* the command's name */
#define	UTTYP	1336		/* its controlling terminal: 0 for none, */
#define	UTTYD	1338		/* and the terminal's device */
#define	BARW	26		/* the meters' bars */
#define	MAXFS	2		/* file systems shown */
#define	FSEVERY	30		/* seconds between looks at them */
#define	GIO_COLOR ('c' << 8)	/* the console: is the display a color one? */
#define	PARK	71		/* where the cursor waits: after the keys' 69 columns */

/* the kernel's variables */
char	*want[] = { "_proc", "_v", "_coremap", "_swapmap", "_nswap", "_swplo", "_maxmem",
	"_bootime", "_time", "_lbolt", "_u", 0 };
#define	S_PROC	0
#define	S_V	1
#define	S_CORE	2
#define	S_SWAP	3
#define	S_NSWAP	4
#define	S_SWPLO	5
#define	S_MAXMEM 6
#define	S_BOOT	7
#define	S_TIME	8
#define	S_LBOLT	9
#define	S_U	10		/* the running process's u area (not needed) */
#define	NNEED	10
long	sym[11];
int	got[11];
int	mypid;

/* cc -DPHASES: xtop writes to the file phases, in the directory it was
 * started in, the processor time it has used at each step of its start
 * (tests/m56) */
#ifdef PHASES
int	nstats, ndirreads, nswapreads, nsymreads;
#define	COUNT(v)	v++
#define	PH(s)		phase(s)
#else
#define	COUNT(v)
#define	PH(s)
#endif

struct pr {
	int	pid, uid, pri, nice, stat, inmem;
	unsigned size;		/* clicks of 16 bytes */
	unsigned addr;		/* in memory, a click; swapped out, a block */
	long	ticks;		/* user and system */
	long	uticks, sticks;
	long	dt;		/* ticks since the last screen */
	int	tty;		/* its controlling terminal's device, or -1 */
	char	comm[15];
} procs[MAXP], *order[MAXP];
int	nprocs, nrun;

struct old {			/* the last look at each process */
	int	pid, inmem, tty;
	unsigned addr;
	long	ticks, uticks, sticks;
	char	comm[15];
} old[MAXP];

/* The swapping since the last look, as xtop sees it happen: processes that
 * were in memory then and are on the swap area now, and the other way.
 * (XENIX 2.1.3 keeps no count of its own: its kernel has no sysinfo.) */
long	swins, swouts;
int	nswapped;		/* processes on the swap area now */
long	dlb;			/* ticks between the last two looks */
int	nold;

unsigned char ptab[MAXP * PSLOT];
int	kmem = -1, mem = -1, swapfd = -1;
int	vproc;
long	swplo, lbolt, lastlbolt, now, boot;
long	duser, dsys, dall;		/* ticks in programs, in the kernel, in all */
unsigned maxmem, freemem, nswap, freeswap;
unsigned physk;			/* the machine's memory, in K */
unsigned incore;		/* clicks the processes in memory have */

int	bflag, lines = 25, cols = 80, delay = 3, count = -1;
int	sortby = 'P';
int	cancolor;		/* the terminal has the console's colors */
int	theme = 'c';		/* 'c' Color, 'm' Monochrome */
char	*CM, *CL, *CE, *SO, *SE;
char	rcfile[100];		/* $HOME/.xtoprc */

/* the file systems: from /etc/mnttab, open while xtop could, their super
 * blocks read every FSEVERY seconds */
struct fsys {
	char	dir[20];
	int	fd;
	long	blocks, free;	/* 512-byte blocks */
} fsys[MAXFS];
int	nfs;
long	fstime;
char	fpu[12], video[24];	/* the 8087, the display: for the screen */
char	tcbuf[1024], tcstr[256];
char	frame[30][320];		/* this screen's lines, escapes and all */
char	dirty[30];		/* lines made again since they were drawn */
char	clean[30];		/* rows blank on the terminal since it was cleared */
int	cut;			/* cells(): the line was longer than the row, */
int	lastat;			/* and the attribute it ends in */
unsigned short scr[30][80];	/* what the terminal shows: each cell a
				 * character and its attribute (see cells) */
int	currow = -1, curcol, curat = -1;	/* where the cursor is, its attribute */
struct termio tsave, traw;
int	rawset;
char	obuf[BUFSIZ];

extern char *getenv(), *tgetstr(), *tgoto(), *strcpy(), *strncpy(), *strchr(), *memchr();
extern long lseek(), time();

long	get4(p) unsigned char *p; { return p[0] | p[1] << 8 | (long)p[2] << 16 | (long)p[3] << 24; }
unsigned get2(p) unsigned char *p; { return p[0] | p[1] << 8; }

/* the kernel's symbols, from the x.out symbol table of /xenix: read 4K at
 * a time (a read for each symbol, as ps does it, took 19 seconds of an
 * 8088's time), and over once all are found. That still took about 1.8 s
 * of xtop's 6 to start (M56), so the addresses found are kept in SYMFILE,
 * with what /xenix was then (its i-node, size and time), and read from
 * there while /xenix is the same file. SYMFILE is installed empty, sysinfo's
 * and for sysinfo alone; when it is not there, nothing is kept. */
#define	SYMFILE	"/usr/lib/xtop.syms"
struct symf {
	char	magic[4];		/* "xts2" */
	unsigned dev, ino;		/* /xenix's */
	long	size, mtime;
	long	sym[11];
	int	got[11];
	/* M58: the terminals' names found, while /dev is as it was then, and
	 * the size of memory, while the machine has not been started again */
	long	devtime;		/* /dev's time */
	int	nttys;
	struct { int dev; char name[4]; } ttys[16];
	long	boot;			/* the time it was started */
	unsigned physk;
} kf;
int	kffd = -1;			/* SYMFILE, open to write it while xtop could */
int	kfdirty;			/* kf has something new to keep */

/* what kf has, into SYMFILE */
keep()
{
	if (kffd >= 0 && lseek(kffd, 0L, 0) == 0L)
		write(kffd, (char *)&kf, sizeof kf);
	kfdirty = 0;
}

nlist(file)
char *file;
{
	int fd = -1, i, k, n, left, stok;
	unsigned char h[64], seg[32];
	register char *p, *name;
	char *buf = frame[0];		/* not in use yet: 4K and a 0 after */
	char *lim, *e, hit[128];	/* hit: the second letters of the names wanted */
	long segpos, segsize, pos, end;
	struct stat st;

	/* SYMFILE: to be written later, when that can be (as sysinfo), or only
	 * read */
	stok = stat(file, &st) == 0;
	if (stok && ((kffd = open(SYMFILE, 2)) >= 0 || (fd = open(SYMFILE, 0)) >= 0)) {
		n = read(kffd >= 0 ? kffd : fd, (char *)&kf, sizeof kf);
		if (kffd < 0)
			close(fd);
		if (n == sizeof kf && strncmp(kf.magic, "xts2", 4) == 0 && kf.dev == st.st_dev
		    && kf.ino == st.st_ino && kf.size == st.st_size && kf.mtime == st.st_mtime) {
			memcpy((char *)sym, (char *)kf.sym, sizeof sym);
			memcpy((char *)got, (char *)kf.got, sizeof got);
			for (k = 0; k < NNEED; k++)
				if (!got[k])
					break;
			if (k == NNEED)
				return 1;
			memset((char *)got, 0, sizeof got);	/* not one of ours: look again */
		}
	}
	if ((fd = open(file, 0)) < 0)
		return 0;
	read(fd, (char *)h, 64);
	segpos = get4(h + 52);
	segsize = get4(h + 56);
	memset(hit, 0, sizeof hit);
	for (left = 0; want[left]; left++)
		hit[want[left][1]] = 1;
	for (i = 0; i < segsize && left; i += 32) {
		lseek(fd, segpos + i, 0);
		read(fd, (char *)seg, 32);
		if (get2(seg) != 3)		/* the symbol table */
			continue;
		pos = get4(seg + 8);
		end = pos + get4(seg + 12);
		/* 4K at a time; each record 8 bytes, then the name and its 0 */
		while (left && pos < end) {
			lseek(fd, pos, 0);
			COUNT(nsymreads);
			if ((n = read(fd, buf, 4096)) <= 8)
				break;
			if (end - pos < n)
				n = (int)(end - pos);
			buf[n] = 0;
			lim = buf + n;
			for (p = buf; p + 8 < lim; p = e + 1) {
				name = p + 8;
				if ((e = name + strlen(name)) >= lim)
					break;		/* the rest of it in the next 4K */
				if (name[0] != '_' || (name[1] & 0200) || !hit[name[1]])
					continue;
				for (k = 0; want[k]; k++)
					if (!got[k] && strcmp(name, want[k]) == 0) {
						sym[k] = get4((unsigned char *)p + 4);
						got[k] = 1;
						left--;
						break;
					}
			}
			if (p == buf)		/* a name longer than 4K */
				break;
			pos += p - buf;
		}
	}
	close(fd);
	for (k = 0; k < NNEED; k++)
		if (!got[k])
			return 0;
	/* to be kept for next time (keep()), with nothing else known yet */
	memset((char *)&kf, 0, sizeof kf);
	if (stok) {
		strncpy(kf.magic, "xts2", 4);
		kf.dev = st.st_dev;
		kf.ino = st.st_ino;
		kf.size = st.st_size;
		kf.mtime = st.st_mtime;
		memcpy((char *)kf.sym, (char *)sym, sizeof sym);
		memcpy((char *)kf.got, (char *)got, sizeof got);
		kfdirty = 1;
	}
	return 1;
}

/* n bytes of the kernel's memory at a */
kread(a, buf, n)
long a;
char *buf;
int n;
{
	lseek(kmem, a, 0);
	return read(kmem, buf, n) == n;
}

/* the free space in a System V map: {nwaiting, ncells, entries}, each
 * entry {size, address}, ended by a size of 0 */
unsigned
mapfree(a)
long a;
{
	unsigned char h[6], e[400];
	int n, i;
	unsigned sum = 0;

	if (!kread(a, (char *)h, 6))
		return 0;
	if ((n = get2(h + 2)) > 100)
		n = 100;
	if (!kread((long)get2(h + 4), (char *)e, n * 4))
		return 0;
	for (i = 0; i < n && get2(e + i * 4); i++)
		sum += get2(e + i * 4);
	return sum;
}

/* the machine's memory, in K: where /dev/mem ends (XENIX's mem driver
 * stops at the top of memory, 640K on a full XT), found by halving */
memsize()
{
	unsigned lo = 64, hi = 1024, mid;	/* lo can be read, hi cannot */
	char c;

	if (kf.boot == boot && kf.physk) {	/* found since the machine started */
		physk = kf.physk;
		return;
	}
	while (hi - lo > 1) {
		mid = (lo + hi) / 2;
		if (lseek(mem, mid * 1024L - 1, 0) >= 0 && read(mem, &c, 1) == 1)
			lo = mid;
		else
			hi = mid;
	}
	physk = kf.physk = lo;
	kf.boot = boot;
	kfdirty = 1;
}

/* a look at the system: the proc table, each process's times and name,
 * and the maps */
sample()
{
	register struct pr *p;
	register unsigned char *s;
	unsigned char b[6], u[106], u2[106];
	int i, fd;
	struct old *o, *oe;
	unsigned addr;
	long a, ticks;

	kread(sym[S_LBOLT], (char *)b, 4);
	lbolt = get4(b);
	kread(sym[S_TIME], (char *)b, 4);
	now = get4(b);
	kread(sym[S_MAXMEM], (char *)b, 2);
	maxmem = get2(b);
	freemem = mapfree(sym[S_CORE]);
	freeswap = mapfree(sym[S_SWAP]);
	kread(sym[S_PROC], (char *)ptab, vproc * PSLOT);
	nprocs = nrun = 0;
	duser = dsys = 0;
	swins = swouts = nswapped = 0;
	incore = 0;
	for (i = 0; i < vproc; i++) {
		s = ptab + i * PSLOT;
		if (s[0] == 0)
			continue;
		p = &procs[nprocs++];
		p->stat = s[0];
		p->inmem = s[1] & 01;
		p->pri = (char)s[2];
		p->nice = (char)s[5];
		p->uid = get2(s + 10);
		p->pid = get2(s + 18);
		p->size = get2(s + 44);
		if (p->inmem)
			incore += p->size;
		p->uticks = p->sticks = 0;
		strcpy(p->comm, "");
		if (p->stat == 3)
			nrun++;
		addr = get2(s + 22);
		/* the last look at it: none if o is past the last */
		for (o = old, oe = old + nold; o < oe && o->pid != p->pid; o++)
			;
		p->tty = -1;
		if (p->stat == 5)		/* a zombie has no u area */
			strcpy(p->comm, "<defunct>");
		else if (!p->inmem && o < oe && !o->inmem && o->addr == addr) {
			/* still swapped out where it was: as it was, without
			 * reading the disk again */
			p->uticks = o->uticks;
			p->sticks = o->sticks;
			p->tty = o->tty;
			strcpy(p->comm, o->comm);
		} else {
			/* the u area: at addr clicks in memory, or on swap at
			 * (addr + swplo) blocks of 1024 bytes */
			if (p->inmem) {
				fd = mem;
				a = addr * 16L;
			} else {
				fd = swapfd;
				a = (addr + swplo) * 1024L;
				COUNT(nswapreads);
			}
			if (fd >= 0 && lseek(fd, a + UTIME, 0) >= 0 && read(fd, (char *)u, 106) == 106) {
				/* xtop itself: the kernel keeps the running process's
				 * u area in its own _u, and writes it back to the
				 * process only when another process runs, so the copy
				 * in memory can be seconds old; the live one, if it is
				 * this process's */
				if (p->pid == mypid && got[S_U] && kread(sym[S_U] + UTIME, (char *)u2, 106)
				    && strncmp((char *)u2 + UCOMM - UTIME, (char *)u + UCOMM - UTIME, 14) == 0)
					memcpy((char *)u, (char *)u2, 8);
				p->uticks = get4(u);
				p->sticks = get4(u + 4);
				if (get2(u + UTTYP - UTIME))
					p->tty = get2(u + UTTYD - UTIME);
				strncpy(p->comm, (char *)u + UCOMM - UTIME, 14);
				p->comm[14] = 0;
			}
		}
		if (p->pid == 0)
			strcpy(p->comm, "swapper");
		p->ticks = p->uticks + p->sticks;
		/* the ticks since the last look */
		p->addr = addr;
		p->dt = 0;
		if (!p->inmem && p->stat != 5)
			nswapped++;
		if (o < oe && o->inmem != p->inmem)	/* swapped since the last look */
			if (p->inmem)
				swins++;
			else
				swouts++;
		if (o < oe) {
			if (p->ticks >= o->ticks) {
				p->dt = p->ticks - o->ticks;
				duser += p->uticks - o->uticks;
			}
		} else if (lastlbolt) {		/* new since the last look */
			p->dt = p->ticks;
			duser += p->uticks;
		}
		if (nprocs == MAXP)
			break;
	}
	dlb = lastlbolt ? lbolt - lastlbolt : 0;
	dall = dlb > 0 ? dlb : 1;
	for (i = 0, ticks = 0; i < nprocs; i++)
		ticks += procs[i].dt;
	dsys = ticks - duser;
	if (dsys < 0)
		dsys = 0;
	if (ticks > dall)		/* rounding at the edges of the ticks */
		dall = ticks;
	for (i = 0; i < nprocs; i++) {
		p = &procs[i];
		old[i].pid = p->pid;
		old[i].inmem = p->inmem;
		old[i].tty = p->tty;
		old[i].addr = p->addr;
		old[i].ticks = p->ticks;
		old[i].uticks = p->uticks;
		old[i].sticks = p->sticks;
		strcpy(old[i].comm, p->comm);
	}
	nold = nprocs;
	lastlbolt = lbolt;
}

/* the order of the lines */
before(a, b)
register struct pr *a, *b;
{
	long x, y;

	switch (sortby) {
	case 'M': x = a->size; y = b->size; break;
	case 'T': x = a->ticks; y = b->ticks; break;
	case 'N': return a->pid - b->pid;
	default: x = a->dt; y = b->dt; break;
	}
	if (x != y)
		return x > y ? -1 : 1;
	return a->pid - b->pid;
}

sortprocs()
{
	register int i, j;
	struct pr *t;

	for (i = 0; i < nprocs; i++)
		order[i] = &procs[i];
	for (i = 1; i < nprocs; i++)	/* insertion: there are few */
		for (j = i; j > 0 && before(order[j - 1], order[j]) > 0; j--) {
			t = order[j];
			order[j] = order[j - 1];
			order[j - 1] = t;
		}
}

/* the terminals' names, from /dev, as ps shows them: co for the console,
 * 02 for tty02, 1a for tty1a. /dev's names are read once, at the start;
 * what device each one is (a stat, which on an XT costs the kernel most
 * of a tenth of a second, for each of XENIX's 63 terminals) is looked up
 * only when a process has a terminal not found yet, going on in /dev's
 * order from where the last look stopped. The console and the first
 * screens come first, so it is a few looks, not 63 (M54: 63 took xtop's
 * start from under 3 seconds to 7.5). */
struct { int dev; char name[4]; char kept; } ttys[64];	/* kept: in SYMFILE */
int	nttys;
char	tnames[64][15];		/* the terminals in /dev, from tnext on not looked up yet */
int	ntnames, tnext, scanned;

ttyscan()
{
	int fd, n;
	register char *p, *name;
	char buf[1024];

	if ((fd = open("/dev", 0)) < 0)
		return;
	/* 1K of entries at a time, not a read for each of /dev's 190 or so
	 * (those took about a second: M56) */
	while (ntnames < 64 && (n = read(fd, buf, 1024)) >= 16) {
		COUNT(ndirreads);
		for (p = buf; p + 16 <= buf + n && ntnames < 64; p += 16) {
			name = p + 2;		/* after the inode number */
			if (get2(p) == 0 || (strncmp(name, "tty", 3) || name[3] == 0)
			    && strncmp(name, "console", 14))
				continue;
			strncpy(tnames[ntnames], name, 14);
			tnames[ntnames++][14] = 0;
		}
	}
	close(fd);
}

char *
ttyname(dev)
int dev;
{
	int i;
	struct stat st;
	char path[24], *n;

	if (dev == -1)
		return "?";
	for (i = 0; i < nttys; i++)
		if (ttys[i].dev == dev) {
			if (!ttys[i].kept)
				kept(i);
			return ttys[i].name;
		}
	if (!scanned) {
		ttyscan();
		scanned = 1;
	}
	while (tnext < ntnames) {
		n = tnames[tnext++];
		sprintf(path, "/dev/%s", n);
		COUNT(nstats);
		if (stat(path, &st) < 0 || (st.st_mode & S_IFMT) != S_IFCHR)
			continue;
		for (i = 0; i < nttys && ttys[i].dev != st.st_rdev; i++)
			;
		if (i < nttys && n[0] != 'c')
			continue;		/* the console's other name: co it stays */
		ttys[i].dev = st.st_rdev;
		strncpy(ttys[i].name, n[0] == 'c' ? "co" : n + 3, 3);
		ttys[i].name[3] = 0;
		if (i == nttys)
			nttys++;
		if (ttys[i].dev == dev) {
			kept(i);
			return ttys[i].name;
		}
	}
	return "?";
}

/* terminal i's name, to be kept in SYMFILE */
kept(i)
int i;
{
	int k;

	for (k = 0; k < kf.nttys && kf.ttys[k].dev != ttys[i].dev; k++)
		;
	ttys[i].kept = 1;		/* (or no room for it) */
	if (k == 16)
		return;
	kf.ttys[k].dev = ttys[i].dev;
	strncpy(kf.ttys[k].name, ttys[i].name, 4);
	if (k == kf.nttys)
		kf.nttys++;
	ttys[i].kept = 1;
	kfdirty = 1;
}

/* the terminals' names: those kept in SYMFILE while /dev is as it was;
 * otherwise, and for any other, /dev read when one is first wanted */
ttystart()
{
	struct stat st;
	int i;

	if (stat("/dev", &st) < 0)
		st.st_mtime = 0;
	if (kf.devtime == st.st_mtime && kf.nttys <= 16)
		for (i = 0; i < kf.nttys; i++) {
			ttys[i].dev = kf.ttys[i].dev;
			strncpy(ttys[i].name, kf.ttys[i].name, 4);
			ttys[i].name[3] = 0;
			ttys[i].kept = 1;
			nttys++;
		}
	else {
		kf.devtime = st.st_mtime;
		kf.nttys = 0;
		kfdirty = 1;
	}
}

/* names of users, from /etc/passwd */
struct { unsigned id; char name[9]; } users[24];
int	nusers;

char *
username(id)
unsigned id;
{
	FILE *f;
	char line[200], *p, *q;
	unsigned v;
	int i;

	for (i = 0; i < nusers; i++)
		if (users[i].id == id)
			return users[i].name;
	i = nusers < 24 ? nusers++ : 23;
	users[i].id = id;
	sprintf(users[i].name, "%u", id);
	if ((f = fopen("/etc/passwd", "r")) != NULL) {
		while (fgets(line, sizeof line, f)) {
			if ((p = strchr(line, ':')) == 0 || (q = strchr(p + 1, ':')) == 0)
				continue;
			for (v = 0, q++; *q >= '0' && *q <= '9'; q++)
				v = v * 10 + *q - '0';
			if (v == id && *q == ':') {
				*p = 0;
				strncpy(users[i].name, line, 8);
				users[i].name[8] = 0;
				break;
			}
		}
		fclose(f);
	}
	return users[i].name;
}

/* Pieces of the screen's lines put together by hand: sprintf, with all its
 * conversions, was a quarter of xtop's own time on its first screen (M58).
 * rnum: v, right-justified in w places; lstr: at most max of s, then blanks
 * up to w; rstr: s, right-justified in w places. Each returns the place
 * after what it put. */
char *
rnum(o, v, w)
register char *o;
int v, w;
{
	char d[7];
	register char *q = d + 7;
	unsigned u = v < 0 ? -v : v;

	do
		*--q = '0' + u % 10;
	while ((u /= 10) != 0);
	if (v < 0)
		*--q = '-';
	for (w -= d + 7 - q; w > 0; w--)
		*o++ = ' ';
	while (q < d + 7)
		*o++ = *q++;
	return o;
}

char *
lstr(o, s, w, max)
register char *o, *s;
int w, max;
{
	char *b = o, *e = o + max;

	while (*s && o < e)
		*o++ = *s++;
	for (w -= o - b; w > 0; w--)
		*o++ = ' ';
	return o;
}

char *
rstr(o, s, w)
register char *o, *s;
int w;
{
	for (w -= strlen(s); w > 0; w--)
		*o++ = ' ';
	while (*s)
		*o++ = *s++;
	return o;
}

/* a size of k kilobytes in a few characters: 27K, 1.6M */
char *
ksize(k)
long k;
{
	static char buf[4][10];
	static int which;
	char *b = buf[which++ & 3], *e;
	long t;

	if (k < 1000) {
		e = rnum(b, (int)k, 0);
		e[0] = 'K';
		e[1] = 0;
	} else {
		t = (k * 10 + 512) / 1024;
		if (t < 100)
			sprintf(b, "%ld.%ldM", t / 10, t % 10);
		else
			sprintf(b, "%ldM", (k + 512) / 1024);
	}
	return b;
}

/* The look of the screen, as escapes in its lines, by role. In Color the
 * console's colors (one number to a sequence, as it wants), the keys'
 * letters bright cyan. In Monochrome white on black: the heading in reverse
 * video, the meters, the keys' letters and the numbers to notice bright;
 * on a terminal other than the console only the heading, in its standout
 * mode. To a file or a pipe nothing. draw() turns these into what the
 * terminal needs. */
#define	PLAIN	0
#define	GREEN	1
#define	RED	2
#define	CYAN	3
#define	BLUE	4
#define	YELLOW	5
#define	BOLD	6
#define	HEAD	7
#define	KEYC	8
char	*colors[] = { "\033[0m\033[37m\033[40m", "\033[1m\033[32m", "\033[1m\033[31m",
	"\033[1m\033[36m", "\033[1m\033[34m", "\033[1m\033[33m", "\033[1m",
	"\033[0m\033[30m\033[42m", "\033[1m\033[36m" };
char	*mono[] = { "\033[0m", "\033[1m", "\033[1m", "\033[1m", "\033[1m", "\033[1m", "\033[1m",
	"\033[0m\033[7m", "\033[1m" };
char	*monoso[] = { "\033[0m", "", "", "", "", "", "", "\033[7m", "" };
char	*none[] = { "", "", "", "", "", "", "", "", "" };
char	**look = none;

#define	col(r)	look[r]

/* the theme in use, and its escapes: on the console (TERM ansi) its own
 * for Monochrome, elsewhere termcap's standout */
int	ansi;			/* the terminal takes the console's escapes */

settheme(t)
int t;
{
	if (t == 'c' && !cancolor)
		t = 'm';
	theme = t;
	look = bflag ? none : t == 'c' ? colors : ansi ? mono : SO ? monoso : none;
}

/* a meter: label[|||||      text], the bar in up to three parts, each in
 * its color, in proportion to total */
meter(out, label, n, v, c, total, text)
char *out, *label, *text;
int n, c[];
long v[], total;
{
	int i, j, t, cur, upto[3];
	long sum = 0;
	char *o = out;

	if (total <= 0)
		total = 1;
	for (j = 0; j < n; j++) {
		sum += v[j];
		upto[j] = (int)(sum * BARW / total);
	}
	t = strlen(text);
	o += sprintf(o, " %s%s%s[", col(CYAN), label, col(PLAIN));
	for (i = 0, cur = -1; i < BARW; i++) {
		for (j = 0; j < n && i >= upto[j]; j++)	/* the part i is in */
			;
		if (j != cur && look != none) {
			strcpy(o, look[j < n ? c[j] : PLAIN]);
			o += strlen(o);
			cur = j;
		}
		if (i >= BARW - t)
			*o++ = text[i - (BARW - t)];
		else
			*o++ = j < n ? '|' : ' ';
	}
	sprintf(o, "%s]", col(PLAIN));
}

/* A line as the terminal shows it: for each column a cell of 16 bits, the
 * character in the low byte and its attribute in the high one, the
 * attribute reverse video (0200), bold (0100), foreground (070) and
 * background (07) colors, from
 * the escapes top writes, one number to each. The columns after the line
 * are blanks in white on black. Returns the columns up to the last that is
 * not such a blank. (Pointers, not subscripts: on an 8088 a subscript into
 * a table of rows costs a multiplication, 130 clock cycles.) */
#define	PLAINA	070		/* white on black */
#define	BLANK	(' ' | PLAINA << 8)
cells(s, cell)
register char *s;
unsigned short *cell;
{
	register unsigned short *p = cell;
	unsigned short *lim = cell + cols - 1, *end = cell, *stop = cell + cols;
	unsigned a = PLAINA << 8;
	int n;

	while (*s && p < lim)
		if (*s == 033 && s[1] == '[') {
			for (s += 2, n = 0; *s >= '0' && *s <= '9'; s++)
				n = n * 10 + *s - '0';
			if (*s)
				s++;
			if (n == 0)
				a &= ~(0300 << 8);
			else if (n == 1)
				a |= 0100 << 8;
			else if (n == 7)
				a |= 0200 << 8;
			else if (n >= 30 && n <= 37)
				a = a & ~(070 << 8) | (n - 30) << 11;
			else if (n >= 40 && n <= 47)
				a = a & ~(07 << 8) | (n - 40) << 8;
		} else {
			*p = *s++ & 0377 | a;
			if (*p++ != BLANK)
				end = p;
		}
	/* cut: more to show than the line holds (escapes after the last
	 * character it holds are not more) */
	while (*s == 033 && s[1] == '[') {
		for (s += 2; *s >= '0' && *s <= '9'; s++)
			;
		if (*s)
			s++;
	}
	cut = *s != 0;
	lastat = a >> 8;
	while (p < stop)
		*p++ = BLANK;
	return end - cell;
}

/* the terminal to attribute a, from curat, in as few escapes as will do:
 * ESC[0m ends bold, and on some terminals the colors too, so after it
 * both colors are set again */
char	fg[] = "\033[30m", bg[] = "\033[40m";
int	outc(ch) int ch; { return putchar(ch); }

setattr(a)
int a;
{
	int was = curat;

	if (a == was || look == none) {
		curat = a;
		return;
	}
	if (look == mono) {		/* the console without its colors */
		if (was < 0 || (was & ~a & 0300)) {	/* something to turn off */
			fputs("\033[0m", stdout);
			was = 0;
		}
		if ((a & 0100) && !(was & 0100))
			fputs("\033[1m", stdout);
		if ((a & 0200) && !(was & 0200))
			fputs("\033[7m", stdout);
		curat = a;
		return;
	}
	if (look == monoso) {		/* reverse video or not, by termcap */
		if ((a & 0200) && (was < 0 || !(was & 0200)))
			tputs(SO, 1, outc);
		else if (!(a & 0200) && (was < 0 || (was & 0200)))
			tputs(SE, 1, outc);
		curat = a;
		return;
	}
	if (was < 0 || (was & 0100) && !(a & 0100)) {
		fputs("\033[0m", stdout);
		was = 0377;			/* colors unknown */
	}
	if ((a & 0100) && (was == 0377 || !(was & 0100)))
		fputs("\033[1m", stdout);
	if (was == 0377 || ((a ^ was) & 070)) {
		fg[3] = '0' + ((a >> 3) & 07);
		fputs(fg, stdout);
	}
	if (was == 0377 || ((a ^ was) & 07)) {
		bg[3] = '0' + (a & 07);
		fputs(bg, stdout);
	}
	curat = a;
}

/* a cell: its attribute if the terminal is not already in it, its
 * character */
put1(v)
register unsigned v;
{
	if ((v >> 8) != curat)
		setattr(v >> 8);
	putchar(v & 0377);
}

/* the cursor to row r, column c */
move(r, c)
int r, c;
{
	if (r != currow || c != curcol)
		tputs(tgoto(CM, c, r), 1, outc);
	currow = r;
	curcol = c;
}

/* a clear screen, in white on black */
clearscr()
{
	curat = -1;
	setattr(PLAINA);
	tputs(CL, 1, outc);
	forget(0, 1);
	currow = curcol = 0;
}

/* forget what the terminal shows from row r on: a cleared screen (blank),
 * or something written over it (not even blanks, so it is all drawn again) */
forget(r, blank)
int r, blank;
{
	register unsigned short *p, v = blank ? BLANK : 0;
	unsigned short *e;
	int r0 = r;

	if (r >= lines)
		return;
	for (p = scr[r], e = p + 80; p < e; )	/* one row, then copies of it */
		*p++ = v;
	for (r++; r < lines; r++)
		memcpy((char *)scr[r], (char *)scr[r0], sizeof scr[0]);
	memset(dirty + r0, 1, lines - r0);
	memset(clean + r0, blank, lines - r0);
}

/* the screen, into frame[]. A process's line is made again only when
 * something in it has changed: on an 8088, printf is most of xtop's time */
int	cpuc[] = { GREEN, RED }, memc[] = { GREEN, YELLOW, BLUE }, swpc[] = { RED }, dskc[] = { BLUE };
struct rowkey {
	int	pid, uid, tty, pri, nice, stat, inmem, pct;
	unsigned size;
	long	ticks;
	char	comm[15];
};
struct rowc { struct rowkey k; int ok; } rowc[30];
int	once;			/* the heading and the keys made */

/* line r of the screen is s: to be drawn if it has changed */
setline(r, s)
int r;
char *s;
{
	if (strcmp(frame[r], s) != 0) {
		strcpy(frame[r], s);
		dirty[r] = 1;
	}
}

/* the time since the system started: 3 days, 08:58:12. From the clock's
 * ticks since then (lbolt, 20 a second, as times() gives them), not the
 * time of day less the start's: setting the clock moves those (M66) */
char *
uptime()
{
	static char b[30];
	unsigned long t = (unsigned long)lbolt / 20, d;
	unsigned h, m, sec;

	d = t / 86400L;
	t %= 86400L;
	h = (unsigned)(t / 3600);
	m = (unsigned)(t % 3600) / 60;
	sec = (unsigned)(t % 60);
	if (d)
		sprintf(b, "%lu day%s, %02u:%02u:%02u", d, d == 1 ? "" : "s", h, m, sec);
	else
		sprintf(b, "%02u:%02u:%02u", h, m, sec);
	return b;
}

/* the file systems' super blocks, as df reads them: the second 1024
 * bytes, the magic number at their end, the size and free blocks in
 * blocks of 512 or 1024 bytes */
fslook()
{
	register struct fsys *f;
	unsigned char sb[1024];
	long mult;

	for (f = fsys; f < fsys + nfs; f++) {
		f->blocks = 0;
		if (lseek(f->fd, 1024L, 0) < 0 || read(f->fd, (char *)sb, 1024) != 1024 ||
		    get4(sb + 1016) != 0x2b5544L)
			continue;
		mult = get4(sb + 1020) == 2 ? 2 : 1;
		f->blocks = get4(sb + 2) * mult;
		f->free = get4(sb + 618) * mult;
	}
	fstime = now;
}

/* what a meter line was made from, so that it is made again only when
 * that changes (it is sprintf, and on an 8088 sprintf is dear) */
long	was[8][4];
int	gen = 1, madeat;	/* themes so far; the one they were made in */

same(r, a, b, c, d)
int r;
long a, b, c, d;
{
	register long *w = was[r];

	if (w[0] == a && w[1] == b && w[2] == c && w[3] == d && madeat == gen)
		return 1;
	w[0] = a;
	w[1] = b;
	w[2] = c;
	w[3] = d;
	return 0;
}

build()
{
	char m[200], t[40], line[320], *o, *s;
	long v[3], used, kern, total, secs, t2;
	unsigned pct;
	int i, n, row, rows, top;
	register struct pr *p;
	struct rowkey k;
	struct rowc *rc;
	struct fsys *f;
	static char st[] = "?SWRIZT", mem[200];
	static int sorted, fsmade = -1, vmade = -1;

	/* the processor: the processes' ticks in programs and in the kernel,
	 * of all the ticks since the last look */
	if (!same(0, duser, dsys, dall, (long)nprocs << 8 | nrun)) {
		pct = (unsigned)((duser + dsys) * 1000 / dall);
		sprintf(t, "%u.%u%%", pct / 10, pct % 10);
		v[0] = duser;
		v[1] = dsys;
		meter(m, "CPU", 2, v, cpuc, dall, t);
		sprintf(line, "%s    Processes: %s%d%s, %s%d%s running", m, col(BOLD), nprocs, col(PLAIN),
			col(GREEN), nrun, col(PLAIN));
		setline(0, line);
	}

	/* memory, in clicks: the processes' own; the rest of what XENIX has
	 * given out, which is the programs' shared text; and the kernel's,
	 * all the machine has that XENIX keeps from the processes */
	used = maxmem - freemem;
	total = (long)physk * 64;
	if ((kern = total - maxmem) < 0) {	/* the size of memory unknown */
		kern = 0;
		total = maxmem;
	}
	if (!same(1, used, (long)incore, total, 0L)) {
		v[0] = incore < used ? incore : used;
		v[1] = used - v[0];
		v[2] = kern;
		sprintf(t, "%s/", ksize((used + kern + 32) / 64));
		strcat(t, ksize((total + 32) / 64));
		meter(mem, "Mem", 3, v, memc, total, t);
	}
	sprintf(line, "%s    Uptime: %s%s%s", mem, col(BOLD), uptime(), col(PLAIN));
	setline(1, line);
	/* the swap area: its space in use, and beside it the processes out
	 * there, and how many went out and came back in since the last look
	 * (when that was a second or more ago) */
	secs = (dlb + HZ / 2) / HZ;
	if (!same(2, (long)freeswap, (long)nswap, (long)nswapped << 16 | secs, swouts << 16 | swins)) {
		sprintf(t, "%s/", ksize((long)(nswap - freeswap)));
		strcat(t, ksize((long)nswap));
		v[0] = nswap - freeswap;
		meter(m, "Swp", 1, v, swpc, (long)nswap, t);
		sprintf(line, "%s    Swapped out: %s%d%s", m, col(BOLD), nswapped, col(PLAIN));
		if (secs > 0)
			sprintf(line + strlen(line), "   last %lds: %s%ld%s out, %s%ld%s in", secs,
				col(BOLD), swouts, col(PLAIN), col(BOLD), swins, col(PLAIN));
		setline(2, line);
	}

	/* the file systems: space in use of all, and free beside, made again
	 * when they are looked at */
	if (now - fstime >= FSEVERY || now < fstime)
		fslook();
	if (fsmade != gen || fstime != was[7][0]) {
		was[7][0] = fstime;
		fsmade = gen;
		for (i = 0, f = fsys; i < nfs; i++, f++) {
			if (f->blocks <= 0) {
				sprintf(line, " %sDsk%s[%*s]    %s: cannot be read", col(CYAN), col(PLAIN), BARW, "", f->dir);
				setline(3 + i, line);
				continue;
			}
			sprintf(t, "%s/", ksize((f->blocks - f->free) / 2));
			strcat(t, ksize(f->blocks / 2));
			v[0] = f->blocks - f->free;
			meter(m, "Dsk", 1, v, dskc, f->blocks, t);
			sprintf(line, "%s    %s: %s%s%s free", m, f->dir, col(BOLD), ksize(f->free / 2), col(PLAIN));
			setline(3 + i, line);
		}
	}
	madeat = gen;
	top = 3 + nfs;			/* the machine, a blank line, the heading, the processes */
	if (vmade != gen) {
		sprintf(line, "%*sFPU: %s%s%s%s%s", BARW + 10, "", col(BOLD), fpu, col(PLAIN),
			*video ? "   Video: " : "", video);
		setline(top, line);
		setline(top + 1, "");
		vmade = gen;
	}
	if (sorted != sortby || !once) {	/* the heading, its right end the order */
		sprintf(t, "Sort: %s ", sortby == 'M' ? "memory" : sortby == 'T' ? "time" : sortby == 'N' ? "id" : "CPU");
		sprintf(line, "%s%-*s%s%s", col(HEAD), cols - 1 - strlen(t),
			"  PID USER     TTY PRI  NI  SIZE   RES S  CPU%    TIME+ COMMAND", t, col(PLAIN));
		setline(top + 2, line);
		sorted = sortby;
	}
	if (!once) {			/* the keys, their letters to stand out */
		if (*col(KEYC))
			sprintf(line, "%sh%s:Help  %sP%s:CPU  %sM%s:Mem  %sT%s:Time  %sN%s:PID  %sk%s:Kill  %sd%s:Delay  %sc%s:Theme  %sq%s:Quit",
				col(KEYC), col(PLAIN), col(KEYC), col(PLAIN), col(KEYC), col(PLAIN), col(KEYC), col(PLAIN),
				col(KEYC), col(PLAIN), col(KEYC), col(PLAIN), col(KEYC), col(PLAIN), col(KEYC), col(PLAIN),
				col(KEYC), col(PLAIN));
		else
			strcpy(line, "h:Help  P:CPU  M:Mem  T:Time  N:PID  k:Kill  d:Delay  c:Theme  q:Quit");
		setline(lines - 1, line);
		once = 1;
	}

	rows = lines - top - 4;
	for (i = 0, row = top + 3; i < rows; i++, row++) {
		if (i >= nprocs) {
			if (frame[row][0])
				dirty[row] = 1;
			frame[row][0] = 0;
			rowc[row].ok = 0;
			continue;
		}
		p = order[i];
		memset((char *)&k, 0, sizeof k);
		k.pid = p->pid;
		k.uid = p->uid;
		k.tty = p->tty;
		k.pri = p->pri;
		k.nice = p->nice;
		k.stat = p->stat;
		k.inmem = p->inmem;
		k.pct = (int)(p->dt * 1000 / dall);
		k.size = p->size;
		k.ticks = p->ticks;
		strcpy(k.comm, p->comm);
		rc = &rowc[row];
		if (rc->ok && memcmp((char *)&k, (char *)&rc->k, sizeof k) == 0)
			continue;		/* the same as last time */
		rc->k = k;
		rc->ok = 1;
		dirty[row] = 1;
		/* as sprintf's "%s%5d %-8.8s %-3.3s %3d %3d %5s %5s %c %3d.%d
		 * %3ld:%02d.%02d %.19s%s" would make it */
		o = lstr(frame[row], p->stat == 3 ? col(BOLD) : "", 0, 40);
		o = rnum(o, p->pid, 5);
		*o++ = ' ';
		o = lstr(o, username(p->uid), 8, 8);
		*o++ = ' ';
		o = lstr(o, ttyname(p->tty), 3, 3);
		*o++ = ' ';
		o = rnum(o, p->pri, 3);
		*o++ = ' ';
		o = rnum(o, p->nice - 20, 3);
		*o++ = ' ';
		s = ksize(((long)p->size + 63) / 64);
		o = rstr(o, s, 5);
		*o++ = ' ';
		o = rstr(o, p->inmem ? s : "0", 5);
		*o++ = ' ';
		*o++ = st[p->stat > 6 ? 0 : p->stat];
		*o++ = ' ';
		o = rnum(o, k.pct / 10, 3);
		*o++ = '.';
		*o++ = '0' + k.pct % 10;
		*o++ = ' ';
		secs = p->ticks / HZ;
		if ((t2 = secs / 60) < 10000)
			o = rnum(o, (int)t2, 3);
		else {
			sprintf(o, "%3ld", t2);
			o += strlen(o);
		}
		n = (int)(secs - t2 * 60);
		*o++ = ':';
		*o++ = '0' + n / 10;
		*o++ = '0' + n % 10;
		n = (int)(p->ticks % HZ) * (100 / HZ);
		*o++ = '.';
		*o++ = '0' + n / 10;
		*o++ = '0' + n % 10;
		*o++ = ' ';
		o = lstr(o, p->comm, 0, 19);
		strcpy(o, p->stat == 3 ? col(PLAIN) : "");
	}
}

/* a new theme: everything made and drawn again */
retheme(t)
int t;
{
	int r;

	setattr(PLAINA);		/* white on black, in the old theme */
	settheme(t);
	once = 0;
	gen++;
	for (r = 0; r < 30; r++) {
		rowc[r].ok = 0;
		frame[r][0] = 0;
	}
	clearscr();
}

/* $HOME/.xtoprc: the theme chosen last */
readrc()
{
	FILE *f;
	char l[40];

	if (*rcfile == 0 || (f = fopen(rcfile, "r")) == NULL)
		return;
	while (fgets(l, sizeof l, f))
		if (strncmp(l, "theme ", 6) == 0)
			theme = l[6] == 'm' ? 'm' : 'c';
	fclose(f);
}

saverc()
{
	FILE *f;

	if (*rcfile && (f = fopen(rcfile, "w")) != NULL) {
		fprintf(f, "theme %s\n", theme == 'm' ? "mono" : "color");
		fclose(f);
	}
}

/* the file systems in /etc/mnttab, opened while xtop may: a record of 150
 * bytes each, the device's name in /dev (15 bytes), then the directory */
fsopen()
{
	FILE *m;
	unsigned char rec[150];
	char dev[30];
	struct fsys *f;

	if ((m = fopen("/etc/mnttab", "r")) == NULL)
		return;
	while (nfs < MAXFS && fread((char *)rec, 150, 1, m) == 1) {
		if (rec[0] == 0)
			continue;
		f = &fsys[nfs];
		sprintf(dev, "/dev/%.14s", rec);
		if ((f->fd = open(dev, 0)) < 0)
			continue;
		strncpy(f->dir, (char *)rec + 15, 19);
		f->dir[19] = 0;
		nfs++;
	}
	fclose(m);
}

/* put frame[] on the screen: of the lines made again, only the characters
 * that changed, as curses does. Drawing on the console is the kernel's
 * work, and on an XT a full screen of it costs top more than making it. */
draw()
{
	unsigned short cell[80], *row;
	register unsigned short *np, *sp;
	int r, c, w, end, k;

	if (bflag) {
		for (r = 0; r < lines; r++)
			printf("%s\n", frame[r]);
		printf("\n");
		fflush(stdout);
		return;
	}
	w = cols - 1;
	for (r = 0; r < lines; r++) {
		if (!dirty[r])
			continue;
		dirty[r] = 0;
		end = cells(frame[r], cell);
		row = scr[r];
		if (clean[r] && ansi && !cut) {
			/* a row blank since the screen was cleared, on the console:
			 * the line as it is, its escapes the console's own (on the
			 * first screen, a third of xtop's own time less: M58) */
			clean[r] = 0;
			memcpy((char *)row, (char *)cell, sizeof cell);
			if (end == 0)
				continue;
			move(r, 0);
			setattr(PLAINA);
			fputs(frame[r], stdout);
			currow = -1;
			curat = lastat;
			continue;
		}
		clean[r] = 0;
		for (c = 0, np = cell, sp = row; c < w; c++, np++, sp++) {
			if (*np == *sp)
				continue;
			if (c >= end) {		/* the rest of the new line is blank */
				move(r, c);
				setattr(PLAINA);
				tputs(CE, 1, outc);
				while (c++ < 80)
					*sp++ = BLANK;
				break;
			}
			if (r == currow && c > curcol && c - curcol <= 4)
				for (k = curcol; k < c; k++)	/* rewrite, cheaper than a move */
					put1(row[k]);
			else
				move(r, c);
			put1(*sp = *np);
			curcol = c + 1;
		}
	}
	move(lines - 1, PARK < cols - 1 ? PARK : cols - 2);	/* the cursor past the keys */
	fflush(stdout);
}

/* the terminal: raw, for keys one at a time with a wait of t seconds */
rawmode(t)
int t;
{
	if (bflag)
		return;
	traw = tsave;
	traw.c_lflag &= ~(ICANON | ECHO);
	traw.c_cc[VMIN] = 0;
	traw.c_cc[VTIME] = t * 10;
	ioctl(0, TCSETA, &traw);
	rawset = 1;
}

done()
{
	if (rawset)
		ioctl(0, TCSETA, &tsave);
	if (!bflag) {
		setattr(PLAINA);
		tputs(CL, 1, outc);
		fflush(stdout);
	}
	exit(0);
}

/* a line typed on the bottom row, after prompt */
char *
ask(prompt)
char *prompt;
{
	static char buf[20];
	int n = 0;
	char ch;

	traw.c_cc[VMIN] = 1;
	traw.c_cc[VTIME] = 0;
	ioctl(0, TCSETA, &traw);
	tputs(tgoto(CM, 0, lines - 1), 1, outc);
	setattr(PLAINA);
	fputs(prompt, stdout);
	tputs(CE, 1, outc);
	fflush(stdout);
	while (read(0, &ch, 1) == 1 && ch != '\r' && ch != '\n') {
		if ((ch == '\b' || ch == 0177) && n > 0) {
			n--;
			printf("\b \b");
		} else if (ch >= '0' && ch <= '9' && n < 10) {
			buf[n++] = ch;
			putchar(ch);
		} else if (ch == 033 || ch == 'q')
			n = 0, ch = '\n';
		if (ch == '\n')
			break;
		fflush(stdout);
	}
	buf[n] = 0;
	forget(lines - 1, 0);		/* draw the bottom row again */
	currow = -1;
	curat = -1;
	rawmode(delay);
	return buf;
}

help()
{
	static char *text[] = {
		"xtop - the processes using the machine, kept up to date",
		"",
		"CPU  the processor's time since the last screen: in programs (green),",
		"     and in the kernel (red)",
		"Mem  the memory in use, of all the machine has: the processes' own",
		"     (green), the programs' shared text (yellow), the kernel's (blue)",
		"Swp  the swap area in use, the part of the hard disk XENIX moves whole",
		"     processes out to when memory is full; beside it, how many are out",
		"     now, and how many went out and came back in since the last screen",
		"Dsk  a file system's space in use",
		"",
		"TTY  the process's terminal (co the console, ? none);",
		"SIZE the process's size; RES how much of it is in memory (0 when it",
		"     is swapped out); S its state: R running, S sleeping, W waiting,",
		"     Z ended (a zombie), T stopped",
		"",
		"P, M, T, N   sort by processor, memory, time or process id",
		"k            send a process a signal (15 ends it, 9 kills it)",
		"d            the seconds between screens",
		"c            Color or Monochrome (kept in $HOME/.xtoprc)",
		"q            quit",
		"",
		"Press any key to go back.",
		0
	};
	int i;
	char ch;

	setattr(PLAINA);
	tputs(CL, 1, outc);
	for (i = 0; text[i]; i++)
		printf("%s\r\n", text[i]);
	fflush(stdout);
	traw.c_cc[VMIN] = 1;
	traw.c_cc[VTIME] = 0;
	ioctl(0, TCSETA, &traw);
	read(0, &ch, 1);
	rawmode(delay);
	clearscr();
}

#ifdef PHASES
#include <sys/times.h>
extern long times();
int	phfd = -1;

/* a line of phases: the step just done, the ticks (1/20 s) of processor
 * so far in xtop's own code and in the kernel, the clock (lbolt, once the
 * kernel's symbols are known; times()'s own value, in case it is the
 * clock), and the counts of the reads and stats that may cost */
phase(s)
char *s;
{
	struct tms t;
	long r, lb = -1;
	unsigned char x[4];
	char b[120];

	r = times(&t);
	if (got[S_LBOLT] && kmem >= 0 && kread(sym[S_LBOLT], (char *)x, 4))
		lb = get4(x);
	if (phfd < 0)
		phfd = creat("phases", 0644);
	sprintf(b, "%-8s user %3ld sys %3ld lbolt %7ld times %ld  dir %d stat %d swap %d sym %d\n",
		s, t.tms_utime, t.tms_stime, lb, r, ndirreads, nstats, nswapreads, nsymreads);
	write(phfd, b, strlen(b));
}
#endif

main(argc, argv)
int argc;
char **argv;
{
	char *term, *ap, *s, ch;
	unsigned char b[64];
	unsigned cw, sw;
	int i, n, pid, sig, want = 0, vcolor;
	char vname[10];

	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-b") == 0)
			bflag++;
		else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "-m") == 0)
			want = argv[i][1];
		else if (strcmp(argv[i], "-d") == 0 && i + 1 < argc)
			delay = atoi(argv[++i]);
		else if (strcmp(argv[i], "-n") == 0 && i + 1 < argc)
			count = atoi(argv[++i]);
		else {
			fprintf(stderr, "usage: xtop [-b] [-c | -m] [-d seconds] [-n times]\n");
			exit(2);
		}
	}
	if (delay < 1)
		delay = 1;
	if (delay > 25)
		delay = 25;
	mypid = getpid();
	PH("start");
	if (!nlist("/xenix")) {
		fprintf(stderr, "xtop: cannot read the kernel's symbols in /xenix\n");
		exit(1);
	}
	PH("nlist");
	kmem = open("/dev/kmem", 0);
	mem = open("/dev/mem", 0);
	swapfd = open("/dev/swap", 0);
	if (kmem < 0 || mem < 0) {
		fprintf(stderr, "xtop: cannot read /dev/kmem and /dev/mem\n");
		exit(1);
	}
	PH("opens");
	fsopen();
	PH("fsopen");
	setuid(getuid());		/* sysinfo's part is done */
	kread(sym[S_BOOT], (char *)b, 4);
	boot = get4(b);
	memsize();
	PH("memsize");
	ttystart();
	PH("ttyscan");
	setbuf(stdout, obuf);		/* a screen in a few writes, not a line at a time */
	kread(sym[S_V], (char *)b, 64);
	if ((vproc = get2(b + 24)) > MAXP)
		vproc = MAXP;
	kread(sym[S_SWPLO], (char *)b, 4);
	swplo = get4(b);
	kread(sym[S_NSWAP], (char *)b, 2);
	nswap = get2(b);
	PH("kernel");

	/* the 8087: after fninit its control word is 03FF; later ones' 037F */
	fpprobe(&cw, &sw);
	strcpy(fpu, cw == 0x5a5a ? "none" : (cw & 0x80) ? "8087" : "x87");
	/* the display, when this is the console: a color one is a CGA (XENIX
	 * 2.1.3 does not start with an EGA); a monochrome one an IBM MDA or a
	 * Hercules card, told apart by the retrace bit of its status port */
	if ((vcolor = ioctl(0, GIO_COLOR, 0)) < 0)
		vcolor = ioctl(1, GIO_COLOR, 0);
	PH("fpu");
	if (vcolor >= 0)
		strcpy(vname, vcolor ? "CGA" : hercules() ? "Hercules" : "MDA");

	if ((s = getenv("HOME")) != 0 && strlen(s) < sizeof rcfile - 10)
		sprintf(rcfile, "%s/.xtoprc", strcmp(s, "/") ? s : "");
	if (!bflag) {
		if ((term = getenv("TERM")) == 0 || tgetent(tcbuf, term) <= 0) {
			fprintf(stderr, "xtop: no termcap entry for the terminal (TERM)\n");
			exit(1);
		}
		ap = tcstr;
		CM = tgetstr("cm", &ap);
		CL = tgetstr("cl", &ap);
		CE = tgetstr("ce", &ap);
		SO = tgetstr("so", &ap);
		SE = tgetstr("se", &ap);
		if (!SO || !SE)
			SO = SE = 0;
		if (!CM || !CL || !CE) {
			fprintf(stderr, "xtop: the terminal cannot move its cursor\n");
			exit(1);
		}
		if ((n = tgetnum("li")) > 0)
			lines = n;
		if ((n = tgetnum("co")) > 0)
			cols = n;
		/* Color for the console's escapes on a color display */
		ansi = strcmp(term, "ansi") == 0;
		cancolor = ansi && vcolor != 0;
		theme = cancolor ? 'c' : 'm';
		readrc();
		ioctl(0, TCGETA, &tsave);
		signal(SIGINT, done);
		signal(SIGQUIT, done);
		signal(SIGHUP, done);
		rawmode(delay);
	}
	PH("video+tc");
	if (lines > 30)
		lines = 30;
	if (cols > 80)			/* (the lines are made for 80) */
		cols = 80;
	if (cols < 60)
		cols = 60;
	if (vcolor >= 0)
		sprintf(video, "%s %dx%d", vname, cols, lines);
	settheme(want ? want : theme);
	if (!bflag)
		clearscr();
	PH("clear");
	sample();
	PH("sample1");
	if (bflag)
		sleep(delay);
	sample();
	PH("sample2");
	for (;;) {
		sortprocs();
		build();
		draw();
		PH("screen");
		if (kfdirty)
			keep();
		if (count > 0 && --count == 0)
			break;
		if (bflag) {
			sleep(delay);
			sample();
			continue;
		}
		n = read(0, &ch, 1);
		if (n == 1)
			switch (ch) {
			case 'q':
			case 'Q':
				done();
			case 'P': case 'p': sortby = 'P'; break;
			case 'M': case 'm': sortby = 'M'; break;
			case 'T': case 't': sortby = 'T'; break;
			case 'N': case 'n': sortby = 'N'; break;
			case 'c': case 'C':
				if (cancolor) {
					retheme(theme == 'c' ? 'm' : 'c');
					saverc();
				}
				break;
			case 'h': case 'H': case '?':
				help();
				break;
			case 'k': case 'K':
				s = ask("Kill process: ");
				if ((pid = atoi(s)) > 0) {
					s = ask("Signal (15): ");
					sig = *s ? atoi(s) : 15;
					if (kill(pid, sig) < 0) {
						ask("Cannot: not yours, or gone. Press Return.");
					}
				}
				break;
			case 'd': case 'D':
				s = ask("Seconds between screens: ");
				if ((n = atoi(s)) >= 1 && n <= 25)
					delay = n;
				rawmode(delay);
				break;
			}
		sample();
	}
	done();
}
