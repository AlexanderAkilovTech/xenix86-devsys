/* tctest.c - drive the termcap routines from the standard input, for
 * tests/unit/test_termcap.py. Each line is a command; each answer one line,
 * with control characters written as \ooo:
 *	e name		tgetent: the result
 *	n id		tgetnum
 *	f id		tgetflag
 *	s id		tgetstr
 *	g id col line	tgoto with the string capability id
 *	p id affcnt sp	tputs of capability id, at ospeed sp, padding with PC */
#include <stdio.h>

extern char *tgetstr(), *tgoto();
extern char PC, *UP, *BC;
extern short ospeed;

char	buf[1024], area[2048], *ap;
char	out[1024];
int	nout;

show(s)
register char *s;
{
	if (s == 0) {
		printf("(null)\n");
		return;
	}
	for (; *s; s++)
		if (*s < ' ' || *s >= 0177)
			printf("\\%03o", *s & 0377);
		else
			putchar(*s);
	putchar('\n');
}

collect(c)
int c;
{
	out[nout++] = c;
}

char *
cap(id)
char *id;
{
	char *a = area + 1024;

	return tgetstr(id, &a);
}

main()
{
	char line[200], cmd[10], id[60];
	int a, b, r;

	while (fgets(line, sizeof line, stdin)) {
		a = b = 0;
		id[0] = 0;
		sscanf(line, "%s %s %d %d", cmd, id, &a, &b);
		switch (cmd[0]) {
		case 'e':
			r = tgetent(buf, id);
			printf("%d\n", r);
			ap = area;
			UP = tgetstr("up", &ap);
			BC = tgetstr("bc", &ap);
			PC = 0;
			break;
		case 'n':
			printf("%d\n", tgetnum(id));
			break;
		case 'f':
			printf("%d\n", tgetflag(id));
			break;
		case 's':
			show(cap(id));
			break;
		case 'g':
			show(tgoto(cap(id), a, b));
			break;
		case 'p':
			nout = 0;
			ospeed = b;
			PC = '@';
			tputs(cap(id), a, collect);
			out[nout] = 0;
			show(out);
			break;
		}
	}
	exit(0);
}
