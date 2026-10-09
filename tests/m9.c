/* Milestone 2: the first C program built by the xenix86-devsys toolchain. */

extern char *getenv();

main(argc, argv, envp)
int argc;
char **argv, **envp;
{
	register int i;
	char *home, *term;

	printf("hello, world - from C on XENIX 2.1.3\n");
	printf("argc = %d\n", argc);
	for (i = 0; i < argc; i++)
		printf("argv[%d] = \"%s\"\n", i, argv[i]);
	home = getenv("HOME");
	term = getenv("TERM");
	printf("HOME = %s, TERM = %s\n", home ? home : "(unset)", term ? term : "(unset)");
	printf("my pid is %d, uid %d\n", getpid(), getuid());
	printf("hex %x, octal %o, padded [%5d] [%-5d] [%05d]\n", 0xBEEF, 8, 42, 42, 42);
	return 7;
}
