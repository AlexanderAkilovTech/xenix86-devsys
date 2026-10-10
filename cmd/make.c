/* make - maintain groups of programs, for XENIX 2.1.3 (8086), xenix86-devsys
 *
 * usage: make [-f makefile] [-nsiktqper] [name=value ...] [target ...]
 *
 * Written from scratch in K&R C, following the make of Version 7 and System V:
 * macros ($(NAME), ${NAME}, $X, $(NAME:old=new), $(@D), $(@F)), the automatic
 * macros $@ $< $* $?, suffix rules and .SUFFIXES, single-suffix rules,
 * double-colon rules, .DEFAULT, .IGNORE, .SILENT and .PRECIOUS, include, and
 * the environment as macros. Commands beginning with @ are not echoed, with -
 * their failure is ignored. A command without shell metacharacters is run
 * directly; others go to /bin/sh -c.
 *	-f file	read file instead of makefile or Makefile ("-" is standard input)
 *	-n	print the commands but do not run them
 *	-s	silent: do not print commands	-i  ignore command errors
 *	-k	keep going with other targets after an error
 *	-t	touch targets instead of remaking them
 *	-q	question: exit 0 if the targets are up to date, 1 if not
 *	-p	print the macros and rules	-e  environment overrides the makefile
 *	-r	no built-in rules
 */
#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>

#define	HASHSIZE	128
#define	MAXARGS		100
#define	LINESIZE	1024

struct	list {			/* a list of names */
	struct list *next;
	struct name *n;
};

struct	cmd {			/* a command line */
	struct cmd *next;
	char	*text;
};

struct	dep {			/* one rule: prerequisites and commands */
	struct dep *next;
	struct list *prereqs;
	struct cmd *cmds;
	int	dcolon;		/* defined with :: */
};

struct	name {			/* a target, a file, or a suffix rule */
	struct name *next;	/* hash chain */
	char	*name;
	struct dep *deps;
	long	mtime;		/* 0: does not exist */
	int	state;		/* 0 not yet made, 1 being made, 2 made */
	int	failed;
	int	target;		/* named on the left of a colon */
};

struct	macro {
	struct macro *next;
	char	*name;
	char	*value;
	int	level;		/* 0 environment, 1 makefile, 2 command line */
};

struct name *names[HASHSIZE];
struct macro *macros[HASHSIZE];
struct list *suffixes;		/* from .SUFFIXES, in order */
struct name *firstarg;		/* the first target in the makefile */

int	nflag, sflag, iflag, kflag, tflag, qflag, pflag, eflag, rflag;
int	outofdate;		/* for -q */
int	readlevel = 1;		/* level of macros being defined */
char	*curtarget;		/* being remade, for interrupts */
long	curtime;		/* its time before the commands ran */

/* the automatic macros, for the target being remade */
char	*m_at = "", *m_lt = "", *m_star = "", *m_quest = "";

char	*builtin[] = {
	".SUFFIXES: .o .c .s",
	"MAKE = make",
	"CC = cc",
	"CFLAGS = -O",
	"AS = as86",
	"LD = ld86",
	"AR = ar86",
	"ARFLAGS = rv",
	".c.o:",
	"\t$(CC) $(CFLAGS) -c $<",
	".s.o:",
	"\t$(CC) -c $<",
	".c:",
	"\t$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<",
	".s:",
	"\t$(CC) $(LDFLAGS) -o $@ $<",
	0
};

extern char *malloc(), *getenv(), *strchr(), *strrchr(), *strcpy(), *strcat();
extern long time();
extern char **environ;
extern int errno;

char	*expand();
struct name *lookup();

/* ---------- memory and strings ---------- */

char *
xalloc(n)
unsigned n;
{
	register char *p;

	if ((p = malloc(n)) == 0) {
		fprintf(stderr, "make: out of memory\n");
		exit(2);
	}
	return p;
}

char *
copy(s)
char *s;
{
	return strcpy(xalloc(strlen(s) + 1), s);
}

char *
copyn(s, n)
char *s;
int n;
{
	register char *p = xalloc(n + 1);

	strncpy(p, s, n);
	p[n] = 0;
	return p;
}

hash(s)
register char *s;
{
	register unsigned h = 0;

	while (*s)
		h = h * 31 + *s++;
	return h % HASHSIZE;
}

/* a growable string buffer */
struct	buf {
	char	*s;
	int	len, size;
};

bput(b, c)
register struct buf *b;
int c;
{
	char *n;

	if (b->len + 1 >= b->size) {
		b->size = b->size ? b->size * 2 : 64;
		n = xalloc(b->size);
		if (b->s) {
			strncpy(n, b->s, b->len);
			free(b->s);
		}
		b->s = n;
	}
	b->s[b->len++] = c;
	b->s[b->len] = 0;
}

bputs(b, s)
struct buf *b;
register char *s;
{
	while (*s)
		bput(b, *s++);
}

/* ---------- names and macros ---------- */

struct name *
lookup(s, create)
char *s;
int create;
{
	register struct name *n;
	register int h = hash(s);

	for (n = names[h]; n; n = n->next)
		if (!strcmp(n->name, s))
			return n;
	if (!create)
		return 0;
	n = (struct name *)xalloc(sizeof *n);
	n->name = copy(s);
	n->deps = 0;
	n->mtime = 0;
	n->state = 0;
	n->failed = 0;
	n->target = 0;
	n->next = names[h];
	names[h] = n;
	return n;
}

struct macro *
findmacro(s)
char *s;
{
	register struct macro *m;

	for (m = macros[hash(s)]; m; m = m->next)
		if (!strcmp(m->name, s))
			return m;
	return 0;
}

setmacro(name, value, level)
char *name, *value;
int level;
{
	register struct macro *m;

	if ((m = findmacro(name)) != 0) {
		if (m->level > level && !(eflag && m->level == 0 && level == 1))
			return;
		if (eflag && m->level == 0 && level == 1)
			return;		/* -e: the environment wins over the makefile */
		free(m->value);
	} else {
		m = (struct macro *)xalloc(sizeof *m);
		m->name = copy(name);
		m->next = macros[hash(name)];
		macros[hash(name)] = m;
	}
	m->value = copy(value);
	m->level = level;
}

/* the directory part (D) or file part (F) of each word of s */
char *
dirfile(s, which)
char *s;
int which;
{
	struct buf b;
	register char *p, *w, *slash;
	int n;

	b.s = 0; b.len = b.size = 0;
	bputs(&b, "");
	for (p = s; *p; ) {
		while (*p == ' ' || *p == '\t')
			p++;
		if (!*p)
			break;
		for (w = p; *p && *p != ' ' && *p != '\t'; p++)
			;
		slash = 0;
		for (n = 0; w + n < p; n++)
			if (w[n] == '/')
				slash = w + n;
		if (b.len)
			bput(&b, ' ');
		if (which == 'D') {
			if (!slash)
				bput(&b, '.');
			else if (slash == w)
				bput(&b, '/');
			else
				for (; w < slash; w++)
					bput(&b, *w);
		} else
			for (w = slash ? slash + 1 : w; w < p; w++)
				bput(&b, *w);
	}
	return b.s;
}

/* the value of macro "name", with :old=new substitution if given */
char *
macrovalue(name)
char *name;
{
	register char *colon, *eq, *v, *p, *w;
	char *old, *new, *raw;
	struct buf b;
	struct macro *m;
	int ol, wl;

	colon = strchr(name, ':');
	if (colon)
		*colon = 0;
	if (!strcmp(name, "@")) raw = m_at;
	else if (!strcmp(name, "<")) raw = m_lt;
	else if (!strcmp(name, "*")) raw = m_star;
	else if (!strcmp(name, "?")) raw = m_quest;
	else if (name[1] && !name[2] && (name[1] == 'D' || name[1] == 'F') && strchr("@<*?", name[0])) {
		char one[2];

		one[0] = name[0];
		one[1] = 0;
		raw = dirfile(macrovalue(one), name[1]);
	} else
		raw = (m = findmacro(name)) ? m->value : "";
	v = expand(raw);
	if (!colon)
		return v;
	*colon = ':';
	if ((eq = strchr(colon + 1, '=')) == 0)
		return v;
	old = copyn(colon + 1, eq - colon - 1);
	new = eq + 1;
	ol = strlen(old);
	b.s = 0; b.len = b.size = 0;
	bputs(&b, "");
	for (p = v; *p; ) {
		while (*p == ' ' || *p == '\t')
			bput(&b, *p++);
		for (w = p; *p && *p != ' ' && *p != '\t'; p++)
			;
		wl = p - w;
		if (wl >= ol && !strncmp(p - ol, old, ol)) {
			for (; w < p - ol; w++)
				bput(&b, *w);
			bputs(&b, new);
		} else
			for (; w < p; w++)
				bput(&b, *w);
	}
	return b.s;
}

/* expand macros in s; returns a new string */
char *
expand(s)
char *s;
{
	struct buf b;
	register char *p, *q;
	char name[128], close;
	int depth;
	static int level;

	if (++level > 40) {
		fprintf(stderr, "make: macros nested too deeply (a macro refers to itself?)\n");
		exit(2);
	}
	b.s = 0; b.len = b.size = 0;
	bputs(&b, "");
	for (p = s; *p; p++) {
		if (*p != '$') {
			bput(&b, *p);
			continue;
		}
		p++;
		if (*p == '$') {
			bput(&b, '$');
			continue;
		}
		if (*p == 0)
			break;
		if (*p == '(' || *p == '{') {
			close = *p == '(' ? ')' : '}';
			depth = 1;
			for (q = ++p; *q; q++) {
				if (*q == '(' || *q == '{')
					depth++;
				else if ((*q == ')' || *q == '}') && --depth == 0)
					break;
			}
			if (q - p >= sizeof name)
				q = p + sizeof name - 1;
			strncpy(name, p, q - p);
			name[q - p] = 0;
			p = *q ? q : q - 1;
			if (strchr(name, '$')) {	/* a macro name made of macros */
				q = expand(name);
				strncpy(name, q, sizeof name - 1);
				name[sizeof name - 1] = 0;
			}
		} else {
			name[0] = *p;
			name[1] = 0;
		}
		bputs(&b, macrovalue(name));
	}
	level--;
	return b.s;
}

/* ---------- reading makefiles ---------- */

char	*fname;			/* the makefile being read, for messages */
int	lineno;

/* read one logical line: continuations joined, the newline removed */
char *
getline(f)
FILE *f;
{
	static struct buf b;
	register int c, prev;

	if (b.s)
		b.len = 0, b.s[0] = 0;
	else
		bputs(&b, "");
	prev = 0;
	while ((c = getc(f)) != EOF) {
		if (c == '\n') {
			lineno++;
			if (prev == '\\') {	/* continued: drop the backslash and the indent */
				b.s[--b.len] = 0;
				while ((c = getc(f)) == ' ' || c == '\t')
					;
				if (c != EOF)
					ungetc(c, f);
				bput(&b, ' ');
				prev = ' ';
				continue;
			}
			return b.s;
		}
		bput(&b, c);
		prev = c;
	}
	return b.len ? b.s : 0;
}

struct list *
words(s)
char *s;
{
	register char *p, *w;
	struct list *head = 0, **tail = &head, *l;
	char *word;

	for (p = s; *p; ) {
		while (*p == ' ' || *p == '\t')
			p++;
		if (!*p)
			break;
		for (w = p; *p && *p != ' ' && *p != '\t'; p++)
			;
		word = copyn(w, p - w);
		l = (struct list *)xalloc(sizeof *l);
		l->n = lookup(word, 1);
		free(word);
		l->next = 0;
		*tail = l;
		tail = &l->next;
	}
	return head;
}

/* is s a suffix rule (.c.o or .c) for the current .SUFFIXES? */
issuffixrule(s)
char *s;
{
	register struct list *a, *b;
	register int la;

	if (s[0] != '.')
		return 0;
	for (a = suffixes; a; a = a->next) {
		la = strlen(a->n->name);
		if (strncmp(s, a->n->name, la))
			continue;
		if (s[la] == 0)
			return 1;
		for (b = suffixes; b; b = b->next)
			if (!strcmp(s + la, b->n->name))
				return 1;
	}
	return 0;
}

struct dep *lastdeps;		/* rules that following command lines belong to */
struct list *lasttargets;

rule(targets, prereqs, dcolon, cmd)
struct list *targets, *prereqs;
int dcolon;
char *cmd;
{
	register struct list *t, *l;
	register struct dep *d;
	struct list *p, **tail;
	struct cmd *c;

	lasttargets = targets;
	for (t = targets; t; t = t->next) {
		t->n->target = 1;
		if (!strcmp(t->n->name, ".SUFFIXES")) {
			if (!prereqs)
				suffixes = 0;
			for (tail = &suffixes; *tail; tail = &(*tail)->next)
				;
			for (l = prereqs; l; l = l->next) {
				p = (struct list *)xalloc(sizeof *p);
				p->n = l->n;
				p->next = 0;
				*tail = p;
				tail = &p->next;
			}
			continue;
		}
		if (!firstarg && t->n->name[0] != '.' )
			firstarg = t->n;
		/* one rule per target for ":", a new one each time for "::" */
		d = 0;
		if (!dcolon)
			for (d = t->n->deps; d && d->dcolon; d = d->next)
				;
		if (!d) {		/* a new rule goes last: :: rules run in order */
			struct dep **dt;

			d = (struct dep *)xalloc(sizeof *d);
			d->prereqs = 0;
			d->cmds = 0;
			d->dcolon = dcolon;
			d->next = 0;
			for (dt = &t->n->deps; *dt; dt = &(*dt)->next)
				;
			*dt = d;
		}
		for (tail = &d->prereqs; *tail; tail = &(*tail)->next)
			;
		for (l = prereqs; l; l = l->next) {
			p = (struct list *)xalloc(sizeof *p);
			p->n = l->n;
			p->next = 0;
			*tail = p;
			tail = &p->next;
		}
		if (cmd) {
			c = (struct cmd *)xalloc(sizeof *c);
			c->text = copy(cmd);
			c->next = 0;
			d->cmds = c;
		}
	}
}

/* add a command line to the rules just read */
addcmd(text)
char *text;
{
	register struct list *t;
	register struct dep *d;
	struct cmd *c, **tail;

	for (t = lasttargets; t; t = t->next) {
		if (!strcmp(t->n->name, ".SUFFIXES"))
			continue;
		/* the single-colon rule, or the newest (last) double-colon one */
		for (d = t->n->deps; d; d = d->next)
			if (!d->dcolon || !d->next)
				break;
		if (!d)
			continue;
		for (tail = &d->cmds; *tail; tail = &(*tail)->next)
			;
		c = (struct cmd *)xalloc(sizeof *c);
		c->text = copy(text);
		c->next = 0;
		*tail = c;
	}
}

syntax(msg)
char *msg;
{
	fprintf(stderr, "make: %s, line %d: %s\n", fname, lineno, msg);
	exit(2);
}

readfile(f, name)
FILE *f;
char *name;
{
	register char *line, *p, *q;
	char *e, *cmd, *oldname;
	int oldline, dcolon;
	FILE *inc;

	oldname = fname;
	oldline = lineno;
	fname = name;
	lineno = 0;
	lasttargets = 0;
	while ((line = getline(f)) != 0) {
		if (line[0] == '\t') {		/* a command */
			if (!lasttargets) {
				for (p = line; *p == ' ' || *p == '\t'; p++)
					;
				if (*p == 0 || *p == '#')
					continue;
				syntax("command line before any rule");
			}
			addcmd(line + 1);
			continue;
		}
		if ((p = strchr(line, '#')) != 0)
			*p = 0;
		for (p = line; *p == ' ' || *p == '\t'; p++)
			;
		if (*p == 0)
			continue;
		lasttargets = 0;
		if (!strncmp(p, "include", 7) && (p[7] == ' ' || p[7] == '\t')) {
			e = expand(p + 8);
			for (p = e; *p == ' ' || *p == '\t'; p++)
				;
			for (q = p + strlen(p); q > p && (q[-1] == ' ' || q[-1] == '\t'); )
				*--q = 0;
			if ((inc = fopen(p, "r")) == 0)
				syntax("cannot open include file");
			readfile(inc, copy(p));
			fclose(inc);
			lasttargets = 0;
			continue;
		}
		/* a macro definition has = before any : (other than := forms) */
		for (q = p; *q && *q != '=' && *q != ':'; q++)
			;
		if (*q == '=') {
			e = q;
			while (e > p && (e[-1] == ' ' || e[-1] == '\t'))
				e--;
			*e = 0;
			for (q++; *q == ' ' || *q == '\t'; q++)
				;
			for (e = q + strlen(q); e > q && (e[-1] == ' ' || e[-1] == '\t'); )
				*--e = 0;
			setmacro(p, q, readlevel);
			continue;
		}
		if (*q != ':')
			syntax("not a rule or a macro definition");
		*q++ = 0;
		dcolon = *q == ':';
		if (dcolon)
			q++;
		cmd = 0;
		if ((e = strchr(q, ';')) != 0) {
			*e++ = 0;
			cmd = e;
		}
		rule(words(expand(p)), words(expand(q)), dcolon, cmd);
	}
	fname = oldname;
	lineno = oldline;
}

/* ---------- making ---------- */

long
filetime(s)
char *s;
{
	struct stat st;

	return stat(s, &st) < 0 ? 0L : st.st_mtime;
}

/* shell metacharacters: such a command needs /bin/sh (= only in the first
   word, where it makes an assignment) */
ismeta(s)
register char *s;
{
	register int first = 1;

	for (; *s; s++) {
		if (strchr("#|^();&<>*?[]$`'\"\\\n", *s))
			return 1;
		if (*s == '=' && first)
			return 1;
		if ((*s == ' ' || *s == '\t') && s[1] != ' ' && s[1] != '\t')
			first = 0;
	}
	return 0;
}

onintr()
{
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	if (curtarget && !lookup(".PRECIOUS", 0) && filetime(curtarget) != curtime) {
		fprintf(stderr, "\nmake: interrupt: %s removed\n", curtarget);
		unlink(curtarget);
	} else
		fprintf(stderr, "\nmake: interrupt\n");
	exit(2);
}

/* run one command; returns its exit status */
runcmd(cmd)
char *cmd;
{
	char *av[MAXARGS];
	register char *p;
	register int n, pid, w;
	int status;

	fflush(stdout);
	if ((pid = fork()) == 0) {
		signal(SIGINT, SIG_DFL);
		signal(SIGQUIT, SIG_DFL);
		if (ismeta(cmd)) {
			execl("/bin/sh", "sh", "-c", cmd, (char *)0);
			fprintf(stderr, "make: cannot run /bin/sh\n");
			_exit(127);
		}
		n = 0;
		for (p = cmd; *p; ) {
			while (*p == ' ' || *p == '\t')
				*p++ = 0;
			if (!*p)
				break;
			if (n < MAXARGS - 1)
				av[n++] = p;
			while (*p && *p != ' ' && *p != '\t')
				p++;
		}
		av[n] = 0;
		if (n == 0)
			_exit(0);
		execvp(av[0], av);
		fprintf(stderr, "make: %s: not found\n", av[0]);
		_exit(127);
	}
	if (pid == -1) {
		fprintf(stderr, "make: cannot fork\n");
		return 1;
	}
	while ((w = wait(&status)) != pid && w != -1)
		;
	return w == -1 ? 1 : status;
}

/* run the commands for target n; nonzero if one failed */
docmds(n, cmds)
struct name *n;
struct cmd *cmds;
{
	register struct cmd *c;
	register char *t;
	int silent, ignore, st;
	long now;

	for (c = cmds; c; c = c->next) {
		t = expand(c->text);
		silent = sflag || lookup(".SILENT", 0) != 0;
		ignore = iflag || lookup(".IGNORE", 0) != 0;
		for (;; t++) {
			if (*t == '@')
				silent = 1;
			else if (*t == '-')
				ignore = 1;
			else if (*t != ' ' && *t != '\t')
				break;
		}
		if (*t == 0)
			continue;
		if (!silent || nflag)
			printf("%s\n", t);
		if (nflag)
			continue;
		curtarget = n->name;
		curtime = filetime(n->name);
		st = runcmd(t);
		curtarget = 0;
		if (st) {
			if (st & 0177)
				printf("*** Termination code %d", st & 0177);
			else
				printf("*** Error code %d", (st >> 8) & 0377);
			if (ignore) {
				printf(" (ignored)\n");
				continue;
			}
			printf("\n");
			return 1;
		}
	}
	return 0;
}

touch(s)
char *s;
{
	int fd;

	printf("touch(%s)\n", s);
	if (nflag)
		return;
	if (utime(s, (long *)0) < 0 && (fd = creat(s, 0666)) >= 0)
		close(fd);
}

/* find a suffix rule that can make n: returns the rule, and sets *src to the
   source file and *stem to the name without its suffix */
struct dep *
implicit(n, src, stem)
struct name *n;
char **src, **stem;
{
	register struct list *s, *t;
	register struct name *r;
	char *base, *cand, *rname;
	int nl, tl;

	nl = strlen(n->name);
	for (t = suffixes; ; t = t->next) {
		/* rules for the target's own suffix, then single-suffix rules (.c:) */
		if (t) {
			tl = strlen(t->n->name);
			if (tl >= nl || strcmp(n->name + nl - tl, t->n->name))
				continue;
		} else
			tl = 0;
		base = copyn(n->name, nl - tl);
		for (s = suffixes; s; s = s->next) {
			rname = xalloc(strlen(s->n->name) + tl + 1);
			strcpy(rname, s->n->name);
			if (t)
				strcat(rname, t->n->name);
			r = lookup(rname, 0);
			free(rname);
			if (!r || !r->deps || !r->deps->cmds)
				continue;
			cand = xalloc(strlen(base) + strlen(s->n->name) + 1);
			strcpy(cand, base);
			strcat(cand, s->n->name);
			if (filetime(cand) || (lookup(cand, 0) && lookup(cand, 0)->target)) {
				*src = cand;
				*stem = base;
				return r->deps;
			}
			free(cand);
		}
		free(base);
		if (!t)
			break;
	}
	return 0;
}

/* bring n up to date; nonzero on failure */
make(n)
register struct name *n;
{
	register struct dep *d;
	register struct list *l;
	struct dep *rd;
	struct cmd *cmds;
	struct buf quest;
	char *src, *star;
	long newest;
	int errs, remake, any;

	if (n->state == 2)
		return n->failed;
	if (n->state == 1) {
		fprintf(stderr, "make: %s depends on itself\n", n->name);
		return 0;
	}
	n->state = 1;
	n->mtime = filetime(n->name);
	errs = 0;
	newest = 0;
	cmds = 0;
	src = 0;
	star = 0;
	quest.s = 0; quest.len = quest.size = 0;
	bputs(&quest, "");

	/* double-colon rules: each stands alone */
	for (d = n->deps; d; d = d->next)
		if (d->dcolon) {
			remake = n->mtime == 0 || d->prereqs == 0;
			for (l = d->prereqs; l; l = l->next) {
				errs |= make(l->n);
				if (l->n->mtime > n->mtime)
					remake = 1;
			}
			if (remake && !errs && d->cmds)
				errs |= build(n, d->cmds, "", "", "");
			if (errs && !kflag)
				break;
		}

	/* the single-colon rule, and an implicit rule if it has no commands */
	any = 0;
	for (d = n->deps; d; d = d->next)
		if (!d->dcolon) {
			any = 1;
			cmds = d->cmds;
			for (l = d->prereqs; l; l = l->next) {
				errs |= make(l->n);
				if (errs && !kflag)
					break;
				if (l->n->mtime > newest)
					newest = l->n->mtime;
				if (l->n->mtime > n->mtime || n->mtime == 0) {
					if (quest.len)
						bput(&quest, ' ');
					bputs(&quest, l->n->name);
				}
			}
		}
	if (!cmds && n->name[0] != '.' && (rd = implicit(n, &src, &star)) != 0) {
		struct name *s = lookup(src, 1);

		errs |= make(s);
		if (s->mtime > newest)
			newest = s->mtime;
		if (s->mtime > n->mtime || n->mtime == 0) {
			if (quest.len)
				bput(&quest, ' ');
			bputs(&quest, s->name);
		}
		cmds = rd->cmds;
		any = 1;
	}
	if (errs) {
		n->state = 2;
		n->failed = 1;
		if (!kflag)
			return 1;
		fprintf(stderr, "make: %s not remade because of errors\n", n->name);
		return 1;
	}
	if (n->mtime == 0 || newest > n->mtime) {
		if (cmds)
			errs = build(n, cmds, src ? src : "", star ? star : "", quest.s);
		else if (!any) {			/* nothing says how to make it */
			struct name *def = lookup(".DEFAULT", 0);

			rd = def ? def->deps : 0;
			if (rd && rd->cmds)
				errs = build(n, rd->cmds, "", "", "");
			else {
				fprintf(stderr, "make: Don't know how to make %s.  Stop.\n", n->name);
				errs = 1;
			}
		} else		/* prerequisites but no commands: it is as new as they are */
			n->mtime = newest ? newest : time((long *)0);
	}
	n->state = 2;
	n->failed = errs;
	free(quest.s);
	return errs;
}

/* remake n with cmds; the automatic macros are set for them */
build(n, cmds, lt, star, quest)
struct name *n;
struct cmd *cmds;
char *lt, *star, *quest;
{
	char *oat = m_at, *olt = m_lt, *ostar = m_star, *oq = m_quest;
	register char *p;
	int errs;

	outofdate = 1;
	if (qflag)
		return 0;
	m_at = n->name;
	m_lt = lt;
	if (*star)
		m_star = star;
	else {
		m_star = copy(n->name);
		if ((p = strrchr(m_star, '.')) != 0 && !strchr(p, '/'))
			*p = 0;
	}
	m_quest = quest;
	if (tflag) {
		touch(n->name);
		errs = 0;
	} else
		errs = docmds(n, cmds);
	m_at = oat; m_lt = olt; m_star = ostar; m_quest = oq;
	n->mtime = filetime(n->name);
	if (n->mtime == 0 || nflag)
		n->mtime = time((long *)0);
	return errs;
}

/* ---------- -p ---------- */

printdb()
{
	register int i;
	register struct macro *m;
	register struct name *n;
	register struct dep *d;
	register struct list *l;
	struct cmd *c;

	for (i = 0; i < HASHSIZE; i++)
		for (m = macros[i]; m; m = m->next)
			printf("%s = %s\n", m->name, m->value);
	printf("\n");
	for (i = 0; i < HASHSIZE; i++)
		for (n = names[i]; n; n = n->next)
			for (d = n->deps; d; d = d->next) {
				printf("%s:%s", n->name, d->dcolon ? ":" : "");
				for (l = d->prereqs; l; l = l->next)
					printf(" %s", l->n->name);
				printf("\n");
				for (c = d->cmds; c; c = c->next)
					printf("\t%s\n", c->text);
			}
}

main(argc, argv)
int argc;
char **argv;
{
	register char *a, *p;
	register int i, j;
	char *files[10], **e, *eq;
	int nfiles = 0, errs = 0, ntargets = 0;
	FILE *f;
	struct name *n;

	for (i = 1; i < argc; i++) {
		a = argv[i];
		if (a[0] == '-' && a[1]) {
			for (j = 1; a[j]; j++)
				switch (a[j]) {
				case 'f':
					if (++i >= argc) {
						fprintf(stderr, "make: -f needs a file name\n");
						exit(2);
					}
					if (nfiles < 10)
						files[nfiles++] = argv[i];
					break;
				case 'n': nflag++; break;
				case 's': sflag++; break;
				case 'i': iflag++; break;
				case 'k': kflag++; break;
				case 't': tflag++; break;
				case 'q': qflag++; break;
				case 'p': pflag++; break;
				case 'e': eflag++; break;
				case 'r': rflag++; break;
				default:
					fprintf(stderr, "make: unknown option -%c\n", a[j]);
					exit(2);
				}
			argv[i] = 0;
		} else if ((eq = strchr(a, '=')) != 0) {
			*eq = 0;
			setmacro(a, eq + 1, 2);
			*eq = '=';
			argv[i] = 0;
		}
	}
	for (e = environ; *e; e++)
		if ((eq = strchr(*e, '=')) != 0 && eq != *e) {
			p = copyn(*e, eq - *e);
			setmacro(p, eq + 1, 0);
			free(p);
		}
	if (!rflag) {
		readlevel = 1;
		for (i = 0; builtin[i]; i++) {
			struct buf b;

			/* builtin lines are read like a makefile; a command follows its rule */
			if (builtin[i][0] == '\t')
				addcmd(builtin[i] + 1);
			else {
				b.s = 0; b.len = b.size = 0;
				bputs(&b, builtin[i]);
				lasttargets = 0;
				if ((p = strchr(b.s, '=')) != 0 && (!strchr(b.s, ':') || strchr(b.s, ':') > p)) {
					char *v = p + 1;

					while (p > b.s && p[-1] == ' ')
						p--;
					*p = 0;
					while (*v == ' ')
						v++;
					if (!findmacro(b.s) || findmacro(b.s)->level < 1)
						setmacro(b.s, v, 0);
				} else {
					p = strchr(b.s, ':');
					*p++ = 0;
					rule(words(b.s), words(p), 0, (char *)0);
				}
				free(b.s);
			}
		}
		firstarg = 0;
	}
	if (nfiles == 0) {
		if ((f = fopen("makefile", "r")) != 0)
			files[nfiles++] = "makefile";
		else if ((f = fopen("Makefile", "r")) != 0)
			files[nfiles++] = "Makefile";
		if (f)
			fclose(f);
	}
	for (i = 0; i < nfiles; i++) {
		if (!strcmp(files[i], "-"))
			f = stdin;
		else if ((f = fopen(files[i], "r")) == 0) {
			fprintf(stderr, "make: cannot open %s\n", files[i]);
			exit(2);
		}
		readfile(f, files[i]);
		if (f != stdin)
			fclose(f);
	}
	if (pflag)
		printdb();

	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
		signal(SIGINT, onintr);
	if (signal(SIGQUIT, SIG_IGN) != SIG_IGN)
		signal(SIGQUIT, onintr);

	for (i = 1; i < argc; i++)
		if (argv[i]) {
			ntargets++;
			n = lookup(argv[i], 1);
			outofdate = 0;
			errs |= make(n);
			if (!errs && !outofdate && !qflag)
				printf("`%s' is up to date.\n", n->name);
			if (errs && !kflag)
				break;
		}
	if (ntargets == 0) {
		if (!firstarg) {
			if (nfiles == 0 && !pflag) {
				fprintf(stderr, "make: no makefile and no target\n");
				exit(2);
			}
			exit(0);
		}
		outofdate = 0;
		errs = make(firstarg);
		if (!errs && !outofdate && !qflag)
			printf("`%s' is up to date.\n", firstarg->name);
	}
	if (errs) {
		fprintf(stderr, "Stop.\n");
		exit(1);
	}
	exit(qflag ? outofdate : 0);
}
