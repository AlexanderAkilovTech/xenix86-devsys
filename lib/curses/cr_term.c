/* cr_term.c - curses: starting and ending, the terminal's modes, its
 * capabilities from termcap, and the output buffer. xenix86-devsys, K&R C. */
#include "curses.h"
#include <termio.h>

extern char *getenv(), *tgetstr(), *malloc();
extern short ospeed;
extern char PC, *BC, *UP;

WINDOW	*stdscr, *curscr, *_newscr;
int	LINES, COLS;
struct _caps _cap;
int	_echoit = 1, _nlmode = 1, _cbreak, _endwin, _onlcr, _tty = -1;
chtype	_curattr;
int	_cury = -1, _curx = -1;
char	*_keystr[NKEYS];
int	_keycode[NKEYS];
int	_nkeys;

static char	tbuf[1024];		/* the termcap entry */
static char	capbuf[600];		/* the capabilities, decoded */
static char	*capp = capbuf;
static struct termio shellmode, progmode, savemode;
static int	havemodes;
static char	obuf[256];
static int	on;

/* output, buffered: _putc for tputs, and the flush */
_putc(c)
int c;
{
	if (on == sizeof obuf)
		_flush();
	obuf[on++] = c;
	return c;
}

_flush()
{
	if (on > 0)
		write(1, obuf, on);
	on = 0;
}

_puts(s)			/* a capability, with its padding */
char *s;
{
	if (s)
		tputs(s, 1, _putc);
}

static char *
cap(id)
char *id;
{
	return tgetstr(id, &capp);
}

/* the function keys: termcap name, curses code */
static char *keynames[] = {
	"ku", "kd", "kl", "kr", "kh", "kb", "kH", "kD", "kI", "kL", "kA", "kN",
	"kP", "kE", "kS", "kC", "k0", "k1", "k2", "k3", "k4", "k5", "k6", "k7",
	"k8", "k9", "k;",
	"HM", "EN", "PU", "PD",		/* XENIX's names for Home, End, PgUp, PgDn */
	0
};
static int keycodes[] = {
	KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_HOME, KEY_BACKSPACE, KEY_LL,
	KEY_DC, KEY_IC, KEY_DL, KEY_IL, KEY_NPAGE, KEY_PPAGE, KEY_EOL, KEY_EOS,
	KEY_CLEAR, KEY_F(0), KEY_F(1), KEY_F(2), KEY_F(3), KEY_F(4), KEY_F(5),
	KEY_F(6), KEY_F(7), KEY_F(8), KEY_F(9), KEY_F(10),
	KEY_HOME, KEY_END, KEY_PPAGE, KEY_NPAGE
};

static
setmodes(t)
struct termio *t;
{
	if (_tty >= 0)
		ioctl(_tty, TCSETAW, t);
}

/* the program's modes, from the settings: echo is always curses' own */
static
progmodes()
{
	if (!havemodes)
		return;
	progmode = shellmode;
	progmode.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL);
	if (_cbreak) {
		progmode.c_lflag &= ~ICANON;
		progmode.c_cc[VMIN] = 1;
		progmode.c_cc[VTIME] = 0;
	}
	if (_cbreak == 2) {
		progmode.c_lflag &= ~ISIG;
		progmode.c_iflag &= ~(IXON | BRKINT | ISTRIP);
	}
	if (!_nlmode)
		progmode.c_iflag &= ~ICRNL;
	setmodes(&progmode);
}

WINDOW *
initscr()
{
	char *term, *p;
	int i, n;

	if (stdscr)
		return stdscr;
	if ((term = getenv("TERM")) == 0 || *term == 0)
		term = "unknown";
	if (tgetent(tbuf, term) != 1) {
		fprintf(stderr, "Sorry, I don't know anything about your \"%s\" terminal.\n", term);
		exit(1);
	}
	_cap.cm = cap("cm");
	if (_cap.cm == 0) {
		fprintf(stderr, "Sorry, a \"%s\" cannot move its cursor.\n", term);
		exit(1);
	}
	_cap.cl = cap("cl");	_cap.ce = cap("ce");	_cap.cd = cap("cd");
	_cap.ho = cap("ho");	_cap.up = cap("up");	_cap.dn = cap("do");
	_cap.nd = cap("nd");	_cap.le = cap("le");	_cap.cr = cap("cr");
	if (_cap.le == 0 && (_cap.le = cap("bc")) == 0 && tgetflag("bs"))
		_cap.le = "\b";
	if (_cap.cr == 0 && !tgetflag("nc"))
		_cap.cr = "\r";
	_cap.so = cap("so");	_cap.se = cap("se");	_cap.us = cap("us");
	_cap.ue = cap("ue");	_cap.mr = cap("mr");	_cap.md = cap("md");
	_cap.mb = cap("mb");	_cap.mh = cap("mh");	_cap.me = cap("me");
	_cap.ti = cap("ti");	_cap.te = cap("te");	_cap.vi = cap("vi");
	_cap.ve = cap("ve");	_cap.ks = cap("ks");	_cap.ke = cap("ke");
	_cap.vb = cap("vb");	_cap.al = cap("al");	_cap.dl = cap("dl");
	if ((_cap.bl = cap("bl")) == 0)
		_cap.bl = "\007";
	_cap.am = tgetflag("am");
	_cap.xn = tgetflag("xn");
	_cap.ms = tgetflag("ms");
	if (tgetnum("sg") > 0)		/* standout takes room on the screen */
		_cap.so = _cap.se = _cap.us = _cap.ue = 0;
	UP = _cap.up;
	BC = _cap.le;
	if ((p = cap("pc")) != 0)
		PC = *p;
	for (i = n = 0; keynames[i]; i++)
		if ((p = cap(keynames[i])) != 0 && *p && n < NKEYS) {
			_keystr[n] = p;
			_keycode[n++] = keycodes[i];
		}
	_nkeys = n;

	LINES = tgetnum("li");
	COLS = tgetnum("co");
	if ((p = getenv("LINES")) != 0 && atoi(p) > 0)
		LINES = atoi(p);
	if ((p = getenv("COLUMNS")) != 0 && atoi(p) > 0)
		COLS = atoi(p);
	if (LINES <= 0)
		LINES = 24;
	if (COLS <= 0)
		COLS = 80;

	_tty = isatty(1) ? 1 : isatty(0) ? 0 : -1;
	if (_tty >= 0 && ioctl(_tty, TCGETA, &shellmode) >= 0) {
		havemodes = 1;
		ospeed = shellmode.c_cflag & CBAUD;
		_onlcr = (shellmode.c_oflag & (OPOST | ONLCR)) == (OPOST | ONLCR);
	} else {
		_tty = -1;
		_onlcr = 1;
	}
	savemode = shellmode;
	progmodes();

	if ((stdscr = newwin(LINES, COLS, 0, 0)) == 0 ||
	    (curscr = newwin(LINES, COLS, 0, 0)) == 0 ||
	    (_newscr = newwin(LINES, COLS, 0, 0)) == 0) {
		fprintf(stderr, "curses: out of memory\n");
		exit(1);
	}
	curscr->_clear = TRUE;
	_puts(_cap.ti);
	_curattr = 0;
	_cury = _curx = -1;
	return stdscr;
}

/* back to the modes the shell had, the cursor at the bottom */
endwin()
{
	if (!stdscr || _endwin)
		return OK;
	_setattr(0);
	_mvcur(LINES - 1, 0);
	_puts(_cap.ke);
	_puts(_cap.ve);
	_puts(_cap.te);
	_flush();
	if (havemodes)
		setmodes(&shellmode);
	_endwin = 1;
	return OK;
}

/* the next refresh after endwin, or after a shell escape */
_reenter()
{
	_endwin = 0;
	progmodes();
	_puts(_cap.ti);
	curscr->_clear = TRUE;
	_cury = _curx = -1;
}

cbreak()	{ _cbreak = 1; progmodes(); return OK; }
nocbreak()	{ _cbreak = 0; progmodes(); return OK; }
raw()		{ _cbreak = 2; progmodes(); return OK; }
noraw()		{ _cbreak = 0; progmodes(); return OK; }
echo()		{ _echoit = 1; return OK; }
noecho()	{ _echoit = 0; return OK; }
nl()		{ _nlmode = 1; progmodes(); return OK; }
nonl()		{ _nlmode = 0; progmodes(); return OK; }

savetty()	{ savemode = progmode; return OK; }
resetty()	{ progmode = savemode; setmodes(&progmode); return OK; }
def_prog_mode()	{ return OK; }
reset_prog_mode() { progmodes(); return OK; }
def_shell_mode() { if (havemodes) ioctl(_tty, TCGETA, &shellmode); return OK; }
reset_shell_mode() { if (havemodes) setmodes(&shellmode); return OK; }

/* for getch: wait for input, or not (nodelay), or a tenth of a second at a
 * time (a function key's later characters) */
_rdmode(min, tenths)
int min, tenths;
{
	struct termio t;

	if (!havemodes)
		return;
	t = progmode;
	t.c_lflag &= ~ICANON;
	t.c_cc[VMIN] = min;
	t.c_cc[VTIME] = tenths;
	ioctl(_tty, TCSETA, &t);
}

_rdrestore()
{
	if (havemodes)
		ioctl(_tty, TCSETA, &progmode);
}

char
erasechar()
{
	return havemodes ? shellmode.c_cc[VERASE] : '\b';
}

char
killchar()
{
	return havemodes ? shellmode.c_cc[VKILL] : '\025';
}

static short speeds[] = { 0, 50, 75, 110, 134, 150, 200, 300, 600, 1200,
	1800, 2400, 4800, 9600, 19200, 38400 };

baudrate()
{
	return speeds[ospeed & 017];
}

/* the long name of the terminal: the last name in its entry */
char *
longname()
{
	static char name[64];
	register char *p, *q;

	for (p = tbuf, q = tbuf; *p && *p != ':'; p++)
		if (*p == '|')
			q = p + 1;
	for (p = name; *q && *q != ':' && *q != '|' && p < name + 63; )
		*p++ = *q++;
	*p = 0;
	return name;
}

beep()
{
	_puts(_cap.bl);
	_flush();
	return OK;
}

flash()
{
	_puts(_cap.vb ? _cap.vb : _cap.bl);
	_flush();
	return OK;
}

/* send the keypad into the mode where its keys send what termcap says */
keypad(win, bf)
WINDOW *win;
int bf;
{
	win->_use_keypad = bf;
	_puts(bf ? _cap.ks : _cap.ke);
	return OK;
}

/* the cursor: 0 invisible, 1 normal */
curs_set(vis)
int vis;
{
	_puts(vis ? _cap.ve : _cap.vi);
	return OK;
}
