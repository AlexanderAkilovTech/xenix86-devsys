! fpcore.s - the frame, unpacking, rounding and packing shared by the
! software floating point routines. See fpdefs.inc for the layout.
include lib/fp/fpdefs.inc
	.text

! Fent: called first thing by an entry point; builds the frame.
! Changes CX only, so arguments in AX and BX survive.
	export	Fent
Fent:
	pop	cx
	push	bp
	push	si
	push	di
	mov	bp,sp
	sub	sp,#FRAME
	jmp	cx

! Fret, Fret8, Fret16: undo the frame and return to the compiled code,
! dropping 0, 8 or 16 bytes of operands. They leave the flags alone.
	export	Fret
Fret:
	mov	sp,bp
	pop	di
	pop	si
	pop	bp
	ret

	export	Fret8
Fret8:
	mov	sp,bp
	pop	di
	pop	si
	pop	bp
	ret	8

	export	Fret16
Fret16:
	mov	sp,bp
	pop	di
	pop	si
	pop	bp
	ret	16

! Fstx, Fsty8: pack A into X (or Y) and return (to be jumped to).
	export	Fstx
Fstx:
	lea	di,X[bp]
	call	Fpk
	jmp	Fret

	export	Fsty8
Fsty8:
	lea	di,Y[bp]
	call	Fpk
	jmp	Fret8

! Fldxd, Fldxf, Fldxy: unpack X into A, and into B the double or float at
! [BX], or Y.
	export	Fldxd
Fldxd:
	mov	si,bx
	lea	di,B[bp]
	call	Funpk
	jmp	ldx

	export	Fldxf
Fldxf:
	mov	si,bx
	lea	di,B[bp]
	call	Funpkf
	jmp	ldx

	export	Fldxy
Fldxy:
	lea	si,Y[bp]
	lea	di,B[bp]
	call	Funpk
ldx:
	lea	si,X[bp]
	lea	di,A[bp]
	jmp	Funpk

! Funpk: unpack the double at [SI] into [DI]. Changes AX, BX, CX, DX.
	export	Funpk
Funpk:
	push	si
	mov	dx,6[si]
	mov	ax,dx
	and	ax,#$8000
	mov	U_SGN[di],ax
	mov	ax,dx
	and	ax,#$7FF0
	mov	cl,#4
	shr	ax,cl			! the biased exponent
	and	dx,#$000F		! the top 4 bits of the fraction
	mov	cx,4[si]
	mov	bx,2[si]
	mov	si,[si]
	xchg	ax,si			! SI = exponent, DX:CX:BX:AX = fraction
	mov	word U_KIND[di],#K_FIN
	cmp	si,#$7FF
	je	upspec
	or	si,si
	jz	updenorm
	or	dl,#$10			! the leading 1 of a normal number
	call	shl11
	jmp	upm
updenorm:
	call	Fiszr
	jz	upzero
	call	shl11
	inc	si			! a denormal's exponent is 1, unnormalized
	call	Fnrm
	jmp	upm
upzero:
	mov	word U_KIND[di],#K_ZERO
	jmp	upm
upspec:					! infinity or NaN
	call	Fiszr
	mov	word U_KIND[di],#K_INF
	jz	upm
	mov	word U_KIND[di],#K_NAN
	call	shl11
upm:
	mov	U_M0[di],ax
	mov	U_M1[di],bx
	mov	U_M2[di],cx
	mov	U_M3[di],dx
	mov	U_EXP[di],si
	pop	si
	ret

! Funpkf: unpack the float at [SI] into [DI]. Changes AX, BX, CX, DX.
	export	Funpkf
Funpkf:
	push	si
	mov	dx,2[si]
	mov	ax,dx
	and	ax,#$8000
	mov	U_SGN[di],ax
	mov	ax,dx
	and	ax,#$7F80
	mov	cl,#7
	shr	ax,cl			! the biased exponent
	and	dx,#$007F		! the top 7 bits of the fraction
	mov	cx,[si]
	mov	si,ax
	xor	ax,ax
	xor	bx,bx
	mov	dh,dl			! fraction << 40
	mov	dl,ch
	mov	ch,cl
	xor	cl,cl
	mov	word U_KIND[di],#K_FIN
	cmp	si,#$FF
	je	ufspec
	or	si,si
	jz	ufden
	or	dh,#$80
	add	si,#1023-127
	jmp	upm
ufden:
	call	Fiszr
	jz	upzero
	mov	si,#1-127+1023
	call	Fnrm
	jmp	upm
ufspec:					! infinity or NaN
	call	Fiszr
	mov	word U_KIND[di],#K_INF
	jz	upm
	mov	word U_KIND[di],#K_NAN
	jmp	upm

! Fiszr: ZF set if DX:CX:BX:AX is zero; changes nothing else.
	export	Fiszr
Fiszr:
	push	ax
	or	ax,bx
	or	ax,cx
	or	ax,dx
	pop	ax
	ret

! Fnrm: shift DX:CX:BX:AX (not zero) left until its top bit is set,
! taking the shift from the exponent in SI.
	export	Fnrm
Fnrm:
	or	dx,dx
	jnz	nr2
nr1:
	mov	dx,cx
	mov	cx,bx
	mov	bx,ax
	xor	ax,ax
	sub	si,#16
	or	dx,dx
	jz	nr1
nr2:
	js	nr4
nr3:
	shl	ax,1
	rcl	bx,1
	rcl	cx,1
	rcl	dx,1
	dec	si
	or	dx,dx
	jns	nr3
nr4:
	ret

! Fshr: shift DX:CX:BX:AX right SI bits; DI nonzero if a 1 was shifted
! out. Changes SI and DI.
	export	Fshr
Fshr:
	xor	di,di
	cmp	si,#64
	jb	sr1
	or	di,ax
	or	di,bx
	or	di,cx
	or	di,dx
	xor	ax,ax
	xor	bx,bx
	xor	cx,cx
	xor	dx,dx
	ret
sr1:
	cmp	si,#16
	jb	sr2
	or	di,ax
	mov	ax,bx
	mov	bx,cx
	mov	cx,dx
	xor	dx,dx
	sub	si,#16
	jmp	sr1
sr2:
	or	si,si
	jz	sr9
sr3:
	shr	dx,1
	rcr	cx,1
	rcr	bx,1
	rcr	ax,1
	jnc	sr4
	or	di,#1
sr4:
	dec	si
	jnz	sr3
sr9:
	ret

! Fsrj: shift right like Fshr, but set bit 0 if anything was lost (the
! sticky bit), so later rounding still sees it. Keeps DI.
	export	Fsrj
Fsrj:
	push	di
	call	Fshr
	or	di,di
	jz	sj1
	or	al,#1
sj1:
	pop	di
	ret

! Frnd2: DX:CX:BX:AX has two bits below the last one kept: the round bit
! and the sticky bit. Drop them, rounding to nearest, ties to even.
Frnd2:
	mov	si,ax
	shr	dx,1
	rcr	cx,1
	rcr	bx,1
	rcr	ax,1
	shr	dx,1
	rcr	cx,1
	rcr	bx,1
	rcr	ax,1
	test	si,#2
	jz	rn9
	test	si,#5
	jz	rn9
	add	ax,#1
	adc	bx,#0
	adc	cx,#0
	adc	dx,#0
rn9:
	ret

! Fpk: pack A into the double at [DI], rounding. Changes AX, BX, CX, DX, SI.
	export	Fpk
Fpk:
	mov	ax,A_M0[bp]
	mov	bx,A_M1[bp]
	mov	cx,A_M2[bp]
	mov	dx,A_M3[bp]
	mov	si,A_KIND[bp]
	cmp	si,#K_FIN
	je	pkfin
	cmp	si,#K_NAN
	je	pknan
	cmp	si,#K_ZERO
	jne	pkinf
	xor	ax,ax
	xor	bx,bx
	xor	cx,cx
	xor	dx,dx
	jmp	pkout
pkinf:
	xor	ax,ax
	xor	bx,bx
	xor	cx,cx
	mov	dx,#$7FF0
	jmp	pkout
pknan:
	mov	si,#11
	call	Fsrj
	or	dx,#$7FF8		! exponent all ones, and quiet
	jmp	pkout
pkfin:
	mov	si,A_EXP[bp]
	cmp	si,#$7FF
	jge	pkinf			! overflow
	mov	si,#10			! shift right 11, or more for a denormal,
	sub	si,A_EXP[bp]		! less the two rounding bits
	cmp	si,#9
	jge	pk1
	mov	si,#9
pk1:
	call	Fsrj
	call	Frnd2
	mov	si,A_EXP[bp]		! add the exponent, less 1 for the leading 1
	dec	si			! (a denormal has none: 0); a carry out of
	jg	pk2			! the fraction carries into the exponent
	xor	si,si
pk2:
	push	cx
	mov	cl,#4
	shl	si,cl
	pop	cx
	add	dx,si
pkout:
	or	dx,A_SGN[bp]
	mov	[di],ax
	mov	2[di],bx
	mov	4[di],cx
	mov	6[di],dx
	ret

! Fpkf: pack A into the float at [DI], rounding. Changes AX, BX, CX, DX, SI.
	export	Fpkf
Fpkf:
	mov	ax,A_M0[bp]
	mov	bx,A_M1[bp]
	mov	cx,A_M2[bp]
	mov	dx,A_M3[bp]
	mov	si,A_KIND[bp]
	cmp	si,#K_FIN
	je	pffin
	cmp	si,#K_NAN
	je	pfnan
	xor	ax,ax
	xor	bx,bx
	cmp	si,#K_ZERO
	je	pfout
pfinf:
	xor	ax,ax
	mov	bx,#$7F80
	jmp	pfout
pfnan:					! the top of the fraction, quiet
	mov	al,ch
	mov	ah,dl
	mov	bl,dh
	xor	bh,bh
	or	bx,#$7FC0
	jmp	pfout
pffin:
	mov	si,A_EXP[bp]
	sub	si,#1023-127		! the float's biased exponent
	cmp	si,#$FF
	jge	pfinf
	push	si
	neg	si			! shift right 40, more for a denormal,
	add	si,#39			! less the two rounding bits
	cmp	si,#38
	jge	pf1
	mov	si,#38
pf1:
	call	Fsrj
	call	Frnd2
	pop	si
	dec	si
	jg	pf2
	xor	si,si
pf2:
	push	cx
	mov	cl,#7
	shl	si,cl
	pop	cx
	add	bx,si
pfout:
	or	bx,A_SGN[bp]
	mov	[di],ax
	mov	2[di],bx
	ret

! Fnan: the result of an operation with a NaN operand: the NaN made quiet
! (if both are NaNs, the one whose fraction is larger once both are quiet),
! in A.
	export	Fnan
Fnan:
	or	byte A_M3+1[bp],#$40	! quiet (no harm if A is not a NaN)
	or	byte B_M3+1[bp],#$40
	cmp	word A_KIND[bp],#K_NAN
	je	fn1
	jmp	Fcpyb
fn1:
	cmp	word B_KIND[bp],#K_NAN
	jne	fnq
	mov	ax,A_M3[bp]
	cmp	ax,B_M3[bp]
	jne	fn2
	mov	ax,A_M2[bp]
	cmp	ax,B_M2[bp]
	jne	fn2
	mov	ax,A_M1[bp]
	cmp	ax,B_M1[bp]
	jne	fn2
	mov	ax,A_M0[bp]
	cmp	ax,B_M0[bp]
fn2:
	jb	Fcpyb
fnq:
	ret

! Fdnan: the default NaN in A, as the 8087 makes for an invalid operation.
	export	Fdnan
Fdnan:
	mov	word A_KIND[bp],#K_NAN
	mov	word A_SGN[bp],#$8000
	mov	word A_M3[bp],#$4000
	xor	ax,ax
	mov	A_M2[bp],ax
	mov	A_M1[bp],ax
	mov	A_M0[bp],ax
	ret

! Fcpyb: A = B. Fswap: exchange A and B. Change AX, CX, SI, DI.
	export	Fcpyb
Fcpyb:
	lea	si,B[bp]
	lea	di,A[bp]
	mov	cx,#7
cb1:
	mov	ax,[si]
	mov	[di],ax
	inc	si
	inc	si
	inc	di
	inc	di
	loop	cb1
	ret

	export	Fswap
Fswap:
	lea	si,B[bp]
	lea	di,A[bp]
	mov	cx,#7
sw1:
	mov	ax,[si]
	xchg	ax,[di]
	mov	[si],ax
	inc	si
	inc	si
	inc	di
	inc	di
	loop	sw1
	ret

! Fone: B = 1, with the sign in AX.
	export	Fone
Fone:
	mov	B_SGN[bp],ax
	mov	word B_KIND[bp],#K_FIN
	mov	word B_EXP[bp],#1023
	mov	word B_M3[bp],#$8000
	xor	ax,ax
	mov	B_M2[bp],ax
	mov	B_M1[bp],ax
	mov	B_M0[bp],ax
	ret

! shift DX:CX:BX:AX left 11 bits
shl11:
	mov	dh,dl
	mov	dl,ch
	mov	ch,cl
	mov	cl,bh
	mov	bh,bl
	mov	bl,ah
	mov	ah,al
	xor	al,al
	shl	ax,1
	rcl	bx,1
	rcl	cx,1
	rcl	dx,1
	shl	ax,1
	rcl	bx,1
	rcl	cx,1
	rcl	dx,1
	shl	ax,1
	rcl	bx,1
	rcl	cx,1
	rcl	dx,1
	ret
