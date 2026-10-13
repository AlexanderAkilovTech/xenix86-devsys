/* cr_refresh.c - curses: bringing the terminal's screen up to date. Each
 * window's changes go into _newscr (wnoutrefresh); doupdate then compares
 * _newscr with curscr, what the screen holds, and sends only what differs,
 * moving the cursor the cheapest way it can. xenix86-devsys, K&R C. */
#include "curses.h"

extern char *tgoto();

static int
len(s)
register char *s;
{
	register int n = 0;

	if (s == 0)
		return 1000;
	while (*s++)
		n++;
	return n;
}

/* the cost of sending s n times; 1000 (too much) if there is no s. (In 16
 * bits, n times a large cost would overflow and look cheap.) */
static int
times(n, s)
int n;
char *s;
{
	register int l = len(s);

	if (s == 0 || n > 1000 / l)
		return 1000;
	return n * l;
}

/* set the terminal's attributes to a */
_setattr(a)
chtype a;
{
	register chtype on;

	a &= A_ATTRIBUTES;
	if (a == _curattr)
		return;
	if (_curattr & ~a) {		/* something to turn off: turn all off */
		if (_cap.me)
			_puts(_cap.me);
		else {
			if ((_curattr & A_UNDERLINE) && _cap.ue)
				_puts(_cap.ue);
			if ((_curattr & ~A_UNDERLINE) && _cap.se)
				_puts(_cap.se);
		}
		_curattr = 0;
	}
	on = a & ~_curattr;
	if ((on & A_STANDOUT) ||
	    (on & A_REVERSE) && !_cap.mr || (on & A_BOLD) && !_cap.md)
		_puts(_cap.so);
	if (on & A_UNDERLINE)
		_puts(_cap.us);
	if ((on & A_REVERSE) && _cap.mr)
		_puts(_cap.mr);
	if ((on & A_BOLD) && _cap.md)
		_puts(_cap.md);
	if (on & A_BLINK)
		_puts(_cap.mb);
	if (on & A_DIM)
		_puts(_cap.mh);
	_curattr = a;
}

/* the cost of moving right on line y from x to nx by writing the characters
 * already there, or 1000 if that cannot be done */
static int
rewrite(y, x, nx)
int y, x, nx;
{
	register chtype *p = curscr->_y[y];
	int n = nx - x;

	if (_curattr)
		return 1000;
	for (; x < nx; x++)
		if (p[x] & A_ATTRIBUTES)
			return 1000;
	return n;
}

/* the cheapest way to move right (dir 1) or left (dir -1) on line y,
 * from x to nx: its cost, and in *how 'w' rewrite, 'n' nd, 'l' le */
static int
horiz(y, x, nx, how)
int y, x, nx;
char *how;
{
	register int c, best;

	*how = 0;
	if (x == nx)
		return 0;
	if (nx > x) {
		best = rewrite(y, x, nx);
		*how = 'w';
		if ((c = times(nx - x, _cap.nd)) < best) {
			best = c;
			*how = 'n';
		}
	} else {
		best = times(x - nx, _cap.le);
		*how = 'l';
	}
	return best;
}

static
dohoriz(y, x, nx, how)
int y, x, nx, how;
{
	register chtype *p = curscr->_y[y];

	switch (how) {
	case 'w':
		for (; x < nx; x++)
			_putc(p[x] & A_CHARTEXT);
		break;
	case 'n':
		for (; x < nx; x++)
			_puts(_cap.nd);
		break;
	case 'l':
		for (; x > nx; x--)
			_puts(_cap.le);
		break;
	}
}

/* move the terminal's cursor to ny, nx */
_mvcur(ny, nx)
int ny, nx;
{
	char how, how2;
	int best, c, c2, x, i;
	char *cm;

	if (ny == _cury && nx == _curx)
		return;
	if (_curattr && !_cap.ms)
		_setattr(0);
	cm = tgoto(_cap.cm, nx, ny);
	best = len(cm);
	if (_cury >= 0 && _curx >= 0 && _curx < COLS) {
		/* on the same line: back, forward, or a return and forward */
		if (ny == _cury) {
			c = horiz(ny, _curx, nx, &how);
			c2 = _cap.cr ? len(_cap.cr) + horiz(ny, 0, nx, &how2) : 1000;
			if (c <= c2 && c < best) {
				dohoriz(ny, _curx, nx, how);
				_curx = nx;
				return;
			}
			if (c2 < best) {
				_puts(_cap.cr);
				dohoriz(ny, 0, nx, how2);
				_curx = nx;
				return;
			}
		/* a few lines down or up, then along */
		} else if (ny > _cury && ny - _cury <= 4 && _cap.dn ||
			   ny < _cury && _cury - ny <= 4 && _cap.up) {
			x = _curx;
			if (ny > _cury) {
				c = times(ny - _cury, _cap.dn);
				if (_onlcr && _cap.dn[0] == '\n' && _cap.dn[1] == 0)
					x = 0;
			} else
				c = times(_cury - ny, _cap.up);
			c2 = horiz(ny, x, nx, &how);
			if (c + c2 < best) {
				for (i = _cury; i < ny; i++)
					_puts(_cap.dn);
				for (i = _cury; i > ny; i--)
					_puts(_cap.up);
				dohoriz(ny, x, nx, how);
				_cury = ny;
				_curx = nx;
				return;
			}
		}
	}
	_puts(cm);
	_cury = ny;
	_curx = nx;
}

/* copy a window's changes into _newscr */
wnoutrefresh(w)
register WINDOW *w;
{
	register int y, x;
	register chtype *src, *dst;
	int first, last, sy;

	if (_endwin)
		_reenter();
	if (w->_clear) {
		curscr->_clear = TRUE;
		w->_clear = FALSE;
	}
	for (y = 0; y < w->_maxy; y++) {
		if ((first = w->_firstch[y]) == _NOCHANGE)
			continue;
		last = w->_lastch[y];
		sy = w->_begy + y;
		src = w->_y[y];
		dst = _newscr->_y[sy] + w->_begx;
		for (x = first; x <= last; x++)
			dst[x] = src[x];
		_touch(_newscr, sy, w->_begx + first, w->_begx + last);
		w->_firstch[y] = w->_lastch[y] = _NOCHANGE;
	}
	if (!w->_leave) {
		_newscr->_cury = w->_begy + w->_cury;
		_newscr->_curx = w->_begx + w->_curx;
	}
	_newscr->_leave = w->_leave;
	return OK;
}

/* send what line y needs */
static
updline(y)
int y;
{
	register chtype *n = _newscr->_y[y], *c = curscr->_y[y];
	register int x;
	int first = _newscr->_firstch[y], last = _newscr->_lastch[y];
	int blank, run;

	while (first <= last && n[first] == c[first])
		first++;
	while (last >= first && n[last] == c[last])
		last--;
	if (first > last)
		return;
	/* where the rest of the new line is blank */
	for (blank = COLS; blank > first && n[blank - 1] == ' '; blank--)
		;
	if (blank > last || !_cap.ce)
		blank = COLS;		/* no clearing worth doing */
	else {
		/* worth it only if the old line has something there */
		for (x = blank, run = 0; x < COLS; x++)
			if (c[x] != ' ')
				run++;
		if (run <= len(_cap.ce))
			blank = COLS;
	}
	for (x = first; x <= last && x < blank; x++) {
		if (n[x] == c[x])
			continue;
		/* (over a short stretch that is already right, _mvcur may
		 * send its characters again: cheaper than moving) */
		if (x != _curx || y != _cury)
			_mvcur(y, x);
		if (y == LINES - 1 && x == COLS - 1 && _cap.am && !_cap.xn) {
			c[x] = n[x];		/* writing there would scroll */
			continue;
		}
		_setattr(n[x]);
		_putc(n[x] & A_CHARTEXT);
		c[x] = n[x];
		if (++_curx >= COLS) {
			if (!_cap.am)
				_curx = COLS - 1;
			else if (_cap.xn)
				_cury = _curx = -1;
			else {
				_curx = 0;
				if (++_cury >= LINES)
					_cury = _curx = -1;
			}
		}
	}
	if (blank < COLS) {
		_mvcur(y, blank);
		_setattr(0);
		_puts(_cap.ce);
		for (x = blank; x < COLS; x++)
			c[x] = ' ';
	}
}

doupdate()
{
	register int y, x;

	if (_endwin)
		_reenter();
	if (curscr->_clear) {
		_setattr(0);
		if (_cap.cl) {
			_puts(_cap.cl);
			_cury = _curx = 0;
			for (y = 0; y < LINES; y++)
				for (x = 0; x < COLS; x++)
					curscr->_y[y][x] = ' ';
		} else
			for (y = 0; y < LINES; y++)		/* make sure of every place */
				for (x = 0; x < COLS; x++)
					curscr->_y[y][x] = 0;
		touchwin(_newscr);
		curscr->_clear = FALSE;
	}
	for (y = 0; y < LINES; y++)
		if (_newscr->_firstch[y] != _NOCHANGE) {
			updline(y);
			_newscr->_firstch[y] = _newscr->_lastch[y] = _NOCHANGE;
		}
	if (!_newscr->_leave)
		_mvcur(_newscr->_cury, _newscr->_curx);
	_setattr(0);
	_flush();
	return OK;
}

wrefresh(w)
WINDOW *w;
{
	if (w == curscr) {
		curscr->_clear = TRUE;
		return doupdate();
	}
	wnoutrefresh(w);
	return doupdate();
}

/* copy what overlaps of src onto dst: all of it (overwrite), or only what
 * is not blank (overlay) */
static
copywin(src, dst, skipblank)
register WINDOW *src, *dst;
int skipblank;
{
	register int y, x;
	int top, bot, left, right;
	chtype ch;

	top = src->_begy > dst->_begy ? src->_begy : dst->_begy;
	left = src->_begx > dst->_begx ? src->_begx : dst->_begx;
	bot = src->_begy + src->_maxy < dst->_begy + dst->_maxy ?
		src->_begy + src->_maxy : dst->_begy + dst->_maxy;
	right = src->_begx + src->_maxx < dst->_begx + dst->_maxx ?
		src->_begx + src->_maxx : dst->_begx + dst->_maxx;
	for (y = top; y < bot; y++)
		for (x = left; x < right; x++) {
			ch = src->_y[y - src->_begy][x - src->_begx];
			if (skipblank && (ch & A_CHARTEXT) == ' ')
				continue;
			if (dst->_y[y - dst->_begy][x - dst->_begx] != ch) {
				dst->_y[y - dst->_begy][x - dst->_begx] = ch;
				_touch(dst, y - dst->_begy, x - dst->_begx, x - dst->_begx);
			}
		}
	return OK;
}

overlay(src, dst)
WINDOW *src, *dst;
{
	return copywin(src, dst, 1);
}

overwrite(src, dst)
WINDOW *src, *dst;
{
	return copywin(src, dst, 0);
}
