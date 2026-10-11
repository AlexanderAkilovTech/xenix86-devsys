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
 *
 * If ld86 wrote a symbol table (cc links with ld86 -y, which keeps whole
 * names), it becomes an x.out one, as SCO's ld writes it: a third segment
 * table entry, of type 3 (symbols) in the segmented format, whose records
 * are a type, a segment number and a value, each followed by the name and a
 * 0; the segment names _TEXT and _DATA come first. tools/aout2xout.py does
 * the same on the host.
 * xenix86-devsys, K&R C. */
#include <stdio.h>

#define	HDRLEN	96		/* header + extension, padded */
#define	SEGTAB	64		/* two 32-byte segment table entries */
#define	SEGTABS	96		/* three, with the symbol table */
#define	TEXTSEG	63		/* the segment numbers of the small model */
#define	DATASEG	71
#define	SYMLEN	50		/* the longest name XENIX's tools take */

/* x.out symbol types (SCO's sys/relsym.h) */
#define	S_ABS	1
#define	S_TEXT	2
#define	S_DATA	3
#define	S_BSS	4
#define	S_SEG	8
#define	S_EXTERN 040
/* ld86's (nlist n_sclass) */
#define	N_SECT	07
#define	C_CLASS	0370
#define	C_EXT	020

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

wr(p, n)
char *p;
int n;
{
	if (n > 0 && write(out, p, n) != n)
		die("write error", "");
}

/* the symbol being built: type, segment, value and name so far */
unsigned stype, sseg;
long svalue;
char sname[SYMLEN + 1];
int slen, spending;
long ssize;			/* the bytes of symbols written */

/* write one x.out symbol */
putsym(type, segno, value, name, len)
unsigned type, segno;
long value;
char *name;
int len;
{
	unsigned char r[8];

	put2(r, type);
	put2(r + 2, segno);
	put4(r + 4, value);
	wr(r, 8);
	wr(name, len);
	wr("", 1);
	ssize += 8 + len + 1;
}

flushsym()
{
	if (spending)
		putsym(stype, sseg, svalue, sname, slen);
	spending = 0;
}

/* convert ld86's symbol table, n bytes of 16-byte nlist records: an 8-byte
 * name, the value, the class; with ld86 -y, the rest of a name longer than
 * 8 characters follows in records of class 0 */
symbols(n)
long n;
{
	unsigned char r[16];
	register int i, sect;

	putsym(S_SEG | S_EXTERN, TEXTSEG, 0L, "_TEXT", 5);
	putsym(S_SEG | S_EXTERN, DATASEG, 0L, "_DATA", 5);
	for (; n >= 16; n -= 16) {
		if (read(in, r, 16) != 16)
			die("symbol table too short", "");
		if (r[12] == 0 && spending) {		/* more of the name */
			for (i = 0; i < 8 && r[i]; i++)
				if (slen < SYMLEN)
					sname[slen++] = r[i];
			continue;
		}
		flushsym();
		sect = r[12] & N_SECT;
		switch (sect) {
		case 1:	stype = S_ABS;	sseg = 0;	break;
		case 2:	stype = S_TEXT;	sseg = TEXTSEG;	break;
		case 3:	stype = S_DATA;	sseg = DATASEG;	break;
		case 4:	stype = S_BSS;	sseg = DATASEG;	break;
		default: stype = 0;	sseg = 0;	break;
		}
		if ((r[12] & C_CLASS) == C_EXT)
			stype |= S_EXTERN;
		svalue = get4(r + 8);
		for (slen = 0; slen < 8 && r[slen]; slen++)
			sname[slen] = r[slen];
		spending = 1;
	}
	flushsym();
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
	unsigned char h[HDRLEN + SEGTABS];	/* the x.out header and segment table */
	long text, data, bss, stack = 4096, pdata, ptext, hlen, nsyms;
	int segtab;
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
	nsyms = get4(ah + 28);			/* bytes of symbol table */
	segtab = nsyms > 0 ? SEGTABS : SEGTAB;

	ptext = (text + 1) & ~1L;		/* text: even length */
	pdata = (data + 15) & ~15L;		/* data: whole paragraphs */
	bss = ((data + bss + 15) & ~15L) - pdata;	/* same end of memory */
	if (bss < 0)
		bss = 0;

	for (i = 0; i < HDRLEN + SEGTABS; i++)
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
	put4(h + 56, (long)segtab);	/* xe_segsize */
	h[70] = 1;			/* xe_ostype: XENIX */
	h[71] = 2;			/* xe_osvers: 2.x */
	put2(h + 72, 63);		/* xe_eseg: entry point is in segment 63, the text */
	seg(h + HDRLEN, 1, 0x8008, TEXTSEG, (long)(HDRLEN + segtab), ptext, ptext);
	seg(h + HDRLEN + 32, 2, 0x8000 | (bss ? 4 : 0), DATASEG, HDRLEN + segtab + ptext, pdata, pdata + bss);

	if (lseek(in, hlen, 0) != hlen || read(in, buf, 2) != 2 ||
	    (buf[0] & 0377) != 0xEB || (buf[1] & 0377) != 0x7E)
		die("%s: crt0.o must be linked first", argv[1]);
	if (lseek(in, hlen + text, 0) != hlen + text || read(in, buf, 4) != 4 ||
	    buf[0] || buf[1] || (buf[2] & 0377) != 0xFF || (buf[3] & 0377) != 0xFF)
		die("%s: no data header (crt0.o must be linked first)", argv[1]);

	if ((out = creat(argv[2], 0777)) < 0)
		die("cannot create %s", argv[2]);
	wr(h, HDRLEN + segtab);
	lseek(in, hlen, 0);
	copy(text, "text");
	zeros((int)(ptext - text));
	copy(data, "data");
	zeros((int)(pdata - data));
	if (nsyms > 0) {	/* the symbols follow the data, in both */
		symbols(nsyms);
		put4(h + 16, ssize);		/* x_syms */
		seg(h + HDRLEN + 64, 3, 1, 0, HDRLEN + segtab + ptext + pdata, ssize, 0L);
		lseek(out, 0L, 0);
		wr(h, HDRLEN + segtab);
	}
	close(out);
	exit(0);
}
