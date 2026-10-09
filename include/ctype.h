/* ctype.h - xenix86-devsys */
#ifndef _CTYPE_H
#define _CTYPE_H
#define _U	01	/* upper case */
#define _L	02	/* lower case */
#define _N	04	/* digit */
#define _S	010	/* white space */
#define _P	020	/* punctuation */
#define _C	040	/* control */
#define _X	0100	/* hex digit */
#define _B	0200	/* blank */

extern char _ctype[];	/* indexed by c + 1, so EOF works */

#define isalpha(c)	((_ctype + 1)[c] & (_U | _L))
#define isupper(c)	((_ctype + 1)[c] & _U)
#define islower(c)	((_ctype + 1)[c] & _L)
#define isdigit(c)	((_ctype + 1)[c] & _N)
#define isxdigit(c)	((_ctype + 1)[c] & _X)
#define isspace(c)	((_ctype + 1)[c] & _S)
#define ispunct(c)	((_ctype + 1)[c] & _P)
#define isalnum(c)	((_ctype + 1)[c] & (_U | _L | _N))
#define isprint(c)	((_ctype + 1)[c] & (_P | _U | _L | _N | _B))
#define isgraph(c)	((_ctype + 1)[c] & (_P | _U | _L | _N))
#define iscntrl(c)	((_ctype + 1)[c] & _C)
#define isascii(c)	((unsigned)(c) <= 0177)
#define toascii(c)	((c) & 0177)
#define _toupper(c)	((c) - 'a' + 'A')
#define _tolower(c)	((c) - 'A' + 'a')
extern int toupper(), tolower();
#endif
