/* termcap.c - xenix86-devsys libtermcap (cc ... -ltermcap): the terminal
 * capability routines of Version 7 and System V Unix, written for this
 * project in K&R C.
 *
 * A terminal's entry in /etc/termcap is a line (continued with \ at the end
 * of a line) of fields separated by colons. The first field gives its
 * names, separated by |; the others are capabilities named by two
 * characters: a flag (am), a number (co#80) or a string (cl=\E[2J\E[H).
 * xx@ cancels a capability, and tc=name appends the entry of another
 * terminal. If the environment's TERMCAP is a path, that file is read
 * instead; if it holds an entry for the terminal asked for, that is used.
 *
 *	tgetent(bp, name)	find name's entry, into bp (1024 bytes):
 *				1 if found, 0 if not, -1 if no termcap file
 *	tgetnum(id)		a number capability, or -1
 *	tgetflag(id)		1 if the flag is there, else 0
 *	tgetstr(id, &area)	a string capability, decoded into *area
 *				(which is moved on past it), or NULL
 *	tgoto(cm, col, line)	a cursor motion string with the position put
 *				in (the first number is the line, unless %r)
 *	tputs(cp, affcnt, outc)	put out a string with outc(), adding the
 *				padding its delay asks for at speed ospeed */
#include <stdio.h>

#define	TBUFSIZE	1024	/* an entry, with what its tc= adds */
#define	MAXTC		16	/* how deep tc= chains may go */

char	PC;			/* the padding character */
char	*BC;			/* cursor left, if not ^H (for tgoto) */
char	*UP;			/* cursor up (for tgoto) */
short	ospeed;			/* the output speed, as termio's CBAUD gives it */

static char *tbuf;		/* the entry tgetent() found */
static char *tfile;		/* the termcap file to read */

extern char *getenv(), *strchr();

/* is name one of the names in the first field of entry e? */
static
named(e, name)
register char *e;
char *name;
{
	register char *n;

	for (;;) {
		for (n = name; *n && *e == *n; e++, n++)
			;
		if (*n == 0 && (*e == '|' || *e == ':' || *e == 0))
			return 1;
		while (*e && *e != '|' && *e != ':')
			e++;
		if (*e != '|')
			return 0;
		e++;
	}
}

/* Reading /etc/termcap. A termcap file is long (XENIX's has hundreds of
 * entries) and an 8088 is slow, so entries are skipped a line at a time:
 * fgets copies a line with the 8086's string instructions, and only the
 * first line of an entry, which holds its names, is looked at. An entry
 * that may be the one wanted is read a character at a time, joining its
 * lines, from the first line on. */
#define	LSIZE	256
static char line[LSIZE];	/* the line read last */
static char *lp;		/* the next character of it for nextc, or 0 */

static
nextc(f)
FILE *f;
{
	if (lp != 0) {
		if (*lp)
			return *lp++ & 0377;
		lp = 0;
	}
	return getc(f);
}

/* to the end of the physical line in line[]: the next line, or EOF */
static
endline(f)
FILE *f;
{
	int n;

	while ((n = strlen(line)) > 0 && line[n - 1] != '\n')
		if (fgets(line, LSIZE, f) == NULL)
			return EOF;
	return 0;
}

/* past the rest of an entry whose first line is in line[]: through the
 * first line that does not end in an odd number of backslashes (an odd
 * number leaves one to join the next line) */
static
skipent(f)
FILE *f;
{
	register int n, k;
	int nl, run = 0;

	for (;;) {
		n = strlen(line);
		nl = n > 0 && line[n - 1] == '\n';
		n -= nl;
		for (k = 0; k < n && line[n - 1 - k] == '\\'; k++)
			;
		run = k == n ? run + k : k;	/* a line of backslashes goes on */
		if (nl) {
			if ((run & 1) == 0)
				return;
			run = 0;
		}
		if (fgets(line, LSIZE, f) == NULL)
			return;
	}
}

/* read the entry named name from the termcap file into bp (whose end is
 * end): the joined lines of the entry; 1, 0 if there is none, -1 if the
 * file cannot be read */
static
readent(bp, end, name)
char *bp, *end, *name;
{
	register FILE *f;
	register char *p;
	register int c;

	if ((f = fopen(tfile, "r")) == NULL)
		return -1;
	for (;;) {
		/* the first line of the next entry, past comments and blank
		 * lines */
		lp = 0;
		if (fgets(line, LSIZE, f) == NULL) {
			fclose(f);
			return 0;
		}
		if (line[0] == '#' || line[0] == '\n') {
			if (endline(f) == EOF) {
				fclose(f);
				return 0;
			}
			continue;
		}
		/* its names all in this line, and not ours: skip it */
		if (strchr(line, ':') != 0 && !named(line, name)) {
			skipent(f);
			continue;
		}
		/* perhaps ours: the whole entry, its lines joined */
		lp = line;
		p = bp;
		while ((c = nextc(f)) != EOF) {
			if (c == '\\') {
				if ((c = nextc(f)) == '\n')
					continue;	/* the entry goes on */
				if (p < end)
					*p++ = '\\';
				if (c == EOF)
					break;
			}
			if (c == '\n')
				break;
			if (p < end)
				*p++ = c;
		}
		*p = 0;
		lp = 0;
		if (named(bp, name)) {
			fclose(f);
			return 1;
		}
		if (c == EOF) {
			fclose(f);
			return 0;
		}
	}
}

/* find a capability's field: a pointer to what follows its name, or 0 */
static char *
field(id)
char *id;
{
	register char *p;

	if ((p = tbuf) == 0)
		return 0;
	for (;;) {
		while (*p && *p != ':')
			p++;
		if (*p++ == 0)
			return 0;
		if (p[0] == id[0] && p[0] && p[1] == id[1] && p[1])
			return p + 2;
	}
}

tgetent(bp, name)
char *bp, *name;
{
	char *t, *p, *q, *tc, *end;
	char tcname[32];
	int r, n;

	tbuf = bp;
	bp[0] = 0;
	end = bp + TBUFSIZE - 1;
	tfile = "/etc/termcap";
	if ((t = getenv("TERMCAP")) != 0 && *t) {
		if (*t == '/')
			tfile = t;
		else if (named(t, name)) {
			strncpy(bp, t, TBUFSIZE - 1);
			bp[TBUFSIZE - 1] = 0;
			goto tc;
		}
	}
	if ((r = readent(bp, end, name)) <= 0)
		return r;
tc:
	/* tc=name: put that entry's capabilities in place of the field */
	for (n = 0; n < MAXTC; n++) {
		if ((tc = field("tc")) == 0 || *tc != '=')
			break;
		for (p = tc + 1, q = tcname; *p && *p != ':' && q < tcname + 31; )
			*q++ = *p++;
		*q = 0;
		tc -= 2;			/* at the "tc" */
		q = tc + strlen(tc) + 1;	/* the other entry goes after the 0 */
		if (q >= end || (r = readent(q, end, tcname)) < 0)
			break;
		if (r == 0) {
			*tc = 0;		/* no such entry: drop the field */
			break;
		}
		while (*q && *q != ':')		/* its names */
			q++;
		while (*p && *p != ':')		/* our rest, then its fields */
			p++;
		strcpy(tc, *p ? p + 1 : p);
		p = tc + strlen(tc);
		if (p > bp && p[-1] == ':' && *q == ':')
			q++;
		strcpy(p, q);
	}
	return 1;
}

tgetnum(id)
char *id;
{
	register char *p;
	register int n;

	if ((p = field(id)) == 0 || *p != '#')
		return -1;
	for (n = 0, p++; *p >= '0' && *p <= '9'; p++)
		n = 10 * n + *p - '0';
	return n;
}

tgetflag(id)
char *id;
{
	register char *p;

	return (p = field(id)) != 0 && (*p == ':' || *p == 0);
}

char *
tgetstr(id, area)
char *id;
char **area;
{
	register char *p, *q;
	register int c, i;
	char *s;

	if ((p = field(id)) == 0 || *p++ != '=')
		return 0;
	s = q = *area;
	while ((c = *p++) != 0 && c != ':') {
		if (c == '^' && *p) {
			c = *p++;
			c = c == '?' ? 0177 : c & 037;
		} else if (c == '\\' && *p) {
			switch (c = *p++) {
			case 'E': case 'e': c = 033; break;
			case 'n': c = '\n'; break;
			case 'r': c = '\r'; break;
			case 't': c = '\t'; break;
			case 'b': c = '\b'; break;
			case 'f': c = '\f'; break;
			default:
				if (c >= '0' && c <= '7') {
					for (c -= '0', i = 1; i < 3 && *p >= '0' && *p <= '7'; i++)
						c = 8 * c + *p++ - '0';
					if (c == 0)
						c = 0200;	/* a 0 would end the string */
				}
			}
		}
		*q++ = c;
	}
	*q++ = 0;
	*area = q;
	return s;
}

/* cursor motion: put line and col into cm, the way its % codes say. The
 * first number put out is the line, the next the column (%r swaps them) */
char *
tgoto(cm, col, line)
char *cm;
int col, line;
{
	static char out[64];
	char added[16];			/* UP or BC to put after, if a value was moved */
	register char *p, *q;
	register int c, which;
	int oncol;			/* which is the column */

	if (cm == 0)
		return "OOPS";
	added[0] = 0;
	oncol = 0;
	which = line;
	for (p = cm, q = out; (c = *p++) != 0; ) {
		if (q >= out + sizeof out - 8)
			return "OOPS";
		if (c != '%') {
			*q++ = c;
			continue;
		}
		switch (c = *p++) {
		case 'd':
			sprintf(q, "%d", which);
			q += strlen(q);
			break;
		case '2':
			sprintf(q, "%02d", which % 100);
			q += 2;
			break;
		case '3':
			sprintf(q, "%03d", which % 1000);
			q += 3;
			break;
		case '+':
			which += *p++;
			/* fall through */
		case '.':
			/* a 0, ^D or newline could be lost on the way to the
			 * terminal: go one further, and come back after with
			 * UP (or BC, for a column), if there is one */
			which &= 0377;
			if ((which == 0 || which == 4 || which == '\n') && (oncol || UP))
				do {
					strcat(added, oncol ? (BC ? BC : "\b") : UP);
					which++;
				} while (which == '\n');
			*q++ = which;
			break;
		case '>':
			if (which > *p++)
				which += *p;
			if (*p)
				p++;
			continue;
		case 'r':
			oncol = 1;
			which = col;
			continue;
		case 'i':
			line++;
			col++;
			which++;
			continue;
		case 'n':
			line ^= 0140;
			col ^= 0140;
			which ^= 0140;
			continue;
		case 'B':
			which = which / 10 * 16 + which % 10;
			continue;
		case 'D':
			which -= 2 * (which % 16);
			continue;
		case '%':
			*q++ = '%';
			continue;
		default:
			return "OOPS";
		}
		oncol = !oncol;		/* a number went out: on to the other */
		which = oncol ? col : line;
	}
	*q = 0;
	strcat(q, added);
	return out;
}

/* tenths of a millisecond a character takes at each speed (B0 to B9600,
 * and EXTA as 19200) */
static short tmspc10[] = { 0, 2000, 1333, 909, 743, 666, 500, 333, 166, 83, 55, 41, 20, 10, 5 };

tputs(cp, affcnt, outc)
register char *cp;
int affcnt;
int (*outc)();
{
	register int delay;
	int mspc10;

	if (cp == 0)
		return;
	for (delay = 0; *cp >= '0' && *cp <= '9'; cp++)	/* milliseconds */
		delay = 10 * delay + *cp - '0';
	delay *= 10;					/* in tenths */
	if (*cp == '.') {
		cp++;
		if (*cp >= '0' && *cp <= '9')
			delay += *cp - '0';
		while (*cp >= '0' && *cp <= '9')
			cp++;
	}
	if (*cp == '*') {
		cp++;
		delay *= affcnt;
	}
	while (*cp)
		(*outc)(*cp++);
	if (delay == 0 || ospeed <= 0 || ospeed >= sizeof tmspc10 / sizeof tmspc10[0])
		return;
	mspc10 = tmspc10[ospeed];
	for (delay = (delay + mspc10 / 2) / mspc10; delay > 0; delay--)
		(*outc)(PC);
}
