! isru.s - BCC integer helper: AX op BX, result in AX (DX and CL not preserved).
! Written for xenix86-devsys.

	.text
	export	isru		! logical shift right by BL
isru:
	mov	cl,bl
	shr	ax,cl
	ret
