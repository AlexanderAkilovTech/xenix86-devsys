! fpush.s - moving doubles and floats: Fpushd, Fpushf, Fnegd, Fnegf, Fneg,
! Fpulld, Fpullf, dtof.
include lib/fp/fpdefs.inc
	.globl	Fent
	.globl	Fpkf
	.globl	Fret
	.globl	Fret8
	.globl	Fstx
	.globl	Funpk
	.globl	Funpkf
	.text

! Fpushd: push the double at [BX]
	export	Fpushd
Fpushd:
	pop	cx
	push	6[bx]
	push	4[bx]
	push	2[bx]
	push	[bx]
	jmp	cx

! Fnegd: push the double at [BX], negated
	export	Fnegd
Fnegd:
	pop	cx
	mov	dx,6[bx]
	xor	dh,#$80
	push	dx
	push	4[bx]
	push	2[bx]
	push	[bx]
	jmp	cx

! Fneg: negate the double on top of the stack
	export	Fneg
Fneg:
	mov	bx,sp
	xor	byte 9[bx],#$80
	ret

! Fpulld: pop the double on top of the stack into [BX]
	export	Fpulld
Fpulld:
	pop	cx
	pop	[bx]
	pop	2[bx]
	pop	4[bx]
	pop	6[bx]
	jmp	cx

! Fpushf: push the float at [BX] as a double; Fnegf: negated
	export	Fpushf
Fpushf:
	xor	ax,ax
	jmp	pf
	export	Fnegf
Fnegf:
	mov	ax,#$8000
pf:
	pop	cx
	sub	sp,#8
	push	cx
	call	Fent
	push	ax
	mov	si,bx
	lea	di,A[bp]
	call	Funpkf
	pop	ax
	xor	A_SGN[bp],ax
	br	Fstx

! Fpullf: pop the double on top of the stack into the float at [BX]
	export	Fpullf
Fpullf:
	call	Fent
	mov	W0[bp],bx
	lea	si,X[bp]
	lea	di,A[bp]
	call	Funpk
	mov	di,W0[bp]
	call	Fpkf
	br	Fret8

! dtof: push the double at [BX] as a float, in 8 bytes of stack (the float
! in the lower 4)
	export	dtof
dtof:
	pop	cx
	sub	sp,#8
	push	cx
	call	Fent
	mov	si,bx
	lea	di,A[bp]
	call	Funpk
	lea	di,X[bp]
	call	Fpkf
	br	Fret
