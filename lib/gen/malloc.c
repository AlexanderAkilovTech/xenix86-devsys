/* malloc.c - xenix86-devsys libc: sbrk, malloc, free, realloc (K&R C)
 *
 * XENIX 2.1.3 (8086) small-model memory layout, measured on a running system:
 *	DS:0 [ data | bss ][ fixed stack ][ argv/envp + strings ][ heap -> ]
 * brk() below the end of the argument strings fails with EINVAL, so the heap
 * starts just above the highest argument or environment string.
 * malloc/free: first-fit free list in the style of K&R chapter 8, taking each
 * block from the front of a free area, and a realloc that shrinks in place. */

extern char **environ, **_argv;

static char *curbrk;

static char *
brkinit()
{
	register char **v, *s;
	char *top = 0;

	for (v = _argv; v && *v; v++)
		for (s = *v; ; s++)
			if (*s == 0) { if (s > top) top = s; break; }
	for (v = environ; v && *v; v++)
		for (s = *v; ; s++)
			if (*s == 0) { if (s > top) top = s; break; }
	if (top == 0)
		top = (char *)_argv;	/* no strings: the argv array is the highest thing */
	return (char *)(((unsigned)top + 16) & ~15);
}

char *
sbrk(incr)
int incr;
{
	register char *old, *new;

	if (curbrk == 0)
		curbrk = brkinit();
	old = curbrk;
	new = old + incr;
	/* compare as unsigned addresses: the data segment ends at 64K */
	if ((incr > 0 && (unsigned)new < (unsigned)old) || (incr < 0 && (unsigned)new > (unsigned)old))
		return (char *)-1;
	if (brk(new) < 0)
		return (char *)-1;
	curbrk = new;
	return old;
}

typedef struct hdr {
	struct hdr *next;	/* next free block */
	unsigned size;		/* size in units, including this header */
} HDR;

#define NALLOC	64		/* minimum units to request from sbrk */

static HDR base;
static HDR *freep;

free(ap)
char *ap;
{
	register HDR *bp, *p;

	if (ap == 0)
		return;
	bp = (HDR *)ap - 1;
	for (p = freep; !(bp > p && bp < p->next); p = p->next)
		if (p >= p->next && (bp > p || bp < p->next))
			break;
	if (bp + bp->size == p->next) {
		bp->size += p->next->size;
		bp->next = p->next->next;
	} else
		bp->next = p->next;
	if (p + p->size == bp) {
		p->size += bp->size;
		p->next = bp->next;
	} else
		p->next = bp;
	freep = p;
}

static HDR *
morecore(nu)
unsigned nu;
{
	register char *cp;
	register HDR *up;
	unsigned n, half;

	if (nu < NALLOC)
		nu = NALLOC;
	if (nu > 0xFFF0 / sizeof(HDR))
		return 0;
	n = nu * sizeof(HDR);
	/* sbrk takes an int, so a request of 32K or more goes in two halves */
	half = n > 0x7FFF ? n / 2 : n;
	if ((cp = sbrk(half)) == (char *)-1)
		return 0;
	if (half != n && sbrk(n - half) != cp + half) {
		sbrk(-(int)half);
		return 0;
	}
	up = (HDR *)cp;
	up->size = nu;
	free((char *)(up + 1));
	return freep;
}

char *
malloc(nbytes)
unsigned nbytes;
{
	register HDR *p, *prevp;
	HDR *q;
	unsigned nunits;

	if (nbytes > 0xFFF0 - sizeof(HDR))
		return 0;
	nunits = (nbytes + sizeof(HDR) - 1) / sizeof(HDR) + 1;
	if ((prevp = freep) == 0) {
		base.next = freep = prevp = &base;
		base.size = 0;
	}
	for (p = prevp->next; ; prevp = p, p = p->next) {
		if (p->size >= nunits) {
			if (p->size - nunits < 2)	/* too small to split: take it all */
				prevp->next = p->next;
			else {
				/* Take the front of the free block and leave the rest free.
				 * (K&R takes the back.) A block clipped later with realloc
				 * then frees a tail that merges with the free space after it. */
				q = p + nunits;
				q->size = p->size - nunits;
				q->next = p->next;
				prevp->next = q;
				p->size = nunits;
			}
			freep = prevp;
			return (char *)(p + 1);
		}
		if (p == freep)
			if ((p = morecore(nunits)) == 0)
				return 0;
	}
}

char *
realloc(old, nbytes)
char *old;
unsigned nbytes;
{
	register HDR *bp, *tail;
	register char *new;
	char *s, *d;
	unsigned n, nunits;

	if (old == 0)
		return malloc(nbytes);
	if (nbytes > 0xFFF0 - sizeof(HDR))
		return 0;
	bp = (HDR *)old - 1;
	nunits = (nbytes + sizeof(HDR) - 1) / sizeof(HDR) + 1;
	if (nunits <= bp->size) {
		/* shrinking (or the same): give a tail of two units or more back */
		if (bp->size - nunits >= 2) {
			tail = bp + nunits;
			tail->size = bp->size - nunits;
			bp->size = nunits;
			free((char *)(tail + 1));
		}
		return old;
	}
	n = (bp->size - 1) * sizeof(HDR);
	if ((new = malloc(nbytes)) == 0)
		return 0;
	for (s = old, d = new; n-- > 0; )
		*d++ = *s++;
	free(old);
	return new;
}
