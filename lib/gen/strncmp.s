! strncmp(a, b, n) - compare at most n characters: <0, 0, >0
! Written for xenix86-devsys. BCC calling convention: arguments on the stack,
! SI, DI and BP preserved, result in AX. Uses the 8086 string instructions,
! which address the destination through ES (crt0 sets ES = DS).

	.text
	export	_strncmp
_strncmp:
	push	si
	push	di
	mov	bx,sp
	mov	si,6[bx]
	mov	di,8[bx]
	mov	cx,10[bx]
	cld
sn1:
	jcxz	sn2
	dec	cx
	lodsb
	scasb
	jne	sn3
	test	al,al
	jnz	sn1
sn2:
	xor	ax,ax
	jmp	sn4
sn3:
	xor	ah,ah
	mov	cl,-1[di]
	xor	ch,ch
	sub	ax,cx
sn4:
	pop	di
	pop	si
	ret
