! strcmp(a, b) - compare strings: <0, 0, >0 (characters compared unsigned)
! Written for xenix86-devsys. BCC calling convention: arguments on the stack,
! SI, DI and BP preserved, result in AX. Uses the 8086 string instructions,
! which address the destination through ES (crt0 sets ES = DS).

	.text
	export	_strcmp
_strcmp:
	push	si
	push	di
	mov	bx,sp
	mov	si,6[bx]
	mov	di,8[bx]
	cld
sc1:
	lodsb
	scasb
	jne	sc2
	test	al,al
	jnz	sc1
	xor	ax,ax
	jmp	sc3
sc2:
	xor	ah,ah		! difference of the first unequal characters
	mov	cl,-1[di]
	xor	ch,ch
	sub	ax,cx
sc3:
	pop	di
	pop	si
	ret
