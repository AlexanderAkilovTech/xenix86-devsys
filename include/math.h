/* math.h - xenix86-devsys: the math library (cc ... -lm), System V style.
 * frexp, ldexp and modf are in the C library itself. */
#ifndef _MATH_H
#define _MATH_H

double	atof();
double	frexp(), ldexp(), modf();
double	fabs(), floor(), ceil(), fmod();
double	sqrt(), hypot(), cabs();
double	exp(), log(), log10(), pow();
double	sin(), cos(), tan(), asin(), acos(), atan(), atan2();
double	sinh(), cosh(), tanh();

extern double _huge;		/* infinity */
#define	HUGE_VAL	_huge
#define	HUGE		_huge

/* what a math function passes to matherr(), which a program may define
 * to handle errors its own way (return nonzero to say it did) */
struct exception {
	int	type;
	char	*name;
	double	arg1, arg2;
	double	retval;
};

#define	DOMAIN		1	/* argument out of the domain */
#define	SING		2	/* singularity */
#define	OVERFLOW	3	/* result too large */
#define	UNDERFLOW	4	/* result too small */
#define	TLOSS		5	/* total loss of significance */
#define	PLOSS		6	/* partial loss of significance */

#define	M_E		2.7182818284590452354
#define	M_LOG2E		1.4426950408889634074
#define	M_LOG10E	0.43429448190325182765
#define	M_LN2		0.69314718055994530942
#define	M_LN10		2.30258509299404568402
#define	M_PI		3.14159265358979323846
#define	M_PI_2		1.57079632679489661923
#define	M_PI_4		0.78539816339744830962
#define	M_1_PI		0.31830988618379067154
#define	M_2_PI		0.63661977236758134308
#define	M_2_SQRTPI	1.12837916709551257390
#define	M_SQRT2		1.41421356237309504880
#define	M_SQRT1_2	0.70710678118654752440

#endif
