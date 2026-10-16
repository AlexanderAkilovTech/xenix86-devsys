! hwprobe.s - what the machine has, for xtop (and anything else that asks):
! is there an 8087, and is a monochrome display a Hercules card or an IBM
! one (MDA); and outb, for cursor to set the display adapter's registers.
! Written for xenix86-devsys.
	.text

! fpprobe(cw, sw) unsigned *cw, *sw;  fninit, then the control and status
! words: after fninit an 8087's are 03FF and 0000. as86 does not put the
! WAIT (fwait) in front of 8087 instructions that an 8088 needs, and an
! 8088 without an 8087 does nothing for them but read their memory operand:
! so only the no-wait forms are used, with a delay after each, and the
! words they store are first filled with 5A5A, which stays there when no
! 8087 stored anything (tests/fpu87s.s, verified on XENIX with an 8087
! fitted and without one).
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

! hercules()  1 if the monochrome display's status port (3BA) shows the
! vertical retrace (bit 7) coming and going, as a Hercules card's does; 0
! for an IBM monochrome adapter, whose bit 7 does not change. 16384 reads
! last several frames even on a 4.77 MHz 8088, at 50 a second. Read only:
! nothing is written to the card.
	export	_hercules
_hercules:
	mov	dx,#$3BA
	in	al,dx
	and	al,#$80
	mov	ah,al
	mov	cx,#$4000
hloop:
	in	al,dx
	and	al,#$80
	cmp	al,ah
	jne	herc
	loop	hloop
	xor	ax,ax
	ret
herc:
	mov	ax,#1
	ret

! outb(port, byte)  writes byte to an I/O port (an 8086 has no I/O
! protection, so a XENIX program may: ABI.md, the console and the CGA)
	export	_outb
_outb:
	push	bp
	mov	bp,sp
	mov	dx,4[bp]
	mov	al,6[bp]
	out	dx,al
	pop	bp
	ret

! cpuprobe()  1 for an NEC V20 or V30 (or an 80186), 0 for an 8088 or 8086:
! the later chips use only the low 5 bits of a shift's count in CL, so a
! shift by 33 is a shift by 1; an 8088 shifts all 33 times, leaving 0.
	export	_cpuprobe
_cpuprobe:
	mov	cl,#33
	mov	ax,#1
	shl	ax,cl
	shr	ax,#1
	ret
