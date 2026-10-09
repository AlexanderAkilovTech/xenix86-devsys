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

! getsp() - current stack pointer as seen by the caller (diagnostics)
	export	_getsp
_getsp:
	mov	ax,sp
	add	ax,*2
	ret
