! fpdefs.inc - layout shared by the software floating point routines.
! Each routine includes it (as86 resolves the name from the directory
! build.sh runs in, the top of the repository).
!
! BCC compiles every float and double operation into a call to one of these
! routines. Doubles travel on the stack (lowest word at the lowest address),
! a second operand is addressed by BX, and the routines may change AX, BX, CX
! and DX but keep SI, DI and BP, like any function. They work on the IEEE
! formats: double, 1 sign, 11 exponent and 52 fraction bits; float, 1, 8, 23.
! Results are rounded to nearest, ties to even; overflow gives infinity, and
! invalid operations (0/0, inf - inf, 0 * inf) the NaN the 8087 gives,
! FFF8 0000 0000 0000, so that the software and 8087 versions agree.
!
! Fent builds this frame (BP is the frame pointer):
!	X[bp]	the double on top of the caller's stack
!	Y[bp]	the double below it
!	A, B	the two operands, unpacked
!	W	scratch, 8 words
! An unpacked number: a 64-bit mantissa M0..M3 (M3 most significant), whose
! top bit is set for every finite nonzero number; the exponent, biased as a
! double's (value = M / 2^63 * 2^(EXP - 1023)), signed, and not limited to
! the double's range; the sign (0 or 8000 hex); and the kind. A NaN keeps its
! fraction in M, shifted as for a normal number, without the leading 1.

X	=	8
Y	=	16
FRAME	=	44

W0	=	-44
W1	=	-42
W2	=	-40
W3	=	-38
W4	=	-36
W5	=	-34
W6	=	-32
W7	=	-30

A	=	-28
A_M0	=	-28
A_M1	=	-26
A_M2	=	-24
A_M3	=	-22
A_EXP	=	-20
A_SGN	=	-18
A_KIND	=	-16

B	=	-14
B_M0	=	-14
B_M1	=	-12
B_M2	=	-10
B_M3	=	-8
B_EXP	=	-6
B_SGN	=	-4
B_KIND	=	-2

! the same fields, from the start of an unpacked number
U_M0	=	0
U_M1	=	2
U_M2	=	4
U_M3	=	6
U_EXP	=	8
U_SGN	=	10
U_KIND	=	12

! kinds
K_ZERO	=	0
K_FIN	=	1
K_INF	=	2
K_NAN	=	3
