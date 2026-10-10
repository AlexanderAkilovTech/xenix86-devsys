/* aout2x - convert an ld86 a.out into a XENIX 2.1.3 8086 x.out executable
 *
 * usage: aout2x [-s stacksize] input output
 *
 * The input must be linked with "ld86 -0 -i" (separate instruction and data
 * space) with our crt0.o first, so that text starts with the reserved block
 * XENIX expects (EB 7E) and data with its header (00 00 FF FF). The output is
 * laid out like SCO's own small-model binaries: a 32-byte header, a 44-byte
 * extension, a segment table at offset 96, then text and data. Data keeps the
 * addresses ld86 gave it; it is padded to a 16-byte paragraph and the bss is
 * shrunk to match, so the program sees the same memory image.
 * xenix86-devsys, K&R C. */
#include <stdio.h>

#define	HDRLEN	96		/* header + extension, padded */
#define	SEGTAB	64		/* two 32-byte segment table entries */

char	*prog = "aout2x";
char	buf[1024];
int	in, out;

die(msg, arg)
char *msg, *arg;
{
	fprintf(stderr, "%s: ", prog);
	fprintf(stderr, msg, arg);
	fprintf(stderr, "\n");
	exit(1);
}

long
get4(p)
register unsigned char *p;
{
	return (long)(p[0] | p[1] << 8) | (long)(p[2] | p[3] << 8) << 16;
}

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

/* one segment table entry */
seg(p, type, attr, num, pos, size, vsize)
unsigned char *p;
unsigned type, attr, num;
long pos, size, vsize;
{
	put2(p, type);
	put2(p + 2, attr);
	put2(p + 4, num);
	put4(p + 8, pos);
	put4(p + 12, size);
	put4(p + 16, vsize);
}

/* copy n bytes from the input to the output */
copy(n, what)
long n;
char *what;
{
	register int k;

	while (n > 0) {
		k = n > sizeof buf ? sizeof buf : (int)n;
		if (read(in, buf, k) != k)
			die("%s: input too short", what);
		if (write(out, buf, k) != k)
			die("write error", "");
		n -= k;
	}
}

zeros(n)
register int n;
{
	static char z[16];

	if (n > 0 && write(out, z, n) != n)
		die("write error", "");
}

main(argc, argv)
int argc;
char **argv;
{
	unsigned char ah[32];		/* the ld86 a.out header */
	unsigned char h[HDRLEN + SEGTAB];	/* the x.out header and segment table */
	long text, data, bss, stack = 4096, pdata, ptext, hlen;
	extern long atol(), lseek();
	register int i;

	if (argc > 2 && argv[1][0] == '-' && argv[1][1] == 's') {
		stack = atol(argv[2]);
		argc -= 2;
		argv += 2;
	}
	if (argc != 3) {
		fprintf(stderr, "usage: %s [-s stacksize] input output\n", prog);
		exit(2);
	}
	if ((in = open(argv[1], 0)) < 0)
		die("cannot open %s", argv[1]);
	if (read(in, ah, 32) != 32 || ah[0] != 01 || ah[1] != 03)
		die("%s: not an ld86 a.out", argv[1]);
	if (!(ah[2] & 040))
		die("%s: not separate I&D (link with ld86 -i)", argv[1]);
	hlen = ah[4];
	text = get4(ah + 8);
	data = get4(ah + 12);
	bss = get4(ah + 16);

	ptext = (text + 1) & ~1L;		/* text: even length */
	pdata = (data + 15) & ~15L;		/* data: whole paragraphs */
	bss = ((data + bss + 15) & ~15L) - pdata;	/* same end of memory */
	if (bss < 0)
		bss = 0;

	for (i = 0; i < HDRLEN + SEGTAB; i++)
		h[i] = 0;
	put2(h, 0x0206);		/* x_magic */
	put2(h + 2, 44);		/* x_ext: size of the extension */
	put4(h + 4, ptext);
	put4(h + 8, pdata);
	put4(h + 12, bss);
	h[28] = 0x44;			/* x_cpu: 8086, word-swapped longs */
	h[29] = 0x64;			/* x_relsym */
	put2(h + 30, 0xc80f);		/* x_renv: segmented, fixed stack, pure, separate I&D, executable */
	put4(h + 48, stack);		/* xe_stksize */
	put4(h + 52, (long)HDRLEN);	/* xe_segpos */
	put4(h + 56, (long)SEGTAB);	/* xe_segsize */
	h[70] = 1;			/* xe_ostype: XENIX */
	h[71] = 2;			/* xe_osvers: 2.x */
	put2(h + 72, 63);		/* xe_eseg: entry point is in segment 63, the text */
	seg(h + HDRLEN, 1, 0x8008, 63, (long)(HDRLEN + SEGTAB), ptext, ptext);
	seg(h + HDRLEN + 32, 2, 0x8000 | (bss ? 4 : 0), 71, HDRLEN + SEGTAB + ptext, pdata, pdata + bss);

	if (lseek(in, hlen, 0) != hlen || read(in, buf, 2) != 2 ||
	    (buf[0] & 0377) != 0xEB || (buf[1] & 0377) != 0x7E)
		die("%s: crt0.o must be linked first", argv[1]);
	if (lseek(in, hlen + text, 0) != hlen + text || read(in, buf, 4) != 4 ||
	    buf[0] || buf[1] || (buf[2] & 0377) != 0xFF || (buf[3] & 0377) != 0xFF)
		die("%s: no data header (crt0.o must be linked first)", argv[1]);

	if ((out = creat(argv[2], 0777)) < 0)
		die("cannot create %s", argv[2]);
	if (write(out, h, sizeof h) != sizeof h)
		die("write error", "");
	lseek(in, hlen, 0);
	copy(text, "text");
	zeros((int)(ptext - text));
	copy(data, "data");
	zeros((int)(pdata - data));
	close(out);
	exit(0);
}
