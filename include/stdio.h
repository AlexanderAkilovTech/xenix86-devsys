/* stdio.h - xenix86-devsys: standard I/O (Version 7 / System V style) */
#ifndef _STDIO_H
#define _STDIO_H

#define BUFSIZ	512
#define _NFILE	20
#define EOF	(-1)
#ifndef NULL
#define NULL	0
#endif

typedef struct _iobuf {
	char	*_ptr;		/* next character position */
	int	_cnt;		/* characters left to read, or space left to write */
	char	*_base;		/* buffer */
	int	_flag;
	int	_file;		/* file descriptor */
	int	_bufsiz;
} FILE;

#define _IOREAD		01
#define _IOWRT		02
#define _IONBF		04
#define _IOMYBUF	010
#define _IOEOF		020
#define _IOERR		040
#define _IOSTRG		0100
#define _IOLBF		0200
#define _IORW		0400

extern FILE _iob[_NFILE];

#define stdin	(&_iob[0])
#define stdout	(&_iob[1])
#define stderr	(&_iob[2])

#define getc(p)		(--(p)->_cnt >= 0 ? *(p)->_ptr++ & 0377 : _filbuf(p))
#define putc(x, p)	(--(p)->_cnt >= 0 && !((p)->_flag & _IOLBF) ? \
			 (*(p)->_ptr++ = (x)) & 0377 : _flsbuf((x), (p)))
#define getchar()	getc(stdin)
#define putchar(x)	putc((x), stdout)
#define feof(p)		(((p)->_flag & _IOEOF) != 0)
#define ferror(p)	(((p)->_flag & _IOERR) != 0)
#define fileno(p)	((p)->_file)
#define clearerr(p)	((p)->_flag &= ~(_IOERR | _IOEOF))

extern FILE *fopen(), *fdopen(), *freopen();
extern char *fgets(), *gets();
extern long ftell();

#endif
