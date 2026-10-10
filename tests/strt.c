/* string routines (lib/gen/*.s, 8086 string instructions) against reference C versions */
#include <stdio.h>

extern char *strcpy(), *strcat(), *memcpy(), *memset();

static unsigned long seed = 1;
rnd(n)
int n;
{
	seed = seed * 1103515245L + 12345;
	return (int)((seed >> 16) & 0x7fff) % n;
}

r_strlen(s) char *s; { int n = 0; while (s[n]) n++; return n; }
sgn(x) int x; { return x < 0 ? -1 : x > 0; }
r_strcmp(a, b)
char *a, *b;
{
	while (*a && *a == *b)
		a++, b++;
	return sgn((*a & 0377) - (*b & 0377));
}
r_strncmp(a, b, n)
char *a, *b;
int n;
{
	for (; n > 0; n--, a++, b++)
		if (*a != *b || *a == 0)
			return sgn((*a & 0377) - (*b & 0377));
	return 0;
}

/* a random string of length 0..max over a small alphabet, sometimes with bytes over 127 */
mk(s, max)
char *s;
int max;
{
	register int i, n = rnd(max + 1);

	for (i = 0; i < n; i++)
		s[i] = rnd(4) ? 'a' + rnd(3) : 0200 + rnd(3);
	s[n] = 0;
}

main()
{
	char a[80], b[80], c[200], d[200];
	register int i, j, n, bad = 0, tests = 0;
	char *r;

	for (i = 0; i < 3000; i++) {
		mk(a, 20);
		mk(b, 20);
		if (rnd(3) == 0)
			strcpy(b, a);		/* equal strings */
		if (rnd(4) == 0 && a[0])
			b[rnd(r_strlen(a) + 1)] = 0;	/* prefixes */
		tests += 6;
		if (strlen(a) != r_strlen(a)) bad++;
		if (sgn(strcmp(a, b)) != r_strcmp(a, b)) bad++;
		n = rnd(25);
		if (sgn(strncmp(a, b, n)) != r_strncmp(a, b, n)) bad++;
		/* strcpy and strcat: right bytes, right return value, nothing past the end */
		memset(c, 'X', 200);
		r = strcpy(c + 1, a);
		if (r != c + 1 || r_strcmp(c + 1, a) || c[0] != 'X' || c[2 + r_strlen(a)] != 'X') bad++;
		r = strcat(c + 1, b);
		if (r != c + 1 || r_strlen(c + 1) != r_strlen(a) + r_strlen(b) || r_strcmp(c + 1 + r_strlen(a), b)
		    || c[2 + r_strlen(a) + r_strlen(b)] != 'X') bad++;
		/* memcpy and memset, including n = 0 */
		n = rnd(60);
		for (j = 0; j < 200; j++) d[j] = j;
		r = memcpy(d + 5, c, n);
		for (j = 0; j < 200; j++)
			if (d[j] != ((j >= 5 && j < 5 + n) ? c[j - 5] : (char)j)) { bad++; break; }
		if (r != d + 5) bad++;
		r = memset(d + 7, 0201, n);
		for (j = 7; j < 7 + n; j++)
			if (d[j] != (char)0201) { bad++; break; }
		if (r != d + 7) bad++;
	}
	printf("%d string tests, %d failures\n", tests, bad);
	return bad != 0;
}
