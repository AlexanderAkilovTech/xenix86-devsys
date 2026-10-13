! memchr(s, c, n) - the first byte c in the n bytes at s, or 0
! Written for xenix86-devsys. BCC calling convention: arguments on the stack,
! SI, DI and BP preserved, result in AX. Uses the 8086 string instructions,
! which address the string through ES (crt0 sets ES = DS): repne scasb, a
! few clock cycles a byte, where a loop in C takes some forty.
	.text
	export	_memchr
_memchr:
	push	di
	mov	bx,sp
	mov	di,4[bx]
	mov	al,6[bx]
	mov	cx,8[bx]
	xor	dx,dx
	jcxz	none
	cld
	repne
	scasb
	jne	none
	lea	dx,-1[di]
none:
	mov	ax,dx
	pop	di
	ret
