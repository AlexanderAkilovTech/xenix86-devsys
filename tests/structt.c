/* structs: passed by value (directly and through a function pointer),
   assigned, returned, nested; long fields and odd sizes */
#include <stdio.h>
#include <string.h>

struct md { char *name; int mode; long date; unsigned size; long off; char tag[5]; };
struct pt { int x, y; };
struct box { struct pt a, b; char c; };
int bad;

check(what, cond)
char *what;
int cond;
{
	if (!cond) {
		printf("FAIL %s\n", what);
		bad++;
	}
}

byval(m, x)
struct md m;
int x;
{
	check("value: name", !strcmp(m.name, "member.o"));
	check("value: fields", m.mode == 0644 && m.date == 1791594085L && m.size == 40000 && m.off == 70000L);
	check("value: array in struct", !strcmp(m.tag, "abcd"));
	check("value: following argument", x == 3 || x == 4 || x == 5);
	m.mode = 0;		/* changes the copy only */
	return m.size == 40000;
}

struct pt
mkpt(x, y)
int x, y;
{
	struct pt p;

	p.x = x;
	p.y = y;
	return p;
}

struct box
grow(b, d)
struct box b;
int d;
{
	b.a.x -= d; b.a.y -= d;
	b.b.x += d; b.b.y += d;
	return b;
}

main()
{
	struct md m, m2;
	struct pt p;
	struct box b, b2;
	int (*fn)();
	char name[9];

	strcpy(name, "member.o");
	m.name = name; m.mode = 0644; m.date = 1791594085L; m.size = 40000; m.off = 70000L;
	strcpy(m.tag, "abcd");
	check("call by value", byval(m, 3));
	check("caller's copy unchanged", m.mode == 0644);
	fn = byval;
	check("through a pointer", fn(m, 4) && (*fn)(m, 5));
	m2 = m;
	check("assignment", m2.date == m.date && m2.off == 70000L && !strcmp(m2.tag, "abcd"));
	p = mkpt(-7, 9);
	check("returned by value", p.x == -7 && p.y == 9);
	b.a = mkpt(1, 2);
	b.b = mkpt(3, 4);
	b.c = 'z';
	b2 = grow(b, 10);
	check("nested, passed and returned", b2.a.x == -9 && b2.a.y == -8 && b2.b.x == 13 && b2.b.y == 14 && b2.c == 'z');
	check("original box unchanged", b.a.x == 1 && b.b.y == 4);
	printf("%d struct checks failed\n", bad);
	return bad != 0;
}
