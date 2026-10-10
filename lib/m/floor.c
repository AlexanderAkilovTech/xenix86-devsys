/* floor.c - xenix86-devsys libm: floor(x) and ceil(x), exactly. */
#include "mathint.h"

double
floor(x)
double x;
{
	double i;

	if (modf(x, &i) < 0.0)
		return i - 1.0;
	return i;
}

double
ceil(x)
double x;
{
	double i;

	if (modf(x, &i) > 0.0)
		return i + 1.0;
	return i;
}
