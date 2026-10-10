/* string.h - xenix86-devsys */
#ifndef _STRING_H
#define _STRING_H
#ifndef _SIZE_T
#define _SIZE_T
typedef	unsigned	size_t;
#endif
extern char *strcpy(), *strncpy(), *strcat(), *strncat(), *strchr(), *strrchr();
extern char *index(), *rindex(), *memcpy(), *memset();
extern int strlen(), strcmp(), strncmp(), memcmp();
extern int strspn(), strcspn();
extern char *strpbrk(), *strtok(), *strdup(), *memchr(), *memccpy(), *memmove();
#endif
