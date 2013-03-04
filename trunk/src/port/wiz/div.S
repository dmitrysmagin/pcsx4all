.text
.align 4

;@ q RN 0 ; input denominator d, output quotient q
;@ r RN 1 ; input numerator n, output remainder r
;@ s RN 2 ; scratch register
;@ m RN 3 ; scratch register
;@ a RN 12 ; scratch register

d .req r0
q .req r0
n .req r1
r .req r1
s .req r2
m .req r3
a .req r12

.global udiv_32by32_arm9e @ unsigned udiv_32by32_arm9e(unsigned d, unsigned n)
.global ludiv_32by32_arm9e @ unsigned long long ludiv_32by32_arm9e(unsigned d, unsigned n)

udiv_32by32_arm9e:
ludiv_32by32_arm9e:
	CLZ s, q ;@ 01 : find normalizing shift
	MOVS a, q, LSL s ;@ 02 : perform a lookup on the
	ADD a, pc, a, LSR#25 ;@ 03 : most significant 7 bits
	LDRNEB a, [a, #t32-b32-64] ;@ 04 : of divisor
b32:
	SUBS s, s, #7 ;@ 05 : correct shift
	RSB m, q, #0 ;@ 06 : m = -d
	MOVPL q, a, LSL s ;@ 07 : q approx (1 << 32)/d
	;@ 1st Newton iteration follows
	MULPL a, q, m ;@ 08 : a = -q*d	
	BMI udiv_by_large_d ;@ 09 : large d trap
	SMLAWT q, q, a, q ;@ 10 : q approx q-(q*q*d >> 32)
	TEQ m, m, ASR#1 ;@ 11 : check for d=0 or d=1
	;@ 2nd Newton iteration follows
	MULNE a, q, m ;@ 12 : a = -q*d
	MOVNE s, #0 ;@ 13 : s = 0
	SMLALNE s, q, a, q ;@ 14 : q = q-(q*q*d >> 32)
	BEQ udiv_by_0_or_1 ;@ 15 : trap d=0 or d=1
	;@ q now accurate enough for a remainder r, 0<=r<3*d
	UMULL s, q, r, q ;@ 16 : q = (r*q) >> 32
	ADD r, r, m ;@ 17 : r = n-d
	MLA r, q, m, r ;@ 18 : r = n-(q+1)*d
	;@ since 0 <= n-q*d < 3*d, thus -d <= r < 2*d
	CMN r, m ;@ 19 : t = r-d
	SUBCS r, r, m ;@ 20 : if (t<-d || t>=0) r=r+d
	ADDCC q, q, #1 ;@ 21 : if (-d<=t && t<0) q=q+1
	ADDPL r, r, m, LSL#1 ;@ 22 : if (t>=0) { r=r-2*d
	ADDPL q, q, #2 ;@ 23 : q=q+2 }
	BX lr ;@ 24 : return {q, r}
udiv_by_large_d:
	;@ at this point we know d >= 2?(31-6)=2?25
	SUB a, a, #4 ;@ 25 : set q to be an
	RSB s, s, #0 ;@ 26 : underestimate of
	MOV q, a, LSR s ;@ 27 : (1 << 32)/d
	UMULL s, q, r, q ;@ 28 : q = (n*q) >> 32
	MLA r, q, m, r ;@ 29 : r = n-q*d
	;@ q now accurate enough for a remainder r, 0<=r<4*d
	CMN m, r, LSR#1 ;@ 30 : if (r/2 >= d)
	ADDCS r, r, m, LSL#1 ;@ 31 : { r=r-2*d;
	ADDCS q, q, #2 ;@ 32 : q=q+2; }
	CMN m, r ;@ 33 : if (r >= d)
	ADDCS r, r, m ;@ 34 : { r=r-d;
	ADDCS q, q, #1 ;@ 35 : q=q+1; }
	BX lr ;@ 36 : return {q, r}
udiv_by_0_or_1:
	;@ carry set if d=1, carry clear if d=0
	MOVCS q, r ;@ 37 : if (d==1) { q=n;
	MOVCS r, #0 ;@ 38 : r=0; }
	MOVCC q, #-1 ;@ 39 : if (d==0) { q=-1;
	MOVCC r, #-1 ;@ 40 : r=-1; }
	BX lr ;@ 41 : return {q,r}

;@ table for 32 by 32 bit Newton Raphson divisions
;@ table[0] = 255
;@ table[i] = (1 << 14)/(64+i) for i=1,2,3,...,63
t32:
.byte 0xff, 0xfc, 0xf8, 0xf4, 0xf0, 0xed, 0xea, 0xe6
.byte 0xe3, 0xe0, 0xdd, 0xda, 0xd7, 0xd4, 0xd2, 0xcf
.byte 0xcc, 0xca, 0xc7, 0xc5, 0xc3, 0xc0, 0xbe, 0xbc
.byte 0xba, 0xb8, 0xb6, 0xb4, 0xb2, 0xb0, 0xae, 0xac
.byte 0xaa, 0xa8, 0xa7, 0xa5, 0xa3, 0xa2, 0xa0, 0x9f
.byte 0x9d, 0x9c, 0x9a, 0x99, 0x97, 0x96, 0x94, 0x93
.byte 0x92, 0x90, 0x8f, 0x8e, 0x8d, 0x8c, 0x8a, 0x89
.byte 0x88, 0x87, 0x86, 0x85, 0x84, 0x83, 0x82, 0x81

;@ a RN 12 ; sign
sign .req r12

.global sdiv_32by32_arm9e @ signed sdiv_32by32_arm9e(signed d, signed n)
.global lsdiv_32by32_arm9e @ signed long long sdiv_32by32_arm9e(signed d, signed n)

sdiv_32by32_arm9e:
lsdiv_32by32_arm9e:
  ANDS sign, d, #1 << 31 ;@ sign=(d<0 ? 1 << 31 : 0);
  RSBMI d, d, #0 ;@ if (d<0) d=-d;
  EORS sign, sign, r, ASR#32 ;@ if (r<0) sign=~sign;
  RSBCS r, r, #0 ;@ if (r<0) r=-r;
  stmfd sp!,{sign}
	CLZ s, q ;@ 01 : find normalizing shift
	MOVS a, q, LSL s ;@ 02 : perform a lookup on the
	ADD a, pc, a, LSR#25 ;@ 03 : most significant 7 bits
	LDRNEB a, [a, #t33-b33-64] ;@ 04 : of divisor
b33:
	SUBS s, s, #7 ;@ 05 : correct shift
	RSB m, q, #0 ;@ 06 : m = -d
	MOVPL q, a, LSL s ;@ 07 : q approx (1 << 32)/d
	;@ 1st Newton iteration follows
	MULPL a, q, m ;@ 08 : a = -q*d	
	BMI sdiv_by_large_d ;@ 09 : large d trap
	SMLAWT q, q, a, q ;@ 10 : q approx q-(q*q*d >> 32)
	TEQ m, m, ASR#1 ;@ 11 : check for d=0 or d=1
	;@ 2nd Newton iteration follows
	MULNE a, q, m ;@ 12 : a = -q*d
	MOVNE s, #0 ;@ 13 : s = 0
	SMLALNE s, q, a, q ;@ 14 : q = q-(q*q*d >> 32)
	BEQ sdiv_by_0_or_1 ;@ 15 : trap d=0 or d=1
	;@ q now accurate enough for a remainder r, 0<=r<3*d
	UMULL s, q, r, q ;@ 16 : q = (r*q) >> 32
	ADD r, r, m ;@ 17 : r = n-d
	MLA r, q, m, r ;@ 18 : r = n-(q+1)*d
	;@ since 0 <= n-q*d < 3*d, thus -d <= r < 2*d
	CMN r, m ;@ 19 : t = r-d
	SUBCS r, r, m ;@ 20 : if (t<-d || t>=0) r=r+d
	ADDCC q, q, #1 ;@ 21 : if (-d<=t && t<0) q=q+1
	ADDPL r, r, m, LSL#1 ;@ 22 : if (t>=0) { r=r-2*d
	ADDPL q, q, #2 ;@ 23 : q=q+2 }
  ldmfd sp!,{sign}
  MOVS sign, sign, LSL#1 ;@ C=sign[31], N=sign[30]
  RSBCS d, d, #0 ;@ if (sign[31]) d=-d;
  RSBMI r, r, #0 ;@ if (sign[30]) r=-r;
	BX lr ;@ 24 : return {q, r}
sdiv_by_large_d:
	;@ at this point we know d >= 2?(31-6)=2?25
	SUB a, a, #4 ;@ 25 : set q to be an
	RSB s, s, #0 ;@ 26 : underestimate of
	MOV q, a, LSR s ;@ 27 : (1 << 32)/d
	UMULL s, q, r, q ;@ 28 : q = (n*q) >> 32
	MLA r, q, m, r ;@ 29 : r = n-q*d
	;@ q now accurate enough for a remainder r, 0<=r<4*d
	CMN m, r, LSR#1 ;@ 30 : if (r/2 >= d)
	ADDCS r, r, m, LSL#1 ;@ 31 : { r=r-2*d;
	ADDCS q, q, #2 ;@ 32 : q=q+2; }
	CMN m, r ;@ 33 : if (r >= d)
	ADDCS r, r, m ;@ 34 : { r=r-d;
	ADDCS q, q, #1 ;@ 35 : q=q+1; }
  ldmfd sp!,{sign}
  MOVS sign, sign, LSL#1 ;@ C=sign[31], N=sign[30]
  RSBCS d, d, #0 ;@ if (sign[31]) d=-d;
  RSBMI r, r, #0 ;@ if (sign[30]) r=-r;
	BX lr ;@ 36 : return {q, r}
sdiv_by_0_or_1:
	;@ carry set if d=1, carry clear if d=0
	MOVCS q, r ;@ 37 : if (d==1) { q=n;
	MOVCS r, #0 ;@ 38 : r=0; }
	MOVCC q, #-1 ;@ 39 : if (d==0) { q=-1;
	MOVCC r, #-1 ;@ 40 : r=-1; }
  ldmfd sp!,{sign}
  MOVS sign, sign, LSL#1 ;@ C=sign[31], N=sign[30]
  RSBCS d, d, #0 ;@ if (sign[31]) d=-d;
  RSBMI r, r, #0 ;@ if (sign[30]) r=-r;
	BX lr ;@ 41 : return {q,r}

;@ table for 32 by 32 bit Newton Raphson divisions
;@ table[0] = 255
;@ table[i] = (1 << 14)/(64+i) for i=1,2,3,...,63
t33:
.byte 0xff, 0xfc, 0xf8, 0xf4, 0xf0, 0xed, 0xea, 0xe6
.byte 0xe3, 0xe0, 0xdd, 0xda, 0xd7, 0xd4, 0xd2, 0xcf
.byte 0xcc, 0xca, 0xc7, 0xc5, 0xc3, 0xc0, 0xbe, 0xbc
.byte 0xba, 0xb8, 0xb6, 0xb4, 0xb2, 0xb0, 0xae, 0xac
.byte 0xaa, 0xa8, 0xa7, 0xa5, 0xa3, 0xa2, 0xa0, 0x9f
.byte 0x9d, 0x9c, 0x9a, 0x99, 0x97, 0x96, 0x94, 0x93
.byte 0x92, 0x90, 0x8f, 0x8e, 0x8d, 0x8c, 0x8a, 0x89
.byte 0x88, 0x87, 0x86, 0x85, 0x84, 0x83, 0x82, 0x81
