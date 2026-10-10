! memset(d, c, n) - set n bytes of d to c, return d
! Written for xenix86-devsys. BCC calling convention: arguments on the stack,
! SI, DI and BP preserved, result in AX. Uses the 8086 string instructions,
! which address the destination through ES (crt0 sets ES = DS).

	.text
	export	_memset
_memset:
	push	di
	mov	bx,sp
	mov	di,4[bx]
	mov	al,6[bx]
	mov	cx,8[bx]
	mov	dx,di
	cld
	rep
	stosb
	mov	ax,dx
	pop	di
	ret
