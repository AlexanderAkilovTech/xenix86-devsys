/* stdlib.h - xenix86-devsys */
#ifndef _STDLIB_H
#define _STDLIB_H
#ifndef _SIZE_T
#define _SIZE_T
typedef	unsigned	size_t;
#endif

#define	EXIT_SUCCESS	0
#define	EXIT_FAILURE	1

char	*malloc();
char	*calloc();
char	*realloc();
char	*getenv();
long	atol();
long	labs();
double	atof();
double	strtod();
long	strtol();
unsigned long strtoul();

#endif
