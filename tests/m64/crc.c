/* crc - for each file named on the standard input, one a line: its mode,
 * links, owner, group and size, and the CRC-32 of its bytes (as zlib's
 * crc32), for checking an installed set against the files it was made
 * from, on the host (M64, check.py). A dot on the standard error for each
 * file, to show it is working.
 * xenix86-devsys test program. */
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

unsigned short tlo[256], thi[256];
char	buf[4096];

main()
{
	char name[200];
	struct stat st;
	unsigned short lo, hi, l, h;
	int i, k, n, fd;
	register char *p;

	/* the table, in two 16-bit halves: 0xEDB88320 reflected */
	for (i = 0; i < 256; i++) {
		l = i;
		h = 0;
		for (k = 0; k < 8; k++)
			if (l & 1) {
				l = (l >> 1 | h << 15) ^ 0x8320;
				h = h >> 1 ^ 0xEDB8;
			} else {
				l = l >> 1 | h << 15;
				h >>= 1;
			}
		tlo[i] = l;
		thi[i] = h;
	}
	while (fgets(name, sizeof name, stdin)) {
		if ((p = (char *)strchr(name, '\n')) != 0)
			*p = 0;
		fputc('.', stderr);
		if (stat(name, &st) < 0) {
			printf("%s missing\n", name);
			continue;
		}
		printf("%s %o %d %d %d %ld", name, st.st_mode, st.st_nlink, st.st_uid, st.st_gid, st.st_size);
		if ((st.st_mode & S_IFMT) == S_IFREG) {
			if ((fd = open(name, 0)) < 0) {
				printf(" unreadable\n");
				continue;
			}
			lo = hi = 0xFFFF;
			while ((n = read(fd, buf, sizeof buf)) > 0)
				for (p = buf; n > 0; n--) {
					i = (lo ^ *p++) & 0xFF;
					lo = (lo >> 8 | hi << 8) ^ tlo[i];
					hi = hi >> 8 ^ thi[i];
				}
			close(fd);
			printf(" %04x%04x", hi ^ 0xFFFF, lo ^ 0xFFFF);
		}
		printf("\n");
	}
	fputc('\n', stderr);
	exit(0);
}
