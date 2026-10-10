! fadd.s - addition and subtraction: Fadd, Faddd, Faddf, Fsub, Fsubd, Fsubf.
! Fadd and Fsub take both operands on the stack: X op Y replaces Y, X is
! dropped. The d and f forms take X on the stack and a double or float at
! [BX]; the result replaces X. (X is the left operand.)
include lib/fp/fpdefs.inc
	.globl	Fcpyb
	.globl	Fdnan
	.globl	Fent
	.globl	Fiszr
	.globl	Fldxd
	.globl	Fldxf
	.globl	Fldxy
	.globl	Fnan
	.globl	Fnrm
	.globl	Fsrj
	.globl	Fstx
	.globl	Fsty8
	.globl	Fswap
	.text

	export	Fadd
Fadd:
	call	Fent
	call	Fldxy
	call	Faddc
	br	Fsty8

	export	Faddd
Faddd:
	call	Fent
	call	Fldxd
	call	Faddc
	br	Fstx

	export	Faddf
Faddf:
	call	Fent
	call	Fldxf
	call	Faddc
	br	Fstx

	export	Fsub
Fsub:
	call	Fent
	call	Fldxy
	call	Fsubc
	br	Fsty8

	export	Fsubd
Fsubd:
	call	Fent
	call	Fldxd
	call	Fsubc
	br	Fstx

	export	Fsubf
Fsubf:
	call	Fent
	call	Fldxf
	call	Fsubc
	br	Fstx

! Fsubc: A = A - B. Faddc: A = A + B.
	export	Fsubc
Fsubc:
	cmp	word B_KIND[bp],#K_NAN	! a NaN keeps its sign
	je	Faddc
	xor	word B_SGN[bp],#$8000

	export	Faddc
Faddc:
	mov	ax,A_KIND[bp]
	mov	dx,B_KIND[bp]
	cmp	ax,#K_NAN
	je	adnan
	cmp	dx,#K_NAN
	je	adnan
	cmp	ax,#K_INF
	jne	ad1
	cmp	dx,#K_INF
	bne	ad9			! inf + y = inf
	mov	ax,A_SGN[bp]
	cmp	ax,B_SGN[bp]
	beq	ad9			! inf + inf = inf
	br	Fdnan			! inf - inf: invalid
adnan:
	br	Fnan
adb:
	br	Fcpyb
ad1:
	cmp	dx,#K_INF
	je	adb			! x + inf = inf
	or	ax,ax
	jnz	ad2
	or	dx,dx
	jnz	adb			! 0 + y = y
	mov	ax,B_SGN[bp]		! 0 + 0 is -0 only if both are -0
	and	A_SGN[bp],ax
	ret
ad2:
	or	dx,dx
	beq	ad9			! x + 0 = x
	mov	ax,A_EXP[bp]		! make A the one with the larger exponent
	cmp	ax,B_EXP[bp]
	jge	ad3
	call	Fswap
ad3:
	mov	si,A_EXP[bp]		! line B up with A
	sub	si,B_EXP[bp]
	mov	ax,B_M0[bp]
	mov	bx,B_M1[bp]
	mov	cx,B_M2[bp]
	mov	dx,B_M3[bp]
	call	Fsrj
	mov	si,A_SGN[bp]
	cmp	si,B_SGN[bp]
	jne	adsub
	add	ax,A_M0[bp]
	adc	bx,A_M1[bp]
	adc	cx,A_M2[bp]
	adc	dx,A_M3[bp]
	jnc	adst
	rcr	dx,1			! carry out: shift it back in, keeping
	rcr	cx,1			! the bit that falls off as sticky
	rcr	bx,1
	rcr	ax,1
	jnc	ad4
	or	al,#1
ad4:
	inc	word A_EXP[bp]
	jmp	adst
adsub:					! |A| - |B|, negative only if the exponents
	sub	A_M0[bp],ax		! are equal and B's mantissa is larger
	sbb	A_M1[bp],bx
	sbb	A_M2[bp],cx
	sbb	A_M3[bp],dx
	mov	ax,A_M0[bp]
	mov	bx,A_M1[bp]
	mov	cx,A_M2[bp]
	mov	dx,A_M3[bp]
	jnc	ad5
	not	ax
	not	bx
	not	cx
	not	dx
	add	ax,#1
	adc	bx,#0
	adc	cx,#0
	adc	dx,#0
	xor	word A_SGN[bp],#$8000
ad5:
	call	Fiszr
	jnz	ad6
	mov	A_KIND[bp],ax		! x - x is +0 (rounding to nearest)
	mov	A_SGN[bp],ax
	ret
ad6:
	mov	si,A_EXP[bp]
	call	Fnrm
	mov	A_EXP[bp],si
adst:
	mov	A_M0[bp],ax
	mov	A_M1[bp],bx
	mov	A_M2[bp],cx
	mov	A_M3[bp],dx
ad9:
	ret
