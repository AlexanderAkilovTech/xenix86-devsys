/* fabs.c - xenix86-devsys libm: fabs(x), |x|, by clearing the sign bit. */
#include "mathint.h"

double
fabs(x)
double x;
{
	union _dw u;

	u.d = x;
	u.w[3] &= 0x7FFF;
	return u.d;
}
