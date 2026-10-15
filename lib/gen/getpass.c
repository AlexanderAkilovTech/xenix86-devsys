/* getpass - read a password from the terminal without showing it: the
 * prompt to /dev/tty, its echo off while the line is typed, at most 8
 * characters kept (crypt reads no more). Version 7's interface.
 * Written for xenix86-devsys. */
#include <stdio.h>
#include <signal.h>
#include <termio.h>

extern char *strncpy();

char *
getpass(prompt)
char *prompt;
{
	static char pbuf[9];
	struct termio t;
	int fd, n, echo;
	int (*sig)();
	char c;

	if ((fd = open("/dev/tty", 2)) < 0)
		fd = 0;
	sig = signal(SIGINT, SIG_IGN);
	echo = ioctl(fd, TCGETA, &t) == 0 && (t.c_lflag & ECHO);
	if (echo) {
		t.c_lflag &= ~ECHO;
		ioctl(fd, TCSETAW, &t);
	}
	write(fd ? fd : 2, prompt, strlen(prompt));
	n = 0;
	while (read(fd, &c, 1) == 1 && c != '\n')
		if (n < 8)
			pbuf[n++] = c;
	pbuf[n] = 0;
	write(fd ? fd : 2, "\n", 1);
	if (echo) {
		t.c_lflag |= ECHO;
		ioctl(fd, TCSETAW, &t);
	}
	signal(SIGINT, sig);
	if (fd)
		close(fd);
	return pbuf;
}
