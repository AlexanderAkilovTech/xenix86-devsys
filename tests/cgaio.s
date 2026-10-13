! cgaio.s - I/O port output for cgatest. An 8086 has no I/O protection, so
! a XENIX program can program the CGA's registers itself.
	.text

! outb(port, val)  one byte to an I/O port
	export	_outb
_outb:
	push	bp
	mov	bp,sp
	mov	dx,4[bp]
	mov	al,6[bp]
	out	dx,al
	pop	bp
	ret

! crtc(reg, val)  a 6845 register: the index to 3D4, the value to 3D5, with
! interrupts off between, so that the console driver cannot come in between
	export	_crtc
_crtc:
	push	bp
	mov	bp,sp
	pushf
	cli
	mov	dx,#$3D4
	mov	al,4[bp]
	out	dx,al
	inc	dx
	mov	al,6[bp]
	out	dx,al
	popf
	pop	bp
	ret

! farput(seg, off, buf, n)  copy n bytes from buf to seg:off, any memory at
! all: an 8086 program can load ES with any segment. XENIX's /dev/mem stops
! at the end of memory, before the video memory at B800:0.
	export	_farput
_farput:
	push	bp
	mov	bp,sp
	push	si
	push	di
	push	es
	mov	es,4[bp]
	mov	di,6[bp]
	mov	si,8[bp]
	mov	cx,10[bp]
	cld
	rep
	movsb
	pop	es
	pop	di
	pop	si
	pop	bp
	ret

! farget(seg, off, buf, n)  copy n bytes from seg:off to buf
	export	_farget
_farget:
	push	bp
	mov	bp,sp
	push	si
	push	di
	push	ds
	push	es
	push	ds
	pop	es
	mov	di,8[bp]
	mov	cx,10[bp]
	mov	si,6[bp]
	mov	ds,4[bp]
	cld
	rep
	movsb
	pop	es
	pop	ds
	pop	di
	pop	si
	pop	bp
	ret
