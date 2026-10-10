/* fltscan.c - xenix86-devsys libc: scanf's floating point conversions.
 * Linked when the compiler marks a program as needing them, by referring to
 * __xfptvc; otherwise scanf gets the stub in fltsnone.c. build.sh makes this
 * module class 1 so that its _fltscan wins over the stub's
 * (tools/objclass.py). */
#include <stdio.h>
#include <ctype.h>

double	strtod();

int	__xfptvc;

static char text[840];

/* _fltscan(f, width, p, size, nread): read [+-]ddd[.ddd][e[+-]ddd], at most
 * width characters, from f and store it through p as a double (size 'l') or
 * a float (p null: just read it). Counts the characters taken in *nread.
 * Returns 0 if there was no number. */
_fltscan(f, width, p, size, nread)
register FILE *f;
int width;
char *p;
int size, *nread;
{
	register char *s = text;
	register int c;
	int any = 0;

	c = getc(f);
	if ((c == '-' || c == '+') && width > 0) {
		*s++ = c;
		width--;
		c = getc(f);
	}
	while (width > 0 && isdigit(c) && s < text + 830) {
		*s++ = c;
		any = 1;
		width--;
		c = getc(f);
	}
	if (c == '.' && width > 0) {
		*s++ = c;
		width--;
		c = getc(f);
		while (width > 0 && isdigit(c) && s < text + 830) {
			*s++ = c;
			any = 1;
			width--;
			c = getc(f);
		}
	}
	if (any && (c == 'e' || c == 'E') && width > 0) {
		*s++ = c;
		width--;
		c = getc(f);
		if ((c == '-' || c == '+') && width > 0) {
			*s++ = c;
			width--;
			c = getc(f);
		}
		while (width > 0 && isdigit(c) && s < text + 838) {
			*s++ = c;
			width--;
			c = getc(f);
		}
	}
	if (c != EOF)
		ungetc(c, f);
	*nread += s - text;
	*s = 0;
	if (!any)
		return 0;
	if (p) {
		if (size == 'l')
			*(double *)p = strtod(text, (char **)0);
		else
			*(float *)p = strtod(text, (char **)0);
	}
	return 1;
}
