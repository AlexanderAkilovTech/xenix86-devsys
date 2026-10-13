/* df - report free disk space
 *
 * usage: df [-tfvih] [filesystem | file ...]
 *
 * For each mounted file system (from /etc/mnttab), or those named, or the
 * ones holding the files named: the free blocks (of 512 bytes) and free
 * i-nodes, read from the file system's super block, in the forms XENIX's
 * own df prints:
 *	-t	the total blocks too, and those the i-list takes
 *	-f	only the free blocks
 *	-i	a table of i-nodes: used, free and the percentage used
 * and, changed or added here:
 *	-v	a table: the size, used, free and the percentage used, in K,
 *		M, G and T (of 1024), where XENIX's counts blocks; custom's
 *		"Display current disk usage" shows it, and reads nothing from it
 *	-h	a table in human sizes, as other systems' df -h
 * custom reads the free blocks from the first form, df /dev/root, which
 * stays as XENIX prints it.
 * XENIX keeps its file systems in blocks of 1024 bytes (s_type S_B1024) or
 * 512; the super block is the second 1024-byte block, its magic number
 * 0x2b5544 at its end. /etc/mnttab has a record of 150 bytes for each
 * mounted file system: the device's name in /dev (15 bytes), the mount
 * directory (129), a read-only flag and the time it was mounted.
 * xenix86-devsys, K&R C. */
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#define	SBOFF	1024L		/* where the super block is */
#define	MAGIC	0x2b5544L
#define	MNTSIZE	150

struct fs {
	char	dev[100];	/* /dev/root */
	char	dir[130];	/* / */
	long	blocks;		/* 512-byte blocks */
	long	free;
	long	inodes;
	long	ifree;
	long	ilist;		/* blocks taken by the boot and super blocks and i-list */
};

int	tflag, vflag, hflag, iflag, fflag, errors, nshown;
long	tblocks, tfree, tinodes, tifree;

long	get4(p) unsigned char *p; { return p[0] | p[1] << 8 | (long)p[2] << 16 | (long)p[3] << 24; }
unsigned get2(p) unsigned char *p; { return p[0] | p[1] << 8; }

extern char *strcpy(), *strncpy();

/* read the super block of f->dev into f; 0 if it cannot be read (and,
 * unless quiet, say why) */
super(f, quiet)
register struct fs *f;
int quiet;
{
	unsigned char sb[1024];
	int fd, n;
	long mult, type;

	if ((fd = open(f->dev, 0)) < 0) {
		if (!quiet) {
			fprintf(stderr, "df: cannot open %s\n", f->dev);
			errors++;
		}
		return 0;
	}
	lseek(fd, SBOFF, 0);
	n = read(fd, (char *)sb, 1024);
	close(fd);
	if (n != 1024 || get4(sb + 1016) != MAGIC) {
		if (!quiet) {
			fprintf(stderr, "df: %s: not a XENIX file system\n", f->dev);
			errors++;
		}
		return 0;
	}
	type = get4(sb + 1020);
	mult = type == 2 ? 2 : 1;		/* 1024-byte blocks are two */
	f->blocks = get4(sb + 2) * mult;
	f->free = get4(sb + 618) * mult;
	f->inodes = (long)(get2(sb) - 2) * (type == 2 ? 16 : 8);
	f->ilist = get2(sb) * mult;
	f->ifree = get2(sb + 622);
	return 1;
}

/* a size of k kilobytes in 5 characters or fewer, rounded as ls -h rounds:
 * 512K, 7.2M, 18M */
char *
human(k)
long k;
{
	static char buf[4][12];
	static int which;
	static char unit[] = "KMGT";
	register char *b = buf[which++ & 3];
	long r = 0, tenths;
	int u = 0;

	if (k < 1024) {
		sprintf(b, "%ldK", k);
		return b;
	}
	while (k >= 1024 && u < 3) {
		r = k % 1024;	/* the fraction, in 1024ths */
		k /= 1024;
		u++;
	}
	tenths = k * 10 + (r * 10 + 512) / 1024;
	if (tenths < 100)
		sprintf(b, "%ld.%ld%c", tenths / 10, tenths % 10, unit[u]);
	else {
		k += r >= 512;
		if (k >= 1024 && u < 3)
			sprintf(b, "1.0%c", unit[u + 1]);
		else
			sprintf(b, "%ld%c", k, unit[u]);
	}
	return b;
}

static int
percent(used, total)
long used, total;
{
	if (total <= 0)
		return 0;
	return (int)((used * 100 + total / 2) / total);
}

show(f)
register struct fs *f;
{
	long used, iused;

	if (!f->blocks && !super(f, 0))
		return;
	used = f->blocks - f->free;
	iused = f->inodes - f->ifree;
	if (hflag) {
		if (nshown++ == 0)
			printf("Filesystem     Size   Used  Avail Capacity  Mounted on\n");
		printf("%-12s %6s %6s %6s %7d%%  %s\n", f->dev, human(f->blocks / 2),
			human(used / 2), human(f->free / 2), percent(used, f->blocks), f->dir);
	} else if (vflag || iflag) {
		/* XENIX's table: blocks with -v, i-nodes with -i, or both */
		if (nshown++ == 0) {
			printf("Mount Dir   Filesystem  ");
			if (vflag)
				printf("  size\t  used\t  free\t%% used");
			if (vflag && iflag)
				printf("\t");
			if (iflag)
				printf("iused\tifree\t%%iused");
			printf("\n");
		}
		printf("%-12s%-10s", f->dir, f->dev);
		if (vflag)
			printf("%8s\t%6s\t%6s\t%5d%%", human(f->blocks / 2), human(used / 2),
				human(f->free / 2), percent(used, f->blocks));
		if (iflag)
			printf("\t%5ld\t%5ld\t%5d%%", iused, f->ifree, percent(iused, f->inodes));
		printf("\n");
	} else {
		/* XENIX's own: the free blocks and i-nodes, and with -t the totals */
		nshown++;
		if (fflag)
			printf("%-8s(%-10s): %8ld blocks\n", f->dir, f->dev, f->free);
		else
			printf("%-8s(%-10s): %8ld blocks %7ld i-nodes\n", f->dir, f->dev,
				f->free, f->ifree);
		if (tflag)
			printf("%21s(%8ld total blocks, %4ld for i-nodes)\n", "", f->blocks,
				f->ilist);
	}
	tblocks += f->blocks;
	tfree += f->free;
	tinodes += f->inodes;
	tifree += f->ifree;
}

/* the mounted file systems; fill f with the one on the device rdev, or
 * with all of them (show each) if rdev is -1 */
static int
mounted(rdev, f)
int rdev;
struct fs *f;
{
	FILE *m;
	unsigned char rec[MNTSIZE];
	struct stat st;
	int found = 0;

	if ((m = fopen("/etc/mnttab", "r")) == NULL) {
		fprintf(stderr, "df: cannot read /etc/mnttab\n");
		errors++;
		return 0;
	}
	while (fread((char *)rec, MNTSIZE, 1, m) == 1) {
		if (rec[0] == 0)
			continue;
		sprintf(f->dev, "/dev/%.14s", rec);
		f->blocks = 0;
		strncpy(f->dir, (char *)rec + 15, 129);
		f->dir[129] = 0;
		if (rdev == -1) {
			show(f);
			found++;
		} else if (stat(f->dev, &st) == 0 && st.st_rdev == rdev) {
			found++;
			break;
		}
	}
	fclose(m);
	return found;
}

main(argc, argv)
int argc;
char **argv;
{
	struct fs f;
	struct stat st;
	char *p;
	int i;

	/* have the kernel write its super blocks out, so that what is read
	 * from the disks is up to date */
	sync();
	for (i = 1; i < argc && argv[i][0] == '-'; i++)
		for (p = argv[i] + 1; *p; p++)
			switch (*p) {
			case 't': tflag++; break;
			case 'v': vflag++; break;
			case 'h': hflag++; break;
			case 'f': fflag++; break;
			case 'i': iflag++; break;
			default:
				fprintf(stderr, "usage: df [-tfvih] [filesystem | file ...]\n");
				exit(2);
			}
	if (i >= argc)
		mounted(-1, &f);
	for (; i < argc; i++) {
		if (stat(argv[i], &st) < 0) {
			sprintf(f.dev, "/dev/%.24s", argv[i]);	/* df root */
			if (stat(f.dev, &st) < 0) {
				fprintf(stderr, "df: %s: not found\n", argv[i]);
				errors++;
				continue;
			}
		}
		f.blocks = 0;
		if ((st.st_mode & S_IFMT) == S_IFREG) {
			/* a file holding a file system, such as a floppy's image */
			strncpy(f.dev, argv[i], 99);
			f.dev[99] = 0;
			if (super(&f, 1)) {
				strcpy(f.dir, "");
				show(&f);
				continue;
			}
			f.blocks = 0;
		}
		if ((st.st_mode & S_IFMT) == S_IFBLK || (st.st_mode & S_IFMT) == S_IFCHR) {
			/* a device: shown without its mount directory, as XENIX's
			 * df shows it */
			if (argv[i][0] == '/')
				strcpy(f.dev, argv[i]);
			strcpy(f.dir, "");
		} else if (!mounted(st.st_dev, &f)) {
			fprintf(stderr, "df: %s: its file system is not in /etc/mnttab\n", argv[i]);
			errors++;
			continue;
		}
		show(&f);
	}
	if ((hflag || vflag) && tflag && nshown > 1) {
		if (hflag)
			printf("%-12s %6s %6s %6s %7d%%\n", "total", human(tblocks / 2),
				human((tblocks - tfree) / 2), human(tfree / 2),
				percent(tblocks - tfree, tblocks));
		else
			printf("%-12s%-10s%8s\t%6s\t%6s\t%5d%%\n", "total", "", human(tblocks / 2),
				human((tblocks - tfree) / 2), human(tfree / 2), percent(tblocks - tfree, tblocks));
	}
	exit(errors != 0);
}
