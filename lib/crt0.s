! crt0.s - XENIX 2.1.3 (8086) C startup for the xenix86-devsys project.
! Must be the first object linked: its text supplies the fixed reserved block
! (0x00-0x7F) and its data supplies the DS:0 header (datloc0 = 0, stack limit = FFFF).
! Assembled with as86; written for this project.

	.globl	_main
	.globl	_exit

	.text
	export	__syscal
	export	__stkgro
__xblock:
	.byte	$EB,$7E,$EB,$13,$EB,$0E,$EB,$0C,$EB,$FE,$EB,$FE,$EB,$FE,$EB,$FE
	.byte	$EB,$FE,$EB,$FE,$B8,$28,$08,$CD,$05,$EB,$64,$00,$00,$00,$00,$00
	.byte	$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
	.byte	$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
	.byte	$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
	.byte	$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
	.byte	$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
	.byte	$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$C3
__syscal = __xblock + $02
__stkgro = __xblock + $04

! Kernel starts execution here (text offset 0x80).
! On entry SP -> argc, argv[0..argc-1], NULL, envp[...], NULL
	export	start
start:
	mov	bx,sp
	mov	ax,[bx]		! argc
	lea	cx,2[bx]		! argv
	mov	si,ax
	shl	si,*1
	add	si,cx
	add	si,*2		! envp = argv + (argc + 1) * 2
	mov	[_environ],si
	push	si
	push	cx
	push	ax
	call	_main
	add	sp,*6
	push	ax
	call	_exit		! exit(main's return value)
hang:
	jmp	hang

	export	___mkargv	! referenced by every BCC-compiled main(); nothing to do here
___mkargv:
	ret

	.data
	export	__datloc
__datloc:
	.word	0		! DS:0 datloc0
	export	STKHQQ
STKHQQ:
	.word	$FFFF		! DS:2 stack limit
	export	_environ
_environ:
	.word	0
	export	_errno
_errno:
	.word	0
