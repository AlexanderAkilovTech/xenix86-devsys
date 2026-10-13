/* ls - list the contents of directories
 *
 * usage: ls [-1ACFRabcdfghilmnopqrstux] [name ...]
 *
 * Also installed as l (ls -l), lc (ls -C), lf (ls -CF), lr (ls -CR) and
 * lx (ls -x), as XENIX's ls is. For each directory named, its entries; for
 * each other file, the file; with no names, the current directory. The
 * options are XENIX's:
 *	-l	long form: mode, links, owner, group, size and time
 *	-n, -o, -g  the long form with numbers for names, without the
 *		group, or without the owner
 *	-C, -x, -m, -1  in columns down or across, as a list with commas,
 *		or one to a line (the default)
 *	-t, -u, -c, -r  sorted by modification, access or change time,
 *		newest first, or in reverse
 *	-a, -A	every entry, or every one but . and .. (the super-user's
 *		default, which -A turns off)
 *	-d, -R, -f  directories as files, recursively, or every name taken
 *		as a directory and its entries unsorted
 *	-i, -s, -F, -p, -b, -q  the i-number, the size in blocks, a / after
 *		directories (and * after programs), and odd characters
 *		as \ooo or ?
 * and, added here:
 *	-h	sizes in K, M, G and T (of 1024), with -l and -s
 *
 * What XENIX's ls does, and this one copies: the super-user sees the
 * names that begin with a dot, all but . and .., without -a, and -A
 * turns that off for him; a listing
 * with -s or -l begins with a "total" of the blocks, which -C, -x and -m
 * lay out as if it were the first name; -m prints the numbers of -i and
 * -s without padding; -x wins over -C, whichever comes first; and the
 * blocks of -s count an indirect block for every 128 blocks of a file.
 * Files of the same time are in reverse order of their names under -t;
 * XENIX's own order for them is that of its sort, and varies.
 * A directory on XENIX is read as a file of 16-byte entries: a 2-byte
 * i-number (0 for a free slot) and a 14-byte name.
 * xenix86-devsys, K&R C. */
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#define	DIRSIZ	14
#define	WIDTH	80		/* of the screen, for -C, -x and -m */

struct ent {
	char	*name;
	unsigned ino;
	unsigned mode;
	int	nlink;
	unsigned uid, gid;
	unsigned rdev;
	long	size;
	long	time;		/* the one -u or -c choose */
	char	isarg;		/* named on the command line */
	char	ok;		/* stat()ed */
};

int	aflg, Aflg, bflg, cflg, dflg, fflg, gflg, iflg, lflg, mflg, nflg, oflg;
int	pflg, qflg, rflg, sflg, tflg, uflg, xflg, Cflg, Fflg, Rflg, hflg;
int	errors, curcol;
long	sixago, now;
char	path[512];

extern char *malloc(), *strcpy(), *strrchr(), *strchr(), *strncpy(), *ctime();
extern long time();

nomem()
{
	fprintf(stderr, "ls: out of memory\n");
	exit(2);
}

char *
save(s)
char *s;
{
	char *p = malloc(strlen(s) + 1);

	if (p == 0)
		nomem();
	return strcpy(p, s);
}

/* a size of n units of 1024^u bytes (u = 0 for bytes) in 5 characters or
 * fewer, rounded: 512B, 6.7K, 10M, 1.0G */
char *
human(n, u)
long n;
int u;
{
	static char buf[4][12];
	static int which;
	static char unit[] = "BKMGT";
	register char *b = buf[which++ & 3];
	long r = 0, tenths;

	if (n < 1024) {
		sprintf(b, "%ld%c", n, unit[u]);
		return b;
	}
	while (n >= 1024 && u < 4) {
		r = n % 1024;	/* the fraction, in 1024ths */
		n /= 1024;
		u++;
	}
	tenths = n * 10 + (r * 10 + 512) / 1024;
	if (tenths < 100)
		sprintf(b, "%ld.%ld%c", tenths / 10, tenths % 10, unit[u]);
	else {
		n += r >= 512;
		if (n >= 1024 && u < 4)
			sprintf(b, "1.0%c", unit[u + 1]);
		else
			sprintf(b, "%ld%c", n, unit[u]);
	}
	return b;
}

/* the blocks of 512 bytes a file of size bytes takes, as XENIX's ls
 * counts them: its 1024-byte blocks, and an indirect block for every 128
 * of them past the first 10 (and one more past 138, and past 16522) */
long
blocks(size)
long size;
{
	long b, t;

	b = t = (size + 1023) / 1024;
	if (b > 10)
		t += (b - 11) / 128 + 1;
	if (b > 10 + 128)
		t += (b - 10 - 128 - 1) / 16384 + 1;
	if (b > 10 + 128 + 16384L)
		t++;
	return t * 2;
}

/* names of users and groups, from /etc/passwd and /etc/group */
struct idname {
	unsigned id;
	char	name[9];
};
struct idname users[16], groups[16];
int	nusers, ngroups;

char *
idname(id, tab, n, file)
unsigned id;
struct idname *tab;
int *n;
char *file;
{
	FILE *f;
	char line[256], *p, *q;
	int i;
	unsigned v;

	for (i = 0; i < *n; i++)
		if (tab[i].id == id)
			return tab[i].name[0] ? tab[i].name : 0;
	i = *n < 16 ? (*n)++ : (int)(id & 15);
	tab[i].id = id;
	tab[i].name[0] = 0;
	if ((f = fopen(file, "r")) == NULL)
		return 0;
	while (fgets(line, sizeof line, f)) {
		/* name:password:id: */
		if ((p = strchr(line, ':')) == 0 || (q = strchr(p + 1, ':')) == 0)
			continue;
		for (v = 0, q++; *q >= '0' && *q <= '9'; q++)
			v = v * 10 + *q - '0';
		if (v == id && *q == ':') {
			*p = 0;
			strncpy(tab[i].name, line, 8);
			tab[i].name[8] = 0;
			break;
		}
	}
	fclose(f);
	return tab[i].name[0] ? tab[i].name : 0;
}

/* fill in e from stat(name); 0 if it is not there */
getstat(e, name)
register struct ent *e;
char *name;
{
	struct stat st;

	if (stat(name, &st) < 0)
		return 0;
	e->ino = st.st_ino;
	e->mode = st.st_mode;
	e->nlink = st.st_nlink;
	e->uid = st.st_uid;
	e->gid = st.st_gid;
	e->rdev = st.st_rdev;
	e->size = st.st_size;
	e->time = uflg ? st.st_atime : cflg ? st.st_ctime : st.st_mtime;
	e->ok = 1;
	return 1;
}

#define	ISDIR(e)	(((e)->mode & S_IFMT) == S_IFDIR)

/* the order: names (or times, newest first, and names in reverse for
 * the same time); and, but for -d and -f, arguments that are directories
 * after the others */
cmp(a, b)
register struct ent *a, *b;
{
	int r;

	if (a->isarg && !dflg && !fflg && ISDIR(a) != ISDIR(b))
		return ISDIR(a) ? 1 : -1;
	if (tflg)
		r = a->time > b->time ? -1 : a->time < b->time ? 1 : -strcmp(a->name, b->name);
	else
		r = strcmp(a->name, b->name);
	return rflg ? -r : r;
}

/* sort v[0..n-1] by cmp: a merge sort, with t for room */
sort(v, t, n)
struct ent **v, **t;
int n;
{
	int h, i, j, k;

	if (n < 2)
		return;
	h = n / 2;
	sort(v, t, h);
	sort(v + h, t, n - h);
	for (i = 0, j = h, k = 0; i < h && j < n; )
		t[k++] = cmp(v[i], v[j]) <= 0 ? v[i++] : v[j++];
	while (i < h)
		t[k++] = v[i++];
	while (j < n)
		t[k++] = v[j++];
	for (k = 0; k < n; k++)
		v[k] = t[k];
}

/* the name as it is printed: odd characters as \ooo (-b) or ? (-q) */
char *
shown(s)
register char *s;
{
	static char buf[600];
	register char *b = buf;

	for (; *s && b < buf + sizeof buf - 5; s++)
		if ((bflg || qflg) && (*s < ' ' || *s >= 0177)) {
			if (bflg) {
				sprintf(b, "\\%03o", *s & 0377);
				b += 4;
			} else
				*b++ = '?';
		} else
			*b++ = *s;
	*b = 0;
	return buf;
}

/* the mark -F or -p puts after a name, or 0 */
mark(e)
register struct ent *e;
{
	if (!(Fflg || pflg) || !e->ok)
		return 0;
	if (ISDIR(e))
		return '/';
	if (Fflg && (e->mode & S_IFMT) == S_IFREG && (e->mode & 0111))
		return '*';
	return 0;
}

char	*perms[] = { "---", "--x", "-w-", "-wx", "r--", "r-x", "rw-", "rwx" };

/* the mode as -l shows it: drwxr-xr-x */
char *
modes(m)
register unsigned m;
{
	static char s[12];
	unsigned t = m & S_IFMT;

	s[0] = t == S_IFDIR ? 'd' : t == S_IFCHR ? 'c' : t == S_IFBLK ? 'b' :
		t == S_IFIFO ? 'p' : t == S_IFNAM ? 'n' : '-';
	strcpy(s + 1, perms[(m >> 6) & 7]);
	strcpy(s + 4, perms[(m >> 3) & 7]);
	strcpy(s + 7, perms[m & 7]);
	if (m & S_ISUID)
		s[3] = (m & 0100) ? 's' : 'S';
	if (m & S_ISGID)
		s[6] = (m & 0010) ? 's' : 'S';
	if (m & S_ISVTX)
		s[9] = (m & 0001) ? 't' : 'T';
	return s;
}

/* print one entry, without the newline; the columns it took. -m prints
 * the numbers of -i and -s without padding */
entry(e)
register struct ent *e;
{
	char *cp, *nm;
	int n = 0, c;
	long b = e->ok ? blocks(e->size) : 0L;

	if (iflg)
		n += printf(mflg ? "%u " : "%5u ", e->ino);
	if (sflg) {
		if (hflg)
			n += printf(mflg ? "%s " : "%5s ", human(b * 512, 0));
		else
			n += printf(mflg ? "%ld " : "%4ld ", b);
	}
	if (lflg) {
		printf("%s%4d ", modes(e->mode), e->nlink);
		if (!gflg) {
			if (!nflg && (nm = idname(e->uid, users, &nusers, "/etc/passwd")))
				printf("%-9.9s", nm);
			else
				printf("%-9u", e->uid);
		}
		if (!oflg) {
			if (!nflg && (nm = idname(e->gid, groups, &ngroups, "/etc/group")))
				printf("%-9.9s", nm);
			else
				printf("%-9u", e->gid);
		}
		if ((e->mode & S_IFMT) == S_IFCHR || (e->mode & S_IFMT) == S_IFBLK)
			printf("%3d,%3d", (e->rdev >> 8) & 0377, e->rdev & 0377);
		else if (hflg)
			printf("%7s", human(e->size, 0));
		else
			printf("%7ld", e->size);
		cp = ctime(&e->time);
		if (e->time < sixago || e->time > now)
			printf(" %-7.7s %-4.4s ", cp + 4, cp + 20);
		else
			printf(" %-12.12s ", cp + 4);
	}
	n += printf("%s", shown(e->name));
	if ((c = mark(e)) != 0) {
		putchar(c);
		n++;
	}
	return n;
}

/* print cell k of a listing: the total first if there is one */
cell(v, k, total)
struct ent **v;
int k;
long total;
{
	if (total >= 0) {
		if (k == 0) {
			if (hflg)
				return printf("total %s", human(total * 512, 0));
			return printf("total %ld", total);
		}
		k--;
	}
	return entry(v[k]);
}

/* print the entries v[0..n-1] in the form the options ask for, after the
 * total of their blocks if total is not negative */
list(v, n, total)
struct ent **v;
int n;
long total;
{
	int i, j, k, w, cells, maxw, colw, cols, rows;

	cells = n + (total >= 0);
	if (cells == 0)
		return;
	/* the width of the widest entry: its name and what goes with it */
	for (i = 0, maxw = 0; i < n; i++)
		if ((w = strlen(shown(v[i]->name))) > maxw)
			maxw = w;
	maxw += (Fflg || pflg) + (iflg ? 6 : 0) + (sflg ? (hflg ? 6 : 5) : 0);
	if (mflg) {
		for (i = 0, curcol = 0; i < cells; i++) {
			if (i > 0) {
				putchar(',');
				if (++curcol + maxw + 4 > WIDTH) {
					putchar('\n');
					curcol = 0;
				} else {
					putchar(' ');
					curcol++;
				}
			}
			curcol += cell(v, i, total);
		}
		printf("\n");
		return;
	}
	if (!Cflg) {
		for (i = 0; i < cells; i++) {
			cell(v, i, total);
			printf("\n");
		}
		return;
	}
	colw = maxw + 2;
	if ((cols = WIDTH / colw) < 1)
		cols = 1;
	rows = (cells + cols - 1) / cols;
	for (i = 0; i < rows; i++) {
		for (j = 0; j < cols; j++) {
			k = xflg ? i * cols + j : j * rows + i;
			if (k >= cells)
				break;
			w = cell(v, k, total);
			/* pad to the next column, unless it is the last on the line */
			if (xflg ? k + 1 < cells && j + 1 < cols : k + rows < cells)
				while (w++ < colw)
					putchar(' ');
		}
		printf("\n");
	}
}

/* the entries of directory dir, sorted unless -f: in *vp, their number
 * returned, or -1 if dir cannot be read */
readdir(dir, vp)
char *dir;
struct ent ***vp;
{
	int fd, n = 0, room = 0, i, k, stat_them;
	char buf[512], name[DIRSIZ + 1];
	register char *d;
	struct ent *e, **v = 0, **t;

	if ((fd = open(dir, 0)) < 0) {
		fprintf(stderr, "%s unreadable\n", dir);
		errors++;
		return -1;
	}
	stat_them = lflg || sflg || tflg || Fflg || pflg || Rflg;
	while ((i = read(fd, buf, sizeof buf)) >= 16) {
		for (d = buf; d + 16 <= buf + i; d += 16) {
			if ((d[0] | d[1]) == 0)
				continue;
			strncpy(name, d + 2, DIRSIZ);
			name[DIRSIZ] = 0;
			if (name[0] == '.' && !aflg && !(Aflg && name[1] &&
			    !(name[1] == '.' && name[2] == 0)))
				continue;
			if (n == room) {
				room += 64;
				if ((t = (struct ent **)malloc(room * sizeof(struct ent *))) == 0)
					nomem();
				for (k = 0; k < n; k++)
					t[k] = v[k];
				if (v)
					free((char *)v);
				v = t;
			}
			if ((e = v[n++] = (struct ent *)malloc(sizeof(struct ent))) == 0)
				nomem();
			e->name = save(name);
			e->ino = (d[0] & 0377) | (d[1] & 0377) << 8;
			e->isarg = e->ok = 0;
			e->mode = 0;
			e->size = e->time = 0;
			if (stat_them) {
				sprintf(path, "%s/%s", dir, name);
				getstat(e, path);
			}
		}
	}
	close(fd);
	if (!fflg && n > 1) {
		if ((t = (struct ent **)malloc(n * sizeof(struct ent *))) == 0)
			nomem();
		sort(v, t, n);
		free((char *)t);
	}
	*vp = v;
	return n;
}

/* list the directory dir, after its name if title; and the directories in
 * it after it, if -R */
directory(dir, title)
char *dir;
int title;
{
	struct ent **v;
	int n, i;
	long total = -1;
	char *sub;

	if (title)
		printf("\n%s:\n", dir);
	if ((n = readdir(dir, &v)) < 0)
		return;
	if (lflg || sflg)
		for (i = 0, total = 0; i < n; i++)
			if (v[i]->ok)
				total += blocks(v[i]->size);
	list(v, n, total);
	for (i = 0; i < n; i++) {
		if (Rflg && v[i]->ok && ISDIR(v[i]) && strcmp(v[i]->name, ".") &&
		    strcmp(v[i]->name, "..") &&
		    (sub = malloc(strlen(dir) + strlen(v[i]->name) + 2)) != 0) {
			sprintf(sub, "%s/%s", dir, v[i]->name);
			free(v[i]->name);
			free((char *)v[i]);
			directory(sub, 1);
			free(sub);
			continue;
		}
		free(v[i]->name);
		free((char *)v[i]);
	}
	if (v)
		free((char *)v);
}

usage()
{
	fprintf(stderr, "usage: ls -1ACFRabcdfghilmnopqrstux [files]\n");
	exit(2);
}

main(argc, argv)
int argc;
char **argv;
{
	struct ent **args, **t, *e;
	char *me, *p;
	int i, n, names, nfiles;

	if ((me = strrchr(argv[0], '/')) != 0)
		me++;
	else
		me = argv[0];
	if (strcmp(me, "l") == 0)
		lflg++;
	else if (strcmp(me, "lc") == 0)
		Cflg++;
	else if (strcmp(me, "lf") == 0)
		Cflg++, Fflg++;
	else if (strcmp(me, "lr") == 0)
		Cflg++, Rflg++;
	else if (strcmp(me, "lx") == 0)
		Cflg++, xflg++;
	Aflg = getuid() == 0;		/* the super-user sees the names with dots */
	for (i = 1; i < argc && argv[i][0] == '-' && argv[i][1]; i++)
		for (p = argv[i] + 1; *p; p++)
			switch (*p) {
			case 'a': aflg++; break;
			case 'A': Aflg = !Aflg; break;	/* for root, -A hides them */
			case 'b': bflg++; qflg = 0; break;
			case 'c': cflg++; uflg = 0; break;
			case 'd': dflg++; break;
			case 'f': fflg++; break;
			case 'g': gflg++; lflg++; break;
			case 'h': hflg++; break;
			case 'i': iflg++; break;
			case 'l': lflg++; break;
			case 'm': mflg++; Cflg = xflg = 0; break;
			case 'n': nflg++; lflg++; break;
			case 'o': oflg++; lflg++; break;
			case 'p': pflg++; break;
			case 'q': qflg++; bflg = 0; break;
			case 'r': rflg++; break;
			case 's': sflg++; break;
			case 't': tflg++; break;
			case 'u': uflg++; cflg = 0; break;
			case 'x': xflg++; Cflg++; mflg = 0; break;
			case 'C': Cflg++; mflg = 0; break;	/* -x stays */
			case 'F': Fflg++; break;
			case 'R': Rflg++; break;
			case '1': Cflg = xflg = mflg = 0; break;
			default:
				fprintf(stderr, "%s: illegal option -- %c\n", argv[0], *p);
				usage();
			}
	if (fflg) {
		aflg++;
		lflg = tflg = sflg = rflg = 0;
	}
	if (lflg)
		Cflg = xflg = mflg = 0;
	now = time((long *)0);
	sixago = now - 6L * 30 * 24 * 60 * 60;
	now += 60;
	if ((names = argc - i) == 0) {
		argv[--i] = ".";
		names = 1;
	}
	args = (struct ent **)malloc(names * sizeof(struct ent *));
	t = (struct ent **)malloc(names * sizeof(struct ent *));
	if (args == 0 || t == 0)
		nomem();
	for (n = 0; i < argc; i++) {
		if ((e = (struct ent *)malloc(sizeof(struct ent))) == 0)
			nomem();
		e->name = argv[i];
		e->isarg = 1;
		e->ok = 0;
		if (!getstat(e, argv[i])) {
			fprintf(stderr, "%s not found\n", argv[i]);
			errors++;
			continue;
		}
		args[n++] = e;
	}
	sort(args, t, n);
	/* the files first, together; then each directory (with -f, every
	 * name is taken as a directory) */
	nfiles = 0;
	if (!fflg)
		while (nfiles < n && (dflg || !ISDIR(args[nfiles])))
			nfiles++;
	list(args, nfiles, -1L);
	for (i = nfiles; i < n; i++)
		directory(args[i]->name, names > 1);
	exit(errors ? 2 : 0);
}
