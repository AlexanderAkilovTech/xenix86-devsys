/* tz - compiled on XENIX by the installed cc and libc (M64): the times of
 * 2026's daylight time changes, and the ends of signed and unsigned 32-bit
 * time, as the library's ctime gives them, with the zone's name. */
#include <stdio.h>
#include <time.h>

long	t[] = { 1772963999L, 1772964000L, 1793523599L, 1793523600L, 0x7FFFFFFFL, 0x80000000L, 0xFFFFFFFFL };

main()
{
	int i;
	char *s;

	for (i = 0; i < sizeof t / sizeof t[0]; i++) {
		s = ctime(&t[i]);
		printf("%.24s %s\n", s, tzname[localtime(&t[i])->tm_isdst]);
	}
	exit(0);
}
