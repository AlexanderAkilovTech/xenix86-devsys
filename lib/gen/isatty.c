/* isatty.c - xenix86-devsys libc (K&R C) */

#define TCGETA	(('T' << 8) | 1)

isatty(fd)
int fd;
{
	char tbuf[20];

	return ioctl(fd, TCGETA, tbuf) >= 0;
}
