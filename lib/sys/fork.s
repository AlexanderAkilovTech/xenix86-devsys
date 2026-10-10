! fork.s - fork for XENIX 2.1.3 (8086), xenix86-devsys
! The kernel returns BX = 0 in the child, BX != 0 in the parent (AX = child pid).

	.globl	__syscal
	.globl	_errno

	.text
	export	_fork
_fork:
	mov	ax,#2
	call	__syscal
	jc	fk_err
	and	bx,bx
	jz	fk_child
	ret			! parent: AX = child pid
fk_child:
	xor	ax,ax
	ret
fk_err:
	mov	[_errno],ax
	mov	ax,#-1
	ret
