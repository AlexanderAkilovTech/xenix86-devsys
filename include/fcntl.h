/* fcntl.h - xenix86-devsys: open flags (System V values) */
#ifndef _FCNTL_H
#define _FCNTL_H

#define	O_RDONLY	0
#define	O_WRONLY	1
#define	O_RDWR		2
#define	O_NDELAY	04	/* non-blocking I/O */
#define	O_APPEND	010	/* append (writes guaranteed at the end) */
#define	O_CREAT		00400	/* open with file create (uses third open arg) */
#define	O_TRUNC		01000	/* open with truncation */
#define	O_EXCL		02000	/* exclusive open */

int	open();
int	creat();

#endif
