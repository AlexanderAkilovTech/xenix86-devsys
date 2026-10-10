! fincdec.s - ++ and -- on a double or float at [BX]: Fpreincd, Fpredecd,
! Fpostincd, Fpostdecd, and Fpreincf ... Fpostdecf. The variable is updated
! in place. The prefix forms leave nothing on the stack (the compiler reads
! the variable again); the postfix forms push its old value, as it was:
! a double, or a float in the lower half of 8 bytes, as dtof leaves one.
include lib/fp/fpdefs.inc
	.globl	Faddc
	.globl	Fent
	.globl	Fone
	.globl	Fpk
	.globl	Fpkf
	.globl	Fret
	.globl	Funpk
	.globl	Funpkf
	.text

	export	Fpreincd
Fpreincd:
	xor	ax,ax
	jmp	pred
	export	Fpredecd
Fpredecd:
	mov	ax,#$8000
pred:
	call	Fent
	call	setup
	call	Funpk
	jmp	stored

	export	Fpreincf
Fpreincf:
	xor	ax,ax
	jmp	pref
	export	Fpredecf
Fpredecf:
	mov	ax,#$8000
pref:
	call	Fent
	call	setup
	call	Funpkf
	jmp	storef

	export	Fpostincd
Fpostincd:
	xor	ax,ax
	jmp	postd
	export	Fpostdecd
Fpostdecd:
	mov	ax,#$8000
postd:
	pop	cx			! room for the old value under the
	sub	sp,#8			! return address
	push	cx
	call	Fent
	call	setup
	mov	ax,4[si]		! the old value, as it is
	mov	X+4[bp],ax
	mov	ax,6[si]
	mov	X+6[bp],ax
	call	old2
	call	Funpk
stored:
	call	Faddc
	mov	di,W0[bp]
	call	Fpk
	br	Fret

	export	Fpostincf
Fpostincf:
	xor	ax,ax
	jmp	postf
	export	Fpostdecf
Fpostdecf:
	mov	ax,#$8000
postf:
	pop	cx
	sub	sp,#8
	push	cx
	call	Fent
	call	setup
	call	old2
	call	Funpkf
storef:
	call	Faddc
	mov	di,W0[bp]
	call	Fpkf
	br	Fret

! keep the variable's address in W0 and SI, point DI at A, and B = +1 or
! -1 (the sign in AX)
setup:
	mov	W0[bp],bx
	call	Fone
	mov	si,W0[bp]
	lea	di,A[bp]
	ret

! copy the first two words of the variable to X (keeps SI and DI)
old2:
	mov	ax,[si]
	mov	X[bp],ax
	mov	ax,2[si]
	mov	X+2[bp],ax
	ret
