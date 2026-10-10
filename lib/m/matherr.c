/* matherr.c - xenix86-devsys libm: the default matherr, which leaves math
 * errors to the library's own handling (merr.c). A program's own matherr
 * takes its place: build.sh makes this module class 1, so ld86 prefers the
 * program's definition without a warning (see tools/objclass.py). */
#include <math.h>

matherr(x)
struct exception *x;
{
	return 0;
}
