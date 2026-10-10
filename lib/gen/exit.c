/* exit.c - xenix86-devsys libc (K&R C)
 * stdio sets _clean when a stream first gets a buffer, so a program that never
 * uses stdio does not have to carry it just to exit. */

int (*_clean)();

exit(status)
int status;
{
	if (_clean)
		(*_clean)();
	_exit(status);
}
