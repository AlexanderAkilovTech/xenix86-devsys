/* unistd.h - xenix86-devsys: common system call declarations */
#ifndef _UNISTD_H
#define _UNISTD_H

#define	STDIN_FILENO	0
#define	STDOUT_FILENO	1
#define	STDERR_FILENO	2

#define	SEEK_SET	0
#define	SEEK_CUR	1
#define	SEEK_END	2

#define	F_OK	0	/* access(): file exists */
#define	X_OK	1	/* executable */
#define	W_OK	2	/* writable */
#define	R_OK	4	/* readable */

long	lseek();
char	*sbrk();
unsigned sleep();
unsigned alarm();

#endif
