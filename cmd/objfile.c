/* objfile.c - reading XENIX x.out executables, as86 object files and ar
 * archives of them, for nm, size and strip. xenix86-devsys, K&R C.
 *
 * An x.out executable (XENIX 2.1.3, 8086, segmented) has a 32-byte header,
 * an extension, and a segment table of 32-byte entries; the symbol table is
 * the segment of type 3. Its records are a type, a segment number and a
 * value (2 + 2 + 4 bytes), each followed by the name and a 0 (SCO's
 * <sys/a.out.h> and <sys/relsym.h> describe the format).
 *
 * An as86 object file (what as86 and cc -c make) starts with A3 86, a module
 * count and a checksum byte. Each module header gives sizes for up to 16
 * segments (as86 puts text in segment 0 and data in 3), then its symbols:
 * a string table offset, flags and a value whose size the flags give; then
 * the string table, which starts with the module's name.
 *
 * An archive is "!<arch>\n" and members, each with a 60-byte header. */
#include <stdio.h>
#include "objfile.h"

extern char *prog;		/* the program's name, for messages */
extern char *malloc();
extern long lseek(), atol();

int	objfd = -1;
char	*objname;
char	objmem[17];
long	o_text, o_data, o_bss;
int	o_nsyms;
struct osym *o_syms;

static int	kind;		/* F_XOUT, F_OBJ or F_ARCH */
static long	apos;		/* archive: where the next member header is */
static long	mpos;		/* where the object (member) being read starts */
static char	*bufs[17];	/* the tables read for the current object */
static int	nbufs;

/* object symbol flags (Dev86's ld/obj.h) */
#define	A_MASK	0x0010		/* absolute */
#define	I_MASK	0x0040		/* imported */
#define	E_MASK	0x0080		/* exported */
#define	SA_MASK	0x2000		/* storage allocation (local bss) */
#define	SZ_MASK	0xC000		/* how many bytes the value has */

static int csize[4] = { 0, 1, 2, 4 };	/* the sizes the 2-bit codes stand for */

unsigned
get2(p)
register unsigned char *p;
{
	return p[0] | p[1] << 8;
}

long
get4(p)
register unsigned char *p;
{
	return (long)get2(p) | (long)get2(p + 2) << 16;
}

static
complain(what)
char *what;
{
	if (kind == F_ARCH && objmem[0])
		fprintf(stderr, "%s: %s(%s): %s\n", prog, objname, objmem, what);
	else
		fprintf(stderr, "%s: %s: %s\n", prog, objname, what);
	return -1;
}

/* read n bytes at pos; 0 if they are all there */
static
readat(pos, buf, n)
long pos;
char *buf;
unsigned n;
{
	return lseek(objfd, pos, 0) != pos || read(objfd, buf, n) != n;
}

/* a buffer that lasts until the next object is read */
static char *
getbuf(n)
unsigned n;
{
	char *p;

	if (nbufs >= 16 || (p = malloc(n ? n : 1)) == 0)
		return 0;
	return bufs[nbufs++] = p;
}

static
freebufs()
{
	while (nbufs > 0)
		free(bufs[--nbufs]);
	o_syms = 0;
	o_nsyms = 0;
}

/* open a file and see what it is: F_XOUT, F_OBJ or F_ARCH, or 0 (with a
 * message) if it is none of those */
objopen(name)
char *name;
{
	unsigned char m[8];

	objname = name;
	objmem[0] = 0;
	kind = 0;
	if ((objfd = open(name, 0)) < 0) {
		complain("cannot open");
		return 0;
	}
	if (read(objfd, m, 8) < 2)
		goto bad;
	mpos = 0;
	if (m[0] == 0x06 && m[1] == 0x02)
		return kind = F_XOUT;
	if (m[0] == 0xA3 && m[1] == 0x86)
		return kind = F_OBJ;
	if (strncmp(m, "!<arch>\n", 8) == 0) {
		apos = 8;
		return kind = F_ARCH;
	}
bad:
	complain("not an executable, object file or archive");
	objclose();
	return 0;
}

objclose()
{
	freebufs();
	if (objfd >= 0)
		close(objfd);
	objfd = -1;
}

/* an archive's next member that is an object file; 0 at the end */
objnext()
{
	char h[60];
	unsigned char m[2];
	long size;
	register int i;

	for (;;) {
		if (readat(apos, h, 60) || h[58] != '`' || h[59] != '\n')
			return 0;
		h[58] = 0;
		size = atol(h + 48);
		for (i = 0; i < 16; i++)
			objmem[i] = h[i];
		for (i = 16; i > 0 && (objmem[i - 1] == ' ' || objmem[i - 1] == '/'); i--)
			;
		objmem[i] = 0;
		mpos = apos + 60;
		apos = mpos + size + (size & 1);
		/* skip what is not an object file: a symbol table, a long
		 * name table */
		if (objmem[0] && !readat(mpos, m, 2) && m[0] == 0xA3 && m[1] == 0x86)
			return 1;
	}
}

/* read the executable or object file (member): sizes, and with syms, the
 * symbols; 0, or -1 with a message */
objread(syms)
int syms;
{
	freebufs();
	o_text = o_data = o_bss = 0;
	if (kind == F_XOUT)
		return xread(syms);
	return modread(syms);
}

/* nm's letter for an x.out symbol type */
static
xtype(t)
register unsigned t;
{
	register int c;

	switch (t & 0x1F) {
	case 0:	c = 'u'; break;
	case 1:	c = 'a'; break;
	case 2:	c = 't'; break;
	case 3:	c = 'd'; break;
	case 4:	c = 'b'; break;
	case 5:	c = 'c'; break;
	case 8:	return 0;		/* a segment's name: not shown */
	default: c = '?'; break;
	}
	return t & 0x20 ? c - 'a' + 'A' : c;	/* external: upper case */
}

static
xread(syms)
int syms;
{
	unsigned char h[76], s[32];
	unsigned char *tab, *p, *end;
	long segpos, spos, ssize;
	int nseg, i, t;
	register struct osym *o;

	if (readat(0L, h, 76))
		return complain("short header");
	if (!(get2(h + 30) & 0x0800))
		return complain("not a segmented x.out");
	o_text = get4(h + 4);
	o_data = get4(h + 8);
	o_bss = get4(h + 12);
	if (!syms)
		return 0;
	segpos = get4(h + 52);
	nseg = (int)(get4(h + 56) / 32);
	ssize = 0;
	for (i = 0; i < nseg; i++) {
		if (readat(segpos + 32L * i, s, 32))
			return complain("short segment table");
		if (get2(s) == 3) {		/* the symbol table */
			spos = get4(s + 8);
			ssize = get4(s + 12);
		}
	}
	if (ssize == 0)
		return 0;
	if (ssize > 60000L || (tab = (unsigned char *)getbuf((unsigned)ssize)) == 0)
		return complain("symbol table too large");
	if (readat(spos, tab, (unsigned)ssize))
		return complain("short symbol table");
	end = tab + (unsigned)ssize;
	for (i = 0, p = tab; p + 8 < end; i++)	/* count them */
		for (p += 8; p < end && *p++; )
			;
	if ((o_syms = (struct osym *)getbuf(i * sizeof(struct osym))) == 0)
		return complain("out of memory");
	o = o_syms;
	for (p = tab; p + 8 < end; ) {
		t = xtype(get2(p));
		o->value = get4(p + 4);
		o->name = (char *)(p += 8);
		while (p < end && *p++)
			;
		if (p[-1] != 0)
			return complain("bad symbol table");
		if (t) {
			o->type = t;
			o++;
		}
	}
	o_nsyms = o - o_syms;
	return 0;
}

/* read n (0, 1, 2 or 4) bytes as a number */
static long
getn(buf, pos, n)
unsigned char *buf;
long *pos;
int n;
{
	long v;

	v = n == 0 ? 0 : n == 1 ? buf[0] : n == 2 ? (long)get2(buf) : get4(buf);
	*pos += n;
	return v;
}

static
modread(syms)
int syms;
{
	unsigned char h[5], m[24], b[8];
	long pos, segsize[16], v;
	unsigned strsize, nsym, flags, off;
	int nmod, seg, k, i, n;
	char *strtab;
	register struct osym *o;

	if (readat(mpos, h, 5) || ((h[0] + h[1] + h[2] + h[3]) & 0xFF) != h[4])
		return complain("bad object file header");
	nmod = get2(h + 2);
	pos = mpos + 5;
	o = 0;
	while (nmod-- > 0) {
		/* the module header: text offset and size, string table
		 * size, class and revision, segment maximum sizes, and the
		 * codes for the sizes of the 16 segment sizes that follow */
		if (readat(pos, m, 20))
			return complain("short module header");
		pos += 20;
		strsize = get2(m + 8);
		for (seg = 0; seg < 16; seg++) {
			n = csize[3 & (m[16 + (15 - seg) / 4] >> (2 * (seg % 4)))];
			if (readat(pos, b, n))
				return complain("short module header");
			segsize[seg] = getn(b, &pos, n);
			if (seg >= 1 && seg <= 4)
				o_data += segsize[seg];
			else
				o_text += segsize[seg];
		}
		if (readat(pos, b, 2))
			return complain("short module header");
		nsym = get2(b);
		pos += 2;
		if (syms && nsym > 0) {		/* room for these and any before */
			if ((o = (struct osym *)getbuf((o_nsyms + nsym) * sizeof(struct osym))) == 0)
				return complain("out of memory");
			for (i = 0; i < o_nsyms; i++)
				o[i] = o_syms[i];
			o_syms = o;
			o += o_nsyms;
		}
		for (i = 0; i < nsym; i++) {
			if (readat(pos, b, 4))
				return complain("short symbol table");
			off = get2(b);
			flags = get2(b + 2);
			pos += 4;
			n = csize[(flags & SZ_MASK) >> 14];
			if (readat(pos, b, n))
				return complain("short symbol table");
			v = getn(b, &pos, n);
			if (flags & I_MASK)		/* imported: undefined, or common */
				k = v ? 'C' : 'U';
			else {
				if (flags & A_MASK)
					k = 'a';
				else if ((seg = flags & 0xF) >= 1 && seg <= 4)
					k = flags & SA_MASK ? 'b' : 'd';
				else
					k = 't';
				if (flags & E_MASK)
					k += 'A' - 'a';
			}
			if (k == 'C' || k == 'b')	/* storage the module asks for */
				o_bss += v;
			if (syms) {
				o->value = v;
				o->type = k;
				o->name = (char *)off;	/* an offset until the strings are in */
				o++;
				o_nsyms++;
			}
		}
		if (syms) {
			if ((strtab = getbuf(strsize + 1)) == 0)
				return complain("out of memory");
			if (readat(pos, strtab, strsize))
				return complain("short string table");
			strtab[strsize] = 0;
			for (i = o_nsyms - nsym; i < o_nsyms; i++) {
				off = (unsigned)o_syms[i].name;
				o_syms[i].name = strtab + (off < strsize ? off : strsize);
			}
		}
		pos += strsize;
	}
	return 0;
}
