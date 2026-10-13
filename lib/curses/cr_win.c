/* cr_win.c - curses: windows: making, moving, clearing, scrolling.
 * xenix86-devsys, K&R C. */
#include "curses.h"

extern char *malloc();

/* a window of lines x cols; its lines are new memory unless it is a
 * subwindow, whose lines are parts of its parent's */
WINDOW *
_makewin(lines, cols, begy, begx, parent)
int lines, cols, begy, begx;
WINDOW *parent;
{
	register WINDOW *w;
	register int i;
	chtype *p;

	if (lines <= 0 || cols <= 0)
		return 0;
	if ((w = (WINDOW *)malloc(sizeof(WINDOW))) == 0)
		return 0;
	w->_y = (chtype **)malloc(lines * sizeof(chtype *));
	w->_firstch = (short *)malloc(lines * sizeof(short));
	w->_lastch = (short *)malloc(lines * sizeof(short));
	p = 0;
	if (!parent && (p = (chtype *)malloc((unsigned)lines * cols * sizeof(chtype))) == 0
	    || !w->_y || !w->_firstch || !w->_lastch) {
		if (w->_y) free((char *)w->_y);
		if (w->_firstch) free((char *)w->_firstch);
		if (w->_lastch) free((char *)w->_lastch);
		free((char *)w);
		return 0;
	}
	w->_cury = w->_curx = 0;
	w->_maxy = lines;
	w->_maxx = cols;
	w->_begy = begy;
	w->_begx = begx;
	w->_flags = parent ? _SUBWIN : 0;
	w->_attrs = 0;
	w->_clear = w->_leave = w->_scroll = w->_use_keypad = w->_nodelay = 0;
	w->_idlok = 0;
	w->_tmarg = 0;
	w->_bmarg = lines - 1;
	w->_parent = parent;
	for (i = 0; i < lines; i++) {
		if (parent)
			w->_y[i] = parent->_y[begy - parent->_begy + i] + (begx - parent->_begx);
		else {
			w->_y[i] = p;
			p += cols;
		}
		w->_firstch[i] = 0;
		w->_lastch[i] = cols - 1;
	}
	if (!parent)
		werase(w);
	return w;
}

WINDOW *
newwin(lines, cols, begy, begx)
int lines, cols, begy, begx;
{
	if (lines == 0)
		lines = LINES - begy;
	if (cols == 0)
		cols = COLS - begx;
	if (begy < 0 || begx < 0 || begy + lines > LINES || begx + cols > COLS)
		return 0;
	return _makewin(lines, cols, begy, begx, (WINDOW *)0);
}

/* a window within orig, at begy, begx on the screen, sharing its text */
WINDOW *
subwin(orig, lines, cols, begy, begx)
WINDOW *orig;
int lines, cols, begy, begx;
{
	if (lines == 0)
		lines = orig->_begy + orig->_maxy - begy;
	if (cols == 0)
		cols = orig->_begx + orig->_maxx - begx;
	if (begy < orig->_begy || begx < orig->_begx ||
	    begy + lines > orig->_begy + orig->_maxy ||
	    begx + cols > orig->_begx + orig->_maxx)
		return 0;
	return _makewin(lines, cols, begy, begx, orig);
}

delwin(w)
register WINDOW *w;
{
	if (!(w->_flags & _SUBWIN))
		free((char *)w->_y[0]);
	free((char *)w->_y);
	free((char *)w->_firstch);
	free((char *)w->_lastch);
	free((char *)w);
	return OK;
}

/* move a window (not a subwindow) to another place on the screen */
mvwin(w, y, x)
register WINDOW *w;
int y, x;
{
	if (y < 0 || x < 0 || y + w->_maxy > LINES || x + w->_maxx > COLS ||
	    (w->_flags & _SUBWIN))
		return ERR;
	w->_begy = y;
	w->_begx = x;
	touchwin(w);
	return OK;
}

wmove(w, y, x)
register WINDOW *w;
int y, x;
{
	if (y < 0 || x < 0 || y >= w->_maxy || x >= w->_maxx)
		return ERR;
	w->_cury = y;
	w->_curx = x;
	return OK;
}

/* mark part of a line as changed */
_touch(w, y, first, last)
register WINDOW *w;
int y, first, last;
{
	if (w->_firstch[y] == _NOCHANGE || first < w->_firstch[y])
		w->_firstch[y] = first;
	if (w->_lastch[y] == _NOCHANGE || last > w->_lastch[y])
		w->_lastch[y] = last;
}

touchwin(w)
register WINDOW *w;
{
	register int y;

	for (y = 0; y < w->_maxy; y++) {
		w->_firstch[y] = 0;
		w->_lastch[y] = w->_maxx - 1;
	}
	return OK;
}

touchline(w, y, n)
register WINDOW *w;
int y, n;
{
	for (; n > 0 && y < w->_maxy; n--, y++) {
		w->_firstch[y] = 0;
		w->_lastch[y] = w->_maxx - 1;
	}
	return OK;
}

/* blank from x to the end of line y */
_blankto(w, y, x)
register WINDOW *w;
int y, x;
{
	register chtype *p, *end;
	register int changed = 0;

	end = w->_y[y] + w->_maxx;
	for (p = w->_y[y] + x; p < end; p++)
		if (*p != ' ') {
			*p = ' ';
			changed = 1;
		}
	if (changed)
		_touch(w, y, x, w->_maxx - 1);
}

werase(w)
register WINDOW *w;
{
	register int y;

	for (y = 0; y < w->_maxy; y++)
		_blankto(w, y, 0);
	w->_cury = w->_curx = 0;
	return OK;
}

wclear(w)
WINDOW *w;
{
	werase(w);
	w->_clear = TRUE;
	return OK;
}

wclrtoeol(w)
register WINDOW *w;
{
	_blankto(w, w->_cury, w->_curx);
	return OK;
}

wclrtobot(w)
register WINDOW *w;
{
	register int y;

	_blankto(w, w->_cury, w->_curx);
	for (y = w->_cury + 1; y < w->_maxy; y++)
		_blankto(w, y, 0);
	return OK;
}

wsetscrreg(w, top, bot)
register WINDOW *w;
int top, bot;
{
	if (top < 0 || bot >= w->_maxy || top > bot)
		return ERR;
	w->_tmarg = top;
	w->_bmarg = bot;
	return OK;
}

/* scroll the scrolling region n lines up (down if n is negative) */
wscrl(w, n)
register WINDOW *w;
int n;
{
	register int y, x;
	register chtype *p, *q;

	if (!w->_scroll)
		return ERR;
	while (n > 0) {
		for (y = w->_tmarg; y < w->_bmarg; y++) {
			p = w->_y[y];
			q = w->_y[y + 1];
			for (x = 0; x < w->_maxx; x++)
				p[x] = q[x];
		}
		for (x = 0, p = w->_y[w->_bmarg]; x < w->_maxx; x++)
			p[x] = ' ';
		n--;
	}
	while (n < 0) {
		for (y = w->_bmarg; y > w->_tmarg; y--) {
			p = w->_y[y];
			q = w->_y[y - 1];
			for (x = 0; x < w->_maxx; x++)
				p[x] = q[x];
		}
		for (x = 0, p = w->_y[w->_tmarg]; x < w->_maxx; x++)
			p[x] = ' ';
		n++;
	}
	for (y = w->_tmarg; y <= w->_bmarg; y++)
		_touch(w, y, 0, w->_maxx - 1);
	return OK;
}
