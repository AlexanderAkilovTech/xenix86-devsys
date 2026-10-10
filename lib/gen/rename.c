/* rename.c - xenix86-devsys libc (K&R C)
 * XENIX 2.1.3 has no rename system call; as in Version 7, link the new name
 * and remove the old one. Not atomic: for a moment the file has both names. */

rename(from, to)
char *from, *to;
{
	if (link(from, to) < 0) {
		if (unlink(to) < 0 || link(from, to) < 0)
			return -1;
	}
	return unlink(from);
}
