! isl.s - BCC integer helper: AX op BX, result in AX (DX and CL not preserved).
! Written for xenix86-devsys.

	.text
	export	isl		! shift left by BL
	export	islu
isl:
islu:
	mov	cl,bl
	shl	ax,cl
	ret
