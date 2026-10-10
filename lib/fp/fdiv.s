! fdiv.s - division: Fdiv, Fdivd, Fdivf (operands as in fadd.s; X / Y).
include lib/fp/fpdefs.inc
	.globl	Fdnan
	.globl	Fent
	.globl	Fldxd
	.globl	Fldxf
	.globl	Fldxy
	.globl	Fnan
	.globl	Fstx
	.globl	Fsty8
	.text

	export	Fdiv
Fdiv:
	call	Fent
	call	Fldxy
	call	Fdivc
	br	Fsty8

	export	Fdivd
Fdivd:
	call	Fent
	call	Fldxd
	call	Fdivc
	br	Fstx

	export	Fdivf
Fdivf:
	call	Fent
	call	Fldxf
	call	Fdivc
	br	Fstx

! Fdivc: A = A / B
	export	Fdivc
Fdivc:
	mov	ax,A_KIND[bp]
	mov	dx,B_KIND[bp]
	cmp	ax,#K_NAN
	je	dnan
	cmp	dx,#K_NAN
	je	dnan
	mov	si,B_SGN[bp]		! the sign of the quotient
	xor	A_SGN[bp],si
	cmp	ax,#K_INF
	jne	dv1
	cmp	dx,#K_INF
	je	ddnan			! inf / inf: invalid
	ret				! inf / y = inf
dnan:
	br	Fnan
ddnan:
	br	Fdnan
dv1:
	cmp	dx,#K_INF
	jne	dv2
	mov	word A_KIND[bp],#K_ZERO	! x / inf = 0
	ret
dv2:
	or	dx,dx
	jnz	dv3
	or	ax,ax
	jz	ddnan			! 0 / 0: invalid
	mov	word A_KIND[bp],#K_INF	! x / 0 = inf
	ret
dv3:
	or	ax,ax
	jnz	dv4
	ret				! 0 / y = 0
dv4:
	mov	ax,A_EXP[bp]
	sub	ax,B_EXP[bp]
	add	ax,#1023
	mov	A_EXP[bp],ax
	mov	ax,A_M0[bp]		! the remainder R, with a 65th bit in SI
	mov	bx,A_M1[bp]
	mov	cx,A_M2[bp]
	mov	dx,A_M3[bp]
	xor	si,si
	cmp	dx,B_M3[bp]		! if A's mantissa is the smaller, double
	jne	dv5			! it, so the first quotient bit is 1
	cmp	cx,B_M2[bp]
	jne	dv5
	cmp	bx,B_M1[bp]
	jne	dv5
	cmp	ax,B_M0[bp]
dv5:
	jae	dv6
	shl	ax,1
	rcl	bx,1
	rcl	cx,1
	rcl	dx,1
	sbb	si,si
	dec	word A_EXP[bp]
dv6:
	mov	di,#64			! 64 quotient bits, into W0..W3
dv7:
	sub	ax,B_M0[bp]
	sbb	bx,B_M1[bp]
	sbb	cx,B_M2[bp]
	sbb	dx,B_M3[bp]
	jnc	dv8			! R >= B: quotient bit 1
	or	si,si
	jnz	dv8
	add	ax,B_M0[bp]		! R < B: put it back, quotient bit 0
	adc	bx,B_M1[bp]
	adc	cx,B_M2[bp]
	adc	dx,B_M3[bp]
	clc
	jmp	dv9
dv8:
	stc
dv9:
	rcl	word W0[bp],1
	rcl	word W1[bp],1
	rcl	word W2[bp],1
	rcl	word W3[bp],1
	shl	ax,1
	rcl	bx,1
	rcl	cx,1
	rcl	dx,1
	sbb	si,si
	dec	di
	jnz	dv7
	or	si,ax			! a remainder makes the sticky bit
	or	si,bx
	or	si,cx
	or	si,dx
	mov	ax,W0[bp]
	mov	bx,W1[bp]
	mov	cx,W2[bp]
	mov	dx,W3[bp]
	or	si,si
	jz	dv10
	or	al,#1
dv10:
	mov	A_M0[bp],ax
	mov	A_M1[bp],bx
	mov	A_M2[bp],cx
	mov	A_M3[bp],dx
	ret
