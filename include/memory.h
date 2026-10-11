/* memory.h - xenix86-devsys: the System V memory routines */
#ifndef _MEMORY_H
#define _MEMORY_H
#ifndef _SIZE_T
#define _SIZE_T
typedef	unsigned	size_t;
#endif

char	*memcpy();
char	*memset();
char	*memccpy();
char	*memmove();
char	*memchr();
int	memcmp();

#endif
