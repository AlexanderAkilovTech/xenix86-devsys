! regdump support: rawsys(num) makes system call num with no arguments and stores
! AX, BX, CX, DX and carry right after it returns.
	.globl	__syscal
	.text
	export	_rawsys
_rawsys:
	push	bp
	mov	bp,sp
	push	si
	push	di
	mov	ax,4[bp]
	mov	bx,#$1111
	mov	cx,#$2222
	mov	dx,#$3333
	call	__syscal
	mov	[_rax],ax
	mov	[_rbx],bx
	mov	[_rcx],cx
	mov	[_rdx],dx
	mov	ax,#0
	adc	ax,#0
	mov	[_rcf],ax
	pop	di
	pop	si
	pop	bp
	ret
	.data
	export	_rax
_rax:	.word	0
	export	_rbx
_rbx:	.word	0
	export	_rcx
_rcx:	.word	0
	export	_rdx
_rdx:	.word	0
	export	_rcf
_rcf:	.word	0
