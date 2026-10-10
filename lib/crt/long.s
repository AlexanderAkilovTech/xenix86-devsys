! long.s - 32-bit arithmetic helpers called by BCC-generated code (xenix86-devsys)
!
! Convention: left operand in BX:AX (BX = high word), right operand in memory at
! [DI] (low word) / 2[DI] (high word), result in BX:AX. Shift counts are in DI.
! lcmp*/ltst* only set flags. lincl/ldecl modify the long at [BX] in place.
! All helpers preserve CX, DX, SI, DI and BP.

	.text

! ---- add, subtract, logic -------------------------------------------------
	export	laddl
	export	laddul
laddl:
laddul:
	add	ax,[di]
	adc	bx,2[di]
	ret

	export	lsubl
	export	lsubul
lsubl:
lsubul:
	sub	ax,[di]
	sbb	bx,2[di]
	ret

	export	landl
	export	landul
landl:
landul:
	and	ax,[di]
	and	bx,2[di]
	ret

	export	lorl
	export	lorul
lorl:
lorul:
	or	ax,[di]
	or	bx,2[di]
	ret

	export	leorl
	export	leorul
leorl:
leorul:
	xor	ax,[di]
	xor	bx,2[di]
	ret

	export	lnegl
	export	lnegul
lnegl:
lnegul:
	neg	bx
	neg	ax
	sbb	bx,*0
	ret

	export	lcoml
	export	lcomul
lcoml:
lcomul:
	not	ax
	not	bx
	ret

! ---- increment / decrement the long at [BX] --------------------------------
	export	lincl
	export	lincul
lincl:
lincul:
	add	word ptr [bx],*1
	adc	word ptr 2[bx],*0
	ret

	export	ldecl
	export	ldecul
ldecl:
ldecul:
	sub	word ptr [bx],*1
	sbb	word ptr 2[bx],*0
	ret

! ---- tests and compares (flags only) -------------------------------------
! ltstl: flags as if comparing BX:AX with 0 (ZF, and SF/OF usable for signed jumps)
	export	ltstl
	export	ltstul
ltstl:
ltstul:
	test	bx,bx
	jnz	tst_done
	test	ax,ax
	jz	tst_done
	push	cx
	mov	cx,*1
	or	cx,cx		! nonzero, positive
	pop	cx
tst_done:
	ret

! lcmpl: flags as for a signed compare of BX:AX with the long at [DI]
	export	lcmpl
lcmpl:
	cmp	bx,2[di]
	jne	cmp_done
	cmp	ax,[di]
	je	cmp_done
	push	cx
	mov	cx,*1
	jb	cmp_lt
	cmp	cx,*0		! greater: ZF=0, SF=OF
	pop	cx
	ret
cmp_lt:
	cmp	cx,*2		! less: SF!=OF
	pop	cx
	ret
cmp_done:
	ret

! lcmpul: flags as for an unsigned compare
	export	lcmpul
lcmpul:
	cmp	bx,2[di]
	jne	cmpu_done
	cmp	ax,[di]
cmpu_done:
	ret

! ---- shifts by DI -----------------------------------------------------------
	export	lsll
	export	lslul
lsll:
lslul:
	push	cx
	mov	cx,di
	and	cx,*31
	jz	sl_done
sl_loop:
	shl	ax,*1
	rcl	bx,*1
	loop	sl_loop
sl_done:
	pop	cx
	ret

	export	lsrl
lsrl:
	push	cx
	mov	cx,di
	and	cx,*31
	jz	sr_done
sr_loop:
	sar	bx,*1
	rcr	ax,*1
	loop	sr_loop
sr_done:
	pop	cx
	ret

	export	lsrul
lsrul:
	push	cx
	mov	cx,di
	and	cx,*31
	jz	sru_done
sru_loop:
	shr	bx,*1
	rcr	ax,*1
	loop	sru_loop
sru_done:
	pop	cx
	ret

! ---- multiply: low 32 bits of BX:AX * [DI] ----------------------------------
	export	lmull
	export	lmulul
lmull:
lmulul:
	push	cx
	push	dx
	push	si
	mov	si,ax		! a_lo
	mul	word ptr 2[di]	! a_lo * b_hi -> ax (low part only matters)
	mov	cx,ax
	mov	ax,bx
	mul	word ptr [di]	! a_hi * b_lo
	add	cx,ax
	mov	ax,si
	mul	word ptr [di]	! a_lo * b_lo -> dx:ax
	add	dx,cx
	mov	bx,dx
	pop	si
	pop	dx
	pop	cx
	ret

! ---- divide / modulo ------------------------------------------------------
! udiv32: unsigned BX:AX / SI:DI (SI = high). Quotient -> BX:AX, remainder -> DX:CX.
! Clobbers BP. Division by zero traps like the 8086 DIV instruction.
udiv32:
	or	si,si
	jnz	ud_long
	! fast path: 16-bit divisor
	push	ax
	mov	ax,bx
	xor	dx,dx
	div	di		! high word
	mov	bx,ax
	pop	ax
	div	di		! low word, remainder in dx
	mov	cx,dx
	xor	dx,dx
	ret
ud_long:
	push	bp
	xor	dx,dx		! remainder DX:BP
	xor	bp,bp
	mov	cx,*32
ud_loop:
	shl	ax,*1
	rcl	bx,*1
	rcl	bp,*1
	rcl	dx,*1
	cmp	dx,si
	jb	ud_next
	ja	ud_sub
	cmp	bp,di
	jb	ud_next
ud_sub:
	sub	bp,di
	sbb	dx,si
	inc	ax		! set quotient bit
ud_next:
	loop	ud_loop
	mov	cx,bp
	pop	bp
	ret

	export	ldivul
ldivul:
	push	cx
	push	dx
	push	si
	push	di
	mov	si,2[di]
	mov	di,[di]
	call	udiv32
	pop	di
	pop	si
	pop	dx
	pop	cx
	ret

	export	lmodul
lmodul:
	push	cx
	push	dx
	push	si
	push	di
	mov	si,2[di]
	mov	di,[di]
	call	udiv32
	mov	ax,cx
	mov	bx,dx
	pop	di
	pop	si
	pop	dx
	pop	cx
	ret

! signed versions: divide magnitudes, then fix signs
! (quotient negative if signs differ; remainder takes the dividend's sign)

	export	ldivl
ldivl:
	push	cx
	push	dx
	push	si
	push	di
	push	bp
	mov	si,2[di]
	mov	di,[di]
	mov	bp,bx		! bp sign: high bit = sign of quotient
	xor	bp,si
	or	bx,bx
	jns	ld_apos
	neg	bx
	neg	ax
	sbb	bx,*0
ld_apos:
	or	si,si
	jns	ld_bpos
	neg	si
	neg	di
	sbb	si,*0
ld_bpos:
	push	bp
	call	udiv32
	pop	bp
	or	bp,bp
	jns	ld_done
	neg	bx
	neg	ax
	sbb	bx,*0
ld_done:
	pop	bp
	pop	di
	pop	si
	pop	dx
	pop	cx
	ret

	export	lmodl
lmodl:
	push	cx
	push	dx
	push	si
	push	di
	push	bp
	mov	si,2[di]
	mov	di,[di]
	mov	bp,bx		! remainder takes the dividend's sign
	or	bx,bx
	jns	lm_apos
	neg	bx
	neg	ax
	sbb	bx,*0
lm_apos:
	or	si,si
	jns	lm_bpos
	neg	si
	neg	di
	sbb	si,*0
lm_bpos:
	push	bp
	call	udiv32
	pop	bp
	mov	ax,cx
	mov	bx,dx
	or	bp,bp
	jns	lm_done
	neg	bx
	neg	ax
	sbb	bx,*0
lm_done:
	pop	bp
	pop	di
	pop	si
	pop	dx
	pop	cx
	ret
