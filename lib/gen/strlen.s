! strlen(s) - length of a string
! Written for xenix86-devsys. BCC calling convention: arguments on the stack,
! SI, DI and BP preserved, result in AX. Uses the 8086 string instructions,
! which address the destination through ES (crt0 sets ES = DS).

	.text
	export	_strlen
_strlen:
	push	di
	mov	bx,sp
	mov	di,4[bx]
	mov	cx,#-1
	xor	al,al
	cld
	repne
	scasb
	mov	ax,#-2		! scanned length + 1 = -1 - CX
	sub	ax,cx
	pop	di
	ret
