/* execvp.c - xenix86-devsys libc (K&R C) */

extern int errno;

/* execvp: search $PATH when the name has no slash */
execvp(file, argv)
char *file, **argv;
{
	extern char *getenv(), *strchr();
	register char *path, *d;
	char buf[128];
	register int n;

	if (strchr(file, '/'))
		return execv(file, argv);
	if ((path = getenv("PATH")) == 0)
		path = ":/bin:/usr/bin";
	for (;;) {
		d = buf;
		while (*path && *path != ':')
			*d++ = *path++;
		if (d != buf)
			*d++ = '/';
		for (n = 0; file[n]; n++)
			*d++ = file[n];
		*d = 0;
		execv(buf, argv);
		if (errno == 8) {	/* ENOEXEC: hand it to the shell */
			argv[-1] = "sh";
			argv[0] = buf;
			return execv("/bin/sh", argv - 1);
		}
		if (*path == 0)
			return -1;
		path++;
	}
}
