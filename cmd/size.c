/* size - print the sizes of executables, object files and archive members
 *
 * usage: size [-o] [-x] [file...]
 *	-o	in octal
 *	-x	in hexadecimal
 *
 * For each, the sizes of its text, data and bss, and their sum:
 * "text + data + bss = total". For an object file, bss is the storage its
 * common and local uninitialized variables ask for. The default file is
 * a.out. xenix86-devsys, K&R C. */
#include <stdio.h>
#include "objfile.h"

char	*prog = "size";
char	*fmt = "%ld + %ld + %ld = %ld\n";
int	nfiles, errors;

file(name)
char *name;
{
	int k;

	if ((k = objopen(name)) == 0) {
		errors++;
		return;
	}
	while (k != F_ARCH || objnext()) {
		if (objread(0) < 0)
			errors++;
		else {
			if (k == F_ARCH)
				printf("%s(%s): ", name, objmem);
			else if (nfiles > 1)
				printf("%s: ", name);
			printf(fmt, o_text, o_data, o_bss, o_text + o_data + o_bss);
		}
		if (k != F_ARCH)
			break;
	}
	objclose();
}

main(argc, argv)
int argc;
char **argv;
{
	register int i;

	for (i = 1; i < argc && argv[i][0] == '-' && argv[i][1]; i++)
		if (argv[i][1] == 'o' && !argv[i][2])
			fmt = "0%lo + 0%lo + 0%lo = 0%lo\n";
		else if (argv[i][1] == 'x' && !argv[i][2])
			fmt = "0x%lx + 0x%lx + 0x%lx = 0x%lx\n";
		else {
			fprintf(stderr, "usage: size [-o] [-x] [file...]\n");
			exit(2);
		}
	nfiles = argc - i;
	if (nfiles == 0)
		file("a.out");
	for (; i < argc; i++)
		file(argv[i]);
	exit(errors != 0);
}
