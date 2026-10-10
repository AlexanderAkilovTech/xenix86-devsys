/* fltnone.c - xenix86-devsys libc: printf's floating point conversions when
 * the program does not have them. The real _fltfmt (fltcvt.c) is linked
 * when the compiler sees a double passed to a function whose name ends in
 * printf; this one stands in otherwise (see tools/objclass.py), and says so
 * if it is ever called (printf of a constant through a wrapper of another
 * name, say). */

char *
_fltfmt(w, conv, prec, alt)
unsigned *w;
int conv, prec, alt;
{
	static char msg[] = "printf: floating point conversion not linked in\n";

	write(2, msg, sizeof msg - 1);
	exit(1);
	return msg;
}
