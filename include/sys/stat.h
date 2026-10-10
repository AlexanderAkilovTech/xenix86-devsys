/* sys/stat.h - xenix86-devsys: file status (System V layout, 30 bytes) */
#ifndef _SYS_STAT_H
#define _SYS_STAT_H

struct	stat {
	dev_t	st_dev;
	ino_t	st_ino;
	unsigned short st_mode;
	short	st_nlink;
	unsigned short st_uid;
	unsigned short st_gid;
	dev_t	st_rdev;
	off_t	st_size;
	time_t	st_atime;
	time_t	st_mtime;
	time_t	st_ctime;
};

#define	S_IFMT	0170000		/* type of file */
#define	S_IFDIR	0040000		/* directory */
#define	S_IFNAM	0050000		/* XENIX special named file */
#define	S_IFCHR	0020000		/* character special */
#define	S_IFBLK	0060000		/* block special */
#define	S_IFREG	0100000		/* regular */
#define	S_IFIFO	0010000		/* fifo */
#define	S_ISUID	04000		/* set user id on execution */
#define	S_ISGID	02000		/* set group id on execution */
#define	S_ISVTX	01000		/* save swapped text even after use */
#define	S_IREAD	00400		/* read permission, owner */
#define	S_IWRITE 00200		/* write permission, owner */
#define	S_IEXEC	00100		/* execute/search permission, owner */

#define	S_ISDIR(m)	(((m) & S_IFMT) == S_IFDIR)
#define	S_ISCHR(m)	(((m) & S_IFMT) == S_IFCHR)
#define	S_ISBLK(m)	(((m) & S_IFMT) == S_IFBLK)
#define	S_ISREG(m)	(((m) & S_IFMT) == S_IFREG)
#define	S_ISFIFO(m)	(((m) & S_IFMT) == S_IFIFO)

int	stat();
int	fstat();
int	chmod();
int	umask();

#endif
