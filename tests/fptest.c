/* fptest.c - drives the floating point routines through compiled C, for
 * tests/unit/test_fp.py. Each input line is an operation and operands in
 * hex (16 digits for a double, 8 for a float or a long); each output line
 * is the results in hex, or 0/1 for comparisons. The operations are written
 * so that every helper the compiler calls gets used: with an operand in
 * memory (Faddd ...), and with both on the stack (Fadd ..., for results of
 * function calls). */
#include <stdio.h>

union dbl {
	double d;
	unsigned w[4];
};

union flt {
	float f;
	unsigned w[2];
};

double fopf(), fneg();

char line[200];
char *lp;

unsigned long
hex(n)
{
	unsigned long v;
	int c;

	while (*lp == ' ')
		lp++;
	v = 0;
	while (n-- > 0) {
		c = *lp++;
		v = (v << 4) | (c <= '9' ? c - '0' : c - 'a' + 10);
	}
	return v;
}

getd(u)
union dbl *u;
{
	u->w[3] = hex(4);
	u->w[2] = hex(4);
	u->w[1] = hex(4);
	u->w[0] = hex(4);
}

getf(u)
union flt *u;
{
	u->w[1] = hex(4);
	u->w[0] = hex(4);
}

putd(d)
double d;
{
	union dbl u;

	u.d = d;
	printf(" %04x%04x%04x%04x", u.w[3], u.w[2], u.w[1], u.w[0]);
}

putf(u)
union flt *u;
{
	printf(" %04x%04x", u->w[1], u->w[0]);
}

double
idd(d)
double d;
{
	return d;
}

putl(l)
unsigned long l;
{
	printf(" %08lx", l);
}

main()
{
	union dbl x, y;
	union flt p, q, s;
	double d;
	float f;
	int i;
	unsigned u;
	long l;
	unsigned long ul;
	char c;
	signed char sc;
	unsigned char uc;
	int sp0;

	while (fgets(line, sizeof line, stdin)) {
		sp0 = getsp();		/* each helper must leave the stack balanced */
		lp = line + 1;
		printf("%c", line[0]);
		switch (line[0]) {
		case 'a':		/* arithmetic on two doubles */
			getd(&x);
			getd(&y);
			putd(x.d + y.d);
			putd(x.d - y.d);
			putd(x.d * y.d);
			putd(x.d / y.d);
			putd(idd(x.d) + idd(y.d));
			putd(idd(x.d) - idd(y.d));
			putd(idd(x.d) * idd(y.d));
			putd(idd(x.d) / idd(y.d));
			break;
		case 'c':		/* comparisons of two doubles */
			getd(&x);
			getd(&y);
			printf(" %d%d%d%d%d%d", x.d < y.d, x.d <= y.d, x.d == y.d,
				x.d != y.d, x.d > y.d, x.d >= y.d);
			printf(" %d%d%d%d%d%d", idd(x.d) < idd(y.d), idd(x.d) <= idd(y.d),
				idd(x.d) == idd(y.d), idd(x.d) != idd(y.d),
				idd(x.d) > idd(y.d), idd(x.d) >= idd(y.d));
			printf(" %d%d%d", !x.d, x.d ? 1 : 0, !idd(x.d));
			break;
		case 'd':		/* one double, converted */
			getd(&x);
			putd(-x.d);
			putd(-idd(x.d));
			s.f = x.d;
			putf(&s);
			i = x.d;
			u = x.d;
			l = x.d;
			ul = x.d;
			c = x.d;
			uc = x.d;
			printf(" %04x %04x", i, u);
			putl(l);
			putl(ul);
			printf(" %02x %02x", c, uc);
			break;
		case 'f':		/* two floats */
			getf(&p);
			getf(&q);
			s.f = p.f + q.f;
			putf(&s);
			s.f = p.f - q.f;
			putf(&s);
			s.f = p.f * q.f;
			putf(&s);
			s.f = p.f / q.f;
			putf(&s);
			putd(p.f);
			s.f = -p.f;
			putf(&s);
			printf(" %d%d%d%d%d%d", p.f < q.f, p.f <= q.f, p.f == q.f,
				p.f != q.f, p.f > q.f, p.f >= q.f);
			printf(" %d%d", !p.f, p.f ? 1 : 0);
			i = p.f;
			u = p.f;
			l = p.f;
			ul = p.f;
			printf(" %04x %04x", i, u);
			putl(l);
			putl(ul);
			d = p.f;
			d = d + q.f;
			putd(d);
			d = p.f;
			d = d * q.f;
			putd(d);
			break;
		case 'i':		/* integers to double */
			ul = hex(8);
			i = ul;
			u = ul;
			l = ul;
			c = ul;
			sc = ul;
			uc = ul;
			putd((double)i);
			putd((double)u);
			putd((double)l);
			putd((double)ul);
			putd((double)c);
			putd((double)sc);
			putd((double)uc);
			s.f = l;
			putf(&s);
			break;
		case 'h':		/* the helpers BCC does not call itself */
			getd(&x);
			getf(&p);
			putd(fopf(0, x.d, &p.f));
			putd(fopf(1, x.d, &p.f));
			putd(fopf(2, x.d, &p.f));
			putd(fopf(3, x.d, &p.f));
			printf(" %d %d", fcmpf(x.d, &p.f), ftst(x.d));
			putd(fneg(x.d));
			break;
		case 'p':		/* ++ and -- */
			getd(&x);
			d = x.d;
			putd(d++);
			putd(d);
			d = x.d;
			putd(++d);
			d = x.d;
			putd(d--);
			putd(d);
			d = x.d;
			putd(--d);
			f = x.d;
			putd(f++);
			s.f = f;
			putf(&s);
			f = x.d;
			putd(--f);
			s.f = f;
			putf(&s);
			break;
		}
		if (getsp() != sp0)
			printf(" stack moved %d", getsp() - sp0);
		printf("\n");
	}
	exit(0);
}
