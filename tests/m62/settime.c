/* settime - find out how XENIX 2.1.3's stime (system call 25) takes the time,
 * and set the clock past 1999, which XENIX's date cannot (M62).
 *	settime probe		which way works, the clock put back as it was
 *	settime seconds		the clock to that many seconds since 1970
 *	settime jump seconds	the same, keeping in clock.was what it was
 *	settime back		the clock back to what it was, and the time since
 *	settime			the clock now, in seconds and as ctime says it
 * stime is tried first with the seconds in BX (low word) and CX (high), as
 * time (13) returns them in AX and BX; if the clock does not move as it
 * should, with BX a pointer to them. times() gives the ticks since the
 * machine started, whatever the clock says, so the clock can always be put
 * back. xenix86-devsys test program. */
#include <stdio.h>
#include <sys/types.h>
#include <sys/times.h>

extern long time(), times(), atol();
extern char *ctime();

long	t0, k0;
struct tms tb;

/* the clock as it would be now, had it not been touched */
long
now()
{
	return t0 + (times(&tb) - k0 + 10) / 20;
}

bylong(t)
long t;
{
	return sys25((unsigned)t, (unsigned)(t >> 16));
}

byptr(t)
long t;
{
	return sys25((char *)&t, 0);
}

main(argc, argv)
int argc;
char **argv;
{
	long t, d;
	int rc;

	if (argc == 1) {
		t = time((long *)0);
		printf("%ld %s", t, ctime(&t));
		exit(0);
	}
	t0 = time((long *)0);
	k0 = times(&tb);
	if (strcmp(argv[1], "back") == 0) {
		FILE *f = fopen("clock.was", "r");

		if (f == 0 || fscanf(f, "%ld %ld", &t0, &k0) != 2) {
			printf("settime: no clock.was\n");
			exit(1);
		}
		t = now();
	} else if (strcmp(argv[1], "jump") == 0 && argc == 3) {
		FILE *f = fopen("clock.was", "w");

		fprintf(f, "%ld %ld\n", t0, k0);
		fclose(f);
		t = atol(argv[2]);
	} else if (strcmp(argv[1], "probe") != 0)
		t = atol(argv[1]);
	if (strcmp(argv[1], "probe") != 0) {
		rc = bylong(t);
		d = time((long *)0) - t;
		if (rc < 0 || d < -2 || d > 2) {
			rc = byptr(t);
			d = time((long *)0) - t;
		}
		if (rc < 0 || d < -2 || d > 2) {
			printf("settime: the clock did not take it (stime %d, off by %ld); put back\n", rc, d);
			bylong(now());
			byptr(now());
			exit(1);
		}
		t = time((long *)0);
		printf("%ld %s", t, ctime(&t));
		exit(0);
	}
	/* an hour on, in BX and CX */
	rc = bylong(t0 + 3600);
	d = time((long *)0) - now();
	printf("stime, the seconds in BX and CX: returned %d, the clock moved %ld s\n", rc, d);
	if (d >= 3595 && d <= 3605) {
		bylong(now());
		printf("so: the seconds in BX (low word) and CX (high); put back, now %ld s off\n",
			time((long *)0) - now());
		exit(0);
	}
	/* by pointer */
	rc = byptr(now() + 3600);
	d = time((long *)0) - now();
	printf("stime, BX pointing to the seconds: returned %d, the clock moved %ld s\n", rc, d);
	if (d >= 3595 && d <= 3605) {
		byptr(now());
		printf("so: BX points to the seconds; put back, now %ld s off\n", time((long *)0) - now());
		exit(0);
	}
	byptr(now());
	bylong(now());
	printf("neither; the clock put back as near as can be: %ld s off\n", time((long *)0) - now());
	exit(1);
}
