! helpers.s - runtime support routines BCC-generated code calls (unsigned 16-bit divide).
! Operands: AX op BX, result in AX. Written for xenix86-devsys.

	.text
	export	idiv_u
idiv_u:
	xor	dx,dx
	div	bx
	ret

	export	imodu
imodu:
	xor	dx,dx
	div	bx
	mov	ax,dx
	ret
