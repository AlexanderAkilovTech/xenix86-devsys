/* fltsnone.c - xenix86-devsys libc: scanf's floating point conversions when
 * the program does not have them. The real _fltscan (fltscan.c) is linked
 * when the compiler sees a pointer to a float or double passed to a
 * function whose name ends in scanf; this one stands in otherwise (see
 * tools/objclass.py), and says so if it is ever called. */

_fltscan(f, width, p, size, nread)
char *f, *p;
int width, size, *nread;
{
	static char msg[] = "scanf: floating point conversion not linked in\n";

	write(2, msg, sizeof msg - 1);
	exit(1);
}
