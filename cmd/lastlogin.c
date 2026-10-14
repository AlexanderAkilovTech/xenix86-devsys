/* lastlogin - say when you last logged in, and on which terminal, as
 * 4.2BSD's login did: "Last login: Sun Oct 11 13:50:12 on tty02". XENIX's
 * login does not, so the start-up files mkuser gives new accounts (.login,
 * .profile) run this. It finds this login in /etc/utmp, as the entry of the
 * shell that ran it, keeps it in /usr/adm/lastlog, and tells the one before.
 * XENIX's /etc/wtmp is no help: it held only the logins since the machine
 * last started (M60).
 * Set-user-id to bin, which owns /usr/adm/lastlog; it writes only the
 * record of the user who runs it. xenix86-devsys system tools. */
#include <stdio.h>

#define	UTMP	"/etc/utmp"
#define	LASTLOG	"/usr/adm/lastlog"
#define	USER_PROCESS 7		/* utmp's ut_type for a logged-in user */
#define	UTSIZE	36		/* a System V utmp entry: user 8, id 4, line 12,
				 * pid 2, type 2, exit 4, time 4 */

/* a user's record in LASTLOG, at uid times its size: the login before this
 * one, and this one */
struct ll {
	long	wtime;
	char	wline[8];
	long	ntime;
	char	nline[8];
};

extern char *ctime(), *strncpy();
extern long lseek();

long	get4(p) unsigned char *p; { return p[0] | p[1] << 8 | (long)p[2] << 16 | (long)p[3] << 24; }
unsigned get2(p) unsigned char *p; { return p[0] | p[1] << 8; }

main()
{
	unsigned char u[UTSIZE];
	char line[8];
	struct ll r;
	int fd, ppid, found = 0;
	long off, t;

	/* this login: the shell running us is the one login started, so its
	 * utmp entry is a USER_PROCESS with its pid */
	ppid = getppid();
	if ((fd = open(UTMP, 0)) >= 0) {
		while (read(fd, (char *)u, UTSIZE) == UTSIZE)
			if (get2(u + 26) == USER_PROCESS && get2(u + 24) == ppid) {
				t = get4(u + 32);
				strncpy(line, (char *)u + 12, 8);
				found = 1;
				break;
			}
		close(fd);
	}
	off = (long)getuid() * sizeof r;
	if ((fd = open(LASTLOG, 2)) < 0)
		fd = open(LASTLOG, 0);
	if (fd < 0 || lseek(fd, off, 0) != off || read(fd, (char *)&r, sizeof r) != sizeof r)
		memset((char *)&r, 0, sizeof r);
	/* a new login: the last one becomes the one before */
	if (found && r.ntime != t) {
		r.wtime = r.ntime;
		memcpy(r.wline, r.nline, 8);
		r.ntime = t;
		memcpy(r.nline, line, 8);
		if (fd >= 0 && lseek(fd, off, 0) == off)
			write(fd, (char *)&r, sizeof r);
	}
	if (fd >= 0)
		close(fd);
	if (r.wtime)
		printf("Last login: %.19s on %.8s\n", ctime(&r.wtime), r.wline);
	exit(0);
}
