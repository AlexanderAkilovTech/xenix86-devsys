/* bigmem: requests of 32K and more (sbrk takes an int, so malloc grows the heap in halves) */
#include <stdio.h>
extern char *malloc(), *sbrk();

main()
{
	unsigned i;
	char *p, *q;

	for (i = 0xE000; i != 0; i -= 512)	/* the way ld86 sizes its symbol table */
		if ((p = malloc(i)) != 0)
			break;
	q = malloc(100);
	printf("largest block %u bytes; a later malloc(100) %s\n", i,
		q > p + i ? "lies above it" : "OVERLAPS IT");
	p[0] = 1; p[i - 1] = 2;
	return !(i >= 0x8000 && q > p + i);
}
