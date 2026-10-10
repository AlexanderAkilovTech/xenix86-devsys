/* sys/types.h - xenix86-devsys: a stand-in, for cross-compiling only.
 *
 * On XENIX this file is SCO's own: the Development System's machine-dependent
 * part, /usr/include/sys and adb, comes on the Operating System's disk N05
 * and is installed with it (see /etc/dsmd.perms). This stand-in declares the
 * same names with the same types, and like SCO's has no include guard, so a
 * program that compiles here compiles there. */

typedef	struct { int r[1]; } *physadr;
typedef	long		daddr_t;	/* disk address */
typedef	char		*caddr_t;	/* core address */
typedef	unsigned short	ushort;
typedef	unsigned short	ino_t;		/* i-node number */
typedef	char		cnt_t;
typedef	long		time_t;		/* seconds since 1970 */
typedef	int		label_t[6];	/* saved return, sp, si, di, bp */
typedef	short		dev_t;		/* device: major << 8 | minor */
typedef	long		off_t;		/* file offset */
typedef	unsigned long	paddr_t;	/* physical address */
typedef	unsigned short	mloc_t;		/* memory region location */
typedef	unsigned short	msize_t;	/* memory region size */
typedef	unsigned long	key_t;		/* IPC key */
typedef	unsigned	uint;

#ifdef M_I86
#define	faddr_t		char *		/* "far" address: one segment here */
#endif

#ifndef major
#define	major(x)	(((unsigned)(x) >> 8))
#endif
#ifndef minor
#define	minor(x)	((x) & 0377)
#endif
#ifndef makedev
#define	makedev(x, y)	(dev_t)((x) << 8 | (y))
#endif

typedef	unsigned char	uchar_t;
typedef	unsigned long	ulong_t;
#define	GETUCHAR(uc)	(uc)
#define	GETULONG(ul)	(ul)

struct	saddr {			/* segment and offset */
	unsigned short	sa_seg;
	long		sa_off;
};
