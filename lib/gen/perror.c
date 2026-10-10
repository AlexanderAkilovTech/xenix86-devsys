/* perror.c - xenix86-devsys libc (K&R C): error messages for errno */

extern int errno;

char	*sys_errlist[] = {
	"Error 0",
	"Not owner",
	"No such file or directory",
	"No such process",
	"Interrupted system call",
	"I/O error",
	"No such device or address",
	"Arg list too long",
	"Exec format error",
	"Bad file number",
	"No child processes",
	"No more processes",
	"Not enough space",
	"Permission denied",
	"Bad address",
	"Block device required",
	"Device busy",
	"File exists",
	"Cross-device link",
	"No such device",
	"Not a directory",
	"Is a directory",
	"Invalid argument",
	"File table overflow",
	"Too many open files",
	"Not a typewriter",
	"Text file busy",
	"File too large",
	"No space left on device",
	"Illegal seek",
	"Read-only file system",
	"Too many links",
	"Broken pipe",
	"Argument out of domain",
	"Result too large",
};
int	sys_nerr = sizeof sys_errlist / sizeof sys_errlist[0];

static
put(s)
register char *s;
{
	register char *p = s;

	while (*p)
		p++;
	write(2, s, p - s);
}

perror(s)
char *s;
{
	register int e = errno;

	if (s && *s) {
		put(s);
		put(": ");
	}
	put(e >= 0 && e < sys_nerr ? sys_errlist[e] : "Unknown error");
	put("\n");
}
