/* stdio.c - xenix86-devsys libc: buffered standard I/O (K&R C) */
#include <stdio.h>

#define TCGETA	(('T' << 8) | 1)

FILE _iob[_NFILE] = {
	{ 0, 0, 0, _IOREAD, 0, 0 },
	{ 0, 0, 0, _IOWRT, 1, 0 },
	{ 0, 0, 0, _IOWRT | _IONBF, 2, 0 },
};

extern char *malloc();
extern long lseek();

isatty(fd)
int fd;
{
	char tbuf[20];

	return ioctl(fd, TCGETA, tbuf) >= 0;
}

/* give a stream its buffer on first use */
static
_getbuf(p)
register FILE *p;
{
	if (p->_flag & _IONBF) {
		p->_base = p->_ptr = 0;
		p->_bufsiz = 0;
		return;
	}
	if ((p->_base = malloc(BUFSIZ)) == NULL) {
		p->_flag |= _IONBF;
		p->_bufsiz = 0;
		return;
	}
	p->_flag |= _IOMYBUF;
	p->_bufsiz = BUFSIZ;
	if (p == stdout && isatty(1))
		p->_flag |= _IOLBF;
}

_filbuf(p)
register FILE *p;
{
	char c1;

	if (p->_flag & _IORW)
		p->_flag |= _IOREAD;
	if ((p->_flag & _IOREAD) == 0 || (p->_flag & (_IOEOF | _IOSTRG)))
		return EOF;
	if (p == stdin && (stdout->_flag & _IOLBF))
		fflush(stdout);
	if (p->_base == NULL && !(p->_flag & _IONBF))
		_getbuf(p);
	if (p->_flag & _IONBF) {
		p->_cnt = read(p->_file, &c1, 1);
		p->_cnt--;
		if (p->_cnt < 0) {
			p->_flag |= p->_cnt == -1 ? _IOEOF : _IOERR;
			p->_cnt = 0;
			return EOF;
		}
		return c1 & 0377;
	}
	p->_ptr = p->_base;
	p->_cnt = read(p->_file, p->_base, p->_bufsiz);
	if (--p->_cnt < 0) {
		p->_flag |= p->_cnt == -1 ? _IOEOF : _IOERR;
		p->_cnt = 0;
		return EOF;
	}
	return *p->_ptr++ & 0377;
}

_flsbuf(c, p)
int c;
register FILE *p;
{
	register int n;
	char c1;

	if (p->_flag & _IORW) {
		p->_flag |= _IOWRT;
		p->_flag &= ~_IOEOF;
	}
	if ((p->_flag & _IOWRT) == 0)
		return EOF;
	if (p->_base == NULL && !(p->_flag & (_IONBF | _IOSTRG)))
		_getbuf(p);
	if (p->_flag & _IOSTRG) {	/* sprintf: writing past the end is the caller's problem */
		*p->_ptr++ = c;
		return c & 0377;
	}
	if (p->_flag & _IONBF) {
		c1 = c;
		if (write(p->_file, &c1, 1) != 1) {
			p->_flag |= _IOERR;
			return EOF;
		}
		p->_cnt = 0;
		return c & 0377;
	}
	/* line-buffered streams come here for every character (see putc) */
	if (p->_flag & _IOLBF) {
		if (p->_ptr == NULL || p->_cnt < 0 && p->_ptr == p->_base + p->_bufsiz) {
			if (p->_ptr && fflush(p) == EOF)
				return EOF;
		}
		if (p->_ptr == NULL)
			p->_ptr = p->_base;
		*p->_ptr++ = c;
		p->_cnt = p->_base + p->_bufsiz - p->_ptr;
		if (c == '\n' || p->_cnt <= 0)
			if (fflush(p) == EOF)
				return EOF;
		return c & 0377;
	}
	/* fully buffered: buffer is full (or not started) */
	if (p->_ptr != NULL && (n = p->_ptr - p->_base) > 0)
		if (write(p->_file, p->_base, n) != n) {
			p->_flag |= _IOERR;
			return EOF;
		}
	p->_ptr = p->_base;
	*p->_ptr++ = c;
	p->_cnt = p->_bufsiz - 1;
	return c & 0377;
}

fflush(p)
register FILE *p;
{
	register int n;

	if ((p->_flag & (_IOWRT | _IOSTRG)) == _IOWRT && p->_base != NULL
	    && p->_ptr != NULL && (n = p->_ptr - p->_base) > 0) {
		if (write(p->_file, p->_base, n) != n) {
			p->_flag |= _IOERR;
			return EOF;
		}
	}
	if (p->_base != NULL && (p->_flag & _IOWRT)) {
		p->_ptr = p->_base;
		p->_cnt = (p->_flag & _IOLBF) ? 0 : p->_bufsiz;
	}
	if (p->_flag & _IORW)
		p->_flag &= ~_IOWRT;
	return 0;
}

_cleanup()
{
	register FILE *p;

	for (p = _iob; p < _iob + _NFILE; p++)
		if (p->_flag & _IOWRT)
			fflush(p);
}

static FILE *
_findiop()
{
	register FILE *p;

	for (p = _iob + 3; p < _iob + _NFILE; p++)
		if ((p->_flag & (_IOREAD | _IOWRT | _IORW)) == 0)
			return p;
	return NULL;
}

static FILE *
_endopen(file, mode, p)
char *file;
register char *mode;
register FILE *p;
{
	register int fd, rw;

	rw = mode[1] == '+' || (mode[1] && mode[2] == '+');
	switch (*mode) {
	case 'r':
		fd = open(file, rw ? 2 : 0);
		break;
	case 'w':
		fd = creat(file, 0666);
		if (fd >= 0 && rw) {
			close(fd);
			fd = open(file, 2);
		}
		break;
	case 'a':
		if ((fd = open(file, rw ? 2 : 1)) < 0)
			if ((fd = creat(file, 0666)) >= 0 && rw) {
				close(fd);
				fd = open(file, 2);
			}
		if (fd >= 0)
			lseek(fd, 0L, 2);
		break;
	default:
		return NULL;
	}
	if (fd < 0)
		return NULL;
	p->_cnt = 0;
	p->_base = p->_ptr = NULL;
	p->_bufsiz = 0;
	p->_file = fd;
	p->_flag = rw ? _IORW : (*mode == 'r' ? _IOREAD : _IOWRT);
	return p;
}

FILE *
fopen(file, mode)
char *file, *mode;
{
	register FILE *p;

	if ((p = _findiop()) == NULL)
		return NULL;
	return _endopen(file, mode, p);
}

FILE *
freopen(file, mode, p)
char *file, *mode;
register FILE *p;
{
	fclose(p);
	return _endopen(file, mode, p);
}

FILE *
fdopen(fd, mode)
int fd;
char *mode;
{
	register FILE *p;

	if ((p = _findiop()) == NULL)
		return NULL;
	p->_cnt = 0;
	p->_base = p->_ptr = NULL;
	p->_bufsiz = 0;
	p->_file = fd;
	p->_flag = mode[1] == '+' ? _IORW : (*mode == 'r' ? _IOREAD : _IOWRT);
	return p;
}

fclose(p)
register FILE *p;
{
	register int r = EOF;

	if (p->_flag & (_IOREAD | _IOWRT | _IORW)) {
		r = fflush(p);
		if (close(p->_file) < 0)
			r = EOF;
		if (p->_flag & _IOMYBUF)
			free(p->_base);
	}
	p->_flag = 0;
	p->_cnt = 0;
	p->_base = p->_ptr = NULL;
	return r;
}

setbuf(p, buf)
register FILE *p;
char *buf;
{
	if (p->_base != NULL && (p->_flag & _IOMYBUF))
		free(p->_base);
	p->_flag &= ~(_IOMYBUF | _IONBF | _IOLBF);
	if ((p->_base = buf) == NULL) {
		p->_flag |= _IONBF;
		p->_bufsiz = 0;
	} else
		p->_bufsiz = BUFSIZ;
	p->_ptr = p->_base;
	p->_cnt = 0;
}

ungetc(c, p)
int c;
register FILE *p;
{
	if (c == EOF || p->_base == NULL || p->_ptr <= p->_base)
		return EOF;
	p->_cnt++;
	*--p->_ptr = c;
	p->_flag &= ~_IOEOF;
	return c;
}

fseek(p, off, whence)
register FILE *p;
long off;
int whence;
{
	long r;

	if (p->_flag & _IOREAD) {
		if (whence == 1 && p->_base != NULL)
			off -= p->_cnt;
		p->_cnt = 0;
		p->_ptr = p->_base;
	} else if (p->_flag & (_IOWRT | _IORW))
		fflush(p);
	if (p->_flag & _IORW) {
		p->_cnt = 0;
		p->_ptr = p->_base;
		p->_flag &= ~(_IOREAD | _IOWRT);
	}
	p->_flag &= ~_IOEOF;
	r = lseek(p->_file, off, whence);
	return r < 0 ? -1 : 0;
}

long
ftell(p)
register FILE *p;
{
	long pos;

	pos = lseek(p->_file, 0L, 1);
	if (pos < 0)
		return pos;
	if (p->_flag & _IOREAD)
		pos -= p->_cnt;
	else if ((p->_flag & _IOWRT) && p->_base != NULL && p->_ptr != NULL)
		pos += p->_ptr - p->_base;
	return pos;
}

rewind(p)
FILE *p;
{
	fseek(p, 0L, 0);
	clearerr(p);
}

fgetc(p)
FILE *p;
{
	return getc(p);
}

fputc(c, p)
int c;
FILE *p;
{
	return putc(c, p);
}

/* function versions of the getchar/putchar macros, for code without <stdio.h> */
#undef getchar
#undef putchar
getchar()
{
	return getc(stdin);
}

putchar(c)
int c;
{
	return putc(c, stdout);
}

char *
fgets(s, n, p)
char *s;
register int n;
register FILE *p;
{
	register int c;
	register char *q = s;

	while (--n > 0 && (c = getc(p)) != EOF)
		if ((*q++ = c) == '\n')
			break;
	*q = 0;
	return (c == EOF && q == s) ? NULL : s;
}

char *
gets(s)
char *s;
{
	register int c;
	register char *q = s;

	while ((c = getc(stdin)) != '\n' && c != EOF)
		*q++ = c;
	*q = 0;
	return (c == EOF && q == s) ? NULL : s;
}

fputs(s, p)
register char *s;
register FILE *p;
{
	register int r = 0;

	while (*s)
		r = putc(*s++, p);
	return r;
}

puts(s)
register char *s;
{
	while (*s)
		putc(*s++, stdout);
	return putc('\n', stdout);
}

fread(ptr, size, count, p)
char *ptr;
unsigned size, count;
register FILE *p;
{
	register unsigned n = size * count;
	register int c;
	unsigned done = 0;

	while (done < n && (c = getc(p)) != EOF)
		ptr[done++] = c;
	return size ? done / size : 0;
}

fwrite(ptr, size, count, p)
char *ptr;
unsigned size, count;
register FILE *p;
{
	register unsigned n = size * count;
	unsigned done = 0;

	while (done < n) {
		putc(ptr[done], p);
		if (ferror(p))
			break;
		done++;
	}
	return size ? done / size : 0;
}
