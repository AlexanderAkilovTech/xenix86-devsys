! fcomp.s - comparison: Fcomp, Fcompd, Fcompf, Ftst, Ftstd, Ftstf.
! They return the flags as a signed integer comparison of X with Y (or with
! [BX], or with 0) would set them, for the compiler's jl, jg, je, ... after
! them. Fcomp drops both doubles, Fcompd and Fcompf drop X, Ftst drops X.
! -0 equals +0. A NaN compares unequal to everything, itself included, and
! (since a flag result has no "unordered") as greater.
include lib/fp/fpdefs.inc
	.globl	Fent
	.globl	Fpk
	.globl	Fret
	.globl	Fret16
	.globl	Fret8
	.globl	Funpkf
	.text

	export	Fcomp
Fcomp:
	call	Fent
	lea	si,X[bp]
	lea	di,Y[bp]
	call	Fcmpr
	or	ax,ax
	br	Fret16

	export	Fcompd
Fcompd:
	call	Fent
	lea	si,X[bp]
	mov	di,bx
	call	Fcmpr
	or	ax,ax
	br	Fret8

	export	Fcompf
Fcompf:
	call	Fent
	call	ldfw
	lea	si,X[bp]
	lea	di,W0[bp]
	call	Fcmpr
	or	ax,ax
	br	Fret8

	export	Ftst
Ftst:
	call	Fent
	lea	bx,X[bp]
	call	Ftz
	br	Fret8

	export	Ftstd
Ftstd:
	call	Fent
	call	Ftz
	br	Fret

	export	Ftstf
Ftstf:
	call	Fent
	call	ldfw
	lea	bx,W0[bp]
	call	Ftz
	br	Fret

! the float at [BX] as a double in W0..W3
ldfw:
	mov	si,bx
	lea	di,A[bp]
	call	Funpkf
	lea	di,W0[bp]
	br	Fpk

! Fcmpr: compare the doubles at [SI] and [DI]: AX = -1, 0 or 1.
	export	Fcmpr
Fcmpr:
	mov	bx,si
	call	isnan
	jnz	cun
	mov	bx,di
	call	isnan
	jnz	cun
	mov	ax,6[si]		! both zero, whatever their signs?
	or	ax,6[di]
	and	ax,#$7FFF
	or	ax,4[si]
	or	ax,2[si]
	or	ax,[si]
	or	ax,4[di]
	or	ax,2[di]
	or	ax,[di]
	jz	ceq
	mov	ax,6[si]
	mov	dx,6[di]
	xor	dx,ax
	js	cdiff			! the signs differ
	cmp	ax,6[di]		! the same sign: compare the magnitudes
	jne	cne
	mov	ax,4[si]
	cmp	ax,4[di]
	jne	cne
	mov	ax,2[si]
	cmp	ax,2[di]
	jne	cne
	mov	ax,[si]
	cmp	ax,[di]
	jne	cne
ceq:
	xor	ax,ax
	ret
cne:
	mov	ax,#1			! |x| > |y|
	jae	cn1
	neg	ax			! |x| < |y|
cn1:
	test	byte 7[si],#$80		! both negative: the other way round
	jz	cn2
	neg	ax
cn2:
	ret
cdiff:
	mov	ax,#1
	jmp	cn1
cun:
	mov	ax,#1
	ret

! Ftz: the flags for comparing the double at [BX] with 0
Ftz:
	call	isnan
	jnz	tpos
	mov	ax,6[bx]
	and	ax,#$7FFF
	or	ax,4[bx]
	or	ax,2[bx]
	or	ax,[bx]
	jz	tz
	mov	ax,#1
	test	byte 7[bx],#$80
	jz	tz
	neg	ax
tz:
	or	ax,ax
	ret
tpos:
	mov	ax,#1
	or	ax,ax
	ret

! isnan: ZF clear if the double at [BX] is a NaN. Changes AX.
isnan:
	mov	ax,6[bx]
	and	ax,#$7FF0
	cmp	ax,#$7FF0
	jne	nn
	mov	ax,6[bx]
	and	ax,#$000F
	or	ax,4[bx]
	or	ax,2[bx]
	or	ax,[bx]
	ret
nn:
	xor	ax,ax
	ret
