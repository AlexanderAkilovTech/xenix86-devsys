/* stdlib.h - xenix86-devsys */
#ifndef _STDLIB_H
#define _STDLIB_H

#define	EXIT_SUCCESS	0
#define	EXIT_FAILURE	1

char	*malloc();
char	*calloc();
char	*realloc();
char	*getenv();
long	atol();
long	strtol();
unsigned long strtoul();

#endif
