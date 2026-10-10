! fmul.s - multiplication: Fmul, Fmuld, Fmulf (operands as in fadd.s).
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

	export	Fmul
Fmul:
	call	Fent
	call	Fldxy
	call	Fmulc
	br	Fsty8

	export	Fmuld
Fmuld:
	call	Fent
	call	Fldxd
	call	Fmulc
	br	Fstx

	export	Fmulf
Fmulf:
	call	Fent
	call	Fldxf
	call	Fmulc
	br	Fstx

! Fmulc: A = A * B
	export	Fmulc
Fmulc:
	mov	ax,A_KIND[bp]
	mov	dx,B_KIND[bp]
	cmp	ax,#K_NAN
	je	mnan
	cmp	dx,#K_NAN
	je	mnan
	mov	si,B_SGN[bp]		! the sign of the product
	xor	A_SGN[bp],si
	cmp	ax,#K_INF
	jne	mu1
	or	dx,dx
	jz	mdnan			! inf * 0: invalid
	ret				! inf * y = inf
mnan:
	br	Fnan
mdnan:
	br	Fdnan
mu1:
	cmp	dx,#K_INF
	jne	mu2
	or	ax,ax
	jz	mdnan			! 0 * inf
	mov	word A_KIND[bp],#K_INF	! x * inf = inf
	ret
mu2:
	or	ax,ax
	jz	mu9			! 0 * y = 0
	or	dx,dx
	jnz	mu3
	mov	A_KIND[bp],dx		! x * 0 = 0
mu9:
	ret
mu3:
	mov	ax,A_EXP[bp]		! the exponent if the product's top bit
	add	ax,B_EXP[bp]		! is set (one less if not)
	sub	ax,#1022
	mov	A_EXP[bp],ax
	xor	ax,ax			! the 128-bit product into W0..W7, a row
	mov	W0[bp],ax		! for each word of A
	mov	W1[bp],ax
	mov	W2[bp],ax
	mov	W3[bp],ax
	lea	si,A_M0[bp]
	lea	di,W0[bp]
mu4:
	mov	cx,[si]
	mov	ax,B_M0[bp]
	mul	cx
	add	[di],ax
	adc	dx,#0
	mov	bx,dx
	mov	ax,B_M1[bp]
	mul	cx
	add	ax,bx
	adc	dx,#0
	add	2[di],ax
	adc	dx,#0
	mov	bx,dx
	mov	ax,B_M2[bp]
	mul	cx
	add	ax,bx
	adc	dx,#0
	add	4[di],ax
	adc	dx,#0
	mov	bx,dx
	mov	ax,B_M3[bp]
	mul	cx
	add	ax,bx
	adc	dx,#0
	add	6[di],ax
	adc	dx,#0
	mov	8[di],dx
	inc	si
	inc	si
	inc	di
	inc	di
	lea	ax,A_EXP[bp]
	cmp	si,ax
	jne	mu4
	mov	ax,W4[bp]		! the top 64 bits, the rest sticky
	mov	bx,W5[bp]
	mov	cx,W6[bp]
	mov	dx,W7[bp]
	mov	si,W0[bp]
	or	si,W1[bp]
	or	si,W2[bp]
	or	dx,dx
	js	mu5
	dec	word A_EXP[bp]		! top bit clear: one more bit, from W3
	mov	di,W3[bp]
	shl	di,1
	rcl	ax,1
	rcl	bx,1
	rcl	cx,1
	rcl	dx,1
	or	si,di
	jmp	mu6
mu5:
	or	si,W3[bp]
mu6:
	or	si,si
	jz	mu7
	or	al,#1
mu7:
	mov	A_M0[bp],ax
	mov	A_M1[bp],bx
	mov	A_M2[bp],cx
	mov	A_M3[bp],dx
	ret
