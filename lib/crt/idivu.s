! idivu.s - BCC integer helper: AX op BX, result in AX (DX and CL not preserved).
! Written for xenix86-devsys.

	.text
	export	idiv_u		! unsigned divide
idiv_u:
	xor	dx,dx
	div	bx
	ret
