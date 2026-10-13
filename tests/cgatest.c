/* cgatest - what XENIX 2.1.3's console does with color, and whether a
 * program can put a CGA into color graphics.
 *
 *	cgatest		numbered rows of color escape sequences, each put back
 *			to white on black after, to see which come out in color
 *	cgatest -g	then 320 x 200 four-color graphics: the CGA's
 *			registers set from the program, the picture written
 *			straight to the video memory at B800:0, and the 80 x 25
 *			text mode put back after
 *	cgatest -r	only put the console back to white on black and clear
 *			it: for a screen left in colors
 *
 * What the first run showed (M32): the console takes ESC[3nm and ESC[4nm,
 * foreground and background, in the ANSI order (1 red, 2 green, 3 brown,
 * 4 blue, 5 magenta, 6 cyan, 7 white); it acts on only the first number of
 * a sequence (ESC[1;31m is ESC[1m); ESC[0m does not put the colors back;
 * and ESC[=nF and ESC[=nG, SCO's later color sequences, are taken as
 * ESC[nF and ESC[nG, which move the cursor. The BIOS's data at 400 is gone:
 * XENIX's kernel has that memory. With -g the CGA took the graphics mode
 * and its palettes, but the picture never reached it: /dev/mem ends with
 * the memory XENIX has, before the video memory, so now it is copied
 * there with ES loaded with B800 (tests/cgaio.s), which nothing on an
 * 8086 forbids.
 *
 * XENIX 2.1.3's console driver has no graphics modes of its own; its
 * <sys/console.h> has only GIO_COLOR ('c' << 8), which says whether the
 * display is a color one, and GIO_ATTR ('a' << 8). The register values
 * are the IBM BIOS's for modes 4 (320 x 200) and 3 (80 x 25 text).
 * xenix86-devsys test program. */
#include <stdio.h>
#include <termio.h>

#define	GIO_COLOR	('c' << 8)
#define	GIO_ATTR	('a' << 8)

int	mode4[16] = { 0x38, 0x28, 0x2D, 0x0A, 0x7F, 0x06, 0x64, 0x70,
		      0x02, 0x01, 0x06, 0x07, 0x00, 0x00, 0x00, 0x00 };
int	mode3[16] = { 0x71, 0x50, 0x5A, 0x0A, 0x1F, 0x06, 0x19, 0x1C,
		      0x02, 0x07, 0x06, 0x07, 0x00, 0x00, 0x00, 0x00 };
char	screen[16384];		/* the CGA's 16K: even lines, then odd at 8K */
struct termio tsave, traw;
extern int errno;

char	*colors[] = { "black", "red", "green", "brown", "blue", "magenta", "cyan", "white" };

/* white on black again: one number to a sequence, as the console wants */
plain()
{
	printf("\033[0m\033[37m\033[40m");
}

/* the console's color, as escape sequences: one numbered row each */
rows()
{
	int i;

	printf("\nWhich rows below show color?\n\n");
	printf(" 1 ESC[3nm     ");
	for (i = 0; i < 8; i++) {
		printf("\033[3%dm %d ", i, i);
		plain();
	}
	printf("\n 2 ESC[4nm     ");
	for (i = 0; i < 8; i++) {
		printf("\033[4%dm %d ", i, i);
		plain();
	}
	printf("\n 3 ESC[1m ESC[3nm ");
	for (i = 0; i < 8; i++) {
		printf("\033[1m\033[3%dm%d ", i, i);
		plain();
	}
	printf("\n 4 ESC[3nm ESC[4nm ");
	for (i = 0; i < 8; i++) {
		printf("\033[3%dm\033[4%dm%d ", 7 - i, i, i);
		plain();
	}
	printf("\n 5 ESC[1;31m   \033[1;31mtwo numbers in one");
	plain();
	printf("\n 6 ESC[0m      \033[31mred, then ESC[0m:\033[0m still red?");
	plain();
	printf("\n 7 attributes  \033[1mbold");
	plain();
	printf(" \033[4munderline");
	plain();
	printf(" \033[5mblink");
	plain();
	printf(" \033[7mreverse");
	plain();
	printf(" \033[2mdim");
	plain();
	printf(" end\n\nColors 0 to 7: ");
	for (i = 0; i < 8; i++)
		printf("%d %s%s", i, colors[i], i < 7 ? ", " : "\n");
}

/* a point of color c (0 to 3) at x, y */
plot(x, y, c)
int x, y, c;
{
	register char *p = screen + (y & 1) * 8192 + (y >> 1) * 80 + (x >> 2);
	int s = (3 - (x & 3)) * 2;

	*p = (*p & ~(3 << s)) | (c << s);
}

key()
{
	char c;

	read(0, &c, 1);
}

graphics()
{
	int i, x, y;
	char back[16];

	printf("\nGraphics: the screen goes to 320 x 200 in four colors; press a key\n");
	printf("to step through: palette 1, palette 0, then palette 1 on blue. If the\n");
	printf("text screen is wrong after, press Alt-F2 then Alt-F1, or type clear.\n");
	printf("Press Return to start: ");
	fflush(stdout);
	getchar();
	ioctl(0, TCGETA, &tsave);
	traw = tsave;
	traw.c_lflag &= ~(ICANON | ECHO);
	traw.c_cc[VMIN] = 1;
	traw.c_cc[VTIME] = 0;
	ioctl(0, TCSETA, &traw);

	/* the picture: four bars of colors 0 to 3, reversed in the lower
	 * half, a frame and a diagonal in color 3 */
	for (y = 0; y < 200; y++)
		for (x = 0; x < 320; x++)
			plot(x, y, y < 100 ? x / 80 : 3 - x / 80);
	for (x = 0; x < 320; x++) {
		plot(x, 0, 3);
		plot(x, 199, 3);
		plot(x, (int)((long)x * 199 / 319), 3);
	}
	for (y = 0; y < 200; y++) {
		plot(0, y, 3);
		plot(319, y, 3);
	}

	outb(0x3D8, 0x02);		/* graphics, the display off */
	for (i = 0; i < 16; i++)
		crtc(i, mode4[i]);
	outb(0x3D9, 0x30);		/* palette 1 (cyan, magenta, white), bright */
	farput(0xB800, 0, screen, sizeof screen);
	outb(0x3D8, 0x0A);		/* graphics, color, the display on */
	key();
	outb(0x3D9, 0x10);		/* palette 0 (green, red, yellow), bright */
	key();
	outb(0x3D9, 0x31);		/* palette 1, color 0 blue */
	key();
	farget(0xB800, 0, back, sizeof back);

	/* back to 80 x 25 text: the registers, and the memory cleared to
	 * spaces in white on black */
	outb(0x3D8, 0x21);
	for (i = 0; i < 16; i++)
		crtc(i, mode3[i]);
	outb(0x3D9, 0x30);
	for (i = 0; i < sizeof screen; i += 2) {
		screen[i] = ' ';
		screen[i + 1] = 0x07;
	}
	farput(0xB800, 0, screen, sizeof screen);
	outb(0x3D8, 0x29);
	ioctl(0, TCSETA, &tsave);
	plain();
	for (i = 0; i < 25; i++)
		printf("\n");
	printf("\033[2J\033[H");
	printf("Back in text mode. The video memory read back %s what was written.\n",
		back[0] == (char)0xff ? "as" : "differently from");
	printf("(first bytes %02x %02x %02x %02x)\n", back[0] & 0377, back[1] & 0377,
		back[2] & 0377, back[3] & 0377);
}

main(argc, argv)
int argc;
char **argv;
{
	int r;

	if (argc > 1 && strcmp(argv[1], "-r") == 0) {
		plain();
		printf("\033[2J\033[H");
		exit(0);
	}
	r = ioctl(1, GIO_COLOR, 0);
	printf("ioctl GIO_COLOR: %d", r);
	if (r < 0)
		printf(" (errno %d)", errno);
	printf("  (nonzero: the console driver thinks the display is color)\n");
	r = ioctl(1, GIO_ATTR, 0);
	printf("ioctl GIO_ATTR: %d", r);
	if (r < 0)
		printf(" (errno %d)", errno);
	printf("  (the character attribute in use)\n");
	rows();
	if (argc > 1 && strcmp(argv[1], "-g") == 0)
		graphics();
	exit(0);
}
