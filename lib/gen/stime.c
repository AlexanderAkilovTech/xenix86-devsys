/* stime.c - xenix86-devsys libc (K&R C)
 * Set the clock to *tp seconds since 1970; the super-user only. XENIX 2.1.3's
 * system call 25 takes the seconds themselves, the low word in BX and the
 * high in CX, as time (13) returns them, not a pointer to them (M62). */

stime(tp)
long *tp;
{
	return _stime((unsigned)*tp, (unsigned)(*tp >> 16));
}
