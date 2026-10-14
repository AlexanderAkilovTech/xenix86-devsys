! sys25(a, b) - XENIX system call 25, stime, with BX = a and CX = b: which of
! the two ways it takes the time is what tests/m62/settime.c finds out
	.globl	__syscal
	.globl	__sysret
	.text
	export	_sys25
_sys25:
	push	bp
	mov	bp,sp
	push	si
	push	di
	mov	bx,4[bp]
	mov	cx,6[bp]
	mov	ax,#25
	call	__syscal
	br	__sysret
