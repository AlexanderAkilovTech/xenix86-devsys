/* sudo - run one command as root, for an account allowed to, after its
 * password: between su, which gives a whole root shell, and nothing.
 *	sudo command [ argument ... ]
 * /etc/default/sudo says who may (USERS=, login names separated by commas
 * or spaces; root always may) and whose password is asked for:
 * PASSWORD=root, the default, root's own, as su asks; or PASSWORD=self,
 * the account's own, as 4.2BSD's sudo did. An account whose password is
 * empty is not asked for it. Three tries. Each use, allowed or not, is
 * written at the end of /usr/adm/sudolog. sudo is root's, set-user-id.
 * XENIX keeps the passwords in /etc/passwd, hashed with crypt(3) (M69).
 * xenix86-devsys system tools. */
#include <stdio.h>
#include <signal.h>

#define	DEFAULTS "/etc/default/sudo"
#define	LOG	"/usr/adm/sudolog"

extern char *crypt(), *getpass(), *strchr(), *ctime();
extern long time();

char	name[16];		/* the account running sudo */

/* the line of /etc/passwd for uid (by > -1) or for a name: its password
 * field in pw, the name in nm; 0, or -1 if there is none */
pwent(uid, nm, pw)
int uid;
char *nm, *pw;
{
	FILE *f;
	char line[256], *p, *q;
	int n;

	if ((f = fopen("/etc/passwd", "r")) == NULL)
		return -1;
	while (fgets(line, sizeof line, f)) {
		if ((p = strchr(line, ':')) == NULL)
			continue;
		*p++ = 0;
		if ((q = strchr(p, ':')) == NULL)
			continue;
		*q++ = 0;
		if (uid >= 0 ? atoi(q) != uid : strcmp(line, nm) != 0)
			continue;
		if (uid >= 0) {
			strncpy(nm, line, 15);
			nm[15] = 0;
		}
		strncpy(pw, p, 31);
		pw[31] = 0;
		fclose(f);
		return 0;
	}
	fclose(f);
	return -1;
}

/* a line at the end of the log: the time, the account, what happened */
logit(what, argc, argv)
char *what;
int argc;
char **argv;
{
	FILE *f;
	long t;
	int i;

	if ((f = fopen(LOG, "a")) == NULL)
		return;
	time(&t);
	fprintf(f, "%.15s %s %s:", ctime(&t) + 4, name, what);
	for (i = 1; i < argc; i++)
		fprintf(f, " %s", argv[i]);
	fprintf(f, "\n");
	fclose(f);
}

/* is the name in the USERS line of /etc/default/sudo? and whose password:
 * *self is 1 for PASSWORD=self */
allowed(self)
int *self;
{
	FILE *f;
	char line[256], *p, *q;
	int ok = 0, n = strlen(name);

	*self = 0;
	if ((f = fopen(DEFAULTS, "r")) == NULL)
		return 0;
	while (fgets(line, sizeof line, f)) {
		if (strncmp(line, "PASSWORD=", 9) == 0)
			*self = strncmp(line + 9, "self", 4) == 0;
		if (strncmp(line, "USERS=", 6) != 0)
			continue;
		for (p = line + 6; *p; p = q) {
			while (*p == ' ' || *p == ',' || *p == '\t')
				p++;
			for (q = p; *q && *q != ' ' && *q != ',' && *q != '\t' && *q != '\n'; q++)
				;
			if (q - p == n && strncmp(p, name, n) == 0)
				ok = 1;
			if (*q == '\n')
				break;
		}
	}
	fclose(f);
	return ok;
}

main(argc, argv)
int argc;
char **argv;
{
	char pw[32], who[16], prompt[40], *typed;
	int self, tries;

	if (argc < 2) {
		fprintf(stderr, "usage: sudo command [argument ...]\n");
		exit(2);
	}
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	if (pwent(getuid(), name, pw) < 0) {
		fprintf(stderr, "sudo: who are you? (no account for your user id)\n");
		exit(1);
	}
	if (getuid() != 0) {
		if (!allowed(&self)) {
			fprintf(stderr, "sudo: %s may not use sudo (it is not in %s)\n", name, DEFAULTS);
			logit("NOT ALLOWED", argc, argv);
			exit(1);
		}
		strcpy(who, self ? name : "root");
		if (pwent(-1, who, pw) < 0) {
			fprintf(stderr, "sudo: no account %s\n", who);
			exit(1);
		}
		if (pw[0]) {
			sprintf(prompt, "Password for %s: ", who);
			for (tries = 0; ; ) {
				typed = getpass(prompt);
				if (strcmp(crypt(typed, pw), pw) == 0)
					break;
				if (++tries == 3) {
					fprintf(stderr, "sudo: 3 wrong passwords\n");
					logit("WRONG PASSWORD", argc, argv);
					exit(1);
				}
				fprintf(stderr, "Sorry, try again.\n");
			}
		}
	}
	logit("ran", argc, argv);
	if (setgid(0) < 0 || setuid(0) < 0) {
		fprintf(stderr, "sudo: cannot become root (is sudo set-user-id to root?)\n");
		exit(1);
	}
	signal(SIGINT, SIG_DFL);
	signal(SIGQUIT, SIG_DFL);
	execvp(argv[1], argv + 1);
	fprintf(stderr, "sudo: %s: not found\n", argv[1]);
	exit(1);
}
