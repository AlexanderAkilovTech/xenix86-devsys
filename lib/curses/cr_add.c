/* cr_add.c - curses: putting characters into windows, attributes, inserting
 * and deleting, boxes. xenix86-devsys, K&R C. */
#include "curses.h"

/* the cursor has gone past the end of a line: on to the next, scrolling at
 * the bottom of the scrolling region if the window may scroll */
static
newline(w)
register WINDOW *w;
{
	w->_curx = 0;
	if (w->_cury == w->_bmarg) {
		if (!w->_scroll)
			return ERR;
		wscrl(w, 1);
	} else if (w->_cury < w->_maxy - 1)
		w->_cury++;
	else
		return ERR;
	return OK;
}

/* put one character, with its attributes, at the cursor */
static
putone(w, c)
register WINDOW *w;
chtype c;
{
	register int y = w->_cury, x = w->_curx;

	if (w->_y[y][x] != c) {
		w->_y[y][x] = c;
		_touch(w, y, x, x);
	}
	if (++w->_curx >= w->_maxx) {
		w->_curx = w->_maxx - 1;
		if (newline(w) == ERR) {
			w->_curx = w->_maxx - 1;
			return ERR;
		}
	}
	return OK;
}

waddch(w, ch)
register WINDOW *w;
int ch;
{
	register int c = ch & A_CHARTEXT;
	chtype a = (ch & A_ATTRIBUTES) | w->_attrs;
	register char *s;

	switch (c) {
	case '\n':
		wclrtoeol(w);
		return newline(w);
	case '\r':
		w->_curx = 0;
		return OK;
	case '\b':
		if (w->_curx > 0)
			w->_curx--;
		return OK;
	case '\t':
		do {
			if (putone(w, ' ' | a) == ERR)
				return ERR;
		} while (w->_curx % 8 != 0 && w->_curx != w->_maxx - 1);
		return OK;
	}
	if (c < ' ' || c == 0177) {
		for (s = unctrl(c); *s; s++)
			if (putone(w, (*s & 0377) | a) == ERR)
				return ERR;
		return OK;
	}
	return putone(w, c | a);
}

waddstr(w, s)
register WINDOW *w;
register char *s;
{
	while (*s)
		if (waddch(w, *s++ & 0377) == ERR)
			return ERR;
	return OK;
}

waddnstr(w, s, n)
register WINDOW *w;
register char *s;
register int n;
{
	while (*s && n-- != 0)
		if (waddch(w, *s++ & 0377) == ERR)
			return ERR;
	return OK;
}

wattron(w, a)
WINDOW *w;
int a;
{
	w->_attrs |= a & A_ATTRIBUTES;
	return OK;
}

wattroff(w, a)
WINDOW *w;
int a;
{
	w->_attrs &= ~(a & A_ATTRIBUTES);
	return OK;
}

wattrset(w, a)
WINDOW *w;
int a;
{
	w->_attrs = a & A_ATTRIBUTES;
	return OK;
}

/* insert a character at the cursor; the rest of the line moves right */
winsch(w, ch)
register WINDOW *w;
int ch;
{
	register chtype *line = w->_y[w->_cury];
	register int x;

	for (x = w->_maxx - 1; x > w->_curx; x--)
		line[x] = line[x - 1];
	line[w->_curx] = (ch & A_CHARTEXT) | (ch & A_ATTRIBUTES) | w->_attrs;
	_touch(w, w->_cury, w->_curx, w->_maxx - 1);
	return OK;
}

/* delete the character at the cursor; the rest of the line moves left */
wdelch(w)
register WINDOW *w;
{
	register chtype *line = w->_y[w->_cury];
	register int x;

	for (x = w->_curx; x < w->_maxx - 1; x++)
		line[x] = line[x + 1];
	line[w->_maxx - 1] = ' ';
	_touch(w, w->_cury, w->_curx, w->_maxx - 1);
	return OK;
}

/* insert a blank line above the cursor's; the lines below move down */
winsertln(w)
register WINDOW *w;
{
	register int y, x;
	register chtype *p, *q;

	for (y = w->_maxy - 1; y > w->_cury; y--) {
		p = w->_y[y];
		q = w->_y[y - 1];
		for (x = 0; x < w->_maxx; x++)
			p[x] = q[x];
		_touch(w, y, 0, w->_maxx - 1);
	}
	for (x = 0, p = w->_y[w->_cury]; x < w->_maxx; x++)
		p[x] = ' ';
	_touch(w, w->_cury, 0, w->_maxx - 1);
	return OK;
}

/* delete the cursor's line; the lines below move up */
wdeleteln(w)
register WINDOW *w;
{
	register int y, x;
	register chtype *p, *q;

	for (y = w->_cury; y < w->_maxy - 1; y++) {
		p = w->_y[y];
		q = w->_y[y + 1];
		for (x = 0; x < w->_maxx; x++)
			p[x] = q[x];
		_touch(w, y, 0, w->_maxx - 1);
	}
	for (x = 0, p = w->_y[w->_maxy - 1]; x < w->_maxx; x++)
		p[x] = ' ';
	_touch(w, w->_maxy - 1, 0, w->_maxx - 1);
	return OK;
}

/* a box around the edge of the window: vert for the sides, hor for the top
 * and bottom (| and - if they are 0), + at the corners */
box(w, vert, hor)
register WINDOW *w;
int vert, hor;
{
	register int x, y, r = w->_maxx - 1, b = w->_maxy - 1;

	if (vert == 0)
		vert = '|';
	if (hor == 0)
		hor = '-';
	for (x = 1; x < r; x++) {
		w->_y[0][x] = hor;
		w->_y[b][x] = hor;
	}
	for (y = 1; y < b; y++) {
		w->_y[y][0] = vert;
		w->_y[y][r] = vert;
	}
	w->_y[0][0] = w->_y[0][r] = w->_y[b][0] = w->_y[b][r] = '+';
	touchwin(w);
	return OK;
}

/* a printable form of a character: itself, or ^X for a control character */
char *
unctrl(c)
int c;
{
	static char buf[3];

	c &= 0377;
	if (c < ' ' || c == 0177) {
		buf[0] = '^';
		buf[1] = c == 0177 ? '?' : c + '@';
		buf[2] = 0;
	} else {
		buf[0] = c;
		buf[1] = 0;
	}
	return buf;
}
