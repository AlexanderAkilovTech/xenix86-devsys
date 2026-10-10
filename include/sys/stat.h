/* sys/stat.h - xenix86-devsys: a stand-in, for cross-compiling only.
 *
 * On XENIX this file is SCO's own, installed from the Operating System's disk
 * N05 with the Development System (see sys/types.h). This stand-in matches it:
 * the 30-byte System V structure (measured with tests/sysprobe.c) and the same
 * constants. Include <sys/types.h> first. */

struct	stat {
	dev_t	st_dev;		/* device of the directory entry */
	ino_t	st_ino;		/* i-node number */
	ushort	st_mode;	/* type and permissions */
	short	st_nlink;	/* number of links */
	ushort	st_uid;		/* owner */
	ushort	st_gid;		/* group */
	dev_t	st_rdev;	/* device, for character and block special files */
	off_t	st_size;	/* size in bytes */
	time_t	st_atime;	/* last access */
	time_t	st_mtime;	/* last modification */
	time_t	st_ctime;	/* last status change */
};

#define	S_IFMT	0170000		/* type of file */
#define	S_IFDIR	0040000		/* directory */
#define	S_IFCHR	0020000		/* character special */
#define	S_IFBLK	0060000		/* block special */
#define	S_IFREG	0100000		/* regular */
#define	S_IFIFO	0010000		/* fifo */
#define	S_IFNAM	0050000		/* XENIX name space entry */
#define	S_INSEM	01		/* name space entry: semaphore */
#define	S_INSHD	02		/* name space entry: shared data */
#define	S_ISUID	04000		/* set user id on execution */
#define	S_ISGID	02000		/* set group id on execution */
#define	S_ISVTX	01000		/* save swapped text even after use */
#define	S_IREAD	00400		/* read permission, owner */
#define	S_IWRITE 00200		/* write permission, owner */
#define	S_IEXEC	00100		/* execute/search permission, owner */
