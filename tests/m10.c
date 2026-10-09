/* Probe: how does XENIX 2.1.3 (8086) grow the data segment, and where is the stack? */

extern char _end;		/* end of bss, provided by ld86 */
extern int errno;
extern char *getsp();

static int depth;

burn(n)
int n;
{
	char pad[64];
	register int i;

	for (i = 0; i < 64; i++)
		pad[i] = n;
	if (n > 0)
		burn(n - 1);
	if (getsp() < (char *)depth || depth == 0)
		depth = (int)getsp();
	return pad[n & 63];
}

check(base, len, tag)
register char *base;
int len;
char *tag;
{
	register int i, bad = 0;

	for (i = 0; i < len; i++)
		if (base[i] != 0x5a)
			bad++;
	printf("  %s: %d of %d heap bytes damaged\n", tag, bad, len);
}

grow(len)
int len;
{
	register char *base = &_end;
	register int i, r;
	char *sp0 = getsp();

	errno = 0;
	r = brk(base + len);
	printf("brk(_end+%d = %x) -> %d, errno %d, sp %x -> %x\n",
		len, base + len, r, errno, sp0, getsp());
	if (r < 0)
		return;
	for (i = 0; i < len; i++)
		base[i] = 0x5a;
	depth = 0;
	burn(20);
	printf("  deepest stack %x\n", depth);
	check(base, len, "after deep recursion");
}

main(argc, argv)
int argc;
char **argv;
{
	printf("_end %x, sp %x, argv %x, argv[0] at %x \"%s\"\n",
		&_end, getsp(), argv, argv[0], argv[0]);
	grow(2048);
	grow(16384);
	printf("argv[0] still \"%s\"\n", argv[0]);
	grow(0x7000);
	grow(0xC000);
	printf("done\n");
	return 0;
}
