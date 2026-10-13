/* lib/curses/curses.h - what the parts of the curses library share.
 * xenix86-devsys, K&R C. */
#include "../../include/curses.h"

/* the terminal's capabilities, from termcap */
struct _caps {
	char	*cm, *cl, *ce, *cd, *ho, *up, *dn, *nd, *le, *cr;
	char	*so, *se, *us, *ue, *mr, *md, *mb, *mh, *me;
	char	*ti, *te, *vi, *ve, *ks, *ke, *bl, *vb;
	char	*al, *dl, *ic, *dc, *im, *ei;
	int	am, xn, ms;
};
extern struct _caps _cap;

extern WINDOW	*_newscr;	/* what the screen is to look like */
extern int	_echoit;	/* getch echoes */
extern int	_nlmode;	/* newline mapping on (nl) */
extern int	_cbreak;	/* 0 cooked, 1 cbreak, 2 raw */
extern int	_endwin;	/* endwin has been called */
extern int	_onlcr;		/* the terminal turns \n into \r\n */
extern int	_tty;		/* the terminal's file descriptor, -1 if none */
extern chtype	_curattr;	/* the attributes the terminal is in */
extern int	_cury, _curx;	/* where the terminal's cursor is (-1: unknown) */

/* function keys: the string each sends, and its code */
#define	NKEYS	32
extern char	*_keystr[NKEYS];
extern int	_keycode[NKEYS];
extern int	_nkeys;

extern int	_putc();
extern WINDOW	*_makewin();
