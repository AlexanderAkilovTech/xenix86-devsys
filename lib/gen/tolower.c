/* tolower.c - xenix86-devsys libc (K&R C) */
#include <ctype.h>

tolower(c)
register int c;
{
	return isupper(c) ? c - 'A' + 'a' : c;
}
