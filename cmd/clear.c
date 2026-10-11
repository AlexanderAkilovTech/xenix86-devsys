/* clear - clear the terminal's screen
 *
 * usage: clear
 *
 * The code that clears the screen is the terminal's "cl" capability in
 * /etc/termcap (or the file or entry the environment's TERMCAP gives), for
 * the terminal TERM names. xenix86-devsys, K&R C. */
#include <stdio.h>

extern char *getenv(), *tgetstr();

putch(c)
int c;
{
	putchar(c);
}

main()
{
	char buf[1024], area[256];
	char *ap = area, *term, *cl;
	int lines;

	if ((term = getenv("TERM")) == 0 || *term == 0) {
		fprintf(stderr, "clear: TERM is not set\n");
		exit(1);
	}
	switch (tgetent(buf, term)) {
	case -1:
		fprintf(stderr, "clear: cannot read /etc/termcap\n");
		exit(1);
	case 0:
		fprintf(stderr, "clear: unknown terminal type %s\n", term);
		exit(1);
	}
	if ((cl = tgetstr("cl", &ap)) == 0) {
		fprintf(stderr, "clear: a %s cannot clear its screen\n", term);
		exit(1);
	}
	if ((lines = tgetnum("li")) < 1)
		lines = 1;
	tputs(cl, lines, putch);
	exit(0);
}
