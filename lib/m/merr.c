/* merr.c - xenix86-devsys libm: report a math error the System V way. The
 * function builds a struct exception and calls matherr(), which a program
 * may define (the library's own just returns 0). If matherr returns 0,
 * errno is set (EDOM for DOMAIN and SING, ERANGE otherwise) and, for DOMAIN,
 * SING and TLOSS, a message goes to the standard error. The value returned
 * is the one in the struct, which matherr may have changed. */
#include "mathint.h"

static char *what[] = { "", "DOMAIN", "SING", "OVERFLOW", "UNDERFLOW", "TLOSS", "PLOSS" };

static
say(s)
char *s;
{
	write(2, s, strlen(s));
}

double
_merr(type, name, a1, a2, retval)
int type;
char *name;
double a1, a2, retval;
{
	struct exception x;

	x.type = type;
	x.name = name;
	x.arg1 = a1;
	x.arg2 = a2;
	x.retval = retval;
	if (!matherr(&x)) {
		errno = type == DOMAIN || type == SING ? EDOM : ERANGE;
		if (type == DOMAIN || type == SING || type == TLOSS) {
			say(name);
			say(": ");
			say(what[type]);
			say(" error\n");
		}
	}
	return x.retval;
}
