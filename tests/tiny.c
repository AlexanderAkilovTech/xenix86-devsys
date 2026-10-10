/* tiny: uses no stdio, so libc.a should leave stdio out of the program entirely */
main()
{
	write(1, "tiny: no stdio linked\n", 22);
	return 3;
}
