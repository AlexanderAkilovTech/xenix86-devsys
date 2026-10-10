! huge.s - xenix86-devsys libc: _huge, a double infinity, which math.h
! calls HUGE_VAL (and HUGE).
	.data
	export	__huge
__huge:
	.word	0, 0, 0, $7FF0
