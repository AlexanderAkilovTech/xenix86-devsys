! setjmp.s - setjmp and longjmp for XENIX 2.1.3 (8086), xenix86-devsys
!
! jmp_buf layout (words): 0 return IP, 1 SP after the return, 2 BP, 3 SI, 4 DI.
! BCC keeps register variables in SI and DI, and BP is the frame pointer, so these
! are all the registers a caller expects to survive a call. The saved SP is the
! value the caller would see after a normal return from setjmp, so longjmp lands
! in the caller exactly as if setjmp had just returned (with a different AX).

	.text
! int setjmp(env) jmp_buf env;
	export	_setjmp
_setjmp:
	pop	cx		! return address
	mov	bx,sp
	mov	bx,[bx]		! env (the argument is still on the stack for the caller to drop)
	mov	[bx],cx
	mov	2[bx],sp
	mov	4[bx],bp
	mov	6[bx],si
	mov	8[bx],di
	xor	ax,ax
	jmp	cx

! longjmp(env, val) jmp_buf env; int val;  -- setjmp returns val (1 if val is 0)
	export	_longjmp
_longjmp:
	mov	bx,sp
	mov	ax,4[bx]	! val
	mov	bx,2[bx]	! env
	or	ax,ax
	jnz	lj_go
	inc	ax
lj_go:
	mov	sp,2[bx]
	mov	bp,4[bx]
	mov	si,6[bx]
	mov	di,8[bx]
	jmp	[bx]
