/* values.h - xenix86-devsys: machine-dependent values (System V), for the
 * 8086 with IEEE floating point (software, or the 8087). */
#ifndef _VALUES_H
#define _VALUES_H

#define	BITSPERBYTE	8
#define	BITS(type)	(BITSPERBYTE * (int)sizeof(type))

#define	HIBITS		((short)0x8000)
#define	HIBITL		0x80000000L
#define	HIBITI		((int)0x8000)

#define	MAXSHORT	0x7FFF
#define	MAXLONG		0x7FFFFFFFL
#define	MAXINT		0x7FFF

#define	MAXDOUBLE	1.79769313486231570e+308
#define	MAXFLOAT	((float)3.40282346638528860e+38)
#define	MINDOUBLE	4.94065645841246544e-324
#define	MINFLOAT	((float)1.40129846432481707e-45)
#define	DMINEXP		(-1021)
#define	FMINEXP		(-125)
#define	DMAXEXP		1024
#define	FMAXEXP		128
#define	DSIGNIF		53
#define	FSIGNIF		24
#define	DMAXPOWTWO	((double)(1L << 30) * (1L << 22))
#define	FMAXPOWTWO	((float)(1L << 23))
#define	_IEEE		1
#define	_DEXPLEN	11
#define	_FEXPLEN	8
#define	_HIDDENBIT	1
#define	LN_MAXDOUBLE	709.78271289338399678
#define	LN_MINDOUBLE	(-744.44007192138126231)

#endif
