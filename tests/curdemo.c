/* curdemo.c - try curses on a real terminal: attributes, a window with a
 * box, the function keys, and moving a marker with the arrow keys (or h j
 * k l). q quits. xenix86-devsys test program. */
#include <curses.h>

main()
{
	WINDOW *w;
	int c, y, x, n = 0;

	initscr();
	cbreak();
	noecho();
	keypad(stdscr, TRUE);

	attrset(A_STANDOUT);
	mvaddstr(0, 0, " curses on xenix86-devsys ");
	attrset(A_NORMAL);
	mvprintw(1, 0, "terminal: %s, %d lines, %d columns", longname(), LINES, COLS);
	mvaddstr(3, 2, "normal  ");
	attron(A_STANDOUT); addstr("standout"); attroff(A_STANDOUT); addstr("  ");
	attron(A_REVERSE); addstr("reverse"); attroff(A_REVERSE); addstr("  ");
	attron(A_BOLD); addstr("bold"); attroff(A_BOLD); addstr("  ");
	attron(A_UNDERLINE); addstr("underline"); attroff(A_UNDERLINE);
	mvaddstr(LINES - 1, 0, "arrows or h j k l move the @, q quits");
	refresh();

	w = newwin(7, 30, 6, COLS - 34);
	box(w, 0, 0);
	mvwaddstr(w, 0, 2, " a window ");
	mvwaddstr(w, 2, 2, "boxed, drawn on top");
	mvwaddstr(w, 4, 2, "keys pressed: 0");
	wrefresh(w);

	y = 10;
	x = 10;
	for (;;) {
		mvaddch(y, x, '@');
		move(y, x);
		refresh();
		c = getch();
		mvaddch(y, x, ' ');
		if (c == 'q' || c == ERR)
			break;
		switch (c) {
		case KEY_UP: case 'k':		if (y > 3) y--; break;
		case KEY_DOWN: case 'j':	if (y < LINES - 2) y++; break;
		case KEY_LEFT: case 'h':	if (x > 0) x--; break;
		case KEY_RIGHT: case 'l':	if (x < COLS - 36) x++; break;
		}
		n++;
		mvwprintw(w, 4, 2, "keys pressed: %d", n);
		mvwprintw(w, 5, 2, "last: %-12s", c >= 0400 ? "function key" : unctrl(c));
		wrefresh(w);
	}
	endwin();
	printf("curdemo: %d keys\n", n);
	return 0;
}
