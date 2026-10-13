/* sys/times.h - xenix86-devsys: a stand-in, for cross-compiling only.
 *
 * On XENIX this file is SCO's own, installed from the Operating System's disk
 * N05 with the Development System (see sys/types.h). This stand-in matches it:
 * what times(2) fills in, four times in clock ticks (20 a second on XENIX
 * 2.1.3). Include <sys/types.h> first. */

struct	tms {
	time_t	tms_utime;	/* the process's time in its program */
	time_t	tms_stime;	/* in the kernel, for it */
	time_t	tms_cutime;	/* its children's (those waited for), in theirs */
	time_t	tms_cstime;	/* and in the kernel, for them */
};
