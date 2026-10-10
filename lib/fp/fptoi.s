! fptoi.s - double or float at [BX] to an integer, truncating toward zero
! as C requires: dtoc, dtouc, dtos, dtous, dtoi, dtoui, dtol, dtoul, and the
! same from a float, ftoc ... ftoul. The result is in AX (AL), or BX:AX for
! a long (BX high). Values out of range, and NaNs, give what the 8087 gives
! storing them: 8000 (hex) for int and char, 8000 0000 for long, and the low
! bits of those for the unsigned types (unsigned long through a 64-bit
! integer, so it covers 0 to 2^32 - 1 and wraps negatives).
include lib/fp/fpdefs.inc
	.globl	Fent
	.globl	Fret
	.globl	Fshr
	.globl	Funpk
	.globl	Funpkf
	.text

	export	dtoc
	export	dtouc
	export	dtos
	export	dtoi
dtoc:
dtouc:
dtos:
dtoi:
	call	Fent
	call	ldad
	jmp	ti16

	export	ftoc
	export	ftouc
	export	ftos
	export	ftoi
ftoc:
ftouc:
ftos:
ftoi:
	call	Fent
	call	ldaf
	jmp	ti16

	export	dtous
	export	dtoui
	export	dtol
dtous:
dtoui:
dtol:
	call	Fent
	call	ldad
	jmp	ti32

	export	ftous
	export	ftoui
	export	ftol
ftous:
ftoui:
ftol:
	call	Fent
	call	ldaf
	jmp	ti32

	export	dtoul
dtoul:
	call	Fent
	call	ldad
	jmp	tiul

	export	ftoul
ftoul:
	call	Fent
	call	ldaf
	jmp	tiul

ldad:
	mov	si,bx
	lea	di,A[bp]
	br	Funpk

ldaf:
	mov	si,bx
	lea	di,A[bp]
	br	Funpkf

ti16:
	call	Ftrunc
	or	si,si
	jnz	t16i
	or	dx,cx
	or	dx,bx
	jnz	t16i
	cmp	ax,#$8000
	jae	t16i
	test	byte A_SGN+1[bp],#$80
	jz	t16r
	neg	ax
t16r:
	br	Fret
t16i:
	mov	ax,#$8000
	br	Fret

ti32:
	call	Ftrunc
	or	si,si
	jnz	t32i
	or	dx,cx
	jnz	t32i
	cmp	bx,#$8000
	jae	t32i
tneg:
	test	byte A_SGN+1[bp],#$80
	jz	t32r
	neg	bx
	neg	ax
	sbb	bx,#0
t32r:
	br	Fret
t32i:
	mov	bx,#$8000
	xor	ax,ax
	br	Fret

tiul:
	call	Ftrunc
	or	si,si
	jz	tneg
	xor	ax,ax
	xor	bx,bx
	br	Fret

! Ftrunc: |A| truncated to an integer, in DX:CX:BX:AX; SI nonzero if it
! needs more than 63 bits (or A is infinite or a NaN).
	export	Ftrunc
Ftrunc:
	xor	ax,ax
	xor	bx,bx
	xor	cx,cx
	xor	dx,dx
	xor	si,si
	mov	di,A_KIND[bp]
	cmp	di,#K_FIN
	je	tr1
	cmp	di,#K_ZERO
	je	tr9
	inc	si
tr9:
	ret
tr1:
	mov	di,A_EXP[bp]
	sub	di,#1023
	jl	tr9			! below 1
	cmp	di,#62
	jg	trbig
	mov	ax,A_M0[bp]
	mov	bx,A_M1[bp]
	mov	cx,A_M2[bp]
	mov	dx,A_M3[bp]
	mov	si,#63
	sub	si,di
	call	Fshr
	xor	si,si
	ret
trbig:
	inc	si
	ret
