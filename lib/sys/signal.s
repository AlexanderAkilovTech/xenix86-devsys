! signal.s - signal for XENIX 2.1.3 (8086), xenix86-devsys
!
! signal: the kernel is handed the address of a per-signal trampoline in our own
! text, not the C handler. On delivery it pushes FLAGS, CS, IP onto the user stack
! and jumps to the trampoline; the common code calls the C handler with the signal
! number, then restores the interrupted state.

	.globl	__syscal
	.globl	_errno

	.text
! int (*signal(sig, func))()
	export	_signal
_signal:
	push	bp
	mov	bp,sp
	push	si
	push	di
	mov	bx,4[bp]	! sig
	mov	cx,6[bp]	! C handler, or 0 (SIG_DFL) / 1 (SIG_IGN)
	cmp	bx,*1
	jb	sg_inval
	cmp	bx,*20
	jae	sg_inval
	mov	si,bx
	shl	si,*1
	push	[si+__sigtab]	! previous C handler
	mov	[si+__sigtab],cx
	cmp	cx,*1
	jbe	sg_call		! SIG_DFL / SIG_IGN go to the kernel as-is
	mov	cx,bx		! trampoline address = sigent + 4 * sig
	shl	cx,*1
	shl	cx,*1
	add	cx,#sigent
sg_call:
	mov	ax,#48
	call	__syscal
	pop	bx		! previous C handler
	jc	sg_err
	cmp	ax,*1
	jbe	sg_ret		! kernel says it was SIG_DFL / SIG_IGN
	mov	ax,bx
sg_ret:
	pop	di
	pop	si
	pop	bp
	ret
sg_inval:
	mov	ax,#22		! EINVAL
sg_err:
	mov	[_errno],ax
	mov	ax,#-1
	jmp	sg_ret

! common signal entry: called from a trampoline; the return address identifies the signal
sigcommon:
	push	bp
	mov	bp,sp
	push	ax
	push	bx
	push	cx
	push	dx
	push	si
	push	di
	push	es
	mov	ax,2[bp]	! return address = sigent + 4*sig + 3
	sub	ax,#sigent+3
	mov	bx,ax
	shr	bx,*1		! 2*sig: index into __sigtab
	shr	ax,*1
	shr	ax,*1		! sig
	push	ax
	call	[bx+__sigtab]
	pop	ax
	pop	es
	pop	di
	pop	si
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	pop	bp
	add	sp,*2		! drop the trampoline's return address
	pop	[__sigip]	! interrupted IP
	pop	[__sigcs]	! interrupted CS (same segment; not needed for the near jump)
	popf
	br	[__sigip]

! one 4-byte trampoline per signal number 0..19: call sigcommon, then a pad byte
sigent:
	call	sigcommon
	.byte	0
	call	sigcommon
	.byte	0
	call	sigcommon
	.byte	0
	call	sigcommon
	.byte	0
	call	sigcommon
	.byte	0
	call	sigcommon
	.byte	0
	call	sigcommon
	.byte	0
	call	sigcommon
	.byte	0
	call	sigcommon
	.byte	0
	call	sigcommon
	.byte	0
	call	sigcommon
	.byte	0
	call	sigcommon
	.byte	0
	call	sigcommon
	.byte	0
	call	sigcommon
	.byte	0
	call	sigcommon
	.byte	0
	call	sigcommon
	.byte	0
	call	sigcommon
	.byte	0
	call	sigcommon
	.byte	0
	call	sigcommon
	.byte	0
	call	sigcommon
	.byte	0

	.data
__sigip:
	.word	0
__sigcs:
	.word	0
__sigtab:
	.blkw	20
