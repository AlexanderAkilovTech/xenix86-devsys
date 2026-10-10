! idiv.s - BCC integer helper: AX op BX, result in AX (DX and CL not preserved).
! Written for xenix86-devsys.

	.text
	export	idiv_		! signed divide
idiv_:
	cwd
	idiv	bx
	ret
