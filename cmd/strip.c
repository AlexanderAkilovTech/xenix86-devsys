/* strip - remove the symbol table from executables
 *
 * usage: strip file...
 *
 * The symbol table is what nm and adb read; a program runs the same without
 * it, and takes less disk space. strip writes the executable without it to a
 * file in /tmp, then copies that back over the original, which keeps its
 * owner and mode. The result is the same as linking with cc -s. A file
 * that has no symbol table is left alone. Only x.out executables can be
 * stripped: an object file needs its symbols to be linked.
 * xenix86-devsys, K&R C. */
#include <stdio.h>
#include <signal.h>
#include "objfile.h"

#define	MAXSEG	16

char	*prog = "strip";
char	tmp[32];
char	buf[1024];
int	errors;

put2(p, v)
register unsigned char *p;
unsigned v;
{
	p[0] = v;
	p[1] = v >> 8;
}

put4(p, v)
register unsigned char *p;
long v;
{
	put2(p, (unsigned)v);
	put2(p + 2, (unsigned)(v >> 16));
}

err(name, what)
char *name, *what;
{
	fprintf(stderr, "strip: %s: %s\n", name, what);
	errors++;
	return -1;
}

/* copy n bytes from fd in at pos to fd out; 0 if all went well */
copy(in, pos, out, n)
int in, out;
long pos, n;
{
	register int k;
	extern long lseek();

	if (lseek(in, pos, 0) != pos)
		return -1;
	while (n > 0) {
		k = n > sizeof buf ? sizeof buf : (int)n;
		if (read(in, buf, k) != k || write(out, buf, k) != k)
			return -1;
		n -= k;
	}
	return 0;
}

strip(name)
char *name;
{
	unsigned char h[256];		/* the header, up to the segment table */
	unsigned char seg[MAXSEG][32];	/* the segment table */
	int keep[MAXSEG];
	long old[MAXSEG];		/* the segments' positions in the original */
	long segpos, pos, n;
	int nseg, nkeep, i, j, t, in, out;

	if (objopen(name) != F_XOUT) {
		if (objfd >= 0)
			err(name, "not an executable");
		else
			errors++;
		objclose();
		return;
	}
	in = objfd;
	if (lseek(in, 0L, 0) != 0 || read(in, h, 76) != 76)
		goto bad;
	if (!(get2(h + 30) & 0x0800)) {
		err(name, "not a segmented x.out");
		goto done;
	}
	segpos = get4(h + 52);
	nseg = (int)(get4(h + 56) / 32);
	if (segpos < 76 || segpos > sizeof h || nseg < 1 || nseg > MAXSEG)
		goto bad;
	if (lseek(in, 0L, 0) != 0 || read(in, h, (int)segpos) != (int)segpos ||
	    read(in, seg, nseg * 32) != nseg * 32)
		goto bad;
	/* keep every segment but the symbol table (type 3) and its
	 * string table (type 72), in the order they are in the file */
	nkeep = 0;
	for (i = 0; i < nseg; i++) {
		old[i] = get4(seg[i] + 8);
		if ((t = get2(seg[i])) != 3 && t != 72)
			keep[nkeep++] = i;
	}
	if (nkeep == nseg && get4(h + 16) == 0)
		goto done;		/* no symbols: nothing to do */
	for (i = 1; i < nkeep; i++)
		for (j = i; j > 0 && old[keep[j - 1]] > old[keep[j]]; j--) {
			t = keep[j];
			keep[j] = keep[j - 1];
			keep[j - 1] = t;
		}

	sprintf(tmp, "/tmp/stp%d", getpid());
	if ((out = creat(tmp, 0600)) < 0) {
		err(tmp, "cannot create");
		goto done;
	}
	put4(h + 16, 0L);			/* x_syms */
	put4(h + 56, 32L * nkeep);		/* xe_segsize */
	if (write(out, h, (int)segpos) != (int)segpos)
		goto werr;
	pos = segpos + 32L * nkeep;		/* the segments follow the table */
	for (i = 0; i < nkeep; i++) {
		n = get4(seg[keep[i]] + 12);
		if (n > 0)
			put4(seg[keep[i]] + 8, pos);
		pos += n;
		if (write(out, seg[keep[i]], 32) != 32)
			goto werr;
	}
	for (i = 0; i < nkeep; i++) {
		n = get4(seg[keep[i]] + 12);
		if (n > 0 && copy(in, old[keep[i]], out, n) < 0)
			goto werr;
	}
	close(out);
	out = -1;

	/* back over the original, which keeps its owner and mode */
	close(in);
	objfd = -1;
	if ((in = open(tmp, 0)) < 0 || (out = creat(name, 0777)) < 0) {
		err(name, "cannot write");
		goto cleanup;
	}
	pos = lseek(in, 0L, 2);
	if (copy(in, 0L, out, pos) < 0)
		err(name, "write error: the file may be damaged");
	goto cleanup;
werr:
	err(tmp, "write error");
cleanup:
	if (in >= 0)
		close(in);
	if (out >= 0)
		close(out);
	unlink(tmp);
	tmp[0] = 0;
	objfd = -1;
	return;
bad:
	err(name, "bad header");
done:
	objclose();
}

onintr()
{
	if (tmp[0])
		unlink(tmp);
	exit(1);
}

main(argc, argv)
int argc;
char **argv;
{
	register int i;

	if (argc < 2) {
		fprintf(stderr, "usage: strip file...\n");
		exit(2);
	}
	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
		signal(SIGINT, onintr);
	for (i = 1; i < argc; i++)
		strip(argv[i]);
	exit(errors != 0);
}
