! fpu87s.s - 8087 routines for the fpu87 probe. as86 does not put the WAIT
! (fwait) in front of 8087 instructions that an 8088 needs, so it is written
! out. fpprobe uses only the no-wait forms and delays instead, so that it is
! safe on a machine without an 8087, where every 8087 instruction does nothing.
	.text

! fpprobe(cw, sw) unsigned *cw, *sw;
! fninit, then store the control and status words. Without an 8087 the
! stores never happen and both words keep the 5A5A put there first.
	export	_fpprobe
_fpprobe:
	push	bp
	mov	bp,sp
	mov	bx,4[bp]
	mov	word [bx],#$5A5A
	mov	bx,6[bp]
	mov	word [bx],#$5A5A
	fninit
	call	delay
	mov	bx,4[bp]
	fnstcw	[bx]
	call	delay
	mov	bx,6[bp]
	fnstsw	[bx]
	call	delay
	pop	bp
	ret

delay:
	mov	cx,#200
dlay:	loop	dlay
	ret

! fpinit()  fninit
	export	_fpinit
_fpinit:
	fninit
	call	delay
	ret

! fpdiv3(r) unsigned r[4];  r = 1/3
	export	_fpdiv3
_fpdiv3:
	push	bp
	mov	bp,sp
	mov	bx,4[bp]
	fwait
	fld1
	fwait
	fidiv	word [three]
	fwait
	fstp	qword [bx]
	fwait
	pop	bp
	ret

! fpsqrt2(r) unsigned r[4];  r = sqrt(2)
	export	_fpsqrt2
_fpsqrt2:
	push	bp
	mov	bp,sp
	mov	bx,4[bp]
	fwait
	fld1
	fwait
	fadd	st,st
	fwait
	fsqrt
	fwait
	fstp	qword [bx]
	fwait
	pop	bp
	ret

! fppi(r) unsigned r[4];  r = pi, from the 8087's own constant
	export	_fppi
_fppi:
	push	bp
	mov	bp,sp
	mov	bx,4[bp]
	fwait
	fldpi
	fwait
	fstp	qword [bx]
	fwait
	pop	bp
	ret

! fpld(v) int v;  push v onto the 8087 stack
	export	_fpld
_fpld:
	push	bp
	mov	bp,sp
	lea	bx,4[bp]
	fwait
	fild	word [bx]
	fwait
	pop	bp
	ret

! int fpst0()  st(0) as an int, without popping it
	export	_fpst0
_fpst0:
	push	bp
	mov	bp,sp
	sub	sp,*2
	lea	bx,-2[bp]
	fwait
	fist	word [bx]
	fwait
	mov	ax,-2[bp]
	mov	sp,bp
	pop	bp
	ret

! int fpst1()  st(1) as an int; the stack is left as it was
	export	_fpst1
_fpst1:
	push	bp
	mov	bp,sp
	sub	sp,*2
	lea	bx,-2[bp]
	fwait
	fxch	st(1)
	fwait
	fist	word [bx]
	fwait
	fxch	st(1)
	fwait
	mov	ax,-2[bp]
	mov	sp,bp
	pop	bp
	ret

! unsigned fpgetcw()  the control word
	export	_fpgetcw
_fpgetcw:
	push	bp
	mov	bp,sp
	sub	sp,*2
	lea	bx,-2[bp]
	fwait
	fstcw	[bx]
	fwait
	mov	ax,-2[bp]
	mov	sp,bp
	pop	bp
	ret

! fpsetcw(cw) unsigned cw;
	export	_fpsetcw
_fpsetcw:
	push	bp
	mov	bp,sp
	lea	bx,4[bp]
	fwait
	fldcw	[bx]
	fwait
	pop	bp
	ret

	.data
three:	.word	3
