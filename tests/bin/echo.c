/* echo: for tests in the simulator, which has no XENIX /bin */
main(argc, argv)
int argc;
char **argv;
{
	register int i;

	for (i = 1; i < argc; i++) {
		write(1, argv[i], strlen(argv[i]));
		write(1, i < argc - 1 ? " " : "\n", 1);
	}
	if (argc < 2)
		write(1, "\n", 1);
	return 0;
}
