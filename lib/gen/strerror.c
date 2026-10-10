/* strerror.c - xenix86-devsys libc (K&R C) */

extern char *sys_errlist[];
extern int sys_nerr;

char *
strerror(e)
int e;
{
	return e >= 0 && e < sys_nerr ? sys_errlist[e] : "Unknown error";
}
