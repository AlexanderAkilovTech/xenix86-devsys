/* nm - print the symbols of executables, object files and archives
 *
 * usage: nm [-gnopru] [file...]
 *	-g	external (global) symbols only
 *	-n	sort by value rather than by name
 *	-o	put the file's name on every line rather than once
 *	-p	print in symbol table order, unsorted
 *	-r	reverse the sort
 *	-u	undefined symbols only
 *
 * Each symbol is printed as its value in hex, a letter for its type and its
 * name: T text, D data, B bss, A absolute, C common (the value is its size),
 * U undefined; lower case for a symbol local to its file. The default file
 * is a.out. Executables have symbols unless they were linked with cc -s or
 * stripped. xenix86-devsys, K&R C. */
#include <stdio.h>
#include "objfile.h"

char	*prog = "nm";
int	gflag, nflag, oflag, pflag, rflag, uflag;
int	nfiles, errors;

/* compare two symbols for the sort */
cmp(a, b)
register struct osym *a, *b;
{
	register int c;

	/* values compare as unsigned, the way they print */
	if (nflag && a->value != b->value)
		c = (unsigned long)a->value < (unsigned long)b->value ? -1 : 1;
	else if ((c = strcmp(a->name, b->name)) == 0 && a->value != b->value)
		c = (unsigned long)a->value < (unsigned long)b->value ? -1 : 1;
	return rflag ? -c : c;
}

/* Shell's sort: no recursion, no extra memory */
sort(v, n)
struct osym *v;
int n;
{
	register int gap, i, j;
	struct osym t;

	for (gap = n / 2; gap > 0; gap /= 2)
		for (i = gap; i < n; i++)
			for (j = i - gap; j >= 0 && cmp(&v[j], &v[j + gap]) > 0; j -= gap) {
				t = v[j];
				v[j] = v[j + gap];
				v[j + gap] = t;
			}
}

/* print the symbols of what objread() read; where names it */
show(where)
char *where;
{
	register struct osym *s;
	register int i, t;

	if (o_nsyms == 0) {
		fprintf(stderr, "nm: %s: no symbols\n", where);
		errors++;
		return;
	}
	if (!pflag)
		sort(o_syms, o_nsyms);
	if (!oflag && (nfiles > 1 || objmem[0]))
		printf("\n%s:\n", where);
	for (i = 0, s = o_syms; i < o_nsyms; i++, s++) {
		t = s->type;
		if (gflag && (t < 'A' || t > 'Z'))
			continue;
		if (uflag && t != 'U' && t != 'u')
			continue;
		if (oflag)
			printf("%s: ", where);
		if (t == 'U' || t == 'u')
			printf("         %c %s\n", t, s->name);
		else
			printf("%08lx %c %s\n", s->value, t, s->name);
	}
}

file(name)
char *name;
{
	char where[100];

	switch (objopen(name)) {
	case 0:
		errors++;
		return;
	case F_ARCH:
		while (objnext()) {
			if (objread(1) < 0)
				errors++;
			else {
				sprintf(where, "%.60s(%s)", name, objmem);
				show(objmem[0] && !oflag && nfiles <= 1 ? objmem : where);
			}
		}
		break;
	default:
		if (objread(1) < 0)
			errors++;
		else
			show(name);
	}
	objclose();
}

main(argc, argv)
int argc;
char **argv;
{
	register char *a;
	register int i;

	for (i = 1; i < argc && argv[i][0] == '-' && argv[i][1]; i++)
		for (a = argv[i] + 1; *a; a++)
			switch (*a) {
			case 'g': gflag++; break;
			case 'n': nflag++; break;
			case 'o': oflag++; break;
			case 'p': pflag++; break;
			case 'r': rflag++; break;
			case 'u': uflag++; break;
			default:
				fprintf(stderr, "usage: nm [-gnopru] [file...]\n");
				exit(2);
			}
	nfiles = argc - i;
	if (nfiles == 0)
		file("a.out");
	for (; i < argc; i++)
		file(argv[i]);
	exit(errors != 0);
}
