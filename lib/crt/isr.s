! isr.s - BCC integer helper: AX op BX, result in AX (DX and CL not preserved).
! Written for xenix86-devsys.

	.text
	export	isr		! arithmetic shift right by BL
isr:
	mov	cl,bl
	sar	ax,cl
	ret
