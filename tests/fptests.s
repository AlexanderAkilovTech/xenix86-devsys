! fptests.s - for fptest.c: calls the floating point helpers BCC does not
! generate itself (the float-operand forms, and Fneg and Ftst on the stack).
	.globl	Faddf
	.globl	Fsubf
	.globl	Fmulf
	.globl	Fdivf
	.globl	Fcompf
	.globl	Fneg
	.globl	Ftst
	.text

! double fopf(op, x, p) int op; double x; float *p;
! x + *p, x - *p, x * *p or x / *p for op 0 to 3
	export	_fopf
_fopf:
	push	bp
	mov	bp,sp
	push	si
	push	di
	push	12[bp]
	push	10[bp]
	push	8[bp]
	push	6[bp]
	mov	bx,14[bp]
	mov	ax,4[bp]
	cmp	ax,#0
	jne	o1
	call	Faddf
	jmp	o9
o1:	cmp	ax,#1
	jne	o2
	call	Fsubf
	jmp	o9
o2:	cmp	ax,#2
	jne	o3
	call	Fmulf
	jmp	o9
o3:	call	Fdivf
o9:	pop	ax
	pop	bx
	pop	cx
	pop	dx
	pop	di
	pop	si
	pop	bp
	ret

! int fcmpf(x, p) double x; float *p;  -1, 0, 1 from Fcompf's flags
	export	_fcmpf
_fcmpf:
	push	bp
	mov	bp,sp
	push	si
	push	di
	push	10[bp]
	push	8[bp]
	push	6[bp]
	push	4[bp]
	mov	bx,12[bp]
	call	Fcompf
	jmp	flags

! int ftst(x) double x;  -1, 0, 1 from Ftst's flags
	export	_ftst
_ftst:
	push	bp
	mov	bp,sp
	push	si
	push	di
	push	10[bp]
	push	8[bp]
	push	6[bp]
	push	4[bp]
	call	Ftst
flags:
	mov	ax,#0
	je	f9
	mov	ax,#1
	jg	f9
	mov	ax,#-1
f9:	pop	di
	pop	si
	pop	bp
	ret

! double fneg(x) double x;  through Fneg
	export	_fneg
_fneg:
	push	bp
	mov	bp,sp
	push	si
	push	di
	push	10[bp]
	push	8[bp]
	push	6[bp]
	push	4[bp]
	call	Fneg
	pop	ax
	pop	bx
	pop	cx
	pop	dx
	pop	di
	pop	si
	pop	bp
	ret
