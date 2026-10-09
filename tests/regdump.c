/* regdump - print the raw registers each argument-less system call returns.
   Build: ./build.sh tests/regdump.c tests/regdump.s */
extern unsigned rax, rbx, rcx, rdx, rcf;

show(n, name)
int n;
char *name;
{
	rawsys(n);
	printf("%-7s (%2d): ax %04x  bx %04x  cx %04x  dx %04x  carry %d\n",
		name, n, rax, rbx, rcx, rdx, rcf);
}

main()
{
	show(13, "time");
	show(20, "getpid");
	show(24, "getuid");
	show(47, "getgid");
	printf("(1111/2222/3333 means the kernel left that register alone)\n");
	return 0;
}
