/* curses.h - xenix86-devsys: screen updating and cursor movement
 * (cc ... -lcurses), with the interface of System V's curses, kept on the
 * termcap data base. See curses(3). */
#ifndef _CURSES_H
#define _CURSES_H

#include <stdio.h>

typedef	unsigned short chtype;	/* a character and its attributes */
#ifndef bool
#define	bool	char
#endif

#define	TRUE	1
#define	FALSE	0
#define	ERR	(-1)
#define	OK	0

/* attributes, or-ed into a chtype */
#define	A_CHARTEXT	0x00FF
#define	A_ATTRIBUTES	0xFF00
#define	A_NORMAL	0x0000
#define	A_STANDOUT	0x0100
#define	A_UNDERLINE	0x0200
#define	A_REVERSE	0x0400
#define	A_BLINK		0x0800
#define	A_DIM		0x1000
#define	A_BOLD		0x2000

/* function keys, as wgetch returns them with keypad on */
#define	KEY_BREAK	0401
#define	KEY_DOWN	0402
#define	KEY_UP		0403
#define	KEY_LEFT	0404
#define	KEY_RIGHT	0405
#define	KEY_HOME	0406
#define	KEY_BACKSPACE	0407
#define	KEY_F0		0410
#define	KEY_F(n)	(KEY_F0 + (n))
#define	KEY_DL		0510
#define	KEY_IL		0511
#define	KEY_DC		0512
#define	KEY_IC		0513
#define	KEY_EIC		0514
#define	KEY_CLEAR	0515
#define	KEY_EOS		0516
#define	KEY_EOL		0517
#define	KEY_SF		0520
#define	KEY_SR		0521
#define	KEY_NPAGE	0522
#define	KEY_PPAGE	0523
#define	KEY_ENTER	0527
#define	KEY_LL		0533
#define	KEY_END		0550

struct	_win {
	short	_cury, _curx;		/* the cursor */
	short	_maxy, _maxx;		/* lines and columns */
	short	_begy, _begx;		/* where it is on the screen */
	short	_flags;
	chtype	_attrs;			/* attributes given to what is added */
	char	_clear;			/* clear the screen at the next refresh */
	char	_leave;			/* leave the cursor where it falls */
	char	_scroll;		/* scroll at the bottom */
	char	_use_keypad;		/* decode function keys */
	char	_nodelay;		/* getch does not wait */
	char	_idlok;
	short	_tmarg, _bmarg;		/* the scrolling region */
	chtype	**_y;			/* the lines */
	short	*_firstch, *_lastch;	/* what changed in each line */
	struct	_win *_parent;		/* the window a subwindow is part of */
};
typedef	struct _win WINDOW;

#define	_NOCHANGE	(-1)
#define	_SUBWIN		01

extern	WINDOW	*stdscr, *curscr;
extern	int	LINES, COLS;

extern	WINDOW	*initscr(), *newwin(), *subwin();
extern	char	*longname(), *unctrl();
extern	char	erasechar(), killchar();

/* the forms on stdscr, and with a move first */
#define	addch(c)		waddch(stdscr, c)
#define	addstr(s)		waddstr(stdscr, s)
#define	attroff(a)		wattroff(stdscr, a)
#define	attron(a)		wattron(stdscr, a)
#define	attrset(a)		wattrset(stdscr, a)
#define	clear()			wclear(stdscr)
#define	clrtobot()		wclrtobot(stdscr)
#define	clrtoeol()		wclrtoeol(stdscr)
#define	delch()			wdelch(stdscr)
#define	deleteln()		wdeleteln(stdscr)
#define	erase()			werase(stdscr)
#define	getch()			wgetch(stdscr)
#define	getstr(s)		wgetstr(stdscr, s)
#define	inch()			winch(stdscr)
#define	insch(c)		winsch(stdscr, c)
#define	insertln()		winsertln(stdscr)
#define	move(y, x)		wmove(stdscr, y, x)
#define	refresh()		wrefresh(stdscr)
#define	scrl(n)			wscrl(stdscr, n)
#define	setscrreg(t, b)		wsetscrreg(stdscr, t, b)
#define	standend()		wstandend(stdscr)
#define	standout()		wstandout(stdscr)

#define	mvwaddch(w, y, x, c)	(wmove(w, y, x) == ERR ? ERR : waddch(w, c))
#define	mvwaddstr(w, y, x, s)	(wmove(w, y, x) == ERR ? ERR : waddstr(w, s))
#define	mvwdelch(w, y, x)	(wmove(w, y, x) == ERR ? ERR : wdelch(w))
#define	mvwgetch(w, y, x)	(wmove(w, y, x) == ERR ? ERR : wgetch(w))
#define	mvwgetstr(w, y, x, s)	(wmove(w, y, x) == ERR ? ERR : wgetstr(w, s))
#define	mvwinch(w, y, x)	(wmove(w, y, x) == ERR ? ERR : winch(w))
#define	mvwinsch(w, y, x, c)	(wmove(w, y, x) == ERR ? ERR : winsch(w, c))
#define	mvaddch(y, x, c)	mvwaddch(stdscr, y, x, c)
#define	mvaddstr(y, x, s)	mvwaddstr(stdscr, y, x, s)
#define	mvdelch(y, x)		mvwdelch(stdscr, y, x)
#define	mvgetch(y, x)		mvwgetch(stdscr, y, x)
#define	mvgetstr(y, x, s)	mvwgetstr(stdscr, y, x, s)
#define	mvinch(y, x)		mvwinch(stdscr, y, x)
#define	mvinsch(y, x, c)	mvwinsch(stdscr, y, x, c)

#define	winch(w)		((w)->_y[(w)->_cury][(w)->_curx])
#define	getyx(w, y, x)		((y) = (w)->_cury, (x) = (w)->_curx)
#define	getbegyx(w, y, x)	((y) = (w)->_begy, (x) = (w)->_begx)
#define	getmaxyx(w, y, x)	((y) = (w)->_maxy, (x) = (w)->_maxx)
#define	wstandout(w)		wattrset(w, A_STANDOUT)
#define	wstandend(w)		wattrset(w, A_NORMAL)
#define	scroll(w)		wscrl(w, 1)
#define	clearok(w, b)		((w)->_clear = (b))
#define	leaveok(w, b)		((w)->_leave = (b))
#define	scrollok(w, b)		((w)->_scroll = (b))
#define	idlok(w, b)		((w)->_idlok = (b))
#define	nodelay(w, b)		((w)->_nodelay = (b))
#define	intrflush(w, b)		OK
#define	meta(w, b)		OK
#define	typeahead(fd)		OK

#endif
