! imodu.s - BCC integer helper: AX op BX, result in AX (DX and CL not preserved).
! Written for xenix86-devsys.

	.text
	export	imodu		! unsigned remainder
imodu:
	xor	dx,dx
	div	bx
	mov	ax,dx
	ret
