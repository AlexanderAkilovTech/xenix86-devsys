/* cr_print.c - curses: printw and scanw, the forms of printf and scanf that
 * write to and read from windows. xenix86-devsys, K&R C. */
#include "curses.h"

/* the text printf would write, made from fmt and the arguments at ap */
static char *
format(fmt, ap)
char *fmt;
int *ap;
{
	static char buf[512];
	FILE f;

	f._flag = _IOWRT | _IOSTRG;
	f._ptr = f._base = buf;
	f._cnt = sizeof buf - 1;
	f._file = -1;
	_doprnt(fmt, ap, &f);
	*f._ptr = 0;
	return buf;
}

printw(fmt, args)
char *fmt;
int args;
{
	return waddstr(stdscr, format(fmt, &args));
}

wprintw(w, fmt, args)
WINDOW *w;
char *fmt;
int args;
{
	return waddstr(w, format(fmt, &args));
}

mvprintw(y, x, fmt, args)
int y, x;
char *fmt;
int args;
{
	if (wmove(stdscr, y, x) == ERR)
		return ERR;
	return waddstr(stdscr, format(fmt, &args));
}

mvwprintw(w, y, x, fmt, args)
WINDOW *w;
int y, x;
char *fmt;
int args;
{
	if (wmove(w, y, x) == ERR)
		return ERR;
	return waddstr(w, format(fmt, &args));
}

/* read a line from the window and convert it as scanf would */
static
scan(w, fmt, ap)
WINDOW *w;
char *fmt;
int *ap;
{
	char buf[256];
	FILE f;
	register char *p;

	if (wgetstr(w, buf) == ERR)
		return ERR;
	for (p = buf; *p; p++)
		;
	f._flag = _IOREAD | _IOSTRG;
	f._ptr = f._base = buf;
	f._cnt = f._bufsiz = p - buf;
	f._file = -1;
	return _doscan(&f, fmt, (char **)ap);
}

scanw(fmt, args)
char *fmt;
int args;
{
	return scan(stdscr, fmt, &args);
}

wscanw(w, fmt, args)
WINDOW *w;
char *fmt;
int args;
{
	return scan(w, fmt, &args);
}

mvscanw(y, x, fmt, args)
int y, x;
char *fmt;
int args;
{
	if (wmove(stdscr, y, x) == ERR)
		return ERR;
	return scan(stdscr, fmt, &args);
}

mvwscanw(w, y, x, fmt, args)
WINDOW *w;
int y, x;
char *fmt;
int args;
{
	if (wmove(w, y, x) == ERR)
		return ERR;
	return scan(w, fmt, &args);
}
