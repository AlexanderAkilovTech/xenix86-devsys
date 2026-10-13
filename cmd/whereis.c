/* whereis - locate the program, the source and the manual page of commands
 *
 * usage: whereis [-sbmu] [-SBM dir ... -f] name ...
 *
 * For each name: the programs of that name in the usual directories, its
 * sources (name.c and the like, or a directory of that name), and its
 * manual pages (catN/name.N or manN/name.N under /usr/man, and, from the
 * whatis index, the page that describes it if it has another name).
 * A leading path is taken off the name, and so is a suffix of one letter
 * or digit (.c, .1). -b, -m and -s ask for only the programs, manuals or
 * sources; -u lists only names that lack one of the three; -B, -M and -S
 * give the directories to look in instead, up to -f. xenix86-devsys, K&R C. */
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

char	*bindirs[] = { "/bin", "/usr/bin", "/etc", "/lib", "/usr/lib", "/usr/lib/cc",
	"/usr/local/bin", "/usr/local", "/usr/games", 0 };
char	*mandirs[] = { "/usr/man", 0 };
char	*srcdirs[] = { "/usr/src/cmd", "/usr/src/lib", "/usr/src/local", "/usr/src", 0 };
char	*srcsuf[] = { ".c", ".s", ".y", ".l", ".f", ".sh", "", 0 };

char	**bin = bindirs, **man = mandirs, **src = srcdirs;
int	sflag, bflag, mflag, uflag;
char	*found[64];
int	nfound, nbin, nman, nsrc;

extern char *strrchr(), *strchr(), *malloc(), *strcpy();

/* note a path, once */
add(path)
char *path;
{
	register int i;
	char *p;

	for (i = 0; i < nfound; i++)
		if (strcmp(found[i], path) == 0)
			return 0;
	if (nfound < 64 && (p = malloc(strlen(path) + 1)) != 0)
		found[nfound++] = strcpy(p, path);
	return 1;
}

/* does path exist (and, for a program, is it an executable file)? */
there(path, prog)
char *path;
int prog;
{
	struct stat st;

	if (stat(path, &st) < 0)
		return 0;
	if (prog)
		return (st.st_mode & S_IFMT) == S_IFREG && (st.st_mode & 0111);
	return 1;
}

findbin(name)
char *name;
{
	char path[200];
	register char **d;

	for (d = bin; *d; d++) {
		sprintf(path, "%s/%s", *d, name);
		if (there(path, 1) && add(path))
			nbin++;
	}
}

findsrc(name)
char *name;
{
	char path[200];
	register char **d, **s;

	for (d = src; *d; d++)
		for (s = srcsuf; *s; s++) {
			sprintf(path, "%s/%s%s", *d, name, *s);
			if (there(path, 0) && add(path))
				nsrc++;
		}
}

/* the page names the whatis index gives for name: "page(N)" lines */
static
whatis(dir, name)
char *dir, *name;
{
	FILE *f;
	char path[200], line[300], *p, *q, *n;
	int sec;

	sprintf(path, "%s/whatis", dir);
	if ((f = fopen(path, "r")) == NULL)
		return;
	while (fgets(line, sizeof line, f)) {
		if ((p = strchr(line, '(')) == 0 || (q = strchr(p, '\t')) == 0)
			continue;
		sec = p[1];
		*p = 0;
		/* the names, up to " - " */
		for (n = q + 1; *n && !(n[0] == '-' && n[-1] == ' '); ) {
			while (*n == ' ' || *n == ',')
				n++;
			for (q = n; *q && *q != ',' && *q != ' '; q++)
				;
			if (q - n == strlen(name) && strncmp(n, name, q - n) == 0) {
				sprintf(path, "%s/cat%c/%s.%c", dir, sec, line, sec);
				if (there(path, 0) && add(path))
					nman++;
			}
			n = q;
		}
	}
	fclose(f);
}

findman(name)
char *name;
{
	char path[200];
	register char **d;
	register int s;

	for (d = man; *d; d++) {
		for (s = '1'; s <= '8'; s++) {
			sprintf(path, "%s/cat%c/%s.%c", *d, s, name, s);
			if (there(path, 0) && add(path))
				nman++;
			sprintf(path, "%s/man%c/%s.%c", *d, s, name, s);
			if (there(path, 0) && add(path))
				nman++;
		}
		whatis(*d, name);
	}
}

/* the name without its path, an SCCS s. prefix, or a one-letter suffix */
char *
clean(name)
char *name;
{
	static char buf[100];
	register char *p;

	if ((p = strrchr(name, '/')) != 0)
		name = p + 1;
	if (name[0] == 's' && name[1] == '.')
		name += 2;
	strncpy(buf, name, sizeof buf - 1);
	buf[sizeof buf - 1] = 0;
	p = buf + strlen(buf);
	if (p - buf > 2 && p[-2] == '.')
		p[-2] = 0;
	return buf;
}

lookup(arg)
char *arg;
{
	char *name = clean(arg);
	int i, all = !(sflag || bflag || mflag);

	nfound = nbin = nman = nsrc = 0;
	if (all || bflag)
		findbin(name);
	if (all || mflag)
		findman(name);
	if (all || sflag)
		findsrc(name);
	if (uflag && ((all || bflag) && nbin == 1 || !(all || bflag)) &&
	    ((all || mflag) && nman == 1 || !(all || mflag)) &&
	    ((all || sflag) && nsrc == 1 || !(all || sflag)))
		return;		/* -u: only names without one of each */
	printf("%s:", name);
	for (i = 0; i < nfound; i++)
		printf(" %s", found[i]);
	printf("\n");
	for (i = 0; i < nfound; i++)
		free(found[i]);
}

/* -B, -M or -S: the directories up to -f, or the end of the options */
char **
dirlist(argv, i, argc)
char **argv;
int *i, argc;
{
	char **list, **p;

	p = list = (char **)malloc((argc + 1) * sizeof(char *));
	while (++*i < argc && argv[*i][0] != '-')
		*p++ = argv[*i];
	*p = 0;
	if (*i < argc && strcmp(argv[*i], "-f") != 0)
		--*i;		/* another option: look at it next */
	return list;
}

main(argc, argv)
int argc;
char **argv;
{
	int i;
	char *p;

	for (i = 1; i < argc && argv[i][0] == '-'; i++) {
		for (p = argv[i] + 1; *p; p++)
			switch (*p) {
			case 's': sflag++; break;
			case 'b': bflag++; break;
			case 'm': mflag++; break;
			case 'u': uflag++; break;
			case 'f': break;
			case 'S': src = dirlist(argv, &i, argc); goto next;
			case 'B': bin = dirlist(argv, &i, argc); goto next;
			case 'M': man = dirlist(argv, &i, argc); goto next;
			default:
				fprintf(stderr, "usage: whereis [-sbmu] [-SBM dir ... -f] name ...\n");
				exit(2);
			}
	next:	;
	}
	if (i >= argc) {
		fprintf(stderr, "usage: whereis [-sbmu] [-SBM dir ... -f] name ...\n");
		exit(2);
	}
	for (; i < argc; i++)
		lookup(argv[i]);
	exit(0);
}
