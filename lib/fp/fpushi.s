! fpushi.s - integers to double: Fpushc, Fpushuc (AL), Fpushs, Fpushus,
! Fpushi, Fpushui (AX), Fpushl, Fpushul (BX:AX, BX high). The double is
! pushed on the stack. Every one of these is exact.
include lib/fp/fpdefs.inc
	.globl	Fent
	.globl	Fiszr
	.globl	Fnrm
	.globl	Fstx
	.text

	export	Fpushc
Fpushc:
	cbw
	export	Fpushs
Fpushs:
	export	Fpushi
Fpushi:
	cwd
	mov	bx,dx
	export	Fpushl
Fpushl:
	mov	dx,bx
	and	dx,#$8000		! DX = the sign
	jz	pl1
	neg	bx			! and BX:AX the magnitude
	neg	ax
	sbb	bx,#0
	jmp	pl1

	export	Fpushuc
Fpushuc:
	xor	ah,ah
	export	Fpushus
Fpushus:
	export	Fpushui
Fpushui:
	xor	bx,bx
	export	Fpushul
Fpushul:
	xor	dx,dx
pl1:
	pop	cx
	sub	sp,#8
	push	cx
	call	Fent
	mov	A_SGN[bp],dx
	mov	dx,bx
	mov	cx,ax
	xor	ax,ax
	xor	bx,bx
	mov	word A_KIND[bp],#K_ZERO
	call	Fiszr
	jz	pl2
	mov	word A_KIND[bp],#K_FIN
	mov	si,#1023+31
	call	Fnrm
	mov	A_EXP[bp],si
pl2:
	mov	A_M0[bp],ax
	mov	A_M1[bp],bx
	mov	A_M2[bp],cx
	mov	A_M3[bp],dx
	br	Fstx
