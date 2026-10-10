! imul.s - BCC integer helper: AX op BX, result in AX (DX and CL not preserved).
! Written for xenix86-devsys.

	.text
	export	imul_		! multiply (the low 16 bits are the same signed or not)
	export	imul_u
imul_:
imul_u:
	imul	bx
	ret
