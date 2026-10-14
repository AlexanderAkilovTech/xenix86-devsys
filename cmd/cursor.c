/* cursor - the console's cursor as a block or as a line.
 *	cursor block | line
 * The cursor's shape is the display adapter's: the first and last scan
 * lines it covers, in registers 10 and 11 of the 6845 (at 3D4 and 3D5 on a
 * CGA, 3B4 and 3B5 on an MDA or Hercules card), set here as the IBM PC's
 * BIOS sets them: a line is the cell's last two scan lines (6 and 7 of a
 * CGA's 8, 11 and 12 of a monochrome card's 14), a block runs from the top
 * of the cell to the same last line. One adapter serves all the console's
 * screens, so the shape is theirs together. xenix86-devsys system tools. */
#include <stdio.h>

#define	GIO_COLOR ('c' << 8)	/* the console: is the display a color one? */

main(argc, argv)
int argc;
char **argv;
{
	int color, port, first, last;

	if (argc != 2 || strcmp(argv[1], "block") != 0 && strcmp(argv[1], "line") != 0) {
		fprintf(stderr, "usage: cursor block | line\n");
		exit(2);
	}
	if ((color = ioctl(1, GIO_COLOR, 0)) < 0 && (color = ioctl(0, GIO_COLOR, 0)) < 0) {
		fprintf(stderr, "cursor: only on the console\n");
		exit(1);
	}
	port = color ? 0x3D4 : 0x3B4;
	last = color ? 7 : 12;
	first = argv[1][0] == 'b' ? 0 : last - 1;
	outb(port, 10);
	outb(port + 1, first);
	outb(port, 11);
	outb(port + 1, last);
	exit(0);
}
