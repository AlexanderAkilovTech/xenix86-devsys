/* asuser - run a command as another user, from root, to see what it says to
 * someone who is not the super-user (M63).
 *	asuser uid command [ args ]
 * xenix86-devsys test program. */
#include <stdio.h>

main(argc, argv)
int argc;
char **argv;
{
	if (argc < 3) {
		fprintf(stderr, "usage: asuser uid command [ args ]\n");
		exit(2);
	}
	if (setuid(atoi(argv[1])) < 0) {
		perror("asuser: setuid");
		exit(2);
	}
	execv(argv[2], argv + 2);
	perror(argv[2]);
	exit(2);
}
