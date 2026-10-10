! strcpy(d, s) - copy string s to d, return d
! Written for xenix86-devsys. BCC calling convention: arguments on the stack,
! SI, DI and BP preserved, result in AX. Uses the 8086 string instructions,
! which address the destination through ES (crt0 sets ES = DS).

	.text
	export	_strcpy
_strcpy:
	push	si
	push	di
	mov	bx,sp
	mov	di,6[bx]
	mov	si,8[bx]
	mov	dx,di
	cld
cp1:
	lodsb
	stosb
	test	al,al
	jnz	cp1
	mov	ax,dx
	pop	di
	pop	si
	ret
