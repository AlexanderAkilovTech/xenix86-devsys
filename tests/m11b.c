/* realloc: shrinking returns the tail to the free list; growing keeps the contents */
#include <stdio.h>
extern char *malloc(), *realloc(), *sbrk();

main()
{
	register int i, j;
	char *p[200], *start, *q;
	int bad = 0;

	start = sbrk(0);
	/* the bcc-cpp pattern: allocate generously, fill, then clip to the real size */
	for (i = 0; i < 200; i++) {
		p[i] = malloc(140);
		for (j = 0; j < 10; j++)
			p[i][j] = i + j;
		p[i] = realloc(p[i], 10);
	}
	for (i = 0; i < 200; i++)
		for (j = 0; j < 10; j++)
			if (p[i][j] != (char)(i + j))
				bad++;
	printf("200 clipped blocks used %u bytes of heap (unclipped would be over 28000)\n",
		(unsigned)(sbrk(0) - start));
	q = malloc(8);
	for (i = 0; i < 8; i++)
		q[i] = 'a' + i;
	q = realloc(q, 3000);
	for (i = 0; i < 8; i++)
		if (q[i] != 'a' + i)
			bad++;
	printf("grown block keeps its contents; damaged bytes: %d\n", bad);
	return bad != 0 || sbrk(0) - start > 8000;
}
