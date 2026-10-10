/* toupper.c - xenix86-devsys libc (K&R C) */
#include <ctype.h>

toupper(c)
register int c;
{
	return islower(c) ? c - 'a' + 'A' : c;
}
