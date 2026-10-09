/* Milestone 3 probe: sbrk/malloc on XENIX 2.1.3 (8086) */

extern char *malloc(), *sbrk(), *getsp();

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
	return pad[n & 63];
}

fill(p, n, v)
register char *p;
register unsigned n;
int v;
{
	while (n-- > 0)
		*p++ = v;
}

bad(p, n, v)
register char *p;
register unsigned n;
int v;
{
	register int b = 0;

	while (n-- > 0)
		if (*p++ != v)
			b++;
	return b;
}

main(argc, argv)
int argc;
char **argv;
{
	char *a, *b, *c, *last;
	unsigned blocks;

	printf("sp %x, heap starts at %x\n", getsp(), sbrk(0));
	a = malloc(1000);
	b = malloc(5000);
	printf("malloc(1000) = %x, malloc(5000) = %x, break now %x\n", a, b, sbrk(0));
	fill(a, 1000, 0x11);
	fill(b, 5000, 0x22);
	burn(30);
	printf("after deep recursion: a damaged %d, b damaged %d\n", bad(a, 1000, 0x11), bad(b, 5000, 0x22));
	free(a);
	c = malloc(600);
	printf("free(a); malloc(600) = %x (%s)\n", c, (c >= a && c < a + 1000) ? "reused a's block" : "new block");
	blocks = 0;
	last = 0;
	while ((c = malloc(4096)) != 0) {
		fill(c, 4096, 0x33);
		last = c;
		blocks++;
	}
	printf("heap filled: %u more 4K blocks, last at %x, final break %x\n", blocks, last, sbrk(0));
	printf("b still intact: damaged %d; argv[0] \"%s\"\n", bad(b, 5000, 0x22), argv[0]);
	printf("done\n");
	return 0;
}
