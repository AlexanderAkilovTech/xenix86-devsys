/* sys/termio.h - xenix86-devsys: a stand-in used only for cross-compiling.
 * On XENIX, /usr/include/sys/termio.h is SCO's own, from the Operating
 * System's disk N5. These are the System V terminal modes, as XENIX 2.1.3
 * has them (ioctl TCGETA and TCSETA on a terminal). */
#ifndef _SYS_TERMIO_H
#define _SYS_TERMIO_H

#define	NCC	8		/* control characters */

#define	VINTR	0
#define	VQUIT	1
#define	VERASE	2
#define	VKILL	3
#define	VEOF	4
#define	VEOL	5
#define	VEOL2	6
#define	VMIN	4		/* with ICANON off: characters to wait for */
#define	VTIME	5		/* and tenths of a second to wait */
#define	VSWTCH	7

/* c_iflag */
#define	IGNBRK	0000001
#define	BRKINT	0000002
#define	IGNPAR	0000004
#define	PARMRK	0000010
#define	INPCK	0000020
#define	ISTRIP	0000040
#define	INLCR	0000100
#define	IGNCR	0000200
#define	ICRNL	0000400
#define	IUCLC	0001000
#define	IXON	0002000
#define	IXANY	0004000
#define	IXOFF	0010000

/* c_oflag */
#define	OPOST	0000001
#define	OLCUC	0000002
#define	ONLCR	0000004
#define	OCRNL	0000010
#define	ONOCR	0000020
#define	ONLRET	0000040
#define	OFILL	0000100
#define	OFDEL	0000200
#define	TABDLY	0014000
#define	TAB3	0014000

/* c_cflag */
#define	CBAUD	0000017
#define	B0	0
#define	B50	0000001
#define	B75	0000002
#define	B110	0000003
#define	B134	0000004
#define	B150	0000005
#define	B200	0000006
#define	B300	0000007
#define	B600	0000010
#define	B1200	0000011
#define	B1800	0000012
#define	B2400	0000013
#define	B4800	0000014
#define	B9600	0000015
#define	EXTA	0000016
#define	EXTB	0000017
#define	CSIZE	0000060
#define	CS8	0000060
#define	CSTOPB	0000100
#define	CREAD	0000200
#define	PARENB	0000400
#define	PARODD	0001000
#define	HUPCL	0002000
#define	CLOCAL	0004000

/* c_lflag */
#define	ISIG	0000001
#define	ICANON	0000002
#define	XCASE	0000004
#define	ECHO	0000010
#define	ECHOE	0000020
#define	ECHOK	0000040
#define	ECHONL	0000100
#define	NOFLSH	0000200

#define	TIOC	('T' << 8)
#define	TCGETA	(TIOC | 1)
#define	TCSETA	(TIOC | 2)
#define	TCSETAW	(TIOC | 3)
#define	TCSETAF	(TIOC | 4)
#define	TCSBRK	(TIOC | 5)
#define	TCXONC	(TIOC | 6)
#define	TCFLSH	(TIOC | 7)

struct termio {
	unsigned short	c_iflag;	/* input modes */
	unsigned short	c_oflag;	/* output modes */
	unsigned short	c_cflag;	/* control modes */
	unsigned short	c_lflag;	/* line discipline modes */
	char		c_line;		/* line discipline */
	unsigned char	c_cc[NCC];	/* control characters */
	char		c_res;		/* padding */
};

#endif
