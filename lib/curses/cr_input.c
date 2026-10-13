/* cr_input.c - curses: reading the keyboard: characters, function keys,
 * strings. xenix86-devsys, K&R C. */
#include "curses.h"
#include <termio.h>

static int	pending[16];	/* read but not yet returned */
static int	npend;

static int
take()
{
	register int c = pending[0], i;

	for (i = 1; i < npend; i++)
		pending[i - 1] = pending[i];
	npend--;
	return c;
}

/* one byte from the keyboard: waiting for it, or not if nodelay; ERR if
 * there is none */
static int
rdbyte(w)
WINDOW *w;
{
	unsigned char c;
	int n;

	if (npend)
		return take();
	if (w->_nodelay)
		_rdmode(0, 0);
	n = read(0, (char *)&c, 1);
	if (w->_nodelay)
		_rdrestore();
	return n == 1 ? c : ERR;
}

/* does some function key's string begin with these n characters?
 * 2 if one is exactly them, 1 if one is longer, 0 if none */
static int
keymatch(buf, n, code)
char *buf;
int n, *code;
{
	register int k, i, r = 0;
	register char *s;

	for (k = 0; k < _nkeys; k++) {
		s = _keystr[k];
		for (i = 0; i < n && s[i] == buf[i]; i++)
			;
		if (i < n)
			continue;
		if (s[n] == 0) {
			*code = _keycode[k];
			return 2;
		}
		r = 1;
	}
	return r;
}

wgetch(w)
register WINDOW *w;
{
	char buf[16];
	unsigned char ch;
	int c, n, m, i, code;

	wrefresh(w);
	if ((c = rdbyte(w)) == ERR)
		return ERR;
	if (w->_use_keypad && _nkeys) {
		buf[0] = c;
		n = 1;
		while ((m = keymatch(buf, n, &code)) == 1 && n < 15) {
			/* the rest of a function key comes at once: wait a
			 * fifth of a second for it, no longer */
			if (npend)
				ch = take();
			else {
				_rdmode(0, 2);
				i = read(0, (char *)&ch, 1);
				_rdrestore();
				if (i != 1)
					break;
			}
			buf[n++] = ch;
		}
		if (m == 2)
			return code;
		for (i = 1; i < n; i++)		/* not a key: keep the rest */
			pending[npend++] = buf[i] & 0377;
	}
	if (_echoit && c < 0400) {
		waddch(w, c);
		wrefresh(w);
	}
	return c;
}

/* put c back, to be the next character wgetch returns */
ungetch(c)
int c;
{
	register int i;

	if (npend >= 16)
		return ERR;
	for (i = npend; i > 0; i--)
		pending[i] = pending[i - 1];
	pending[0] = c;
	npend++;
	return OK;
}

/* back over one echoed character */
static
rubout(w, c)
register WINDOW *w;
int c;
{
	register int n;

	for (n = c < ' ' || c == 0177 ? 2 : 1; n > 0; n--) {
		if (w->_curx > 0)
			w->_curx--;
		else if (w->_cury > 0) {
			w->_cury--;
			w->_curx = w->_maxx - 1;
		}
		w->_y[w->_cury][w->_curx] = ' ';
		_touch(w, w->_cury, w->_curx, w->_curx);
	}
}

/* read a line into str, with the erase and kill characters working */
wgetstr(w, str)
register WINDOW *w;
char *str;
{
	register char *p = str;
	int c, echoed = _echoit, ec = erasechar() & 0377, kc = killchar() & 0377;

	_echoit = 0;
	for (;;) {
		if ((c = wgetch(w)) == ERR) {
			if (p == str) {
				*p = 0;
				_echoit = echoed;
				return ERR;
			}
			break;
		}
		if (c == '\n' || c == '\r' || c == KEY_ENTER)
			break;
		if (c == ec || c == '\b' || c == 0177 || c == KEY_BACKSPACE) {
			if (p > str) {
				p--;
				if (echoed)
					rubout(w, *p & 0377);
			}
			continue;
		}
		if (c == kc) {
			while (p > str) {
				p--;
				if (echoed)
					rubout(w, *p & 0377);
			}
			continue;
		}
		if (c >= 0400)
			continue;
		*p++ = c;
		if (echoed)
			waddch(w, c);
	}
	*p = 0;
	_echoit = echoed;
	if (echoed)
		wrefresh(w);
	return OK;
}

/* throw away what has been typed and not read */
flushinp()
{
	npend = 0;
	if (_tty >= 0)
		ioctl(_tty, TCFLSH, 0);
	return OK;
}
