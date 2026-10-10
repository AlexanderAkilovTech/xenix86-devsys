/* sysprobe: open() with O_CREAT/O_TRUNC, and the layout of struct stat, checked
   against a file we make and against /etc/passwd as ls -l shows it */
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

main()
{
	struct stat st;
	int fd, n, ok = 1;
	char buf[8];
	extern long lseek(), time();
	long now = time((long *)0);

	unlink("/tmp/probe.tst");
	fd = open("/tmp/probe.tst", O_RDWR | O_CREAT | O_TRUNC, 0640);
	printf("open(O_RDWR|O_CREAT|O_TRUNC) = %d\n", fd);
	if (fd < 0)
		return 1;
	n = write(fd, "hello, stat", 11);
	lseek(fd, 0L, 0);
	n = read(fd, buf, 5);
	printf("wrote 11, read back %d: %.5s\n", n, buf);
	if (fstat(fd, &st) < 0)
		printf("fstat failed\n");
	printf("fstat: size %ld, mode %o, links %d, uid %d, mtime %ld s from now\n",
		st.st_size, st.st_mode, st.st_nlink, st.st_uid, st.st_mtime - now);
	ok = ok && st.st_size == 11 && st.st_mode == 0100640 && st.st_nlink == 1;
	close(fd);
	fd = open("/tmp/probe.tst", O_WRONLY | O_TRUNC);
	fstat(fd, &st);
	printf("reopened with O_TRUNC: size %ld\n", st.st_size);
	ok = ok && st.st_size == 0;
	close(fd);
	unlink("/tmp/probe.tst");
	if (stat("/etc/passwd", &st) == 0)
		printf("/etc/passwd: size %ld, mode %o, uid %d (ls -l said 432, -rw-r--r--, bin)\n",
			st.st_size, st.st_mode, st.st_uid);
	printf(ok ? "probe OK\n" : "probe FAILED\n");
	return !ok;
}
