/* symt.c - a program with a symbol table, for trying nm and adb on XENIX:
 * a global variable, a table, and a function called in a loop. Prints 86. */
#include <stdio.h>

int counter = 5;
int table[10];

square(x)
int x;
{
	return x * x;
}

main()
{
	int i;

	for (i = 0; i < 10; i++)
		table[i] = square(i) + counter;
	printf("%d\n", table[9]);
	return 0;
}
