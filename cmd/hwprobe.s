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

! cpuprobe()  1 for an NEC V20 or V30, 0 for an Intel 8088 or 8086. AAD
! (D5 nn) multiplies AH by its second byte and adds AL: Intel's 8088 uses
! that byte, so AAD 16 on AX 0101 gives 17; NEC's V20 always uses 10, giving
! 11. (A shift by 33, M72's first test, does not tell them apart: the V20
! shifts all 33 times too, as 86Box showed with a V20 fitted.)
	export	_cpuprobe
_cpuprobe:
	mov	ax,#$0101
	db	$D5,$10
	cmp	al,#11
	je	nec
	xor	ax,ax
	ret
nec:
	mov	ax,#1
	ret
