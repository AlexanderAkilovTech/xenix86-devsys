/* man - show manual pages
 *
 * usage: man [section] name...
 *        man -k keyword...
 *
 * The pages are formatted text in /usr/man/catN/page.N (N the section:
 * 1 commands, 2 system calls, 3 library routines), and /usr/man/whatis
 * has a line for each page: "page(N)", a tab, the names it describes,
 * " - " and what it is about. man finds a name there (in the section
 * given, or the first section that has it) and shows its page, through
 * more when the output is a terminal. man -k shows the whatis lines that
 * mention a keyword. MANPATH in the environment names another directory
 * to use instead of /usr/man. xenix86-devsys, K&R C. */
#include <stdio.h>

char	*pagers[] = { "/usr/bin/more", "/bin/more", "/usr/bin/pg", 0 };

extern char *getenv(), *strchr(), *strrchr();

char	*mandir = "/usr/man";
int	errors;

/* does the list of names (to " - ") have name in it? */
named(list, name)
register char *list;
char *name;
{
	register char *n;

	for (;;) {
		while (*list == ' ' || *list == ',')
			list++;
		if (*list == 0 || strncmp(list, "- ", 2) == 0)
			return 0;
		for (n = name; *n && *list == *n; n++, list++)
			;
		if (*n == 0 && (*list == ',' || *list == ' ' || *list == 0))
			return 1;
		while (*list && *list != ',' && *list != ' ')
			list++;
	}
}

/* find name's page: its path in path; 1 if found */
find(name, sec, path)
char *name, *sec, *path;
{
	FILE *f;
	char line[300], *p, *q;
	int s;

	sprintf(path, "%s/whatis", mandir);
	if ((f = fopen(path, "r")) != NULL) {
		while (fgets(line, sizeof line, f)) {
			if ((p = strchr(line, '(')) == 0 || (q = strchr(p, ')')) == 0 || q[1] != '\t')
				continue;
			*p = *q = 0;
			if (sec && strcmp(p + 1, sec))
				continue;
			if (named(q + 2, name) || strcmp(line, name) == 0) {
				sprintf(path, "%s/cat%s/%s.%s", mandir, p + 1, line, p + 1);
				fclose(f);
				return 1;
			}
		}
		fclose(f);
	}
	/* a page not in the index */
	for (s = '1'; s <= '8'; s++) {
		if (sec && *sec != s)
			continue;
		sprintf(path, "%s/cat%c/%s.%c", mandir, s, name, s);
		if (access(path, 4) == 0)
			return 1;
	}
	return 0;
}

/* show a page: through more (or pg) on a terminal, else copied out */
show(path)
char *path;
{
	char buf[512], **pg, *name;
	int fd, n, pid, w, status;

	for (pg = pagers; isatty(1) && *pg; pg++) {
		if (access(*pg, 1) != 0)
			continue;
		name = strrchr(*pg, '/') + 1;
		fflush(stdout);
		if ((pid = fork()) == 0) {
			execl(*pg, name, path, (char *)0);
			_exit(127);
		}
		if (pid > 0) {
			while ((w = wait(&status)) != pid && w != -1)
				;
			return;
		}
		break;
	}
	if ((fd = open(path, 0)) < 0) {
		fprintf(stderr, "man: cannot read %s\n", path);
		errors++;
		return;
	}
	fflush(stdout);
	while ((n = read(fd, buf, sizeof buf)) > 0)
		write(1, buf, n);
	close(fd);
}

/* man -k: the whatis lines with the keyword in them, in any case */
keyword(k)
char *k;
{
	FILE *f;
	char line[300], path[260];
	register char *p, *q;
	int found = 0;

	sprintf(path, "%s/whatis", mandir);
	if ((f = fopen(path, "r")) == NULL) {
		fprintf(stderr, "man: cannot read %s\n", path);
		errors++;
		return;
	}
	while (fgets(line, sizeof line, f)) {
		for (p = line; *p; p++) {
			for (q = k; *q && p[q - k] && lower(p[q - k]) == lower(*q); q++)
				;
			if (*q == 0) {
				fputs(line, stdout);
				found++;
				break;
			}
		}
	}
	fclose(f);
	if (!found) {
		fprintf(stderr, "man: nothing about %s\n", k);
		errors++;
	}
}

lower(c)
int c;
{
	return c >= 'A' && c <= 'Z' ? c + 'a' - 'A' : c;
}

main(argc, argv)
int argc;
char **argv;
{
	char path[260], *sec = 0, *p;
	int i = 1;

	if ((p = getenv("MANPATH")) != 0 && *p)
		mandir = p;
	if (strlen(mandir) > 200) {
		fprintf(stderr, "man: MANPATH too long\n");
		exit(2);
	}
	if (argc > 1 && strcmp(argv[1], "-k") == 0) {
		if (argc < 3)
			goto usage;
		for (i = 2; i < argc; i++)
			keyword(argv[i]);
		exit(errors != 0);
	}
	if (argc > 2 && argv[1][0] >= '1' && argv[1][0] <= '8' && argv[1][1] == 0)
		sec = argv[i++];
	if (i >= argc) {
usage:
		fprintf(stderr, "usage: man [section] name...\n       man -k keyword...\n");
		exit(2);
	}
	for (; i < argc; i++) {
		if (find(argv[i], sec, path))
			show(path);
		else {
			if (sec)
				fprintf(stderr, "man: no entry for %s in section %s\n", argv[i], sec);
			else
				fprintf(stderr, "man: no manual entry for %s\n", argv[i]);
			errors++;
		}
	}
	exit(errors != 0);
}
