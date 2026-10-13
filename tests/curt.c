/* curt.c - drive the curses library from a list of operations, for
 * tests/unit/test_curses.py, then dump what curses believes is on the
 * screen (curscr) and in stdscr, so that both can be checked: the terminal
 * output against curscr, and stdscr against a model of the operations.
 *
 *	curt opsfile > terminal-output 2> dump
 *
 * operations, one per line:
 *	m y x		move			s a text	addstr, attributes a
 *	c a ch		addch (decimal)		e		clrtoeol
 *	b		clrtobot		i ch		insch
 *	d		delch			I		insertln
 *	D		deleteln		E		erase
 *	S n		scroll n lines		r		refresh
 *	w y x h w	a boxed window, shown, then gone (stdscr touched)
 *	p y x n		mvprintw "<%d>"		k		keypad, then read keys
 */
#include <curses.h>

dump(w, name)
WINDOW *w;
char *name;
{
	int y, x;

	fprintf(stderr, "%s\n", name);
	for (y = 0; y < LINES; y++) {
		for (x = 0; x < COLS; x++)
			fprintf(stderr, "%04x", w->_y[y][x]);
		fprintf(stderr, "\n");
	}
}

main(argc, argv)
int argc;
char **argv;
{
	FILE *f;
	char line[300], *p;
	int a, b, c, d, n;
	WINDOW *w;

	if (argc < 2 || (f = fopen(argv[1], "r")) == NULL) {
		fprintf(stderr, "usage: curt opsfile\n");
		exit(2);
	}
	initscr();
	scrollok(stdscr, TRUE);
	while (fgets(line, sizeof line, f)) {
		for (p = line; *p && *p != '\n'; p++)
			;
		*p = 0;
		switch (line[0]) {
		case 'm':
			sscanf(line + 1, "%d %d", &a, &b);
			move(a, b);
			break;
		case 's':
			sscanf(line + 1, "%x", &a);
			for (p = line + 2; *p && *p != ' '; p++)
				;
			attrset(a);
			addstr(p + 1);
			attrset(0);
			break;
		case 'c':
			sscanf(line + 1, "%x %d", &a, &b);
			addch(b | a);
			break;
		case 'e': clrtoeol(); break;
		case 'b': clrtobot(); break;
		case 'i':
			sscanf(line + 1, "%d", &a);
			insch(a);
			break;
		case 'd': delch(); break;
		case 'I': insertln(); break;
		case 'D': deleteln(); break;
		case 'E': erase(); break;
		case 'S':
			sscanf(line + 1, "%d", &a);
			scrl(a);
			break;
		case 'r': refresh(); break;
		case 'w':
			sscanf(line + 1, "%d %d %d %d", &a, &b, &c, &d);
			if ((w = newwin(c, d, a, b)) != 0) {
				box(w, 0, 0);
				mvwaddstr(w, 1, 1, "window");
				wrefresh(w);
				delwin(w);
				touchwin(stdscr);
			}
			break;
		case 'p':
			sscanf(line + 1, "%d %d %d", &a, &b, &c);
			mvprintw(a, b, "<%d>", c);
			break;
		case 'k':
			keypad(stdscr, TRUE);
			noecho();
			cbreak();
			while ((n = getch()) != ERR) {
				fprintf(stderr, "key %d\n", n);
				if (n == 'q')
					break;
			}
			break;
		}
	}
	refresh();
	dump(curscr, "curscr");
	dump(stdscr, "stdscr");
	endwin();
	return 0;
}
