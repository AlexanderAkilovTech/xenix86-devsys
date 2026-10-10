! imod.s - BCC integer helper: AX op BX, result in AX (DX and CL not preserved).
! Written for xenix86-devsys.

	.text
	export	imod		! signed remainder
imod:
	cwd
	idiv	bx
	mov	ax,dx
	ret
