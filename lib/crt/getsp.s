! getsp.s - getsp(): the stack pointer as the caller sees it (for diagnostics).
! Written for xenix86-devsys.

	.text
! getsp() - current stack pointer as seen by the caller (diagnostics)
	export	_getsp
_getsp:
	mov	ax,sp
	add	ax,*2
	ret
