/* mathint.h - xenix86-devsys libm: what the math library's sources share. */
#include <math.h>
#include <errno.h>

double	_merr(), _logdd(), _expdd();
double	frexp(), ldexp(), modf(), fabs(), floor(), sqrt(), exp(), log(), atan();

/* a double and its words, lowest first */
union _dw {
	double	d;
	unsigned w[4];
};

#define	_EXPF(u)	(((u).w[3] >> 4) & 0x7FF)
#define	_ISNAN(u)	(_EXPF(u) == 0x7FF && (((u).w[3] & 0xF) | (u).w[2] | (u).w[1] | (u).w[0]))
#define	_ISINF(u)	(((u).w[3] & 0x7FFF) == 0x7FF0 && ((u).w[2] | (u).w[1] | (u).w[0]) == 0)
