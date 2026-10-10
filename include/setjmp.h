/* setjmp.h - xenix86-devsys */
/* saved IP, SP, BP, SI, DI, and one spare word */
typedef int jmp_buf[6];

int	setjmp();
int	longjmp();
