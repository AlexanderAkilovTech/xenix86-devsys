/* ctype.c - xenix86-devsys libc: character class table (ASCII) */
#include <ctype.h>

char _ctype[1 + 256] = {
	0,
	_C, _C, _C, _C, _C, _C, _C, _C,
	_C, _S|_C, _S|_C, _S|_C, _S|_C, _S|_C, _C, _C,
	_C, _C, _C, _C, _C, _C, _C, _C,
	_C, _C, _C, _C, _C, _C, _C, _C,
	_S|_B, _P, _P, _P, _P, _P, _P, _P,
	_P, _P, _P, _P, _P, _P, _P, _P,
	_N|_X, _N|_X, _N|_X, _N|_X, _N|_X, _N|_X, _N|_X, _N|_X,
	_N|_X, _N|_X, _P, _P, _P, _P, _P, _P,
	_P, _U|_X, _U|_X, _U|_X, _U|_X, _U|_X, _U|_X, _U,
	_U, _U, _U, _U, _U, _U, _U, _U,
	_U, _U, _U, _U, _U, _U, _U, _U,
	_U, _U, _U, _P, _P, _P, _P, _P,
	_P, _L|_X, _L|_X, _L|_X, _L|_X, _L|_X, _L|_X, _L,
	_L, _L, _L, _L, _L, _L, _L, _L,
	_L, _L, _L, _L, _L, _L, _L, _L,
	_L, _L, _L, _P, _P, _P, _P, _C,
};

toupper(c)
register int c;
{
	return islower(c) ? c - 'a' + 'A' : c;
}

tolower(c)
register int c;
{
	return isupper(c) ? c - 'A' + 'a' : c;
}
