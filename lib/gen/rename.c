/* rename.c - xenix86-devsys libc (K&R C)
 * XENIX 2.1.3 has no rename system call; as in Version 7, link the new name
 * and remove the old one. Not atomic: for a moment the file has both names.
 * A file already called to is replaced, but only when the link failed
 * because it exists, and not when to is the same file or a directory: an
 * unlink there would lose the file (root can unlink directories). */
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

extern int errno;

rename(from, to)
char *from, *to;
{
	struct stat a, b;

	if (link(from, to) < 0) {
		if (errno != EEXIST || stat(from, &a) < 0 || stat(to, &b) < 0)
			return -1;
		if (a.st_dev == b.st_dev && a.st_ino == b.st_ino)
			return 0;		/* the same file already */
		if ((b.st_mode & S_IFMT) == S_IFDIR) {
			errno = EISDIR;
			return -1;
		}
		if (unlink(to) < 0 || link(from, to) < 0)
			return -1;
	}
	return unlink(from);
}
