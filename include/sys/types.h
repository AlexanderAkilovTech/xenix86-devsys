/* sys/types.h - xenix86-devsys: basic system types (XENIX System V, 8086) */
#ifndef _SYS_TYPES_H
#define _SYS_TYPES_H

typedef	long		daddr_t;	/* disk address */
typedef	char		*caddr_t;	/* core address */
typedef	unsigned short	ushort;
typedef	unsigned short	ino_t;		/* i-node number */
typedef	short		dev_t;		/* device number */
typedef	long		off_t;		/* file offset */
typedef	unsigned short	mode_t;		/* file mode */
#ifndef _TIME_T
#define _TIME_T
typedef	long		time_t;		/* seconds since 1970 */
#endif
#ifndef _SIZE_T
#define _SIZE_T
typedef	unsigned	size_t;
#endif

#endif
