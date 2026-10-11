/* objfile.h - reading XENIX x.out executables, as86 object files and ar
 * archives of them, for nm, size and strip. xenix86-devsys, K&R C. */

/* what objopen() found */
#define	F_XOUT	1		/* a XENIX x.out executable (segmented) */
#define	F_OBJ	2		/* an as86 object file */
#define	F_ARCH	3		/* an ar archive of object files */

struct osym {			/* a symbol as nm shows it */
	long	value;
	char	*name;
	int	type;		/* nm's letter: T D B A C U; lower case if local */
};

extern int	objfd;		/* the open file */
extern char	*objname;	/* its name */
extern char	objmem[];	/* the archive member being read */
extern long	o_text, o_data, o_bss;	/* sizes of the executable or module */
extern int	o_nsyms;	/* its symbols */
extern struct osym *o_syms;

/* little-endian numbers in a byte buffer, as XENIX and as86 store them */
extern long	get4();
extern unsigned	get2();
