/* cc - the C compiler command for XENIX 2.1.3 (8086), xenix86-devsys
 *
 * usage: cc [options] files...
 *	file.c		preprocessed, compiled and assembled
 *	file.s		assembled
 *	file.o, file.a	passed on to the link
 * options:
 *	-c		compile only, leaving file.o
 *	-S		compile to assembly language, leaving file.s
 *	-E		preprocess only, to the standard output
 *	-o name		name the program (default a.out)
 *	-Dname[=value], -Uname, -Idir
 *			passed to the preprocessor
 *	-lname		link with LIBDIR/libname.a
 *	-F size		stack size in bytes, in hexadecimal as with SCO's cc
 *	-v		show each command as it runs
 *	-O		optimise: copt rewrites the compiler's output with the
 *			rules in LIBDIR, and as86 makes each jump as short as it
 *			can be
 *	-Mc		callersaves: a function keeps no registers for its
 *			caller, which makes smaller code; compile all of a
 *			program's C files this way, or none of them
 *	-M0, -Ms	accepted and ignored (the 8086 small model is the only one)
 *
 * The passes are BCC's: bcc-cpp and bcc-cc1 (in LIBDIR), copt for -O (LIBDIR,
 * with its rules files), as86 and ld86 (in BINDIR); then aout2x (LIBDIR)
 * turns ld86's a.out into a XENIX x.out. bcc-cpp writes floating constants
 * in hexadecimal (-H), because bcc-cc1 has no room to convert decimal ones.
 * LIBDIR also holds crt0.o and libc.a; headers are in INCDIR. If CCROOT is
 * set in the environment, all of these are looked for under that directory.
 * Like the cc of Version 7, cc removes the .o when one C file is compiled and
 * linked in a single command. */
#include <stdio.h>
#include <signal.h>

#define	LIBDIR	"/usr/lib/cc"
#define	BINDIR	"/usr/bin"
#define	INCDIR	"/usr/include"
#define	MAXARGS	200

/* the names SCO's cc defined, and BCC's version */
char	*predef[] = { "-DM_XENIX", "-DM_I86", "-DM_I8086", "-DM_I86SM", "-Dunix",
	"-D__BCC_VERSION__=0x001011L", 0 };

char	*cpp, *cc1, *copt, *as, *ld, *cvt, *crt0, *libc, *libdir, *incdir;
char	*outfile = "a.out";
char	*stack = "1000";		/* hex, as SCO's cc -F */
char	*cppflags[MAXARGS];
char	*files[MAXARGS], *ldargs[MAXARGS];
int	ncpp, nfiles, nld;
int	cflag, sflag, eflag, vflag, oflag, mcflag;
int	errors;
char	tmp_i[32], tmp_s[32], tmp_out[32], tmp_o[32];

extern char *malloc(), *getenv(), *strcpy(), *strcat(), *strrchr();
extern long strtol();

char *
cat3(a, b, c)
char *a, *b, *c;
{
	register char *p;

	if ((p = malloc(strlen(a) + strlen(b) + strlen(c) + 1)) == 0) {
		fprintf(stderr, "cc: out of memory\n");
		exit(1);
	}
	strcpy(p, a);
	strcat(p, b);
	return strcat(p, c);
}

/* the file name with its directory removed and suffix replaced */
char *
setsuf(name, suf)
char *name;
int suf;
{
	register char *b, *p;

	if ((b = strrchr(name, '/')) != 0)
		name = b + 1;
	p = cat3(name, "", "");
	p[strlen(p) - 1] = suf;
	return p;
}

suffix(name)
register char *name;
{
	register int n = strlen(name);

	return n > 2 && name[n - 2] == '.' ? name[n - 1] : 0;
}

cleanup()
{
	unlink(tmp_i);
	unlink(tmp_s);
	unlink(tmp_out);
	unlink(tmp_o);
}

onintr()
{
	signal(SIGINT, SIG_IGN);
	cleanup();
	exit(100);
}

/* run a pass and wait for it; nonzero means it failed */
callsys(path, argv)
char *path, **argv;
{
	register int pid, w;
	int status;
	register char **a;

	if (vflag) {
		for (a = argv; *a; a++)
			fprintf(stderr, "%s%s", a == argv ? path : *a, a[1] ? " " : "\n");
	}
	fflush(stdout);
	if ((pid = fork()) == 0) {
		execv(path, argv);
		fprintf(stderr, "cc: cannot run %s\n", path);
		_exit(100);
	}
	if (pid == -1) {
		fprintf(stderr, "cc: cannot fork\n");
		return 1;
	}
	while ((w = wait(&status)) != pid && w != -1)
		;
	if (w == -1)
		return 1;
	if (status & 0177) {
		if ((status & 0177) != SIGINT)
			fprintf(stderr, "cc: %s died with signal %d\n", path, status & 0177);
		onintr();
	}
	return (status >> 8) & 0377;
}

/* preprocess, compile and assemble one C file; the object is named obj */
compile(src, obj)
char *src, *obj;
{
	char *av[MAXARGS], *sfile;
	register int n, i;

	n = 0;
	av[n++] = "bcc-cpp";
	av[n++] = src;
	if (!eflag) {
		av[n++] = "-o";
		av[n++] = tmp_i;
		av[n++] = "-H";		/* floating constants in hexadecimal */
	}
	av[n++] = "-0";
	for (i = 0; predef[i]; i++)
		av[n++] = predef[i];
	for (i = 0; i < ncpp; i++)
		av[n++] = cppflags[i];
	av[n++] = cat3("-I", incdir, "");
	av[n] = 0;
	if (callsys(cpp, av))
		return 1;
	if (eflag)
		return 0;

	sfile = sflag ? setsuf(src, 's') : tmp_s;
	n = 0;
	av[n++] = "bcc-cc1";
	av[n++] = tmp_i;
	av[n++] = "-o";
	av[n++] = oflag ? tmp_o : sfile;
	av[n++] = "-0";
	if (oflag)
		av[n++] = "-O";
	if (mcflag)
		av[n++] = "-c";
	av[n] = 0;
	if (callsys(cc1, av))
		return 1;
	if (oflag) {
		n = 0;
		av[n++] = "copt";
		av[n++] = tmp_o;
		av[n++] = "-o";
		av[n++] = sfile;
		av[n++] = "-c!";
		av[n++] = cat3("-d", libdir, "");
		av[n++] = "rules.start";
		av[n++] = "rules.86";
		if (mcflag)
			av[n++] = "rules.mc";
		av[n++] = "rules.end";
		av[n] = 0;
		if (callsys(copt, av))
			return 1;
	}
	if (sflag)
		return 0;
	return assemble(sfile, obj, setsuf(src, 's'));
}

assemble(sfile, obj, modname)
char *sfile, *obj, *modname;
{
	char *av[12];

	av[0] = "as86";
	av[1] = sfile;
	av[2] = "-o";
	av[3] = obj;
	av[4] = "-u";
	av[5] = "-w";
	av[6] = "-0";
	av[7] = "-n";
	av[8] = modname;
	av[9] = oflag ? "-O" : 0;
	av[10] = 0;
	return callsys(as, av);
}

main(argc, argv)
int argc;
char **argv;
{
	register char *a;
	register int i, s;
	char *root, *obj;
	char *av[MAXARGS + 10];
	int nc = 0, n;

	if ((root = getenv("CCROOT")) == 0)
		root = "";
	libdir = cat3(root, LIBDIR, "");
	incdir = cat3(root, INCDIR, "");
	cpp = cat3(libdir, "/bcc-cpp", "");
	cc1 = cat3(libdir, "/bcc-cc1", "");
	copt = cat3(libdir, "/copt", "");
	cvt = cat3(libdir, "/aout2x", "");
	crt0 = cat3(libdir, "/crt0.o", "");
	libc = cat3(libdir, "/libc.a", "");
	as = cat3(root, BINDIR, "/as86");
	ld = cat3(root, BINDIR, "/ld86");

	for (i = 1; i < argc; i++) {
		a = argv[i];
		if (*a == '-') {
			switch (a[1]) {
			case 'c': cflag++; continue;
			case 'S': sflag++; cflag++; continue;
			case 'E': eflag++; cflag++; continue;
			case 'v': vflag++; continue;
			case 'O': oflag++; continue;
			case 'M':
				if (a[2] == 'c')
					mcflag++;
				continue;
			case 'D': case 'U': case 'I':
				if (ncpp < MAXARGS)
					cppflags[ncpp++] = a;
				continue;
			case 'o':
				if (++i >= argc)
					goto usage;
				outfile = argv[i];
				s = suffix(outfile);
				if (s == 'c' || s == 's') {
					fprintf(stderr, "cc: -o would overwrite %s\n", outfile);
					exit(8);
				}
				continue;
			case 'F':
				if (++i >= argc)
					goto usage;
				stack = argv[i];
				continue;
			case 'l':
				if (nld < MAXARGS)
					ldargs[nld++] = cat3(libdir, "/lib", cat3(a + 2, ".a", ""));
				continue;
			default:
				fprintf(stderr, "cc: unknown option %s\n", a);
				continue;
			}
		}
		if (nfiles < MAXARGS)
			files[nfiles++] = a;
	}
	if (nfiles == 0) {
usage:
		fprintf(stderr, "usage: cc [-c] [-S] [-E] [-O] [-Mc] [-o name] [-Dname] [-Idir] [-lname] [-F stack] files...\n");
		exit(8);
	}

	n = getpid();
	sprintf(tmp_i, "/tmp/ctm%da", n);
	sprintf(tmp_s, "/tmp/ctm%db", n);
	sprintf(tmp_out, "/tmp/ctm%dc", n);
	sprintf(tmp_o, "/tmp/ctm%dd", n);
	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
		signal(SIGINT, onintr);
	if (signal(SIGTERM, SIG_IGN) != SIG_IGN)
		signal(SIGTERM, onintr);

	n = 0;
	for (i = 0; i < nfiles; i++) {
		a = files[i];
		s = suffix(a);
		if (s == 'c' || s == 's') {
			if (nfiles > 1 && !eflag)
				printf("%s:\n", a);
			obj = setsuf(a, 'o');
			if (s == 'c') {
				nc++;
				errors |= compile(a, obj);
			} else
				errors |= assemble(a, obj, setsuf(a, 's'));
			av[n++] = obj;
		} else
			av[n++] = a;	/* .o, .a: straight to the link */
	}
	if (!cflag && !errors) {
		char *lv[MAXARGS + 10];
		register int m = 0;

		lv[m++] = "ld86";
		lv[m++] = "-0";
		lv[m++] = "-i";
		lv[m++] = "-o";
		lv[m++] = tmp_out;
		lv[m++] = crt0;
		for (i = 0; i < n; i++)
			lv[m++] = av[i];
		for (i = 0; i < nld; i++)
			lv[m++] = ldargs[i];
		lv[m++] = libc;
		lv[m] = 0;
		errors |= callsys(ld, lv);
		if (!errors) {
			char size[12];

			sprintf(size, "%ld", strtol(stack, (char **)0, 16));
			lv[0] = "aout2x";
			lv[1] = "-s";
			lv[2] = size;
			lv[3] = tmp_out;
			lv[4] = outfile;
			lv[5] = 0;
			errors |= callsys(cvt, lv);
		}
		/* one C file compiled and linked: its .o is not kept */
		if (nc == 1 && nfiles == 1 && !errors)
			unlink(av[0]);
	}
	cleanup();
	exit(errors ? 8 : 0);
}
