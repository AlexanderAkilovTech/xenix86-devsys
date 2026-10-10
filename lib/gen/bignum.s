! bignum.s - xenix86-devsys libc: unsigned big integers for the exact decimal
! conversions (printf's e, f, g; strtod). A number is an array of 16-bit
! words, least significant first, and a length with no zero words on top.
! The caller provides the room.
	.text

! int _bnmul(a, n, m, c) unsigned *a; unsigned m, c;
! a = a * m + c; returns the new length
	export	__bnmul
__bnmul:
	push	bp
	mov	bp,sp
	push	si
	push	di
	mov	si,4[bp]
	mov	cx,6[bp]
	mov	di,8[bp]
	mov	bx,10[bp]		! the carry, starting as c
	jcxz	bm2
bm1:
	mov	ax,[si]
	mul	di
	add	ax,bx
	adc	dx,#0
	mov	[si],ax
	mov	bx,dx
	inc	si
	inc	si
	loop	bm1
bm2:
	mov	ax,6[bp]
	or	bx,bx
	jz	bm3
	mov	[si],bx
	inc	ax
bm3:
	pop	di
	pop	si
	pop	bp
	ret

! int _bncmp(a, na, b, nb) unsigned *a, *b;  -1, 0 or 1 as a < = > b
	export	__bncmp
__bncmp:
	push	bp
	mov	bp,sp
	push	si
	push	di
	mov	cx,6[bp]
	mov	ax,cx
	sub	ax,10[bp]
	jne	bc8			! different lengths
	jcxz	bc9			! both zero
	mov	si,4[bp]
	mov	di,8[bp]
	mov	bx,cx
	dec	bx
	shl	bx,1
bc1:
	mov	ax,[bx+si]
	cmp	ax,[bx+di]
	jne	bc7
	dec	bx
	dec	bx
	loop	bc1
	xor	ax,ax
	jmp	bc9
bc7:
	mov	ax,#1
	ja	bc9
	neg	ax
	jmp	bc9
bc8:
	mov	ax,#1
	jg	bc9
	neg	ax
bc9:
	pop	di
	pop	si
	pop	bp
	ret

! int _bnsub(a, na, b, nb) unsigned *a, *b;
! a = a - b (a must be >= b); returns the new length
	export	__bnsub
__bnsub:
	push	bp
	mov	bp,sp
	push	si
	push	di
	mov	si,4[bp]
	mov	di,8[bp]
	mov	cx,10[bp]
	xor	bx,bx
	clc
	jcxz	bs2
bs1:
	mov	ax,[bx+di]
	sbb	[bx+si],ax
	inc	bx
	inc	bx
	loop	bs1
bs2:
	jnc	bs4
bs3:					! carry the borrow up
	sub	word [bx+si],#1
	inc	bx
	inc	bx
	jc	bs3
bs4:
	mov	cx,6[bp]
	mov	bx,cx
	add	bx,bx
bs5:
	jcxz	bs6
	cmp	word -2[bx+si],#0
	jne	bs6
	dec	bx
	dec	bx
	dec	cx
	jmp	bs5
bs6:
	mov	ax,cx
	pop	di
	pop	si
	pop	bp
	ret

! int _bnshl(a, n, s) unsigned *a;  a = a << s bits; returns the new length
	export	__bnshl
__bnshl:
	push	bp
	mov	bp,sp
	push	si
	push	di
	mov	di,6[bp]		! the length, as it grows
	or	di,di
	jz	sh8
	mov	cx,8[bp]
	and	cx,#15
	jz	sh3
	mov	ch,#16			! CL = the bit shift, CH = 16 - it
	sub	ch,cl
	mov	si,4[bp]
	xor	bx,bx			! the bits carried into the next word
	push	di
sh1:
	mov	ax,[si]
	mov	dx,ax
	shl	ax,cl
	or	ax,bx
	mov	[si],ax
	xchg	cl,ch
	shr	dx,cl
	xchg	cl,ch
	mov	bx,dx
	inc	si
	inc	si
	dec	di
	jnz	sh1
	pop	di
	or	bx,bx
	jz	sh3
	mov	[si],bx
	inc	di
sh3:
	mov	ax,8[bp]		! the whole words to shift
	mov	cl,#4
	shr	ax,cl
	or	ax,ax
	jz	sh8
	mov	dx,ax			! DX = words
	mov	si,4[bp]
	mov	bx,di
	dec	bx
	shl	bx,1			! from the top word down
	shl	ax,1
	mov	cx,ax			! CX = the move in bytes
sh4:
	mov	ax,[bx+si]
	push	bx
	add	bx,cx
	mov	[bx+si],ax
	pop	bx
	sub	bx,#2
	jns	sh4
	mov	cx,dx			! zeros below
	xor	ax,ax
sh5:
	mov	[si],ax
	inc	si
	inc	si
	loop	sh5
	add	di,dx
sh8:
	mov	ax,di
	pop	di
	pop	si
	pop	bp
	ret

! int _bnshr1(a, n) unsigned *a;  a = a >> 1; returns the new length
	export	__bnshr1
__bnshr1:
	push	bp
	mov	bp,sp
	push	si
	mov	si,4[bp]
	mov	cx,6[bp]
	mov	ax,cx
	jcxz	r9
	mov	bx,cx
	dec	bx
	shl	bx,1
	clc
r1:
	rcr	word [bx+si],1
	dec	bx
	dec	bx
	loop	r1
	mov	bx,ax
	shl	bx,1
	cmp	word -2[bx+si],#0
	jne	r9
	dec	ax
r9:
	pop	si
	pop	bp
	ret

! _bnmac(acc, g, n, m) unsigned *acc, *g; unsigned m;
! acc += g[0..n-1] * m, carrying as far up acc as it goes (acc has the room)
	export	__bnmac
__bnmac:
	push	bp
	mov	bp,sp
	push	si
	push	di
	mov	di,4[bp]
	mov	si,6[bp]
	mov	cx,8[bp]
	xor	bx,bx			! carry
	jcxz	ma2
ma1:
	mov	ax,[si]
	mul	word 10[bp]
	add	ax,bx
	adc	dx,#0
	add	[di],ax
	adc	dx,#0
	mov	bx,dx
	inc	si
	inc	si
	inc	di
	inc	di
	loop	ma1
ma2:
	add	[di],bx			! and on up
	jnc	ma9
ma3:
	inc	di
	inc	di
	add	word [di],#1
	jc	ma3
ma9:
	pop	di
	pop	si
	pop	bp
	ret
