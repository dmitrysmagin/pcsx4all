/***************************************************************************
*   Copyright (C) 2010 PCSX4ALL Team                                      *
*   Copyright (C) 2010 Franxis and Chui                                   *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   51 Franklin Street, Fifth Floor, Boston, MA 02111-1307 USA.           *
***************************************************************************/

/*
* GTE functions.
*/

#include "gte.h"
#include "psxmem.h"
#include "profiler.h"
// #define USE_GTE_FLAG
// #define USE_OLD_GTE_WITHOUT_PATCH

#define VX(n) (n < 3 ? regs->CP2D.p[n << 1].sw.l : regs->CP2D.p[9].sw.l)
#define VY(n) (n < 3 ? regs->CP2D.p[n << 1].sw.h : regs->CP2D.p[10].sw.l)
#define VZ(n) (n < 3 ? regs->CP2D.p[(n << 1) + 1].sw.l : regs->CP2D.p[11].sw.l)

#define VX_m3(n) (regs->CP2D.p[n << 1].sw.l)
#define VY_m3(n) (regs->CP2D.p[n << 1].sw.h)
#define VZ_m3(n) (regs->CP2D.p[(n << 1) + 1].sw.l)

#define MX11(n) (regs->CP2C.p[(n << 3)].sw.l)
#define MX12(n) (regs->CP2C.p[(n << 3)].sw.h)
#define MX13(n) (regs->CP2C.p[(n << 3) + 1].sw.l)
#define MX21(n) (regs->CP2C.p[(n << 3) + 1].sw.h)
#define MX22(n) (regs->CP2C.p[(n << 3) + 2].sw.l)
#define MX23(n) (regs->CP2C.p[(n << 3) + 2].sw.h)
#define MX31(n) (regs->CP2C.p[(n << 3) + 3].sw.l)
#define MX32(n) (regs->CP2C.p[(n << 3) + 3].sw.h)
#define MX33(n) (regs->CP2C.p[(n << 3) + 4].sw.l)
#define CV1(n) ((s32)regs->CP2C.r[(n << 3) + 5])
#define CV2(n) ((s32)regs->CP2C.r[(n << 3) + 6])
#define CV3(n) ((s32)regs->CP2C.r[(n << 3) + 7])

#define fSX(n) ((regs->CP2D.p)[((n) + 12)].sw.l)
#define fSY(n) ((regs->CP2D.p)[((n) + 12)].sw.h)
#define fSZ(n) ((regs->CP2D.p)[((n) + 17)].w.l) /* (n == 0) => SZ1; */

#define gteVXY0 (regs->CP2D.r[0])
#define gteVX0  (regs->CP2D.p[0].sw.l)
#define gteVY0  (regs->CP2D.p[0].sw.h)
#define gteVZ0  (regs->CP2D.p[1].sw.l)
#define gteVXY1 (regs->CP2D.r[2])
#define gteVX1  (regs->CP2D.p[2].sw.l)
#define gteVY1  (regs->CP2D.p[2].sw.h)
#define gteVZ1  (regs->CP2D.p[3].sw.l)
#define gteVXY2 (regs->CP2D.r[4])
#define gteVX2  (regs->CP2D.p[4].sw.l)
#define gteVY2  (regs->CP2D.p[4].sw.h)
#define gteVZ2  (regs->CP2D.p[5].sw.l)
#define gteRGB  (regs->CP2D.r[6])
#define gteR    (regs->CP2D.p[6].b.l)
#define gteG    (regs->CP2D.p[6].b.h)
#define gteB    (regs->CP2D.p[6].b.h2)
#define gteCODE (regs->CP2D.p[6].b.h3)
#define gteOTZ  (regs->CP2D.p[7].w.l)
#define gteIR0  (regs->CP2D.p[8].sw.l)
#define gteIR1  (regs->CP2D.p[9].sw.l)
#define gteIR2  (regs->CP2D.p[10].sw.l)
#define gteIR3  (regs->CP2D.p[11].sw.l)
#define gteSXY0 (regs->CP2D.r[12])
#define gteSX0  (regs->CP2D.p[12].sw.l)
#define gteSY0  (regs->CP2D.p[12].sw.h)
#define gteSXY1 (regs->CP2D.r[13])
#define gteSX1  (regs->CP2D.p[13].sw.l)
#define gteSY1  (regs->CP2D.p[13].sw.h)
#define gteSXY2 (regs->CP2D.r[14])
#define gteSX2  (regs->CP2D.p[14].sw.l)
#define gteSY2  (regs->CP2D.p[14].sw.h)
#define gteSXYP (regs->CP2D.r[15])
#define gteSXP  (regs->CP2D.p[15].sw.l)
#define gteSYP  (regs->CP2D.p[15].sw.h)
#define gteSZ0  (regs->CP2D.p[16].w.l)
#define gteSZ1  (regs->CP2D.p[17].w.l)
#define gteSZ2  (regs->CP2D.p[18].w.l)
#define gteSZ3  (regs->CP2D.p[19].w.l)
#define gteRGB0  (regs->CP2D.r[20])
#define gteR0    (regs->CP2D.p[20].b.l)
#define gteG0    (regs->CP2D.p[20].b.h)
#define gteB0    (regs->CP2D.p[20].b.h2)
#define gteCODE0 (regs->CP2D.p[20].b.h3)
#define gteRGB1  (regs->CP2D.r[21])
#define gteR1    (regs->CP2D.p[21].b.l)
#define gteG1    (regs->CP2D.p[21].b.h)
#define gteB1    (regs->CP2D.p[21].b.h2)
#define gteCODE1 (regs->CP2D.p[21].b.h3)
#define gteRGB2  (regs->CP2D.r[22])
#define gteR2    (regs->CP2D.p[22].b.l)
#define gteG2    (regs->CP2D.p[22].b.h)
#define gteB2    (regs->CP2D.p[22].b.h2)
#define gteCODE2 (regs->CP2D.p[22].b.h3)
#define gteRES1  (regs->CP2D.r[23])
#define gteMAC0  (((s32 *)regs->CP2D.r)[24])
#define gteMAC1  (((s32 *)regs->CP2D.r)[25])
#define gteMAC2  (((s32 *)regs->CP2D.r)[26])
#define gteMAC3  (((s32 *)regs->CP2D.r)[27])
#define gteIRGB  (regs->CP2D.r[28])
#define gteORGB  (regs->CP2D.r[29])
#define gteLZCS  (regs->CP2D.r[30])
#define gteLZCR  (regs->CP2D.r[31])

#define gteR11R12 (((s32 *)regs->CP2C.r)[0])
#define gteR22R23 (((s32 *)regs->CP2C.r)[2])
#define gteR11 (regs->CP2C.p[0].sw.l)
#define gteR12 (regs->CP2C.p[0].sw.h)
#define gteR13 (regs->CP2C.p[1].sw.l)
#define gteR21 (regs->CP2C.p[1].sw.h)
#define gteR22 (regs->CP2C.p[2].sw.l)
#define gteR23 (regs->CP2C.p[2].sw.h)
#define gteR31 (regs->CP2C.p[3].sw.l)
#define gteR32 (regs->CP2C.p[3].sw.h)
#define gteR33 (regs->CP2C.p[4].sw.l)
#define gteTRX (((s32 *)regs->CP2C.r)[5])
#define gteTRY (((s32 *)regs->CP2C.r)[6])
#define gteTRZ (((s32 *)regs->CP2C.r)[7])
#define gteL11 (regs->CP2C.p[8].sw.l)
#define gteL12 (regs->CP2C.p[8].sw.h)
#define gteL13 (regs->CP2C.p[9].sw.l)
#define gteL21 (regs->CP2C.p[9].sw.h)
#define gteL22 (regs->CP2C.p[10].sw.l)
#define gteL23 (regs->CP2C.p[10].sw.h)
#define gteL31 (regs->CP2C.p[11].sw.l)
#define gteL32 (regs->CP2C.p[11].sw.h)
#define gteL33 (regs->CP2C.p[12].sw.l)
#define gteRBK (((s32 *)regs->CP2C.r)[13])
#define gteGBK (((s32 *)regs->CP2C.r)[14])
#define gteBBK (((s32 *)regs->CP2C.r)[15])
#define gteLR1 (regs->CP2C.p[16].sw.l)
#define gteLR2 (regs->CP2C.p[16].sw.h)
#define gteLR3 (regs->CP2C.p[17].sw.l)
#define gteLG1 (regs->CP2C.p[17].sw.h)
#define gteLG2 (regs->CP2C.p[18].sw.l)
#define gteLG3 (regs->CP2C.p[18].sw.h)
#define gteLB1 (regs->CP2C.p[19].sw.l)
#define gteLB2 (regs->CP2C.p[19].sw.h)
#define gteLB3 (regs->CP2C.p[20].sw.l)
#define gteRFC (((s32 *)regs->CP2C.r)[21])
#define gteGFC (((s32 *)regs->CP2C.r)[22])
#define gteBFC (((s32 *)regs->CP2C.r)[23])
#define gteOFX (((s32 *)regs->CP2C.r)[24])
#define gteOFY (((s32 *)regs->CP2C.r)[25])
#define gteH   (regs->CP2C.p[26].sw.l)
#define gteDQA (regs->CP2C.p[27].sw.l)
#define gteDQB (((s32 *)regs->CP2C.r)[28])
#define gteZSF3 (regs->CP2C.p[29].sw.l)
#define gteZSF4 (regs->CP2C.p[30].sw.l)
#define gteFLAG (regs->CP2C.r[31])

#define GTE_OP(op) ((op >> 20) & 31)
#define GTE_SF(op) ((op >> 19) & 1)
#define GTE_MX(op) ((op >> 17) & 3)
#define GTE_V(op) ((op >> 15) & 3)
#define GTE_CV(op) ((op >> 13) & 3)
#define GTE_CD(op) ((op >> 11 ) & 3) /* not used */
#define GTE_LM(op) ((op >> 10 ) & 1)
#define GTE_CT(op) ((op >> 6) & 15) /* not used */
#define GTE_FUNCT(op) (op & 63)

#define gteop (regs->code & 0x1ffffff)

#define MUL(a,b) ((a)*(b))

INLINE s32 _A1(psxRegisters *regs, s64 n_value) {
#ifdef USE_GTE_FLAG
	if (n_value > 0x7fffffff) {
		gteFLAG |= (1 << 30);
	} else if (n_value < -(s64)0x80000000) {
		gteFLAG |= ((1 << 31) | (1 << 27));
	}
#endif
	return n_value;
}
#define A1(VALUE) _A1(regs,VALUE)

INLINE s32 _A1_FLAG(psxRegisters *regs, s64 n_value) {
	if (n_value > 0x7fffffff) {
		gteFLAG |= (1 << 30);
	} else if (n_value < -(s64)0x80000000) {
		gteFLAG |= ((1 << 31) | (1 << 27));
	}
	return n_value;
}
#define A1_FLAG(VALUE) _A1_FLAG(regs,VALUE)

INLINE s32 _A2(psxRegisters *regs,s64 n_value) {
#ifdef USE_GTE_FLAG
	if (n_value > 0x7fffffff) {
		gteFLAG |= (1 << 29);
	} else if (n_value < -(s64)0x80000000) {
		gteFLAG |= ((1 << 31) | (1 << 26));
	}
#endif
	return n_value;
}
#define A2(VALUE) _A2(regs,VALUE)

INLINE s32 _A2_FLAG(psxRegisters *regs,s64 n_value) {
	if (n_value > 0x7fffffff) {
		gteFLAG |= (1 << 29);
	} else if (n_value < -(s64)0x80000000) {
		gteFLAG |= ((1 << 31) | (1 << 26));
	}
	return n_value;
}
#define A2_FLAG(VALUE) _A2_FLAG(regs,VALUE)

INLINE s32 _A3(psxRegisters *regs,s64 n_value) {
#ifdef USE_GTE_FLAG
	if (n_value > 0x7fffffff) {
		gteFLAG |= (1 << 28);
	} else if (n_value < -(s64)0x80000000) {
		gteFLAG |= ((1 << 31) | (1 << 25));
	}
#endif
	return n_value;
}
#define A3(VALUE) _A3(regs,VALUE)

INLINE s32 _A3_FLAG(psxRegisters *regs,s64 n_value) {
	if (n_value > 0x7fffffff) {
		gteFLAG |= (1 << 28);
	} else if (n_value < -(s64)0x80000000) {
		gteFLAG |= ((1 << 31) | (1 << 25));
	}
	return n_value;
}
#define A3_FLAG(VALUE) _A3_FLAG(regs,VALUE)

INLINE s32 _F(psxRegisters *regs,s64 n_value) {
#ifdef USE_GTE_FLAG
	if (n_value > 0x7fffffff) {
		gteFLAG |= ((1 << 31) | (1 << 16));
	} else if (n_value < -(s64)0x80000000) {
		gteFLAG |= ((1 << 31) | (1 << 15));
	}
#endif
	return n_value;
}
#define F(VALUE) _F(regs,VALUE)

INLINE s32 _F_FLAG(psxRegisters *regs,s64 n_value) {
	if (n_value > 0x7fffffff) {
		gteFLAG |= ((1 << 31) | (1 << 16));
	} else if (n_value < -(s64)0x80000000) {
		gteFLAG |= ((1 << 31) | (1 << 15));
	}
	return n_value;
}
#define F_FLAG(VALUE) _F_FLAG(regs,VALUE)

#define A1_32(VALUE) (VALUE)
#define A2_32(VALUE) (VALUE)
#define A3_32(VALUE) (VALUE)
#define F_32(VALUE)  (VALUE)

INLINE s32 _limB1(psxRegisters *regs,s32 value, s32 l) {
	if (value > 0x7fff) {
#ifdef USE_GTE_FLAG
		gteFLAG |= ((1 << 31) | (1 << 24));
#endif
		value = 0x7fff;
	} else {
		l=(l?0:-0x8000);
		if (value < l) {
#ifdef USE_GTE_FLAG
			gteFLAG |= ((1 << 31) | (1 << 24));
#endif
			value = l;
		}
	}
	return value;
}
#define limB1(VALUE,L) _limB1(regs,VALUE,L)

INLINE s32 _limB1_FLAG(psxRegisters *regs,s32 value, s32 l) {
	if (value > 0x7fff) {
		gteFLAG |= ((1 << 31) | (1 << 24));
		value = 0x7fff;
	} else {
		l=(l?0:-0x8000);
		if (value < l) {
			gteFLAG |= ((1 << 31) | (1 << 24));
			value = l;
		}
	}
	return value;
}
#define limB1_FLAG(VALUE,L) _limB1_FLAG(regs,VALUE,L)

INLINE s32 _limB10(psxRegisters *regs,s32 value) {
	if (value > 0x7fff) {
#ifdef USE_GTE_FLAG
		gteFLAG |= ((1 << 31) | (1 << 24));
#endif
		value = 0x7fff;
	} else if (value < -0x8000) {
#ifdef USE_GTE_FLAG
		gteFLAG |= ((1 << 31) | (1 << 24));
#endif
		value = -0x8000;
	}
	return value;
}
#define limB10(VALUE) _limB10(regs,VALUE)

INLINE s32 _limB10_FLAG(psxRegisters *regs,s32 value) {
	if (value > 0x7fff) {
		gteFLAG |= ((1 << 31) | (1 << 24));
		value = 0x7fff;
	} else if (value < -0x8000) {
		gteFLAG |= ((1 << 31) | (1 << 24));
		value = -0x8000;
	}
	return value;
}
#define limB10_FLAG(VALUE) _limB10_FLAG(regs,VALUE)

INLINE s32 _limB11(psxRegisters *regs,s32 value) {
	if (value > 0x7fff) {
#ifdef USE_GTE_FLAG
		gteFLAG |= ((1 << 31) | (1 << 24));
#endif
		value = 0x7fff;
	} else if (value < 0) {
#ifdef USE_GTE_FLAG
		gteFLAG |= ((1 << 31) | (1 << 24));
#endif
		value = 0;
	}
	return value;
}
#define limB11(VALUE) _limB11(regs,VALUE)

INLINE s32 _limB11_FLAG(psxRegisters *regs,s32 value) {
	if (value > 0x7fff) {
		gteFLAG |= ((1 << 31) | (1 << 24));
		value = 0x7fff;
	} else if (value < 0) {
		gteFLAG |= ((1 << 31) | (1 << 24));
		value = 0;
	}
	return value;
}
#define limB11_FLAG(VALUE) _limB11_FLAG(regs,VALUE)

INLINE s32 _limB2(psxRegisters *regs,s32 value, s32 l) {
	if (value > 0x7fff) {
#ifdef USE_GTE_FLAG
		gteFLAG |= ((1 << 31) | (1 << 23));
#endif
		value = 0x7fff;
	} else {
		l=(l?0:-0x8000);
		if (value < l) {
#ifdef USE_GTE_FLAG
			gteFLAG |= ((1 << 31) | (1 << 23));
#endif
			value = l;
		}
	}
	return value;
}
#define limB2(VALUE,L) _limB2(regs,VALUE,L)

INLINE s32 _limB2_FLAG(psxRegisters *regs,s32 value, s32 l) {
	if (value > 0x7fff) {
		gteFLAG |= ((1 << 31) | (1 << 23));
		value = 0x7fff;
	} else {
		l=(l?0:-0x8000);
		if (value < l) {
			gteFLAG |= ((1 << 31) | (1 << 23));
			value = l;
		}
	}
	return value;
}
#define limB2_FLAG(VALUE,L) _limB2_FLAG(regs,VALUE,L)

INLINE s32 _limB20(psxRegisters *regs,s32 value) {
	if (value > 0x7fff) {
#ifdef USE_GTE_FLAG
		gteFLAG |= ((1 << 31) | (1 << 23));
#endif
		value = 0x7fff;
	} else if (value < -0x8000) {
#ifdef USE_GTE_FLAG
		gteFLAG |= ((1 << 31) | (1 << 23));
#endif
		value = -0x8000;
	}
	return value;
}
#define limB20(VALUE) _limB20(regs,VALUE)

INLINE s32 _limB20_FLAG(psxRegisters *regs,s32 value) {
	if (value > 0x7fff) {
		gteFLAG |= ((1 << 31) | (1 << 23));
		value = 0x7fff;
	} else if (value < -0x8000) {
		gteFLAG |= ((1 << 31) | (1 << 23));
		value = -0x8000;
	}
	return value;
}
#define limB20_FLAG(VALUE) _limB20_FLAG(regs,VALUE)

INLINE s32 _limB21(psxRegisters *regs,s32 value) {
	if (value > 0x7fff) {
#ifdef USE_GTE_FLAG
		gteFLAG |= ((1 << 31) | (1 << 23));
#endif
		value = 0x7fff;
	} else if (value < 0) {
#ifdef USE_GTE_FLAG
		gteFLAG |= ((1 << 31) | (1 << 23));
#endif
		value = 0;
	}
	return value;
}
#define limB21(VALUE) _limB21(regs,VALUE)

INLINE s32 _limB21_FLAG(psxRegisters *regs,s32 value) {
	if (value > 0x7fff) {
		gteFLAG |= ((1 << 31) | (1 << 23));
		value = 0x7fff;
	} else if (value < 0) {
		gteFLAG |= ((1 << 31) | (1 << 23));
		value = 0;
	}
	return value;
}
#define limB21_FLAG(VALUE) _limB21_FLAG(regs,VALUE)

INLINE s32 _limB3(psxRegisters *regs,s32 value, s32 l) {
	if (value > 0x7fff) {
#ifdef USE_GTE_FLAG
		gteFLAG |= (1 << 22);
#endif
		value = 0x7fff;
	} else {
		l=(l?0:-0x8000);
		if (value < l) {
#ifdef USE_GTE_FLAG
			gteFLAG |= (1 << 22);
#endif
			value = l;
		}
	}
	return value;
}
#define limB3(VALUE,L) _limB3(regs,VALUE,L)

INLINE s32 _limB3_FLAG(psxRegisters *regs,s32 value, s32 l) {
	if (value > 0x7fff) {
		gteFLAG |= (1 << 22);
		value = 0x7fff;
	} else {
		l=(l?0:-0x8000);
		if (value < l) {
			gteFLAG |= (1 << 22);
			value = l;
		}
	}
	return value;
}
#define limB3_FLAG(VALUE,L) _limB3_FLAG(regs,VALUE,L)

INLINE s32 _limB30(psxRegisters *regs,s32 value) {
	if (value > 0x7fff) {
#ifdef USE_GTE_FLAG
		gteFLAG |= (1 << 22);
#endif
		value = 0x7fff;
	} else if (value < -0x8000) {
#ifdef USE_GTE_FLAG
		gteFLAG |= (1 << 22);
#endif
		value = -0x8000;
	}
	return value;
}
#define limB30(VALUE) _limB30(regs,VALUE)

INLINE s32 _limB30_FLAG(psxRegisters *regs,s32 value) {
	if (value > 0x7fff) {
		gteFLAG |= (1 << 22);
		value = 0x7fff;
	} else if (value < -0x8000) {
		gteFLAG |= (1 << 22);
		value = -0x8000;
	}
	return value;
}
#define limB30_FLAG(VALUE) _limB30_FLAG(regs,VALUE)

INLINE s32 _limB31(psxRegisters *regs,s32 value) {
	if (value > 0x7fff) {
#ifdef USE_GTE_FLAG
		gteFLAG |= (1 << 22);
#endif
		value = 0x7fff;
	} else if (value < 0) {
#ifdef USE_GTE_FLAG
		gteFLAG |= (1 << 22);
#endif
		value = 0;
	}
	return value;
}
#define limB31(VALUE) _limB31(regs,VALUE)

INLINE s32 _limB31_FLAG(psxRegisters *regs,s32 value) {
	if (value > 0x7fff) {
		gteFLAG |= (1 << 22);
		value = 0x7fff;
	} else if (value < 0) {
		gteFLAG |= (1 << 22);
		value = 0;
	}
	return value;
}
#define limB31_FLAG(VALUE) _limB31_FLAG(regs,VALUE)

INLINE s32 _limC1(psxRegisters *regs,s32 value) {
	if (value > 0x00ff) {
#ifdef USE_GTE_FLAG
		gteFLAG |= (1 << 21);
#endif
		value = 0x00ff;
	} else if (value < 0x0000) {
#ifdef USE_GTE_FLAG
		gteFLAG |= (1 << 21);
#endif
		value = 0x0000;
	}
	return value;
}
#define limC1(VALUE) _limC1(regs,VALUE)

INLINE s32 _limC1_FLAG(psxRegisters *regs,s32 value) {
	if (value > 0x00ff) {
		gteFLAG |= (1 << 21);
		value = 0x00ff;
	} else if (value < 0x0000) {
		gteFLAG |= (1 << 21);
		value = 0x0000;
	}
	return value;
}
#define limC1_FLAG(VALUE) _limC1_FLAG(regs,VALUE)

INLINE s32 _limC2(psxRegisters *regs,s32 value) {
	if (value > 0x00ff) {
#ifdef USE_GTE_FLAG
		gteFLAG |= (1 << 20);
#endif
		value = 0x00ff;
	} else if (value < 0x0000) {
#ifdef USE_GTE_FLAG
		gteFLAG |= (1 << 20);
#endif
		value = 0x0000;
	}
	return value;
}
#define limC2(VALUE) _limC2(regs,VALUE)

INLINE s32 _limC2_FLAG(psxRegisters *regs,s32 value) {
	if (value > 0x00ff) {
		gteFLAG |= (1 << 20);
		value = 0x00ff;
	} else if (value < 0x0000) {
		gteFLAG |= (1 << 20);
		value = 0x0000;
	}
	return value;
}
#define limC2_FLAG(VALUE) _limC2_FLAG(regs,VALUE)

INLINE s32 _limC3(psxRegisters *regs,s32 value) {
	if (value > 0x00ff) {
#ifdef USE_GTE_FLAG
		gteFLAG |= (1 << 19);
#endif
		value = 0x00ff;
	} else if (value < 0x0000) {
#ifdef USE_GTE_FLAG
		gteFLAG |= (1 << 19);
#endif
		value = 0x0000;
	}
	return value;
}
#define limC3(VALUE) _limC3(regs,VALUE)

INLINE s32 _limC3_FLAG(psxRegisters *regs,s32 value) {
	if (value > 0x00ff) {
		gteFLAG |= (1 << 19);
		value = 0x00ff;
	} else if (value < 0x0000) {
		gteFLAG |= (1 << 19);
		value = 0x0000;
	}
	return value;
}
#define limC3_FLAG(VALUE) _limC3_FLAG(regs,VALUE)

INLINE s32 _limD(psxRegisters *regs,s32 value) {
	if (value > 0xffff) {
#ifdef USE_GTE_FLAG
		gteFLAG |= ((1 << 31) | (1 << 18));
#endif
		value = 0xffff;
	} else if (value < 0x0000) {
#ifdef USE_GTE_FLAG
		gteFLAG |= ((1 << 31) | (1 << 18));
#endif
		value = 0x0000;
	}
	return value;
}
#define limD(VALUE) _limD(regs,VALUE)

INLINE s32 _limD_FLAG(psxRegisters *regs,s32 value) {
	if (value > 0xffff) {
		gteFLAG |= ((1 << 31) | (1 << 18));
		value = 0xffff;
	} else if (value < 0x0000) {
		gteFLAG |= ((1 << 31) | (1 << 18));
		value = 0x0000;
	}
	return value;
}
#define limD_FLAG(VALUE) _limD_FLAG(regs,VALUE)

INLINE u32 _limE(psxRegisters *regs,u32 result) {
	if (result > 0x1ffff) {
#ifdef USE_GTE_FLAG
		gteFLAG |= ((1 << 31) | (1 << 17));
#endif
		result=0x1ffff;
	}
	return result;
}
#define limE(VALUE) _limE(regs,VALUE)

INLINE u32 _limE_FLAG(psxRegisters *regs,u32 result) {
	if (result > 0x1ffff) {
		gteFLAG |= ((1 << 31) | (1 << 17));
		result=0x1ffff;
	}
	return result;
}
#define limE_FLAG(VALUE) _limE_FLAG(regs,VALUE)


INLINE s32 _limG1(psxRegisters *regs,s32 value) {
	if (value > 0x3ff) {
#ifdef USE_GTE_FLAG
		gteFLAG |= ((1 << 31) | (1 << 14));
#endif
		value = 0x3ff;
	} else if (value < -0x400) {
#ifdef USE_GTE_FLAG
		gteFLAG |= ((1 << 31) | (1 << 14));
#endif
		value = -0x400;
	}
	return value;
}
#define limG1(VALUE) _limG1(regs,VALUE)

INLINE s32 _limG1_FLAG(psxRegisters *regs,s32 value) {
	if (value > 0x3ff) {
		gteFLAG |= ((1 << 31) | (1 << 14));
		value = 0x3ff;
	} else if (value < -0x400) {
		gteFLAG |= ((1 << 31) | (1 << 14));
		value = -0x400;
	}
	return value;
}
#define limG1_FLAG(VALUE) _limG1_FLAG(regs,VALUE)

INLINE s32 _limG2(psxRegisters *regs,s32 value) {
	if (value > 0x3ff) {
#ifdef USE_GTE_FLAG
		gteFLAG |= ((1 << 31) | (1 << 13));
#endif
		value = 0x3ff;
	} else if (value < -0x400) {
#ifdef USE_GTE_FLAG
		gteFLAG |= ((1 << 31) | (1 << 13));
#endif
		value = -0x400;
	}
	return value;
}
#define limG2(VALUE) _limG2(regs,VALUE)

INLINE s32 _limG2_FLAG(psxRegisters *regs,s32 value) {
	if (value > 0x3ff) {
		gteFLAG |= ((1 << 31) | (1 << 13));
		value = 0x3ff;
	} else if (value < -0x400) {
		gteFLAG |= ((1 << 31) | (1 << 13));
		value = -0x400;
	}
	return value;
}
#define limG2_FLAG(VALUE) _limG2_FLAG(regs,VALUE)

INLINE s32 _limH(psxRegisters *regs,s32 value) {
#ifdef USE_OLD_GTE_WITHOUT_PATCH
	if (value > 0xfff) {
#else
	if (value > 0x1000) {
#endif
#ifdef USE_GTE_FLAG
		gteFLAG |= (1 << 12);
#endif
#ifdef USE_OLD_GTE_WITHOUT_PATCH
		value = 0xfff;
#else
		value = 0x1000;
#endif
	} else if (value < 0x000) {
#ifdef USE_GTE_FLAG
		gteFLAG |= (1 << 12);
#endif
		value = 0x000;
	}
	return value;
}
#define limH(VALUE) _limH(regs,VALUE)

INLINE s32 _limH_FLAG(psxRegisters *regs,s32 value) {
	if (value > 0xfff) {
		gteFLAG |= (1 << 12);
		value = 0xfff;
	} else if (value < 0x000) {
		gteFLAG |= (1 << 12);
		value = 0x000;
	}
	return value;
}
#define limH_FLAG(VALUE) _limH_FLAG(regs,VALUE)

INLINE s32 _limMCFC2(psxRegisters *regs,s32 value) {
	if (value > 0x1f) {
		value = 0x1f;
	} else if (value < 0) {
		value = 0;
	}
	return value;
}
#define limMCFC2(VALUE) _limMCFC2(regs,VALUE)

#ifndef NO_USE_GTE_DIVIDE_TABLE
#include "gte_divide.h"
#else
/*
INLINE u32 DIVIDE(s16 n, u16 d){
	if (n>=0 && n<d*2){
		return ((u32)n<<16)/d;
	}
	return 0xffffffff;
}
*/

#ifndef __arm__
INLINE u32 DIVIDE(s16 n, u16 d) {
	if (n >= 0 && n < d * 2) {
		s32 n_ = n;
		return ((n_ << 16) + d / 2) / d;
		//return (u32)((float)(n_ << 16) / (float)d + (float)0.5);
	}
	return 0xffffffff;
}
#else
INLINE u32 DIVIDE(s16 n, u16 d) {
	if (n >= 0 && n < d * 2) {
		s32 n_ = n;
		return SDIV(((n_ << 16) + d / 2),d);
	}
	return 0xffffffff;
}
#endif

#endif

u32 _gtecalcMFC2(int reg, psxRegisters *regs) {
	pcsx4all_prof_start_with_pause(PCSX4ALL_PROF_GTE,PCSX4ALL_PROF_CPU);
	switch(reg) {
		case 1:
		case 3:
		case 5:
		case 8:
		case 9:
		case 10:
		case 11:
			regs->CP2D.r[reg] = (s32)regs->CP2D.p[reg].sw.l;
			break;

		case 7:
		case 16:
		case 17:
		case 18:
		case 19:
			regs->CP2D.r[reg] = (u32)regs->CP2D.p[reg].w.l;
			break;

		case 15:
			regs->CP2D.r[reg] = gteSXY2;
			break;

		case 28:
#ifdef USE_OLD_GTE_WITHOUT_PATCH
		case 30:
			pcsx4all_prof_end_with_resume(PCSX4ALL_PROF_GTE,PCSX4ALL_PROF_CPU);
			return 0;
#endif

		case 29:
			regs->CP2D.r[reg] = limMCFC2(gteIR1 >> 7) |
									(limMCFC2(gteIR2 >> 7) << 5) |
									(limMCFC2(gteIR3 >> 7) << 10);
			break;
	}
	u32 ret=regs->CP2D.r[reg];
	pcsx4all_prof_end_with_resume(PCSX4ALL_PROF_GTE,PCSX4ALL_PROF_CPU);
	return ret;
}

u32 gtecalcMFC2(int reg) {
	return _gtecalcMFC2(reg, &psxRegs);
}

void _gtecalcMTC2(u32 value, int reg, psxRegisters *regs) {
	pcsx4all_prof_start_with_pause(PCSX4ALL_PROF_GTE,PCSX4ALL_PROF_CPU);
	switch (reg) {
		case 15:
			gteSXY0 = gteSXY1;
			gteSXY1 = gteSXY2;
			gteSXY2 = value;
			gteSXYP = value;
			break;

		case 28:
			gteIRGB = value;
			gteIR1 = (value & 0x1f) << 7;
			gteIR2 = (value & 0x3e0) << 2;
			gteIR3 = (value & 0x7c00) >> 3;
			break;

		case 30:
			{
				gteLZCS = value;
				#if defined(__arm__) && !defined(USE_GTE_EXTRA_EXACT)
				asm ("clz %0, %1" : "=r" (gteLZCR) : "r"(value));
				#else
				int a = gteLZCS;
				if (a > 0) {
					int i;
					for (i = 31; (a & (1 << i)) == 0 && i >= 0; i--);
					gteLZCR = 31 - i;
				} else if (a < 0) {
					int i;
					a ^= 0xffffffff;
					for (i=31; (a & (1 << i)) == 0 && i >= 0; i--);
					gteLZCR = 31 - i;
				} else {
					gteLZCR = 32;
				}
				#endif
			}
			break;

#ifdef USE_OLD_GTE_WITHOUT_PATCH
		case 7:
		case 29:
#endif
		case 31:
			pcsx4all_prof_end_with_resume(PCSX4ALL_PROF_GTE,PCSX4ALL_PROF_CPU);
			return;

		default:
			regs->CP2D.r[reg] = value;
	}
	pcsx4all_prof_end_with_resume(PCSX4ALL_PROF_GTE,PCSX4ALL_PROF_CPU);
}

void gtecalcMTC2(u32 value, int reg) {
	_gtecalcMTC2(value, reg, &psxRegs);
}

void _gtecalcCTC2(u32 value, int reg, psxRegisters *regs ) {
	pcsx4all_prof_start_with_pause(PCSX4ALL_PROF_GTE,PCSX4ALL_PROF_CPU);
	switch (reg) {
		case 4:
		case 12:
		case 20:
		case 26:
		case 27:
		case 29:
		case 30:
			value = (s32)(s16)value;
			break;

		case 31:
			value = value & 0x7ffff000;
			if (value & 0x7f87e000) value |= 0x80000000;
			break;
	}

	regs->CP2C.r[reg] = value;
	pcsx4all_prof_end_with_resume(PCSX4ALL_PROF_GTE,PCSX4ALL_PROF_CPU);
}

void gtecalcCTC2(u32 value, int reg) {
	_gtecalcCTC2(value, reg, &psxRegs);
}

void _gteMFC2(psxRegisters *regs) {
	if (!_fRt_(regs->code)) return;
	regs->GPR.r[_fRt_(regs->code)] = _gtecalcMFC2(_fRd_(regs->code),regs);
}

void gteMFC2(void) {
	if (!_Rt_) return;
	psxRegs.GPR.r[_Rt_] = _gtecalcMFC2(_Rd_,&psxRegs);
	
}

void _gteCFC2(psxRegisters *regs) {
	if (!_fRt_(regs->code)) return;
	regs->GPR.r[_fRt_(regs->code)] = regs->CP2C.r[_fRd_(regs->code)];
}

void gteCFC2(void) {
	if (!_Rt_) return;
	psxRegs.GPR.r[_Rt_] = psxRegs.CP2C.r[_Rd_];
}

void _gteMTC2(psxRegisters *regs) {
	_gtecalcMTC2(regs->GPR.r[_fRt_(regs->code)], _fRd_(regs->code), regs);
}

void gteMTC2(void) {
	_gtecalcMTC2(psxRegs.GPR.r[_Rt_], _Rd_, &psxRegs);
}

void _gteCTC2(psxRegisters *regs) {
	_gtecalcCTC2(regs->GPR.r[_fRt_(regs->code)], _fRd_(regs->code), regs);
}

void gteCTC2(void) {
	_gtecalcCTC2(psxRegs.GPR.r[_Rt_], _Rd_, &psxRegs);
}

#define _foB_ (regs->GPR.r[_fRs_(regs->code)] + _fImm_(regs->code))
#define _oB_ (psxRegs.GPR.r[_Rs_] + _Imm_)

void _gteLWC2(psxRegisters *regs) {
	_gtecalcMTC2(psxMemRead32(_foB_), _fRt_(regs->code), regs);
}

void gteLWC2(void) {
	_gtecalcMTC2(psxMemRead32(_oB_), _Rt_, &psxRegs);
}

void _gteSWC2(psxRegisters *regs) {
	psxMemWrite32(_foB_, _gtecalcMFC2(_Rt_, regs));
}

void gteSWC2(void) {
	psxMemWrite32(_oB_, _gtecalcMFC2(_Rt_, &psxRegs));
}

void _gteRTPS(psxRegisters *regs) {
	pcsx4all_prof_start_with_pause(PCSX4ALL_PROF_GTE,PCSX4ALL_PROF_CPU);
	int quotient;

#ifdef GTE_LOG
	GTE_LOG("GTE RTPS\n");
#endif
	gteFLAG = 0;

	#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
		gteMAC1 = A1((((s64)gteTRX << 12) + (gteR11 * gteVX0) + (gteR12 * gteVY0) + (gteR13 * gteVZ0)) >> 12);
		gteMAC2 = A2((((s64)gteTRY << 12) + (gteR21 * gteVX0) + (gteR22 * gteVY0) + (gteR23 * gteVZ0)) >> 12);
		gteMAC3 = A3((((s64)gteTRZ << 12) + (gteR31 * gteVX0) + (gteR32 * gteVY0) + (gteR33 * gteVZ0)) >> 12);
	#else
		int aux=MUL(gteR11,gteVX0);
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteR12,gteVY0)) , "r"(aux));
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteR13,gteVZ0)) , "r"(aux));
		aux>>=12;
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteTRX) , "r"(aux));
		gteMAC1=A1_32(aux);

		aux=MUL(gteR21,gteVX0);
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteR22,gteVY0)) , "r"(aux));
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteR23,gteVZ0)) , "r"(aux));
		aux>>=12;
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteTRY) , "r"(aux));
		gteMAC2=A2_32(aux);

		aux=MUL(gteR31,gteVX0);
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteR32,gteVY0)) , "r"(aux));
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteR33,gteVZ0)) , "r"(aux));
		aux>>=12;
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteTRZ) , "r"(aux));
		gteMAC3=A3_32(aux);
	#endif
	
	gteIR1 = limB10_FLAG(gteMAC1);
	gteIR2 = limB20_FLAG(gteMAC2);
	gteIR3 = limB30_FLAG(gteMAC3);
	gteSZ0 = gteSZ1;
	gteSZ1 = gteSZ2;
	gteSZ2 = gteSZ3;
	gteSZ3 = limD_FLAG(gteMAC3);
	quotient = limE(DIVIDE(gteH, gteSZ3));
	gteSXY0 = gteSXY1;
	gteSXY1 = gteSXY2;
	
	#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
		gteSX2 = limG1_FLAG(F((s64)gteOFX + ((s64)gteIR1 * quotient)) >> 16);
	#else
		aux=MUL(gteIR1,quotient);
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteOFX) , "r"(aux));
		gteSX2 = limG1_FLAG(F_32(aux) >> 16);
	#endif
	
	#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
		gteSY2 = limG2_FLAG(F((s64)gteOFY + ((s64)gteIR2 * quotient)) >> 16);
	#else
		aux=MUL(gteIR2,quotient);
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteOFY) , "r"(aux));
		gteSY2 = limG2_FLAG(F_32(aux) >> 16);
	#endif

	#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
#ifdef USE_OLD_GTE_WITHOUT_PATCH
		gteMAC0 = F((s64)(gteDQB + ((s64)gteDQA * quotient)) >> 12);
#else
		gteMAC0 = F((s64)(gteDQB + ((s64)gteDQA * quotient)));
#endif
	#else
		aux=MUL(gteDQA,quotient);
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteDQB) , "r"(aux));
#ifdef USE_OLD_GTE_WITHOUT_PATCH
		gteMAC0 = F_32(aux>>12);
#else
		gteMAC0 = F_32(aux);
#endif
	#endif
	
	gteIR0 = limH_FLAG(gteMAC0);
	pcsx4all_prof_end_with_resume(PCSX4ALL_PROF_GTE,PCSX4ALL_PROF_CPU);
}

void gteRTPS(void) {
	_gteRTPS(&psxRegs);
}

void _gteRTPT_(psxRegisters *regs) {
	int quotient;
	s32 _ir1, _ir2;
	gteSZ0 = gteSZ3;
	{
		s32 vx = VX_m3(0);
		s32 vy = VY_m3(0);
		s32 vz = VZ_m3(0);
		s32 mac1, mac2, mac3;

		#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
			mac1 = A1((((s64)gteTRX << 12) + (gteR11 * vx) + (gteR12 * vy) + (gteR13 * vz)) >> 12);
			mac2 = A2((((s64)gteTRY << 12) + (gteR21 * vx) + (gteR22 * vy) + (gteR23 * vz)) >> 12);
			mac3 = A3((((s64)gteTRZ << 12) + (gteR31 * vx) + (gteR32 * vy) + (gteR33 * vz)) >> 12);
		#else
			{
				int aux=MUL(gteR11,vx);
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteR12,vy)) , "r"(aux));
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteR13,vz)) , "r"(aux));
				aux>>=12;
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteTRX) , "r"(aux));
				mac1=A1_32(aux);
			}
			{
				int aux=MUL(gteR21,vx);
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteR22,vy)) , "r"(aux));
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteR23,vz)) , "r"(aux));
				aux>>=12;
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteTRY) , "r"(aux));
				mac2=A2_32(aux);
			}
			{
				int aux=MUL(gteR31,vx);
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteR32,vy)) , "r"(aux));
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteR33,vz)) , "r"(aux));
				aux>>=12;
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteTRZ) , "r"(aux));
				mac3=A3_32(aux);
			}
		#endif
		{
			_ir1=limB10_FLAG(mac1);
			_ir2=limB20_FLAG(mac2);
#ifdef USE_GTE_FLAG
			gteIR1 = _ir1;
			gteIR2 = _ir2;
			gteIR3 = limB30_FLAG(mac3);
#endif
			fSZ(0) = limD_FLAG(mac3);
			quotient = limE(DIVIDE(gteH, fSZ(0)));
	
		#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
			fSX(0) = limG1(F((s64)gteOFX + ((s64)_ir1 * quotient)) >> 16);
		#else
			{
				int aux=MUL(_ir1,quotient);
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteOFX) , "r"(aux));
				fSX(0) = limG1(F_32(aux) >> 16);
			}
		#endif
		
		#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
			fSY(0) = limG2(F((s64)gteOFY + ((s64)_ir2 * quotient)) >> 16);
		#else
			{
				int aux=MUL(_ir2,quotient);
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteOFY) , "r"(aux));
				fSY(0) = limG2(F_32(aux) >> 16);
			}
		#endif
		}
	}
	{
		s32 vx = VX_m3(1);
		s32 vy = VY_m3(1);
		s32 vz = VZ_m3(1);
		s32 mac1, mac2, mac3;

		#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
			mac1 = A1((((s64)gteTRX << 12) + (gteR11 * vx) + (gteR12 * vy) + (gteR13 * vz)) >> 12);
			mac2 = A2((((s64)gteTRY << 12) + (gteR21 * vx) + (gteR22 * vy) + (gteR23 * vz)) >> 12);
			mac3 = A3((((s64)gteTRZ << 12) + (gteR31 * vx) + (gteR32 * vy) + (gteR33 * vz)) >> 12);
		#else
			{
				int aux=MUL(gteR11,vx);
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteR12,vy)) , "r"(aux));
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteR13,vz)) , "r"(aux));
				aux>>=12;
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteTRX) , "r"(aux));
				mac1=A1_32(aux);
			}
			{
				int aux=MUL(gteR21,vx);
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteR22,vy)) , "r"(aux));
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteR23,vz)) , "r"(aux));
				aux>>=12;
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteTRY) , "r"(aux));
				mac2=A2_32(aux);
			}
			{
				int aux=MUL(gteR31,vx);
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteR32,vy)) , "r"(aux));
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteR33,vz)) , "r"(aux));
				aux>>=12;
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteTRZ) , "r"(aux));
				mac3=A3_32(aux);
			}
		#endif
		{
			_ir1=limB10_FLAG(mac1);
			_ir2=limB20_FLAG(mac2);
#ifdef USE_GTE_FLAG
			gteIR1 = _ir1;
			gteIR2 = _ir2;
			gteIR3 = limB30_FLAG(mac3);
#endif
			fSZ(1) = limD_FLAG(mac3);
			quotient = limE(DIVIDE(gteH, fSZ(1)));
	
		#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
			fSX(1) = limG1(F((s64)gteOFX + ((s64)_ir1 * quotient)) >> 16);
		#else
			{
				int aux=MUL(_ir1,quotient);
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteOFX) , "r"(aux));
				fSX(1) = limG1(F_32(aux) >> 16);
			}
		#endif
		
		#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
			fSY(1) = limG2(F((s64)gteOFY + ((s64)_ir2 * quotient)) >> 16);
		#else
			{
				int aux=MUL(_ir2,quotient);
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteOFY) , "r"(aux));
				fSY(1) = limG2(F_32(aux) >> 16);
			}
		#endif
		}
	}
	{
		s32 vx = VX_m3(2);
		s32 vy = VY_m3(2);
		s32 vz = VZ_m3(2);
		s32 mac1, mac2, mac3;

		#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
			mac1 = A1((((s64)gteTRX << 12) + (gteR11 * vx) + (gteR12 * vy) + (gteR13 * vz)) >> 12);
			mac2 = A2((((s64)gteTRY << 12) + (gteR21 * vx) + (gteR22 * vy) + (gteR23 * vz)) >> 12);
			mac3 = A3((((s64)gteTRZ << 12) + (gteR31 * vx) + (gteR32 * vy) + (gteR33 * vz)) >> 12);
		#else
			{
				int aux=MUL(gteR11,vx);
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteR12,vy)) , "r"(aux));
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteR13,vz)) , "r"(aux));
				aux>>=12;
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteTRX) , "r"(aux));
				mac1=A1_32(aux);
			}
			{
				int aux=MUL(gteR21,vx);
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteR22,vy)) , "r"(aux));
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteR23,vz)) , "r"(aux));
				aux>>=12;
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteTRY) , "r"(aux));
				mac2=A2_32(aux);
			}
			{
				int aux=MUL(gteR31,vx);
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteR32,vy)) , "r"(aux));
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteR33,vz)) , "r"(aux));
				aux>>=12;
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteTRZ) , "r"(aux));
				mac3=A3_32(aux);
			}
		#endif
		{
			_ir1=limB10_FLAG(mac1);
			_ir2=limB20_FLAG(mac2);
#ifdef USE_GTE_FLAG
			gteIR1 = _ir1;
			gteIR2 = _ir2;
			gteIR3 = limB30_FLAG(mac3);
#endif
			fSZ(2) = limD_FLAG(mac3);
			quotient = limE(DIVIDE(gteH, fSZ(2)));
	
		#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
			fSX(2) = limG1(F((s64)gteOFX + ((s64)_ir1 * quotient)) >> 16);
		#else
			{
				int aux=MUL(_ir1,quotient);
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteOFX) , "r"(aux));
				fSX(2) = limG1(F_32(aux) >> 16);
			}
		#endif
		
		#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
			fSY(2) = limG2(F((s64)gteOFY + ((s64)_ir2 * quotient)) >> 16);
		#else
			{
				int aux=MUL(_ir2,quotient);
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteOFY) , "r"(aux));
				fSY(2) = limG2(F_32(aux) >> 16);
			}
		#endif
		}
		gteMAC1 = mac1;
		gteMAC2 = mac2;
		gteMAC3 = mac3;
	}
	
	#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
#ifdef USE_OLD_GTE_WITHOUT_PATCH
		gteMAC0 = F((s64)(gteDQB + ((s64)gteDQA * quotient)) >> 12);
#else
		gteMAC0 = F((s64)(gteDQB + ((s64)gteDQA * quotient)));
#endif
	#else
		{
			int aux=MUL(gteDQA,quotient);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteDQB) , "r"(aux));
#ifdef USE_OLD_GTE_WITHOUT_PATCH
			gteMAC0 = F_32(aux >> 12);
#else
			gteMAC0 = F_32(aux);
#endif
		}
	#endif

#ifndef USE_GTE_FLAG
	gteIR1 = _ir1;
	gteIR2 = _ir2;
#endif	
	gteIR0 = limH_FLAG(gteMAC0);
}


void _gteRTPT(psxRegisters *regs) {
	pcsx4all_prof_start_with_pause(PCSX4ALL_PROF_GTE,PCSX4ALL_PROF_CPU);
	int quotient;
	int v;
	s32 vx, vy, vz;

#ifdef GTE_LOG
	GTE_LOG("GTE RTPT\n");
#endif
	gteFLAG = 0;

	gteSZ0 = gteSZ3;
	s16 _ir1, _ir2;
	for (v = 0; v < 3; v++) {
		vx = VX_m3(v);
		vy = VY_m3(v);
		vz = VZ_m3(v);

		#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
			gteMAC1 = A1((((s64)gteTRX << 12) + (gteR11 * vx) + (gteR12 * vy) + (gteR13 * vz)) >> 12);
			gteMAC2 = A2((((s64)gteTRY << 12) + (gteR21 * vx) + (gteR22 * vy) + (gteR23 * vz)) >> 12);
			gteMAC3 = A3((((s64)gteTRZ << 12) + (gteR31 * vx) + (gteR32 * vy) + (gteR33 * vz)) >> 12);
		#else
			int aux=MUL(gteR11,vx);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteR12,vy)) , "r"(aux));
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteR13,vz)) , "r"(aux));
			aux>>=12;
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteTRX) , "r"(aux));
			gteMAC1=A1_32(aux);

			aux=MUL(gteR21,vx);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteR22,vy)) , "r"(aux));
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteR23,vz)) , "r"(aux));
			aux>>=12;
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteTRY) , "r"(aux));
			gteMAC2=A2_32(aux);

			aux=MUL(gteR31,vx);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteR32,vy)) , "r"(aux));
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteR33,vz)) , "r"(aux));
			aux>>=12;
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteTRZ) , "r"(aux));
			gteMAC3=A3_32(aux);
		#endif
		{
			_ir1=limB10_FLAG(gteMAC1);
			_ir2=limB20_FLAG(gteMAC2);
#ifdef USE_GTE_FLAG
			gteIR1 = _ir1;
			gteIR2 = _ir2;
			gteIR3 = limB30_FLAG(gteMAC3);
#endif
			fSZ(v) = limD_FLAG(gteMAC3);
			quotient = limE(DIVIDE(gteH, fSZ(v)));
	
		#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
			fSX(v) = limG1(F((s64)gteOFX + ((s64)_ir1 * quotient)) >> 16);
		#else
			aux=MUL(_ir1,quotient);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteOFX) , "r"(aux));
			fSX(v) = limG1(F_32(aux) >> 16);
		#endif
		
		#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
			fSY(v) = limG2(F((s64)gteOFY + ((s64)_ir2 * quotient)) >> 16);
		#else
			aux=MUL(_ir2,quotient);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteOFY) , "r"(aux));
			fSY(v) = limG2(F_32(aux) >> 16);
		#endif
		}
	}
	
	#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
#ifdef USE_OLD_GTE_WITHOUT_PATCH
		gteMAC0 = F((s64)(gteDQB + ((s64)gteDQA * quotient)));
#else
#endif
	#else
		int aux=MUL(gteDQA,quotient);
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteDQB) , "r"(aux));
#ifdef USE_OLD_GTE_WITHOUT_PATCH
		gteMAC0 = F_32(aux >> 12);
#else
		gteMAC0 = F_32(aux);
#endif
	#endif

#ifndef USE_GTE_FLAG
	gteIR1 = _ir1;
	gteIR2 = _ir2;
	gteIR3 = limB30_FLAG(gteMAC3);
#endif	
	gteIR0 = limH_FLAG(gteMAC0);
	pcsx4all_prof_end_with_resume(PCSX4ALL_PROF_GTE,PCSX4ALL_PROF_CPU);
}

void gteRTPT(void) {
	_gteRTPT(&psxRegs);
}

#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
#define macMVMVA_ncv3_nmx3(CV,MX,SHIFT) \
{ \
	s32 vx=(s32)((s16)(((u32)vxy)&0xFFFF)); \
	s32 vy=(s32)((s16)(((u32)vxy)>>16)); \
	gteMAC1 = A1((((s64)CV1(CV) << 12) + (MX11(MX) * vx) + (MX12(MX) * vy) + (MX13(MX) * vz)) >> SHIFT); \
	gteMAC2 = A2((((s64)CV2(CV) << 12) + (MX21(MX) * vx) + (MX22(MX) * vy) + (MX23(MX) * vz)) >> SHIFT); \
	gteMAC3 = A3((((s64)CV3(CV) << 12) + (MX31(MX) * vx) + (MX32(MX) * vy) + (MX33(MX) * vz)) >> SHIFT); \
}
#define macMVMVA_ncv3_mx3(CV,SHIFT) \
{ \
	gteMAC1 = A1((s64)CV1(CV) << (12-SHIFT)); \
	gteMAC2 = A2((s64)CV2(CV) << (12-SHIFT)); \
	gteMAC3 = A3((s64)CV3(CV) << (12-SHIFT)); \
}
#define macMVMVA_cv3_nmx3(MX,SHIFT) \
{ \
	s32 vx=(s32)((s16)(((u32)vxy)&0xFFFF)); \
	s32 vy=(s32)((s16)(((u32)vxy)>>16)); \
	gteMAC1 = A1((((s64)MX11(MX) * vx) + (MX12(MX) * vy) + (MX13(MX) * vz)) >> SHIFT); \
	gteMAC2 = A2((((s64)MX21(MX) * vx) + (MX22(MX) * vy) + (MX23(MX) * vz)) >> SHIFT); \
	gteMAC3 = A3((((s64)MX31(MX) * vx) + (MX32(MX) * vy) + (MX33(MX) * vz)) >> SHIFT); \
}
#else
#define macMVMVA_ncv3_nmx3(CV,MX,SHIFT) \
{ \
	s32 vx=(s32)((s16)(((u32)vxy)&0xFFFF)); \
	s32 vy=(s32)((s16)(((u32)vxy)>>16)); \
	int aux=MUL(MX11(MX),vx); \
	asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(MX12(MX),vy)) , "r"(aux)); \
	asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(MX13(MX),vz)) , "r"(aux)); \
	aux>>=SHIFT; \
	asm ("qadd %0, %1, %2" : "=r" (aux) : "r"((s32)CV1(CV) << (12-SHIFT)) , "r"(aux)); \
	gteMAC1=A1_32(aux); \
	aux=MUL(MX21(MX),vx); \
	asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(MX22(MX),vy)) , "r"(aux)); \
	asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(MX23(MX),vz)) , "r"(aux)); \
	aux>>=SHIFT; \
	asm ("qadd %0, %1, %2" : "=r" (aux) : "r"((s32)CV2(CV) << (12-SHIFT)) , "r"(aux)); \
	gteMAC2=A2_32(aux); \
	aux=MUL(MX31(MX),vx); \
	asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(MX32(MX),vy)) , "r"(aux)); \
	asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(MX33(MX),vz)) , "r"(aux)); \
	aux>>=SHIFT; \
	asm ("qadd %0, %1, %2" : "=r" (aux) : "r"((s32)CV3(CV) << (12-SHIFT)) , "r"(aux)); \
	gteMAC3=A3_32(aux); \
}
#define macMVMVA_ncv3_mx3(CV,SHIFT) \
{ \
	gteMAC1 = A1_32((s32)CV1(CV) << (12-SHIFT)); \
	gteMAC2 = A2_32((s32)CV2(CV) << (12-SHIFT)); \
	gteMAC3 = A3_32((s32)CV3(CV) << (12-SHIFT)); \
}
#define macMVMVA_cv3_nmx3(MX,SHIFT) \
{ \
	s32 vx=(s32)((s16)(((u32)vxy)&0xFFFF)); \
	s32 vy=(s32)((s16)(((u32)vxy)>>16)); \
	int aux=MUL(MX11(MX),vx); \
	asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(MX12(MX),vy)) , "r"(aux)); \
	asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(MX13(MX),vz)) , "r"(aux)); \
	aux>>=SHIFT; \
	gteMAC1=A1_32(aux); \
	aux=MUL(MX21(MX),vx); \
	asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(MX22(MX),vy)) , "r"(aux)); \
	asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(MX23(MX),vz)) , "r"(aux)); \
	aux>>=SHIFT; \
	gteMAC2=A2_32(aux); \
	aux=MUL(MX31(MX),vx); \
	asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(MX32(MX),vy)) , "r"(aux)); \
	asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(MX33(MX),vz)) , "r"(aux)); \
	aux>>=SHIFT; \
	gteMAC3=A3_32(aux); \
}
#endif
#define macMVMVA_cv3_mx3(SHIFT) \
{ \
	gteMAC1 = A1_32(0); \
	gteMAC2 = A2_32(0); \
	gteMAC3 = A3_32(0); \
}
void _gteMVMVA_cv0_mx0_s12(psxRegisters *regs, u32 vxy, s32 vz) { macMVMVA_ncv3_nmx3(0,0,12); }
void _gteMVMVA_cv0_mx0_s0(psxRegisters *regs, u32 vxy, s32 vz) { macMVMVA_ncv3_nmx3(0,0,0); }
void _gteMVMVA_cv0_mx1_s12(psxRegisters *regs, u32 vxy, s32 vz) { macMVMVA_ncv3_nmx3(0,1,12); }
void _gteMVMVA_cv0_mx1_s0(psxRegisters *regs, u32 vxy, s32 vz) { macMVMVA_ncv3_nmx3(0,1,0); }
void _gteMVMVA_cv0_mx2_s12(psxRegisters *regs, u32 vxy, s32 vz) { macMVMVA_ncv3_nmx3(0,2,12); }
void _gteMVMVA_cv0_mx2_s0(psxRegisters *regs, u32 vxy, s32 vz) { macMVMVA_ncv3_nmx3(0,2,0); }
void _gteMVMVA_cv0_mx3_s12(psxRegisters *regs) { macMVMVA_ncv3_mx3(0,12); }
void _gteMVMVA_cv0_mx3_s0(psxRegisters *regs) { macMVMVA_ncv3_mx3(0,0); }

void _gteMVMVA_cv1_mx0_s12(psxRegisters *regs, u32 vxy, s32 vz) { macMVMVA_ncv3_nmx3(1,0,12); }
void _gteMVMVA_cv1_mx0_s0(psxRegisters *regs, u32 vxy, s32 vz) { macMVMVA_ncv3_nmx3(1,0,0); }
void _gteMVMVA_cv1_mx1_s12(psxRegisters *regs, u32 vxy, s32 vz) { macMVMVA_ncv3_nmx3(1,1,12); }
void _gteMVMVA_cv1_mx1_s0(psxRegisters *regs, u32 vxy, s32 vz) { macMVMVA_ncv3_nmx3(1,1,0); }
void _gteMVMVA_cv1_mx2_s12(psxRegisters *regs, u32 vxy, s32 vz) { macMVMVA_ncv3_nmx3(1,2,12); }
void _gteMVMVA_cv1_mx2_s0(psxRegisters *regs, u32 vxy, s32 vz) { macMVMVA_ncv3_nmx3(1,2,0); }
void _gteMVMVA_cv1_mx3_s12(psxRegisters *regs) { macMVMVA_ncv3_mx3(1,12); }
void _gteMVMVA_cv1_mx3_s0(psxRegisters *regs) { macMVMVA_ncv3_mx3(1,0); }

void _gteMVMVA_cv2_mx0_s12(psxRegisters *regs, u32 vxy, s32 vz) { macMVMVA_ncv3_nmx3(2,0,12); }
void _gteMVMVA_cv2_mx0_s0(psxRegisters *regs, u32 vxy, s32 vz) { macMVMVA_ncv3_nmx3(2,0,0); }
void _gteMVMVA_cv2_mx1_s12(psxRegisters *regs, u32 vxy, s32 vz) { macMVMVA_ncv3_nmx3(2,1,12); }
void _gteMVMVA_cv2_mx1_s0(psxRegisters *regs, u32 vxy, s32 vz) { macMVMVA_ncv3_nmx3(2,1,0); }
void _gteMVMVA_cv2_mx2_s12(psxRegisters *regs, u32 vxy, s32 vz) { macMVMVA_ncv3_nmx3(2,2,12); }
void _gteMVMVA_cv2_mx2_s0(psxRegisters *regs, u32 vxy, s32 vz) { macMVMVA_ncv3_nmx3(2,2,0); }
void _gteMVMVA_cv2_mx3_s12(psxRegisters *regs) { macMVMVA_ncv3_mx3(2,12); }
void _gteMVMVA_cv2_mx3_s0(psxRegisters *regs) { macMVMVA_ncv3_mx3(2,0); }

void _gteMVMVA_cv3_mx0_s12(psxRegisters *regs, u32 vxy, s32 vz) { macMVMVA_cv3_nmx3(0,12); }
void _gteMVMVA_cv3_mx0_s0(psxRegisters *regs, u32 vxy, s32 vz) { macMVMVA_cv3_nmx3(0,0); }
void _gteMVMVA_cv3_mx1_s12(psxRegisters *regs, u32 vxy, s32 vz) { macMVMVA_cv3_nmx3(1,12); }
void _gteMVMVA_cv3_mx1_s0(psxRegisters *regs, u32 vxy, s32 vz) { macMVMVA_cv3_nmx3(1,0); }
void _gteMVMVA_cv3_mx2_s12(psxRegisters *regs, u32 vxy, s32 vz) { macMVMVA_cv3_nmx3(2,12); }
void _gteMVMVA_cv3_mx2_s0(psxRegisters *regs, u32 vxy, s32 vz) { macMVMVA_cv3_nmx3(2,0); }
void _gteMVMVA_cv3_mx3_s12(psxRegisters *regs) { macMVMVA_cv3_mx3(12); }
void _gteMVMVA_cv3_mx3_s0(psxRegisters *regs) { macMVMVA_cv3_mx3(0); }

void _gteMVMVA(psxRegisters *regs) {
	pcsx4all_prof_start_with_pause(PCSX4ALL_PROF_GTE,PCSX4ALL_PROF_CPU);
	int cv = GTE_CV(gteop);
	int mx = GTE_MX(gteop);
	int shift = (GTE_SF(gteop)?12:0);

#ifdef GTE_LOG
	GTE_LOG("GTE MVMVA\n");
#endif
	gteFLAG = 0;

	if (cv!=3)
	{
		if (mx!=3)
		{
			int v = GTE_V(gteop);
			s32 vx = VX(v);
			s32 vy = VY(v);
			s32 vz = VZ(v);
			
			#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
				gteMAC1 = A1((((s64)CV1(cv) << 12) + (MX11(mx) * vx) + (MX12(mx) * vy) + (MX13(mx) * vz)) >> shift);
				gteMAC2 = A2((((s64)CV2(cv) << 12) + (MX21(mx) * vx) + (MX22(mx) * vy) + (MX23(mx) * vz)) >> shift);
				gteMAC3 = A3((((s64)CV3(cv) << 12) + (MX31(mx) * vx) + (MX32(mx) * vy) + (MX33(mx) * vz)) >> shift);
			#else
				int aux=MUL(MX11(mx),vx);
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(MX12(mx),vy)) , "r"(aux));
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(MX13(mx),vz)) , "r"(aux));
				aux>>=shift;
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"((s32)CV1(cv) << (12-shift)) , "r"(aux));
				gteMAC1=A1_32(aux);
				
				aux=MUL(MX21(mx),vx);
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(MX22(mx),vy)) , "r"(aux));
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(MX23(mx),vz)) , "r"(aux));
				aux>>=shift;
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"((s32)CV2(cv) << (12-shift)) , "r"(aux));
				gteMAC2=A2_32(aux);

				aux=MUL(MX31(mx),vx);
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(MX32(mx),vy)) , "r"(aux));
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(MX33(mx),vz)) , "r"(aux));
				aux>>=shift;
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"((s32)CV3(cv) << (12-shift)) , "r"(aux));
				gteMAC3=A3_32(aux);
			#endif
		}
		else
		{
			#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
				gteMAC1 = A1((s64)CV1(cv) << (12-shift));
				gteMAC2 = A2((s64)CV2(cv) << (12-shift));
				gteMAC3 = A3((s64)CV3(cv) << (12-shift));
			#else
				gteMAC1 = A1_32((s32)CV1(cv) << (12-shift));
				gteMAC2 = A2_32((s32)CV2(cv) << (12-shift));
				gteMAC3 = A3_32((s32)CV3(cv) << (12-shift));
			#endif
		}
	}
	else
	{
		if (mx!=3)
		{
			int v = GTE_V(gteop);
			s32 vx = VX(v);
			s32 vy = VY(v);
			s32 vz = VZ(v);
			#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
				gteMAC1 = A1((((s64)MX11(mx) * vx) + (MX12(mx) * vy) + (MX13(mx) * vz)) >> shift);
				gteMAC2 = A2((((s64)MX21(mx) * vx) + (MX22(mx) * vy) + (MX23(mx) * vz)) >> shift);
				gteMAC3 = A3((((s64)MX31(mx) * vx) + (MX32(mx) * vy) + (MX33(mx) * vz)) >> shift);
			#else
				int aux=MUL(MX11(mx),vx);
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(MX12(mx),vy)) , "r"(aux));
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(MX13(mx),vz)) , "r"(aux));
				aux>>=shift;
				gteMAC1=A1_32(aux);
				
				aux=MUL(MX21(mx),vx);
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(MX22(mx),vy)) , "r"(aux));
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(MX23(mx),vz)) , "r"(aux));
				aux>>=shift;
				gteMAC2=A2_32(aux);

				aux=MUL(MX31(mx),vx);
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(MX32(mx),vy)) , "r"(aux));
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(MX33(mx),vz)) , "r"(aux));
				aux>>=shift;
				gteMAC3=A3_32(aux);
			
			#endif
		}
		else
		{
			#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
				gteMAC1 = A1(0);
				gteMAC2 = A2(0);
				gteMAC3 = A3(0);
			#else
				gteMAC1 = A1_32(0);
				gteMAC2 = A2_32(0);
				gteMAC3 = A3_32(0);
			#endif
		}
	}
	
	{
		int lm = GTE_LM(gteop);
		gteIR1 = limB1_FLAG(gteMAC1, lm);
		gteIR2 = limB2_FLAG(gteMAC2, lm);
		gteIR3 = limB3_FLAG(gteMAC3, lm);
	}
	pcsx4all_prof_end_with_resume(PCSX4ALL_PROF_GTE,PCSX4ALL_PROF_CPU);
}

void gteMVMVA(void) {
	_gteMVMVA(&psxRegs);
}

void _gteNCLIP( psxRegisters *regs) {
	pcsx4all_prof_start_with_pause(PCSX4ALL_PROF_GTE,PCSX4ALL_PROF_CPU);
#ifdef GTE_LOG
	GTE_LOG("GTE NCLIP\n");
#endif
#ifdef USE_GTE_FLAG
	gteFLAG = 0;
#endif

	#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
		gteMAC0 = F((s64)gteSX0 * (gteSY1 - gteSY2) + gteSX1 * (gteSY2 - gteSY0) + gteSX2 * (gteSY0 - gteSY1));
	#else
	s16 sX0, sX1, sX2, sY1sY2, sY2sY0, sY0sY1;
	{
		u32 sxy0=regs->CP2D.r[12];
		u32 sxy1=regs->CP2D.r[13];
		sX0=(sxy0&0xFFFF);
		sX1=(sxy1&0xFFFF);
		sY0sY1=((s16)(sxy0>>16))-((s16)(sxy1>>16));
		u32 sxy2=regs->CP2D.r[14];
		sX2=(sxy2&0xFFFF);
		sY1sY2=((s16)(sxy1>>16))-((s16)(sxy2>>16));
		sY2sY0=((s16)(sxy2>>16))-((s16)(sxy0>>16));
	}
		int aux=MUL(sX0,(sY1sY2));
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(sX1,(sY2sY0))) , "r"(aux));
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(sX2,(sY0sY1))) , "r"(aux));
		gteMAC0 = F_32(aux);
	#endif
	pcsx4all_prof_end_with_resume(PCSX4ALL_PROF_GTE,PCSX4ALL_PROF_CPU);
}

void gteNCLIP(void) {
	_gteNCLIP(&psxRegs);
}

void _gteAVSZ3(psxRegisters *regs) {
	pcsx4all_prof_start_with_pause(PCSX4ALL_PROF_GTE,PCSX4ALL_PROF_CPU);
#ifdef GTE_LOG
	GTE_LOG("GTE AVSZ3\n");
#endif
#ifdef USE_GTE_FLAG
	gteFLAG = 0;
#endif

	#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
		gteMAC0 = F((s64)(gteZSF3 * gteSZ1) + (gteZSF3 * gteSZ2) + (gteZSF3 * gteSZ3));
	#else
		int aux=MUL(gteZSF3,gteSZ1);
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteZSF3,gteSZ2)) , "r"(aux));
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteZSF3,gteSZ3)) , "r"(aux));
		gteMAC0 = F_32(aux);
	#endif
	
	gteOTZ = limD(gteMAC0 >> 12);
	pcsx4all_prof_end_with_resume(PCSX4ALL_PROF_GTE,PCSX4ALL_PROF_CPU);
}

void gteAVSZ3(void) {
	_gteAVSZ3(&psxRegs);
}

void _gteAVSZ4(psxRegisters *regs) {
	pcsx4all_prof_start_with_pause(PCSX4ALL_PROF_GTE,PCSX4ALL_PROF_CPU);
#ifdef GTE_LOG
	GTE_LOG("GTE AVSZ4\n");
#endif
#ifdef USE_GTE_FLAG
	gteFLAG = 0;
#endif

	#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
		gteMAC0 = F((s64)(gteZSF4 * (gteSZ0 + gteSZ1 + gteSZ2 + gteSZ3)));
	#else
		gteMAC0 = F_32((s32)(MUL(gteZSF4,(gteSZ0 + gteSZ1 + gteSZ2 + gteSZ3))));
	#endif
	gteOTZ = limD(gteMAC0 >> 12);
	pcsx4all_prof_end_with_resume(PCSX4ALL_PROF_GTE,PCSX4ALL_PROF_CPU);
}

void gteAVSZ4(void) {
	_gteAVSZ4(&psxRegs);
}

#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
#define macSQR(SHIFT) \
{ \
	gteMAC1 = A1((gteIR1 * gteIR1) >> SHIFT); \
	gteMAC2 = A2((gteIR2 * gteIR2) >> SHIFT); \
	gteMAC3 = A3((gteIR3 * gteIR3) >> SHIFT); \
}
#else
#define macSQR(SHIFT) \
{ \
	gteMAC1 = A1_32((MUL(gteIR1,gteIR1)) >> SHIFT); \
	gteMAC2 = A2_32((MUL(gteIR2,gteIR2)) >> SHIFT); \
	gteMAC3 = A3_32((MUL(gteIR3,gteIR3)) >> SHIFT); \
}
#endif
void _gteSQR_s12(psxRegisters *regs){ macSQR(12); }
void _gteSQR_s0(psxRegisters *regs){ macSQR(0); }

void _gteSQR(psxRegisters *regs) {
	pcsx4all_prof_start_with_pause(PCSX4ALL_PROF_GTE,PCSX4ALL_PROF_CPU);
	int shift = (GTE_SF(gteop)?12:0);
	int lm = GTE_LM(gteop);

#ifdef GTE_LOG
	GTE_LOG("GTE SQR\n");
#endif
#ifdef USE_GTE_FLAG
	gteFLAG = 0;
#endif

	#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
		gteMAC1 = A1((gteIR1 * gteIR1) >> shift);
		gteMAC2 = A2((gteIR2 * gteIR2) >> shift);
		gteMAC3 = A3((gteIR3 * gteIR3) >> shift);
	#else
		gteMAC1 = A1_32((MUL(gteIR1,gteIR1)) >> shift);
		gteMAC2 = A2_32((MUL(gteIR2,gteIR2)) >> shift);
		gteMAC3 = A3_32((MUL(gteIR3,gteIR3)) >> shift);
	#endif
#ifdef USE_OLD_GTE_WITHOUT_PATCH
	gteIR1 = limB1(gteMAC1 >> shift, lm);
	gteIR2 = limB2(gteMAC2 >> shift, lm);
	gteIR3 = limB3(gteMAC3 >> shift, lm);
#else
	gteIR1 = limB1(gteMAC1, lm);
	gteIR2 = limB2(gteMAC2, lm);
	gteIR3 = limB3(gteMAC3, lm);
#endif
	pcsx4all_prof_end_with_resume(PCSX4ALL_PROF_GTE,PCSX4ALL_PROF_CPU);
}

void gteSQR(void) {
	_gteSQR(&psxRegs);
}

void _gteNCCS_(psxRegisters *regs) {
	{
		s32 mac1, mac2, mac3;
	#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
		mac1 = A1((((s64)gteL11 * gteVX0) + (gteL12 * gteVY0) + (gteL13 * gteVZ0)) >> 12);
		mac2 = A2((((s64)gteL21 * gteVX0) + (gteL22 * gteVY0) + (gteL23 * gteVZ0)) >> 12);
		mac3 = A3((((s64)gteL31 * gteVX0) + (gteL32 * gteVY0) + (gteL33 * gteVZ0)) >> 12);
	#else
		{
			int aux=MUL((s32)gteL11,gteVX0);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL12,gteVY0)) , "r"(aux));
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL13,gteVZ0)) , "r"(aux));
			aux>>=12;
			mac1=A1_32(aux);
		}
		{
			int aux=MUL((s32)gteL21,gteVX0);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL22,gteVY0)) , "r"(aux));
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL23,gteVZ0)) , "r"(aux));
			aux>>=12;
			mac2=A2_32(aux);
		}
		{
			int aux=MUL((s32)gteL31,gteVX0);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL32,gteVY0)) , "r"(aux));
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL33,gteVZ0)) , "r"(aux));
			aux>>=12;
			mac3=A3_32(aux);
		}
	#endif
	{
		s16 _ir1=limB11(mac1);
		s16 _ir2=limB21(mac2);
		s16 _ir3=limB31(mac3);
#ifdef USE_GTE_FLAG
		gteIR1 = _ir1;
		gteIR2 = _ir2;
		gteIR3 = _ir3;
#endif

	#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
		mac1 = A1((((s64)gteRBK << 12) + (gteLR1 * _ir1) + (gteLR2 * _ir2) + (gteLR3 * _ir3)) >> 12);
		mac2 = A2((((s64)gteGBK << 12) + (gteLG1 * _ir1) + (gteLG2 * _ir2) + (gteLG3 * _ir3)) >> 12);
		mac3 = A3((((s64)gteBBK << 12) + (gteLB1 * _ir1) + (gteLB2 * _ir2) + (gteLB3 * _ir3)) >> 12);
	#else
		{
			int aux=MUL(gteLR1,_ir1);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLR2,_ir2)) , "r"(aux));
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLR3,_ir3)) , "r"(aux));
			aux>>=12;
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteRBK) , "r"(aux));
			mac1=A1_32(aux);
		}
		{
			int aux=MUL(gteLG1,_ir1);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLG2,_ir2)) , "r"(aux));
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLG3,_ir3)) , "r"(aux));
			aux>>=12;
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteGBK) , "r"(aux));
			mac2=A2_32(aux);
		}
		{
			int aux=MUL(gteLB1,_ir1);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLB2,_ir2)) , "r"(aux));
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLB3,_ir3)) , "r"(aux));
			aux>>=12;
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteBBK) , "r"(aux));
			mac3=A3_32(aux);
		}
	#endif
	}
	{
		s16 _ir1=limB11(mac1);
		s16 _ir2=limB21(mac2);
		s16 _ir3=limB31(mac3);
#ifdef USE_GTE_FLAG
		gteIR1 = _ir1;
		gteIR2 = _ir2;
		gteIR3 = _ir3;
#endif

	#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
		mac1 = A1(((s64)gteR * _ir1) >> 8);
		mac2 = A2(((s64)gteG * _ir2) >> 8);
		mac3 = A3(((s64)gteB * _ir3) >> 8);
	#else
		mac1 = A1_32((MUL((s32)gteR,_ir1)) >> 8);
		mac2 = A2_32((MUL((s32)gteG,_ir2)) >> 8);
		mac3 = A3_32((MUL((s32)gteB,_ir3)) >> 8);
	#endif
	}
		gteMAC1 = mac1;
		gteMAC2 = mac2;
		gteMAC3 = mac3;
	}
}

void _gteNCCS(psxRegisters *regs) {
	pcsx4all_prof_start_with_pause(PCSX4ALL_PROF_GTE,PCSX4ALL_PROF_CPU);
#ifdef GTE_LOG
	GTE_LOG("GTE NCCS\n");
#endif
#ifdef USE_GTE_FLAG
	gteFLAG = 0;
#endif

	#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
		gteMAC1 = A1((((s64)gteL11 * gteVX0) + (gteL12 * gteVY0) + (gteL13 * gteVZ0)) >> 12);
		gteMAC2 = A2((((s64)gteL21 * gteVX0) + (gteL22 * gteVY0) + (gteL23 * gteVZ0)) >> 12);
		gteMAC3 = A3((((s64)gteL31 * gteVX0) + (gteL32 * gteVY0) + (gteL33 * gteVZ0)) >> 12);
	#else
		int aux=MUL((s32)gteL11,gteVX0);
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL12,gteVY0)) , "r"(aux));
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL13,gteVZ0)) , "r"(aux));
		aux>>=12;
		gteMAC1=A1_32(aux);

		aux=MUL((s32)gteL21,gteVX0);
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL22,gteVY0)) , "r"(aux));
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL23,gteVZ0)) , "r"(aux));
		aux>>=12;
		gteMAC2=A2_32(aux);

		aux=MUL((s32)gteL31,gteVX0);
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL32,gteVY0)) , "r"(aux));
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL33,gteVZ0)) , "r"(aux));
		aux>>=12;
		gteMAC3=A3_32(aux);
	#endif
	{
		s16 _ir1=limB11(gteMAC1);
		s16 _ir2=limB21(gteMAC2);
		s16 _ir3=limB31(gteMAC3);
#ifdef USE_GTE_FLAG
		gteIR1 = _ir1;
		gteIR2 = _ir2;
		gteIR3 = _ir3;
#endif

	#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
		gteMAC1 = A1((((s64)gteRBK << 12) + (gteLR1 * _ir1) + (gteLR2 * _ir2) + (gteLR3 * _ir3)) >> 12);
		gteMAC2 = A2((((s64)gteGBK << 12) + (gteLG1 * _ir1) + (gteLG2 * _ir2) + (gteLG3 * _ir3)) >> 12);
		gteMAC3 = A3((((s64)gteBBK << 12) + (gteLB1 * _ir1) + (gteLB2 * _ir2) + (gteLB3 * _ir3)) >> 12);
	#else
		aux=MUL(gteLR1,_ir1);
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLR2,_ir2)) , "r"(aux));
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLR3,_ir3)) , "r"(aux));
		aux>>=12;
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteRBK) , "r"(aux));
		gteMAC1=A1_32(aux);
		
		aux=MUL(gteLG1,_ir1);
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLG2,_ir2)) , "r"(aux));
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLG3,_ir3)) , "r"(aux));
		aux>>=12;
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteGBK) , "r"(aux));
		gteMAC2=A2_32(aux);

		aux=MUL(gteLB1,_ir1);
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLB2,_ir2)) , "r"(aux));
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLB3,_ir3)) , "r"(aux));
		aux>>=12;
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteBBK) , "r"(aux));
		gteMAC3=A3_32(aux);
	#endif
	}
	{
		s16 _ir1=limB11(gteMAC1);
		s16 _ir2=limB21(gteMAC2);
		s16 _ir3=limB31(gteMAC3);
#ifdef USE_GTE_FLAG
		gteIR1 = _ir1;
		gteIR2 = _ir2;
		gteIR3 = _ir3;
#endif

	#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
		gteMAC1 = A1(((s64)gteR * _ir1) >> 8);
		gteMAC2 = A2(((s64)gteG * _ir2) >> 8);
		gteMAC3 = A3(((s64)gteB * _ir3) >> 8);
	#else
		gteMAC1 = A1_32((MUL((s32)gteR,_ir1)) >> 8);
		gteMAC2 = A2_32((MUL((s32)gteG,_ir2)) >> 8);
		gteMAC3 = A3_32((MUL((s32)gteB,_ir3)) >> 8);
	#endif
	}
	
	gteIR1 = limB11(gteMAC1);
	gteIR2 = limB21(gteMAC2);
	gteIR3 = limB31(gteMAC3);

	gteRGB0 = gteRGB1;
	gteRGB1 = gteRGB2;
	gteCODE2 = gteCODE;
	gteR2 = limC1(gteMAC1 >> 4);
	gteG2 = limC2(gteMAC2 >> 4);
	gteB2 = limC3(gteMAC3 >> 4);
	pcsx4all_prof_end_with_resume(PCSX4ALL_PROF_GTE,PCSX4ALL_PROF_CPU);
}

void gteNCCS(void) {
	_gteNCCS(&psxRegs);
}

void _gteNCCT_(psxRegisters *regs) {
	{
		s32 vx = VX_m3(0);
		s32 vy = VY_m3(0);
		s32 vz = VZ_m3(0);
		s32 mac1, mac2, mac3;
		
		#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
			mac1= A1((((s64)gteL11 * vx) + (gteL12 * vy) + (gteL13 * vz)) >> 12);
			mac2 = A2((((s64)gteL21 * vx) + (gteL22 * vy) + (gteL23 * vz)) >> 12);
			mac3 = A3((((s64)gteL31 * vx) + (gteL32 * vy) + (gteL33 * vz)) >> 12);
		#else
			{
				int aux=MUL((s32)gteL11,vx);
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL12,vy)) , "r"(aux));
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL13,vz)) , "r"(aux));
				aux>>=12;
				mac1=A1_32(aux);
			}
			{
				int aux=MUL((s32)gteL21,vx);
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL22,vy)) , "r"(aux));
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL23,vz)) , "r"(aux));
				aux>>=12;
				mac2=A2_32(aux);
			}
			{
				int aux=MUL((s32)gteL31,vx);
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL32,vy)) , "r"(aux));
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL33,vz)) , "r"(aux));
				aux>>=12;
				mac3=A3_32(aux);
			}
		#endif
		{
			s16 _ir1=limB11(mac1);
			s16 _ir2=limB21(mac2);
			s16 _ir3=limB31(mac3);
#ifdef USE_GTE_FLAG
			gteIR1 = _ir1;
			gteIR2 = _ir2;
			gteIR3 = _ir3;
#endif
		
		#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
			mac1 = A1((((s64)gteRBK << 12) + (gteLR1 * _ir1) + (gteLR2 * _ir2) + (gteLR3 * _ir3)) >> 12);
			mac2 = A2((((s64)gteGBK << 12) + (gteLG1 * _ir1) + (gteLG2 * _ir2) + (gteLG3 * _ir3)) >> 12);
			mac3 = A3((((s64)gteBBK << 12) + (gteLB1 * _ir1) + (gteLB2 * _ir2) + (gteLB3 * _ir3)) >> 12);
		#else
			{
				int aux=MUL(gteLR1,_ir1);
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLR2,_ir2)) , "r"(aux));
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLR3,_ir3)) , "r"(aux));
				aux>>=12;
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteRBK) , "r"(aux));
				mac1=A1_32(aux);
			}
			{
				int aux=MUL(gteLG1,_ir1);
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLG2,_ir2)) , "r"(aux));
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLG3,_ir3)) , "r"(aux));
				aux>>=12;
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteGBK) , "r"(aux));
				mac2=A2_32(aux);
			}
			{
				int aux=MUL(gteLB1,_ir1);
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLB2,_ir2)) , "r"(aux));
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLB3,_ir3)) , "r"(aux));
				aux>>=12;
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteBBK) , "r"(aux));
				mac3=A3_32(aux);
			}
		#endif
		}
		{
			s16 _ir1=limB11(mac1);
			s16 _ir2=limB21(mac2);
			s16 _ir3=limB31(mac3);
#ifdef USE_GTE_FLAG
			gteIR1 = _ir1;
			gteIR2 = _ir2;
			gteIR3 = _ir3;
#endif
		
		#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
			mac1 = A1(((s64)gteR * _ir1) >> 8);
			mac2 = A2(((s64)gteG * _ir2) >> 8);
			mac3 = A3(((s64)gteB * _ir3) >> 8);
		#else
			mac1 = A1_32((MUL((s32)gteR,_ir1)) >> 8);
			mac2 = A2_32((MUL((s32)gteG,_ir2)) >> 8);
			mac3 = A3_32((MUL((s32)gteB,_ir3)) >> 8);
		#endif
		}

		gteRGB0 = gteRGB1;
		gteRGB1 = gteRGB2;
		gteCODE2 = gteCODE;
		gteR2 = limC1(mac1 >> 4);
		gteG2 = limC2(mac2 >> 4);
		gteB2 = limC3(mac3 >> 4);
	}
	{
		s32 vx = VX_m3(1);
		s32 vy = VY_m3(1);
		s32 vz = VZ_m3(1);
		s32 mac1, mac2, mac3;
		
		#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
			mac1 = A1((((s64)gteL11 * vx) + (gteL12 * vy) + (gteL13 * vz)) >> 12);
			mac2 = A2((((s64)gteL21 * vx) + (gteL22 * vy) + (gteL23 * vz)) >> 12);
			mac3 = A3((((s64)gteL31 * vx) + (gteL32 * vy) + (gteL33 * vz)) >> 12);
		#else
			{
				int aux=MUL((s32)gteL11,vx);
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL12,vy)) , "r"(aux));
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL13,vz)) , "r"(aux));
				aux>>=12;
				mac1=A1_32(aux);
			}
			{
				int aux=MUL((s32)gteL21,vx);
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL22,vy)) , "r"(aux));
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL23,vz)) , "r"(aux));
				aux>>=12;
				mac2=A2_32(aux);
			}
			{
				int aux=MUL((s32)gteL31,vx);
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL32,vy)) , "r"(aux));
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL33,vz)) , "r"(aux));
				aux>>=12;
				mac3=A3_32(aux);
			}
		#endif
		{
			s16 _ir1=limB11(mac1);
			s16 _ir2=limB21(mac2);
			s16 _ir3=limB31(mac3);
#ifdef USE_GTE_FLAG
			gteIR1 = _ir1;
			gteIR2 = _ir2;
			gteIR3 = _ir3;
#endif
		
		#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
			mac1 = A1((((s64)gteRBK << 12) + (gteLR1 * _ir1) + (gteLR2 * _ir2) + (gteLR3 * _ir3)) >> 12);
			mac2 = A2((((s64)gteGBK << 12) + (gteLG1 * _ir1) + (gteLG2 * _ir2) + (gteLG3 * _ir3)) >> 12);
			mac3 = A3((((s64)gteBBK << 12) + (gteLB1 * _ir1) + (gteLB2 * _ir2) + (gteLB3 * _ir3)) >> 12);
		#else
			{
				int aux=MUL(gteLR1,_ir1);
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLR2,_ir2)) , "r"(aux));
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLR3,_ir3)) , "r"(aux));
				aux>>=12;
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteRBK) , "r"(aux));
				mac1=A1_32(aux);
			}
			{
				int aux=MUL(gteLG1,_ir1);
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLG2,_ir2)) , "r"(aux));
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLG3,_ir3)) , "r"(aux));
				aux>>=12;
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteGBK) , "r"(aux));
				mac2=A2_32(aux);
			}
			{
				int aux=MUL(gteLB1,_ir1);
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLB2,_ir2)) , "r"(aux));
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLB3,_ir3)) , "r"(aux));
				aux>>=12;
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteBBK) , "r"(aux));
				mac3=A3_32(aux);
			}
		#endif
		}
		{
			s16 _ir1=limB11(mac1);
			s16 _ir2=limB21(mac2);
			s16 _ir3=limB31(mac3);
#ifdef USE_GTE_FLAG
			gteIR1 = _ir1;
			gteIR2 = _ir2;
			gteIR3 = _ir3;
#endif
		
		#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
			mac1 = A1(((s64)gteR * _ir1) >> 8);
			mac2 = A2(((s64)gteG * _ir2) >> 8);
			mac3 = A3(((s64)gteB * _ir3) >> 8);
		#else
			mac1 = A1_32((MUL((s32)gteR,_ir1)) >> 8);
			mac2 = A2_32((MUL((s32)gteG,_ir2)) >> 8);
			mac3 = A3_32((MUL((s32)gteB,_ir3)) >> 8);
		#endif
		}

		gteRGB0 = gteRGB1;
		gteRGB1 = gteRGB2;
		gteCODE2 = gteCODE;
		gteR2 = limC1(mac1 >> 4);
		gteG2 = limC2(mac2 >> 4);
		gteB2 = limC3(mac3 >> 4);
	}
	{
		s32 vx = VX_m3(2);
		s32 vy = VY_m3(2);
		s32 vz = VZ_m3(2);
		s32 mac1, mac2, mac3;
		
		#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
			mac1 = A1((((s64)gteL11 * vx) + (gteL12 * vy) + (gteL13 * vz)) >> 12);
			mac2 = A2((((s64)gteL21 * vx) + (gteL22 * vy) + (gteL23 * vz)) >> 12);
			mac3 = A3((((s64)gteL31 * vx) + (gteL32 * vy) + (gteL33 * vz)) >> 12);
		#else
			{
				int aux=MUL((s32)gteL11,vx);
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL12,vy)) , "r"(aux));
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL13,vz)) , "r"(aux));
				aux>>=12;
				mac1=A1_32(aux);
			}
			{
				int aux=MUL((s32)gteL21,vx);
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL22,vy)) , "r"(aux));
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL23,vz)) , "r"(aux));
				aux>>=12;
				mac2=A2_32(aux);
			}
			{
				int aux=MUL((s32)gteL31,vx);
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL32,vy)) , "r"(aux));
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL33,vz)) , "r"(aux));
				aux>>=12;
				mac3=A3_32(aux);
			}
		#endif
		{
			s16 _ir1=limB11(mac1);
			s16 _ir2=limB21(mac2);
			s16 _ir3=limB31(mac3);
#ifdef USE_GTE_FLAG
			gteIR1 = _ir1;
			gteIR2 = _ir2;
			gteIR3 = _ir3;
#endif
		
		#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
			mac1 = A1((((s64)gteRBK << 12) + (gteLR1 * _ir1) + (gteLR2 * _ir2) + (gteLR3 * _ir3)) >> 12);
			mac2 = A2((((s64)gteGBK << 12) + (gteLG1 * _ir1) + (gteLG2 * _ir2) + (gteLG3 * _ir3)) >> 12);
			mac3 = A3((((s64)gteBBK << 12) + (gteLB1 * _ir1) + (gteLB2 * _ir2) + (gteLB3 * _ir3)) >> 12);
		#else
			{
				int aux=MUL(gteLR1,_ir1);
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLR2,_ir2)) , "r"(aux));
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLR3,_ir3)) , "r"(aux));
				aux>>=12;
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteRBK) , "r"(aux));
				mac1=A1_32(aux);
			}
			{
				int aux=MUL(gteLG1,_ir1);
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLG2,_ir2)) , "r"(aux));
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLG3,_ir3)) , "r"(aux));
				aux>>=12;
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteGBK) , "r"(aux));
				mac2=A2_32(aux);
			}
			{
				int aux=MUL(gteLB1,_ir1);
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLB2,_ir2)) , "r"(aux));
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLB3,_ir3)) , "r"(aux));
				aux>>=12;
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteBBK) , "r"(aux));
				mac3=A3_32(aux);
			}
		#endif
		}
		{
			s16 _ir1=limB11(mac1);
			s16 _ir2=limB21(mac2);
			s16 _ir3=limB31(mac3);
#ifdef USE_GTE_FLAG
			gteIR1 = _ir1;
			gteIR2 = _ir2;
			gteIR3 = _ir3;
#endif
		
		#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
			mac1 = A1(((s64)gteR * _ir1) >> 8);
			mac2 = A2(((s64)gteG * _ir2) >> 8);
			mac3 = A3(((s64)gteB * _ir3) >> 8);
		#else
			mac1 = A1_32((MUL((s32)gteR,_ir1)) >> 8);
			mac2 = A2_32((MUL((s32)gteG,_ir2)) >> 8);
			mac3 = A3_32((MUL((s32)gteB,_ir3)) >> 8);
		#endif
		}
		gteMAC1 = mac1;
		gteMAC2 = mac2;
		gteMAC3 = mac3;
	}
}

void _gteNCCT(psxRegisters *regs) {
	pcsx4all_prof_start_with_pause(PCSX4ALL_PROF_GTE,PCSX4ALL_PROF_CPU);
	int v;
	s32 vx, vy, vz;

#ifdef GTE_LOG
	GTE_LOG("GTE NCCT\n");
#endif
#ifdef USE_GTE_FLAG
	gteFLAG = 0;
#endif

	for (v = 0; v < 3; v++) {
		vx = VX_m3(v);
		vy = VY_m3(v);
		vz = VZ_m3(v);
		
		#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
			gteMAC1 = A1((((s64)gteL11 * vx) + (gteL12 * vy) + (gteL13 * vz)) >> 12);
			gteMAC2 = A2((((s64)gteL21 * vx) + (gteL22 * vy) + (gteL23 * vz)) >> 12);
			gteMAC3 = A3((((s64)gteL31 * vx) + (gteL32 * vy) + (gteL33 * vz)) >> 12);
		#else
			int aux=MUL((s32)gteL11,vx);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL12,vy)) , "r"(aux));
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL13,vz)) , "r"(aux));
			aux>>=12;
			gteMAC1=A1_32(aux);

			aux=MUL((s32)gteL21,vx);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL22,vy)) , "r"(aux));
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL23,vz)) , "r"(aux));
			aux>>=12;
			gteMAC2=A2_32(aux);

			aux=MUL((s32)gteL31,vx);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL32,vy)) , "r"(aux));
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL33,vz)) , "r"(aux));
			aux>>=12;
			gteMAC3=A3_32(aux);
		#endif
		{
			s16 _ir1=limB11(gteMAC1);
			s16 _ir2=limB21(gteMAC2);
			s16 _ir3=limB31(gteMAC3);
#ifdef USE_GTE_FLAG
			gteIR1 = _ir1;
			gteIR2 = _ir2;
			gteIR3 = _ir3;
#endif
		
		#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
			gteMAC1 = A1((((s64)gteRBK << 12) + (gteLR1 * _ir1) + (gteLR2 * _ir2) + (gteLR3 * _ir3)) >> 12);
			gteMAC2 = A2((((s64)gteGBK << 12) + (gteLG1 * _ir1) + (gteLG2 * _ir2) + (gteLG3 * _ir3)) >> 12);
			gteMAC3 = A3((((s64)gteBBK << 12) + (gteLB1 * _ir1) + (gteLB2 * _ir2) + (gteLB3 * _ir3)) >> 12);
		#else
			aux=MUL(gteLR1,_ir1);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLR2,_ir2)) , "r"(aux));
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLR3,_ir3)) , "r"(aux));
			aux>>=12;
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteRBK) , "r"(aux));
			gteMAC1=A1_32(aux);
			
			aux=MUL(gteLG1,_ir1);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLG2,_ir2)) , "r"(aux));
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLG3,_ir3)) , "r"(aux));
			aux>>=12;
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteGBK) , "r"(aux));
			gteMAC2=A2_32(aux);

			aux=MUL(gteLB1,_ir1);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLB2,_ir2)) , "r"(aux));
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLB3,_ir3)) , "r"(aux));
			aux>>=12;
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteBBK) , "r"(aux));
			gteMAC3=A3_32(aux);
		#endif
		}
		{
			s16 _ir1=limB11(gteMAC1);
			s16 _ir2=limB21(gteMAC2);
			s16 _ir3=limB31(gteMAC3);
#ifdef USE_GTE_FLAG
			gteIR1 = _ir1;
			gteIR2 = _ir2;
			gteIR3 = _ir3;
#endif
		
		#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
			gteMAC1 = A1(((s64)gteR * _ir1) >> 8);
			gteMAC2 = A2(((s64)gteG * _ir2) >> 8);
			gteMAC3 = A3(((s64)gteB * _ir3) >> 8);
		#else
			gteMAC1 = A1_32((MUL((s32)gteR,_ir1)) >> 8);
			gteMAC2 = A2_32((MUL((s32)gteG,_ir2)) >> 8);
			gteMAC3 = A3_32((MUL((s32)gteB,_ir3)) >> 8);
		#endif
		}

		gteRGB0 = gteRGB1;
		gteRGB1 = gteRGB2;
		gteCODE2 = gteCODE;
		gteR2 = limC1(gteMAC1 >> 4);
		gteG2 = limC2(gteMAC2 >> 4);
		gteB2 = limC3(gteMAC3 >> 4);
	}
	gteIR1 = limB11(gteMAC1);
	gteIR2 = limB21(gteMAC2);
	gteIR3 = limB31(gteMAC3);
	pcsx4all_prof_end_with_resume(PCSX4ALL_PROF_GTE,PCSX4ALL_PROF_CPU);
}

void gteNCCT(void) {
	_gteNCCT(&psxRegs);
}

void _gteNCDS_(psxRegisters *regs) {
	{
		s32 mac1, mac2, mac3;
	#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
		mac1 = A1((((s64)gteL11 * gteVX0) + (gteL12 * gteVY0) + (gteL13 * gteVZ0)) >> 12);
		mac2 = A2((((s64)gteL21 * gteVX0) + (gteL22 * gteVY0) + (gteL23 * gteVZ0)) >> 12);
		mac3 = A3((((s64)gteL31 * gteVX0) + (gteL32 * gteVY0) + (gteL33 * gteVZ0)) >> 12);
	#else
		int aux=MUL((s32)gteL11,gteVX0);
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL12,gteVY0)) , "r"(aux));
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL13,gteVZ0)) , "r"(aux));
		aux>>=12;
		mac1=A1_32(aux);

		aux=MUL((s32)gteL21,gteVX0);
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL22,gteVY0)) , "r"(aux));
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL23,gteVZ0)) , "r"(aux));
		aux>>=12;
		mac2=A2_32(aux);

		aux=MUL((s32)gteL31,gteVX0);
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL32,gteVY0)) , "r"(aux));
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL33,gteVZ0)) , "r"(aux));
		aux>>=12;
		mac3=A3_32(aux);
	#endif
	
	{
		s16 _ir1=limB11(mac1);
		s16 _ir2=limB21(mac2);
		s16 _ir3=limB31(mac3);
#ifdef USE_GTE_FLAG
		gteIR1 = _ir1;
		gteIR2 = _ir2;
		gteIR3 = _ir3;
#endif
	
	#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
		mac1 = A1((((s64)gteRBK << 12) + (gteLR1 * _ir1) + (gteLR2 * _ir2) + (gteLR3 * _ir3)) >> 12);
		mac2 = A2((((s64)gteGBK << 12) + (gteLG1 * _ir1) + (gteLG2 * _ir2) + (gteLG3 * _ir3)) >> 12);
		mac3 = A3((((s64)gteBBK << 12) + (gteLB1 * _ir1) + (gteLB2 * _ir2) + (gteLB3 * _ir3)) >> 12);
	#else
		aux=MUL(gteLR1,_ir1);
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLR2,_ir2)) , "r"(aux));
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLR3,_ir3)) , "r"(aux));
		aux>>=12;
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteRBK) , "r"(aux));
		mac1=A1_32(aux);
		
		aux=MUL(gteLG1,_ir1);
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLG2,_ir2)) , "r"(aux));
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLG3,_ir3)) , "r"(aux));
		aux>>=12;
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteGBK) , "r"(aux));
		mac2=A2_32(aux);

		aux=MUL(gteLB1,_ir1);
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLB2,_ir2)) , "r"(aux));
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLB3,_ir3)) , "r"(aux));
		aux>>=12;
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteBBK) , "r"(aux));
		mac3=A3_32(aux);
	#endif
	}

	{
		s16 _ir1=limB11(mac1);
		s16 _ir2=limB21(mac2);
		s16 _ir3=limB31(mac3);
#ifdef USE_GTE_FLAG
		gteIR1 = _ir1;
		gteIR2 = _ir2;
		gteIR3 = _ir3;
#endif
	
	#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
		mac1 = A1(((((s64)gteR << 4) * _ir1) + (gteIR0 * limB10(gteRFC - ((gteR * _ir1) >> 8)))) >> 12);
		mac2 = A2(((((s64)gteG << 4) * _ir2) + (gteIR0 * limB20(gteGFC - ((gteG * _ir2) >> 8)))) >> 12);
		mac3 = A3(((((s64)gteB << 4) * _ir3) + (gteIR0 * limB30(gteBFC - ((gteB * _ir3) >> 8)))) >> 12);
	#else
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(((s32)gteR << 4),_ir1)) , "r"(MUL(gteIR0,limB10(gteRFC - ((MUL(gteR,_ir1)) >> 8)))));
		mac1=A1_32(aux>>12);
	
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(((s32)gteG << 4),_ir2)) , "r"(MUL(gteIR0,limB20(gteGFC - ((MUL(gteG,_ir2)) >> 8)))));
		mac2=A2_32(aux>>12);

		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(((s32)gteB << 4),_ir3)) , "r"(MUL(gteIR0,limB30(gteBFC - ((MUL(gteB,_ir3)) >> 8)))));
		mac3=A3_32(aux>>12);
	#endif
	}
		gteMAC1 = mac1;
		gteMAC2 = mac2;
		gteMAC3 = mac3;
	}
}


void _gteNCDS(psxRegisters *regs) {
	pcsx4all_prof_start_with_pause(PCSX4ALL_PROF_GTE,PCSX4ALL_PROF_CPU);
#ifdef GTE_LOG
	GTE_LOG("GTE NCDS\n");
#endif
#ifdef USE_GTE_FLAG
	gteFLAG = 0;
#endif

	#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
		gteMAC1 = A1((((s64)gteL11 * gteVX0) + (gteL12 * gteVY0) + (gteL13 * gteVZ0)) >> 12);
		gteMAC2 = A2((((s64)gteL21 * gteVX0) + (gteL22 * gteVY0) + (gteL23 * gteVZ0)) >> 12);
		gteMAC3 = A3((((s64)gteL31 * gteVX0) + (gteL32 * gteVY0) + (gteL33 * gteVZ0)) >> 12);
	#else
		int aux=MUL((s32)gteL11,gteVX0);
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL12,gteVY0)) , "r"(aux));
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL13,gteVZ0)) , "r"(aux));
		aux>>=12;
		gteMAC1=A1_32(aux);

		aux=MUL((s32)gteL21,gteVX0);
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL22,gteVY0)) , "r"(aux));
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL23,gteVZ0)) , "r"(aux));
		aux>>=12;
		gteMAC2=A2_32(aux);

		aux=MUL((s32)gteL31,gteVX0);
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL32,gteVY0)) , "r"(aux));
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL33,gteVZ0)) , "r"(aux));
		aux>>=12;
		gteMAC3=A3_32(aux);
	#endif
	
	{
		s16 _ir1=limB11(gteMAC1);
		s16 _ir2=limB21(gteMAC2);
		s16 _ir3=limB31(gteMAC3);
#ifdef USE_GTE_FLAG
		gteIR1 = _ir1;
		gteIR2 = _ir2;
		gteIR3 = _ir3;
#endif
	
	#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
		gteMAC1 = A1((((s64)gteRBK << 12) + (gteLR1 * _ir1) + (gteLR2 * _ir2) + (gteLR3 * _ir3)) >> 12);
		gteMAC2 = A2((((s64)gteGBK << 12) + (gteLG1 * _ir1) + (gteLG2 * _ir2) + (gteLG3 * _ir3)) >> 12);
		gteMAC3 = A3((((s64)gteBBK << 12) + (gteLB1 * _ir1) + (gteLB2 * _ir2) + (gteLB3 * _ir3)) >> 12);
	#else
		aux=MUL(gteLR1,_ir1);
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLR2,_ir2)) , "r"(aux));
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLR3,_ir3)) , "r"(aux));
		aux>>=12;
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteRBK) , "r"(aux));
		gteMAC1=A1_32(aux);
		
		aux=MUL(gteLG1,_ir1);
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLG2,_ir2)) , "r"(aux));
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLG3,_ir3)) , "r"(aux));
		aux>>=12;
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteGBK) , "r"(aux));
		gteMAC2=A2_32(aux);

		aux=MUL(gteLB1,_ir1);
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLB2,_ir2)) , "r"(aux));
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLB3,_ir3)) , "r"(aux));
		aux>>=12;
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteBBK) , "r"(aux));
		gteMAC3=A3_32(aux);
	#endif
	}

	{
		s16 _ir1=limB11(gteMAC1);
		s16 _ir2=limB21(gteMAC2);
		s16 _ir3=limB31(gteMAC3);
#ifdef USE_GTE_FLAG
		gteIR1 = _ir1;
		gteIR2 = _ir2;
		gteIR3 = _ir3;
#endif
	
	#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
		gteMAC1 = A1(((((s64)gteR << 4) * _ir1) + (gteIR0 * limB10(gteRFC - ((gteR * _ir1) >> 8)))) >> 12);
		gteMAC2 = A2(((((s64)gteG << 4) * _ir2) + (gteIR0 * limB20(gteGFC - ((gteG * _ir2) >> 8)))) >> 12);
		gteMAC3 = A3(((((s64)gteB << 4) * _ir3) + (gteIR0 * limB30(gteBFC - ((gteB * _ir3) >> 8)))) >> 12);
	#else
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(((s32)gteR << 4),_ir1)) , "r"(MUL(gteIR0,limB10(gteRFC - ((MUL(gteR,_ir1)) >> 8)))));
		gteMAC1=A1_32(aux>>12);
	
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(((s32)gteG << 4),_ir2)) , "r"(MUL(gteIR0,limB20(gteGFC - ((MUL(gteG,_ir2)) >> 8)))));
		gteMAC2=A2_32(aux>>12);

		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(((s32)gteB << 4),_ir3)) , "r"(MUL(gteIR0,limB30(gteBFC - ((MUL(gteB,_ir3)) >> 8)))));
		gteMAC3=A3_32(aux>>12);
	#endif
	}
	
	gteIR1 = limB11(gteMAC1);
	gteIR2 = limB21(gteMAC2);
	gteIR3 = limB31(gteMAC3);

	gteRGB0 = gteRGB1;
	gteRGB1 = gteRGB2;
	gteCODE2 = gteCODE;
	gteR2 = limC1(gteMAC1 >> 4);
	gteG2 = limC2(gteMAC2 >> 4);
	gteB2 = limC3(gteMAC3 >> 4);
	pcsx4all_prof_end_with_resume(PCSX4ALL_PROF_GTE,PCSX4ALL_PROF_CPU);
}

void gteNCDS(void) {
	_gteNCDS(&psxRegs);
}

void _gteNCDT_(psxRegisters *regs) {
	{
		s32 vx = VX_m3(0);
		s32 vy = VY_m3(0);
		s32 vz = VZ_m3(0);
		s32 mac1, mac2, mac3;
		
	#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
		mac1 = A1((((s64)gteL11 * vx) + (gteL12 * vy) + (gteL13 * vz)) >> 12);
		mac2 = A2((((s64)gteL21 * vx) + (gteL22 * vy) + (gteL23 * vz)) >> 12);
		mac3 = A3((((s64)gteL31 * vx) + (gteL32 * vy) + (gteL33 * vz)) >> 12);
	#else
		{
			int aux=MUL((s32)gteL11,vx);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL12,vy)) , "r"(aux));
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL13,vz)) , "r"(aux));
			aux>>=12;
			mac1=A1_32(aux);
		}
		{
			int aux=MUL((s32)gteL21,vx);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL22,vy)) , "r"(aux));
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL23,vz)) , "r"(aux));
			aux>>=12;
			mac2=A2_32(aux);
		}
		{
			int aux=MUL((s32)gteL31,vx);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL32,vy)) , "r"(aux));
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL33,vz)) , "r"(aux));
			aux>>=12;
			mac3=A3_32(aux);
		}
	#endif
	{
		s16 _ir1=limB11(mac1);
		s16 _ir2=limB21(mac2);
		s16 _ir3=limB31(mac3);
#ifdef USE_GTE_FLAG
		gteIR1 = _ir1;
		gteIR2 = _ir2;
		gteIR3 = _ir3;
#endif
		
	#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
		mac1 = A1((((s64)gteRBK << 12) + (gteLR1 * _ir1) + (gteLR2 * _ir2) + (gteLR3 * _ir3)) >> 12);
		mac2 = A2((((s64)gteGBK << 12) + (gteLG1 * _ir1) + (gteLG2 * _ir2) + (gteLG3 * _ir3)) >> 12);
		mac3 = A3((((s64)gteBBK << 12) + (gteLB1 * _ir1) + (gteLB2 * _ir2) + (gteLB3 * _ir3)) >> 12);
	#else
		{
			int aux=MUL(gteLR1,_ir1);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLR2,_ir2)) , "r"(aux));
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLR3,_ir3)) , "r"(aux));
			aux>>=12;
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteRBK) , "r"(aux));
			mac1=A1_32(aux);
		}
		{
			int aux=MUL(gteLG1,_ir1);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLG2,_ir2)) , "r"(aux));
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLG3,_ir3)) , "r"(aux));
			aux>>=12;
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteGBK) , "r"(aux));
			mac2=A2_32(aux);
		}
		{
			int aux=MUL(gteLB1,_ir1);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLB2,_ir2)) , "r"(aux));
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLB3,_ir3)) , "r"(aux));
			aux>>=12;
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteBBK) , "r"(aux));
			mac3=A3_32(aux);
		}
	#endif
	}
	{
		s16 _ir1=limB11(mac1);
		s16 _ir2=limB21(mac2);
		s16 _ir3=limB31(mac3);
#ifdef USE_GTE_FLAG
		gteIR1 = _ir1;
		gteIR2 = _ir2;
		gteIR3 = _ir3;
#endif

	#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
		mac1 = A1(((((s64)gteR << 4) * _ir1) + (gteIR0 * limB10(gteRFC - ((gteR * _ir1) >> 8)))) >> 12);
		mac2 = A2(((((s64)gteG << 4) * _ir2) + (gteIR0 * limB20(gteGFC - ((gteG * _ir2) >> 8)))) >> 12);
		mac3 = A3(((((s64)gteB << 4) * _ir3) + (gteIR0 * limB30(gteBFC - ((gteB * _ir3) >> 8)))) >> 12);
	#else
		{
			int aux;
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(((s32)gteR << 4),_ir1)) , "r"(MUL(gteIR0,limB10(gteRFC - ((MUL(gteR,_ir1)) >> 8)))));
			mac1=A1_32(aux>>12);
		}
		{
			int aux;
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(((s32)gteG << 4),_ir2)) , "r"(MUL(gteIR0,limB20(gteGFC - ((MUL(gteG,_ir2)) >> 8)))));
			mac2=A2_32(aux>>12);
		}
		{
			int aux;
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(((s32)gteB << 4),_ir3)) , "r"(MUL(gteIR0,limB30(gteBFC - ((MUL(gteB,_ir3)) >> 8)))));
			mac3=A3_32(aux>>12);
		}
	#endif
	}
		
		gteRGB0 = gteRGB1;
		gteRGB1 = gteRGB2;
		gteCODE2 = gteCODE;
		gteR2 = limC1(mac1 >> 4);
		gteG2 = limC2(mac2 >> 4);
		gteB2 = limC3(mac3 >> 4);
	}
	{
		s32 vx = VX_m3(1);
		s32 vy = VY_m3(1);
		s32 vz = VZ_m3(1);
		s32 mac1, mac2, mac3;
		
	#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
		mac1 = A1((((s64)gteL11 * vx) + (gteL12 * vy) + (gteL13 * vz)) >> 12);
		mac2 = A2((((s64)gteL21 * vx) + (gteL22 * vy) + (gteL23 * vz)) >> 12);
		mac3 = A3((((s64)gteL31 * vx) + (gteL32 * vy) + (gteL33 * vz)) >> 12);
	#else
		{
			int aux=MUL((s32)gteL11,vx);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL12,vy)) , "r"(aux));
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL13,vz)) , "r"(aux));
			aux>>=12;
			mac1=A1_32(aux);
		}
		{
			int aux=MUL((s32)gteL21,vx);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL22,vy)) , "r"(aux));
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL23,vz)) , "r"(aux));
			aux>>=12;
			mac2=A2_32(aux);
		}
		{
			int aux=MUL((s32)gteL31,vx);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL32,vy)) , "r"(aux));
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL33,vz)) , "r"(aux));
			aux>>=12;
			mac3=A3_32(aux);
		}
	#endif
	{
		s16 _ir1=limB11(mac1);
		s16 _ir2=limB21(mac2);
		s16 _ir3=limB31(mac3);
#ifdef USE_GTE_FLAG
		gteIR1 = _ir1;
		gteIR2 = _ir2;
		gteIR3 = _ir3;
#endif
		
	#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
		mac1 = A1((((s64)gteRBK << 12) + (gteLR1 * _ir1) + (gteLR2 * _ir2) + (gteLR3 * _ir3)) >> 12);
		mac2 = A2((((s64)gteGBK << 12) + (gteLG1 * _ir1) + (gteLG2 * _ir2) + (gteLG3 * _ir3)) >> 12);
		mac3 = A3((((s64)gteBBK << 12) + (gteLB1 * _ir1) + (gteLB2 * _ir2) + (gteLB3 * _ir3)) >> 12);
	#else
		{
			int aux=MUL(gteLR1,_ir1);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLR2,_ir2)) , "r"(aux));
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLR3,_ir3)) , "r"(aux));
			aux>>=12;
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteRBK) , "r"(aux));
			mac1=A1_32(aux);
		}
		{
			int aux=MUL(gteLG1,_ir1);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLG2,_ir2)) , "r"(aux));
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLG3,_ir3)) , "r"(aux));
			aux>>=12;
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteGBK) , "r"(aux));
			mac2=A2_32(aux);
		}
		{
			int aux=MUL(gteLB1,_ir1);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLB2,_ir2)) , "r"(aux));
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLB3,_ir3)) , "r"(aux));
			aux>>=12;
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteBBK) , "r"(aux));
			mac3=A3_32(aux);
		}
	#endif
	}
	{
		s16 _ir1=limB11(mac1);
		s16 _ir2=limB21(mac2);
		s16 _ir3=limB31(mac3);
#ifdef USE_GTE_FLAG
		gteIR1 = _ir1;
		gteIR2 = _ir2;
		gteIR3 = _ir3;
#endif

	#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
		mac1 = A1(((((s64)gteR << 4) * _ir1) + (gteIR0 * limB10(gteRFC - ((gteR * _ir1) >> 8)))) >> 12);
		mac2 = A2(((((s64)gteG << 4) * _ir2) + (gteIR0 * limB20(gteGFC - ((gteG * _ir2) >> 8)))) >> 12);
		mac3 = A3(((((s64)gteB << 4) * _ir3) + (gteIR0 * limB30(gteBFC - ((gteB * _ir3) >> 8)))) >> 12);
	#else
		{
			int aux;
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(((s32)gteR << 4),_ir1)) , "r"(MUL(gteIR0,limB10(gteRFC - ((MUL(gteR,_ir1)) >> 8)))));
			mac1=A1_32(aux>>12);
		}
		{
			int aux;
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(((s32)gteG << 4),_ir2)) , "r"(MUL(gteIR0,limB20(gteGFC - ((MUL(gteG,_ir2)) >> 8)))));
			mac2=A2_32(aux>>12);
		}
		{
			int aux;
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(((s32)gteB << 4),_ir3)) , "r"(MUL(gteIR0,limB30(gteBFC - ((MUL(gteB,_ir3)) >> 8)))));
			mac3=A3_32(aux>>12);
		}
	#endif
	}
		
		gteRGB0 = gteRGB1;
		gteRGB1 = gteRGB2;
		gteCODE2 = gteCODE;
		gteR2 = limC1(mac1 >> 4);
		gteG2 = limC2(mac2 >> 4);
		gteB2 = limC3(mac3 >> 4);
	}
	{
		s32 vx = VX_m3(2);
		s32 vy = VY_m3(2);
		s32 vz = VZ_m3(2);
		s32 mac1, mac2, mac3;
		
	#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
		mac1 = A1((((s64)gteL11 * vx) + (gteL12 * vy) + (gteL13 * vz)) >> 12);
		mac2 = A2((((s64)gteL21 * vx) + (gteL22 * vy) + (gteL23 * vz)) >> 12);
		mac3 = A3((((s64)gteL31 * vx) + (gteL32 * vy) + (gteL33 * vz)) >> 12);
	#else
		{
			int aux=MUL((s32)gteL11,vx);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL12,vy)) , "r"(aux));
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL13,vz)) , "r"(aux));
			aux>>=12;
			mac1=A1_32(aux);
		}
		{
			int aux=MUL((s32)gteL21,vx);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL22,vy)) , "r"(aux));
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL23,vz)) , "r"(aux));
			aux>>=12;
			mac2=A2_32(aux);
		}
		{
			int aux=MUL((s32)gteL31,vx);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL32,vy)) , "r"(aux));
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL33,vz)) , "r"(aux));
			aux>>=12;
			mac3=A3_32(aux);
		}
	#endif
	{
		s16 _ir1=limB11(mac1);
		s16 _ir2=limB21(mac2);
		s16 _ir3=limB31(mac3);
#ifdef USE_GTE_FLAG
		gteIR1 = _ir1;
		gteIR2 = _ir2;
		gteIR3 = _ir3;
#endif
		
	#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
		mac1 = A1((((s64)gteRBK << 12) + (gteLR1 * _ir1) + (gteLR2 * _ir2) + (gteLR3 * _ir3)) >> 12);
		mac2 = A2((((s64)gteGBK << 12) + (gteLG1 * _ir1) + (gteLG2 * _ir2) + (gteLG3 * _ir3)) >> 12);
		mac3 = A3((((s64)gteBBK << 12) + (gteLB1 * _ir1) + (gteLB2 * _ir2) + (gteLB3 * _ir3)) >> 12);
	#else
		{
			int aux=MUL(gteLR1,_ir1);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLR2,_ir2)) , "r"(aux));
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLR3,_ir3)) , "r"(aux));
			aux>>=12;
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteRBK) , "r"(aux));
			mac1=A1_32(aux);
		}
		{	
			int aux=MUL(gteLG1,_ir1);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLG2,_ir2)) , "r"(aux));
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLG3,_ir3)) , "r"(aux));
			aux>>=12;
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteGBK) , "r"(aux));
			mac2=A2_32(aux);
		}
		{
			int aux=MUL(gteLB1,_ir1);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLB2,_ir2)) , "r"(aux));
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLB3,_ir3)) , "r"(aux));
			aux>>=12;
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteBBK) , "r"(aux));
			mac3=A3_32(aux);
		}
	#endif
	}
	{
		s16 _ir1=limB11(mac1);
		s16 _ir2=limB21(mac2);
		s16 _ir3=limB31(mac3);
#ifdef USE_GTE_FLAG
		gteIR1 = _ir1;
		gteIR2 = _ir2;
		gteIR3 = _ir3;
#endif

	#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
		mac1 = A1(((((s64)gteR << 4) * _ir1) + (gteIR0 * limB10(gteRFC - ((gteR * _ir1) >> 8)))) >> 12);
		mac2 = A2(((((s64)gteG << 4) * _ir2) + (gteIR0 * limB20(gteGFC - ((gteG * _ir2) >> 8)))) >> 12);
		mac3 = A3(((((s64)gteB << 4) * _ir3) + (gteIR0 * limB30(gteBFC - ((gteB * _ir3) >> 8)))) >> 12);
	#else
		{
			int aux;
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(((s32)gteR << 4),_ir1)) , "r"(MUL(gteIR0,limB10(gteRFC - ((MUL(gteR,_ir1)) >> 8)))));
			mac1=A1_32(aux>>12);
		}
		{
			int aux;
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(((s32)gteG << 4),_ir2)) , "r"(MUL(gteIR0,limB20(gteGFC - ((MUL(gteG,_ir2)) >> 8)))));
			mac2=A2_32(aux>>12);
		}
		{
			int aux;
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(((s32)gteB << 4),_ir3)) , "r"(MUL(gteIR0,limB30(gteBFC - ((MUL(gteB,_ir3)) >> 8)))));
			mac3=A3_32(aux>>12);
		}
	#endif
	}
		gteMAC1 = mac1;
		gteMAC2 = mac2;
		gteMAC3 = mac3;
	}
}

void _gteNCDT(psxRegisters *regs) {
	pcsx4all_prof_start_with_pause(PCSX4ALL_PROF_GTE,PCSX4ALL_PROF_CPU);
	int v;
	s32 vx, vy, vz;

#ifdef GTE_LOG
	GTE_LOG("GTE NCDT\n");
#endif
#ifdef USE_GTE_FLAG
	gteFLAG = 0;
#endif

	for (v = 0; v < 3; v++) {
		vx = VX_m3(v);
		vy = VY_m3(v);
		vz = VZ_m3(v);
		
		#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
			gteMAC1 = A1((((s64)gteL11 * vx) + (gteL12 * vy) + (gteL13 * vz)) >> 12);
			gteMAC2 = A2((((s64)gteL21 * vx) + (gteL22 * vy) + (gteL23 * vz)) >> 12);
			gteMAC3 = A3((((s64)gteL31 * vx) + (gteL32 * vy) + (gteL33 * vz)) >> 12);
		#else
			int aux=MUL((s32)gteL11,vx);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL12,vy)) , "r"(aux));
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL13,vz)) , "r"(aux));
			aux>>=12;
			gteMAC1=A1_32(aux);

			aux=MUL((s32)gteL21,vx);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL22,vy)) , "r"(aux));
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL23,vz)) , "r"(aux));
			aux>>=12;
			gteMAC2=A2_32(aux);

			aux=MUL((s32)gteL31,vx);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL32,vy)) , "r"(aux));
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL33,vz)) , "r"(aux));
			aux>>=12;
			gteMAC3=A3_32(aux);
		#endif
		{
			s16 _ir1=limB11(gteMAC1);
			s16 _ir2=limB21(gteMAC2);
			s16 _ir3=limB31(gteMAC3);
#ifdef USE_GTE_FLAG
			gteIR1 = _ir1;
			gteIR2 = _ir2;
			gteIR3 = _ir3;
#endif
		
		#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
			gteMAC1 = A1((((s64)gteRBK << 12) + (gteLR1 * _ir1) + (gteLR2 * _ir2) + (gteLR3 * _ir3)) >> 12);
			gteMAC2 = A2((((s64)gteGBK << 12) + (gteLG1 * _ir1) + (gteLG2 * _ir2) + (gteLG3 * _ir3)) >> 12);
			gteMAC3 = A3((((s64)gteBBK << 12) + (gteLB1 * _ir1) + (gteLB2 * _ir2) + (gteLB3 * _ir3)) >> 12);
		#else
			aux=MUL(gteLR1,_ir1);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLR2,_ir2)) , "r"(aux));
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLR3,_ir3)) , "r"(aux));
			aux>>=12;
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteRBK) , "r"(aux));
			gteMAC1=A1_32(aux);
			
			aux=MUL(gteLG1,_ir1);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLG2,_ir2)) , "r"(aux));
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLG3,_ir3)) , "r"(aux));
			aux>>=12;
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteGBK) , "r"(aux));
			gteMAC2=A2_32(aux);

			aux=MUL(gteLB1,_ir1);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLB2,_ir2)) , "r"(aux));
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLB3,_ir3)) , "r"(aux));
			aux>>=12;
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteBBK) , "r"(aux));
			gteMAC3=A3_32(aux);
		#endif
		}
		{
			s16 _ir1=limB11(gteMAC1);
			s16 _ir2=limB21(gteMAC2);
			s16 _ir3=limB31(gteMAC3);
#ifdef USE_GTE_FLAG
			gteIR1 = _ir1;
			gteIR2 = _ir2;
			gteIR3 = _ir3;
#endif

		#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
			gteMAC1 = A1(((((s64)gteR << 4) * _ir1) + (gteIR0 * limB10(gteRFC - ((gteR * _ir1) >> 8)))) >> 12);
			gteMAC2 = A2(((((s64)gteG << 4) * _ir2) + (gteIR0 * limB20(gteGFC - ((gteG * _ir2) >> 8)))) >> 12);
			gteMAC3 = A3(((((s64)gteB << 4) * _ir3) + (gteIR0 * limB30(gteBFC - ((gteB * _ir3) >> 8)))) >> 12);
		#else
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(((s32)gteR << 4),_ir1)) , "r"(MUL(gteIR0,limB10(gteRFC - ((MUL(gteR,_ir1)) >> 8)))));
			gteMAC1=A1_32(aux>>12);
		
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(((s32)gteG << 4),_ir2)) , "r"(MUL(gteIR0,limB20(gteGFC - ((MUL(gteG,_ir2)) >> 8)))));
			gteMAC2=A2_32(aux>>12);

			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(((s32)gteB << 4),_ir3)) , "r"(MUL(gteIR0,limB30(gteBFC - ((MUL(gteB,_ir3)) >> 8)))));
			gteMAC3=A3_32(aux>>12);
		#endif
		}
		
		gteRGB0 = gteRGB1;
		gteRGB1 = gteRGB2;
		gteCODE2 = gteCODE;
		gteR2 = limC1(gteMAC1 >> 4);
		gteG2 = limC2(gteMAC2 >> 4);
		gteB2 = limC3(gteMAC3 >> 4);
	}
	gteIR1 = limB11(gteMAC1);
	gteIR2 = limB21(gteMAC2);
	gteIR3 = limB31(gteMAC3);
	pcsx4all_prof_end_with_resume(PCSX4ALL_PROF_GTE,PCSX4ALL_PROF_CPU);
}

void gteNCDT(void) {
	_gteNCDT(&psxRegs);
}

#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
#define macOP(SHIFT) \
{ \
	gteMAC1 = A1(((s64)(gteR22 * gteIR3) - (gteR33 * gteIR2)) >> SHIFT); \
	gteMAC2 = A2(((s64)(gteR33 * gteIR1) - (gteR11 * gteIR3)) >> SHIFT); \
	gteMAC3 = A3(((s64)(gteR11 * gteIR2) - (gteR22 * gteIR1)) >> SHIFT); \
}
#else
#define macOP(SHIFT) \
{ \
	int aux; \
	asm ("qsub %0, %1, %2" : "=r" (aux) : "r"(MUL(gteR22,gteIR3)) , "r"(MUL(gteR33,gteIR2))); \
	gteMAC1=A1_32(aux>>SHIFT); \
	asm ("qsub %0, %1, %2" : "=r" (aux) : "r"(MUL(gteR33,gteIR1)) , "r"(MUL(gteR11,gteIR3))); \
	gteMAC2=A2_32(aux>>SHIFT); \
	asm ("qsub %0, %1, %2" : "=r" (aux) : "r"(MUL(gteR11,gteIR2)) , "r"(MUL(gteR22,gteIR1))); \
	gteMAC3=A3_32(aux>>SHIFT); \
}
#endif
void _gteOP_s12(psxRegisters *regs) { macOP(12); }
void _gteOP_s0(psxRegisters *regs) { macOP(0); }

void _gteOP(psxRegisters *regs) {
	pcsx4all_prof_start_with_pause(PCSX4ALL_PROF_GTE,PCSX4ALL_PROF_CPU);
	int shift = (GTE_SF(gteop)?12:0);
	int lm = GTE_LM(gteop);

#ifdef GTE_LOG
	GTE_LOG("GTE OP\n");
#endif
#ifdef USE_GTE_FLAG
	gteFLAG = 0;
#endif

	#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
		gteMAC1 = A1(((s64)(gteR22 * gteIR3) - (gteR33 * gteIR2)) >> shift);
		gteMAC2 = A2(((s64)(gteR33 * gteIR1) - (gteR11 * gteIR3)) >> shift);
		gteMAC3 = A3(((s64)(gteR11 * gteIR2) - (gteR22 * gteIR1)) >> shift);
	#else
		int aux;
		asm ("qsub %0, %1, %2" : "=r" (aux) : "r"(MUL(gteR22,gteIR3)) , "r"(MUL(gteR33,gteIR2)));
		gteMAC1=A1_32(aux>>shift);
		
		asm ("qsub %0, %1, %2" : "=r" (aux) : "r"(MUL(gteR33,gteIR1)) , "r"(MUL(gteR11,gteIR3)));
		gteMAC2=A2_32(aux>>shift);

		asm ("qsub %0, %1, %2" : "=r" (aux) : "r"(MUL(gteR11,gteIR2)) , "r"(MUL(gteR22,gteIR1)));
		gteMAC3=A3_32(aux>>shift);
	#endif
	
	gteIR1 = limB1(gteMAC1, lm);
	gteIR2 = limB2(gteMAC2, lm);
	gteIR3 = limB3(gteMAC3, lm);
	pcsx4all_prof_end_with_resume(PCSX4ALL_PROF_GTE,PCSX4ALL_PROF_CPU);
}

void gteOP(void) {
	_gteOP(&psxRegs);
}

void _gteDCPL_(psxRegisters *regs) {

#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
	s64 RIR1 = ((s64)gteR * gteIR1) >> 8;
	s64 GIR2 = ((s64)gteG * gteIR2) >> 8;
	s64 BIR3 = ((s64)gteB * gteIR3) >> 8;
#else
	s64 RIR1;
	asm ("smull %Q0, %R0, %1, %2" : "=&r" (RIR1) : "r"(gteR) , "r"(gteIR1));
	RIR1=RIR1>>8;
	s64 GIR2;
	asm ("smull %Q0, %R0, %1, %2" : "=&r" (GIR2) : "r"(gteG) , "r"(gteIR2));
	GIR2=GIR2>>8;
	s64 BIR3;
	asm ("smull %Q0, %R0, %1, %2" : "=&r" (BIR3) : "r"(gteB) , "r"(gteIR3));
	BIR3=BIR3>>8;
#endif	
	
#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
	gteMAC1 = A1(RIR1 + ((gteIR0 * limB10(gteRFC - RIR1)) >> 12));
	gteMAC2 = A2(GIR2 + ((gteIR0 * limB10(gteGFC - GIR2)) >> 12));
	gteMAC3 = A3(BIR3 + ((gteIR0 * limB10(gteBFC - BIR3)) >> 12));
#else
	gteMAC1 = A1(RIR1 + ((MUL(gteIR0,limB10(gteRFC - RIR1))) >> 12));
	gteMAC2 = A2(GIR2 + ((MUL(gteIR0,limB10(gteGFC - GIR2))) >> 12));
	gteMAC3 = A3(BIR3 + ((MUL(gteIR0,limB10(gteBFC - BIR3))) >> 12));
#endif
}

void _gteDCPL(psxRegisters *regs) {
	pcsx4all_prof_start_with_pause(PCSX4ALL_PROF_GTE,PCSX4ALL_PROF_CPU);
	int lm = GTE_LM(gteop);

#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
	s64 RIR1 = ((s64)gteR * gteIR1) >> 8;
	s64 GIR2 = ((s64)gteG * gteIR2) >> 8;
	s64 BIR3 = ((s64)gteB * gteIR3) >> 8;
#else
	s64 RIR1;
	asm ("smull %Q0, %R0, %1, %2" : "=&r" (RIR1) : "r"(gteR) , "r"(gteIR1));
	RIR1=RIR1>>8;
	s64 GIR2;
	asm ("smull %Q0, %R0, %1, %2" : "=&r" (GIR2) : "r"(gteG) , "r"(gteIR2));
	GIR2=GIR2>>8;
	s64 BIR3;
	asm ("smull %Q0, %R0, %1, %2" : "=&r" (BIR3) : "r"(gteB) , "r"(gteIR3));
	BIR3=BIR3>>8;
#endif	

#ifdef GTE_LOG
	GTE_LOG("GTE DCPL\n");
#endif
#ifdef USE_GTE_FLAG
	gteFLAG = 0;
#endif

	#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
		gteMAC1 = A1(RIR1 + ((gteIR0 * limB10(gteRFC - RIR1)) >> 12));
		gteMAC2 = A2(GIR2 + ((gteIR0 * limB10(gteGFC - GIR2)) >> 12));
		gteMAC3 = A3(BIR3 + ((gteIR0 * limB10(gteBFC - BIR3)) >> 12));
	#else
		gteMAC1 = A1(RIR1 + ((MUL(gteIR0,limB10(gteRFC - RIR1))) >> 12));
		gteMAC2 = A2(GIR2 + ((MUL(gteIR0,limB10(gteGFC - GIR2))) >> 12));
		gteMAC3 = A3(BIR3 + ((MUL(gteIR0,limB10(gteBFC - BIR3))) >> 12));
	#endif

	gteIR1 = limB1(gteMAC1, lm);
	gteIR2 = limB2(gteMAC2, lm);
	gteIR3 = limB3(gteMAC3, lm);

	gteRGB0 = gteRGB1;
	gteRGB1 = gteRGB2;
	gteCODE2 = gteCODE;
	gteR2 = limC1(gteMAC1 >> 4);
	gteG2 = limC2(gteMAC2 >> 4);
	gteB2 = limC3(gteMAC3 >> 4);
	pcsx4all_prof_end_with_resume(PCSX4ALL_PROF_GTE,PCSX4ALL_PROF_CPU);
}

void gteDCPL(void) {
	_gteDCPL(&psxRegs);
}

#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
#define macGPF(SHIFT) \
{ \
	gteMAC1 = A1(((s64)gteIR0 * gteIR1) >> SHIFT); \
	gteMAC2 = A2(((s64)gteIR0 * gteIR2) >> SHIFT); \
	gteMAC3 = A3(((s64)gteIR0 * gteIR3) >> SHIFT); \
}
#else
#define macGPF(SHIFT) \
{ \
	gteMAC1 = A1_32((MUL((s32)gteIR0,gteIR1)) >> SHIFT); \
	gteMAC2 = A2_32((MUL((s32)gteIR0,gteIR2)) >> SHIFT); \
	gteMAC3 = A3_32((MUL((s32)gteIR0,gteIR3)) >> SHIFT); \
}
#endif
void _gteGPF_s12(psxRegisters *regs) { macGPF(12); }
void _gteGPF_s0(psxRegisters *regs) { macGPF(0); }

void _gteGPF(psxRegisters *regs) {
	pcsx4all_prof_start_with_pause(PCSX4ALL_PROF_GTE,PCSX4ALL_PROF_CPU);
	int shift = (GTE_SF(gteop)?12:0);

#ifdef GTE_LOG
	GTE_LOG("GTE GPF\n");
#endif
#ifdef USE_GTE_FLAG
	gteFLAG = 0;
#endif

	#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
		gteMAC1 = A1(((s64)gteIR0 * gteIR1) >> shift);
		gteMAC2 = A2(((s64)gteIR0 * gteIR2) >> shift);
		gteMAC3 = A3(((s64)gteIR0 * gteIR3) >> shift);
	#else
		gteMAC1 = A1_32((MUL((s32)gteIR0,gteIR1)) >> shift);
		gteMAC2 = A2_32((MUL((s32)gteIR0,gteIR2)) >> shift);
		gteMAC3 = A3_32((MUL((s32)gteIR0,gteIR3)) >> shift);
	#endif
	
	gteIR1 = limB10(gteMAC1);
	gteIR2 = limB20(gteMAC2);
	gteIR3 = limB30(gteMAC3);

	gteRGB0 = gteRGB1;
	gteRGB1 = gteRGB2;
	gteCODE2 = gteCODE;
	gteR2 = limC1(gteMAC1 >> 4);
	gteG2 = limC2(gteMAC2 >> 4);
	gteB2 = limC3(gteMAC3 >> 4);
	pcsx4all_prof_end_with_resume(PCSX4ALL_PROF_GTE,PCSX4ALL_PROF_CPU);
}

void gteGPF(void) {
	_gteGPF(&psxRegs);
}

#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
#define macGPL(SHIFT) \
{ \
	gteMAC1 = A1((((s64)gteMAC1 << SHIFT) + (gteIR0 * gteIR1)) >> SHIFT); \
	gteMAC2 = A2((((s64)gteMAC2 << SHIFT) + (gteIR0 * gteIR2)) >> SHIFT); \
	gteMAC3 = A3((((s64)gteMAC3 << SHIFT) + (gteIR0 * gteIR3)) >> SHIFT); \
}
#else
#define macGPL(SHIFT) \
{ \
	int aux; \
	asm ("qadd %0, %1, %2" : "=r" (aux) : "r"((s32)gteMAC1) , "r"((MUL(gteIR0,gteIR1))>>SHIFT)); \
	gteMAC1=A1_32(aux); \
	asm ("qadd %0, %1, %2" : "=r" (aux) : "r"((s32)gteMAC2) , "r"((MUL(gteIR0,gteIR2))>>SHIFT)); \
	gteMAC2=A2_32(aux); \
	asm ("qadd %0, %1, %2" : "=r" (aux) : "r"((s32)gteMAC3) , "r"((MUL(gteIR0,gteIR3))>>SHIFT)); \
	gteMAC3=A3_32(aux); \
}
#endif
void _gteGPL_s12(psxRegisters *regs) { macGPL(12); }
void _gteGPL_s0(psxRegisters *regs) { macGPL(0); }

void _gteGPL(psxRegisters *regs) {
	pcsx4all_prof_start_with_pause(PCSX4ALL_PROF_GTE,PCSX4ALL_PROF_CPU);
	int shift = (GTE_SF(gteop)?12:0);

#ifdef GTE_LOG
	GTE_LOG("GTE GPL\n");
#endif
#ifdef USE_GTE_FLAG
	gteFLAG = 0;
#endif

	#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
		gteMAC1 = A1((((s64)gteMAC1 << shift) + (gteIR0 * gteIR1)) >> shift);
		gteMAC2 = A2((((s64)gteMAC2 << shift) + (gteIR0 * gteIR2)) >> shift);
		gteMAC3 = A3((((s64)gteMAC3 << shift) + (gteIR0 * gteIR3)) >> shift);
	#else
		int aux;
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"((s32)gteMAC1) , "r"((MUL(gteIR0,gteIR1))>>shift));
		gteMAC1=A1_32(aux);
		
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"((s32)gteMAC2) , "r"((MUL(gteIR0,gteIR2))>>shift));
		gteMAC2=A2_32(aux);

		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"((s32)gteMAC3) , "r"((MUL(gteIR0,gteIR3))>>shift));
		gteMAC3=A3_32(aux);
	#endif
	
	gteIR1 = limB10(gteMAC1);
	gteIR2 = limB20(gteMAC2);
	gteIR3 = limB30(gteMAC3);

	gteRGB0 = gteRGB1;
	gteRGB1 = gteRGB2;
	gteCODE2 = gteCODE;
	gteR2 = limC1(gteMAC1 >> 4);
	gteG2 = limC2(gteMAC2 >> 4);
	gteB2 = limC3(gteMAC3 >> 4);
	pcsx4all_prof_end_with_resume(PCSX4ALL_PROF_GTE,PCSX4ALL_PROF_CPU);
}

void gteGPL(void) {
	_gteGPL(&psxRegs);
}

#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
#define macDPCS(SHIFT) \
{ \
	gteMAC1 = A1(((gteR << 16) + (gteIR0 * limB10(A1((s64)gteRFC - (gteR << 4)) << (12 - SHIFT)))) >> 12); \
	gteMAC2 = A2(((gteG << 16) + (gteIR0 * limB20(A2((s64)gteGFC - (gteG << 4)) << (12 - SHIFT)))) >> 12); \
	gteMAC3 = A3(((gteB << 16) + (gteIR0 * limB30(A3((s64)gteBFC - (gteB << 4)) << (12 - SHIFT)))) >> 12); \
}
#else
#define macDPCS(SHIFT) \
{ \
	int aux; \
	asm ("qsub %0, %1, %2" : "=r" (aux) : "r"((s32)gteRFC) , "r"(gteR << 4)); \
	gteMAC1 = A1_32(((gteR << 16) + (MUL(gteIR0,limB10(aux << (12 - SHIFT))))) >> 12); \
	asm ("qsub %0, %1, %2" : "=r" (aux) : "r"((s32)gteGFC) , "r"(gteG << 4)); \
	gteMAC2 = A2_32(((gteG << 16) + (MUL(gteIR0,limB20(aux << (12 - SHIFT))))) >> 12); \
	asm ("qsub %0, %1, %2" : "=r" (aux) : "r"((s32)gteBFC) , "r"(gteB << 4)); \
	gteMAC3 = A3_32(((gteB << 16) + (MUL(gteIR0,limB30(aux << (12 - SHIFT))))) >> 12); \
}
#endif
void _gteDPCS_s12(psxRegisters *regs) { macDPCS(12); }
void _gteDPCS_s0(psxRegisters *regs) { macDPCS(0); }

void _gteDPCS(psxRegisters *regs) {
	pcsx4all_prof_start_with_pause(PCSX4ALL_PROF_GTE,PCSX4ALL_PROF_CPU);
	int shift = (GTE_SF(gteop)?12:0);

#ifdef GTE_LOG
	GTE_LOG("GTE DPCS\n");
#endif
#ifdef USE_GTE_FLAG
	gteFLAG = 0;
#endif

	#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
		gteMAC1 = A1(((gteR << 16) + (gteIR0 * limB10(A1((s64)gteRFC - (gteR << 4)) << (12 - shift)))) >> 12);
		gteMAC2 = A2(((gteG << 16) + (gteIR0 * limB20(A2((s64)gteGFC - (gteG << 4)) << (12 - shift)))) >> 12);
		gteMAC3 = A3(((gteB << 16) + (gteIR0 * limB30(A3((s64)gteBFC - (gteB << 4)) << (12 - shift)))) >> 12);
	#else
		int aux;
		asm ("qsub %0, %1, %2" : "=r" (aux) : "r"((s32)gteRFC) , "r"(gteR << 4));
		gteMAC1 = A1_32(((gteR << 16) + (MUL(gteIR0,limB10(aux << (12 - shift))))) >> 12);
		
		asm ("qsub %0, %1, %2" : "=r" (aux) : "r"((s32)gteGFC) , "r"(gteG << 4));
		gteMAC2 = A2_32(((gteG << 16) + (MUL(gteIR0,limB20(aux << (12 - shift))))) >> 12);

		asm ("qsub %0, %1, %2" : "=r" (aux) : "r"((s32)gteBFC) , "r"(gteB << 4));
		gteMAC3 = A3_32(((gteB << 16) + (MUL(gteIR0,limB30(aux << (12 - shift))))) >> 12);
	#endif
	
	gteIR1 = limB10(gteMAC1);
	gteIR2 = limB20(gteMAC2);
	gteIR3 = limB30(gteMAC3);
	gteRGB0 = gteRGB1;
	gteRGB1 = gteRGB2;
	gteCODE2 = gteCODE;
	gteR2 = limC1(gteMAC1 >> 4);
	gteG2 = limC2(gteMAC2 >> 4);
	gteB2 = limC3(gteMAC3 >> 4);
	pcsx4all_prof_end_with_resume(PCSX4ALL_PROF_GTE,PCSX4ALL_PROF_CPU);
}

void gteDPCS() {
	_gteDPCS(&psxRegs);
}

void _gteDPCT_(psxRegisters *regs) {
	{
		s32 mac1, mac2, mac3;
	#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
		mac1 = A1((((s64)gteR0 << 16) + ((s64)gteIR0 * (limB10(gteRFC - (gteR0 << 4))))) >> 12);
		mac2 = A2((((s64)gteG0 << 16) + ((s64)gteIR0 * (limB10(gteGFC - (gteG0 << 4))))) >> 12);
		mac3 = A3((((s64)gteB0 << 16) + ((s64)gteIR0 * (limB10(gteBFC - (gteB0 << 4))))) >> 12);
	#else
		{
			int aux;
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"((s32)gteR0 << 16) , "r"(MUL((s32)gteIR0,(limB10(gteRFC - (gteR0 << 4))))));
			mac1=A1_32(aux>>12);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"((s32)gteG0 << 16) , "r"(MUL((s32)gteIR0,(limB10(gteGFC - (gteG0 << 4))))));
			mac2=A2_32(aux>>12);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"((s32)gteB0 << 16) , "r"(MUL((s32)gteIR0,(limB10(gteBFC - (gteB0 << 4))))));
			mac3=A3_32(aux>>12);
		}
	#endif

		gteRGB0 = gteRGB1;
		gteRGB1 = gteRGB2;
		gteCODE2 = gteCODE;
		gteR2 = limC1(mac1 >> 4);
		gteG2 = limC2(mac2 >> 4);
		gteB2 = limC3(mac3 >> 4);
	}
	{
		s32 mac1, mac2, mac3;
	#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
		mac1 = A1((((s64)gteR0 << 16) + ((s64)gteIR0 * (limB10(gteRFC - (gteR0 << 4))))) >> 12);
		mac2 = A2((((s64)gteG0 << 16) + ((s64)gteIR0 * (limB10(gteGFC - (gteG0 << 4))))) >> 12);
		mac3 = A3((((s64)gteB0 << 16) + ((s64)gteIR0 * (limB10(gteBFC - (gteB0 << 4))))) >> 12);
	#else
		{
			int aux;
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"((s32)gteR0 << 16) , "r"(MUL((s32)gteIR0,(limB10(gteRFC - (gteR0 << 4))))));
			mac1=A1_32(aux>>12);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"((s32)gteG0 << 16) , "r"(MUL((s32)gteIR0,(limB10(gteGFC - (gteG0 << 4))))));
			mac2=A2_32(aux>>12);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"((s32)gteB0 << 16) , "r"(MUL((s32)gteIR0,(limB10(gteBFC - (gteB0 << 4))))));
			mac3=A3_32(aux>>12);
		}
	#endif

		gteRGB0 = gteRGB1;
		gteRGB1 = gteRGB2;
		gteCODE2 = gteCODE;
		gteR2 = limC1(mac1 >> 4);
		gteG2 = limC2(mac2 >> 4);
		gteB2 = limC3(mac3 >> 4);
	}
	{
		s32 mac1, mac2, mac3;
	#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
		mac1 = A1((((s64)gteR0 << 16) + ((s64)gteIR0 * (limB10(gteRFC - (gteR0 << 4))))) >> 12);
		mac2 = A2((((s64)gteG0 << 16) + ((s64)gteIR0 * (limB10(gteGFC - (gteG0 << 4))))) >> 12);
		mac3 = A3((((s64)gteB0 << 16) + ((s64)gteIR0 * (limB10(gteBFC - (gteB0 << 4))))) >> 12);
	#else
		{
			int aux;
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"((s32)gteR0 << 16) , "r"(MUL((s32)gteIR0,(limB10(gteRFC - (gteR0 << 4))))));
			mac1=A1_32(aux>>12);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"((s32)gteG0 << 16) , "r"(MUL((s32)gteIR0,(limB10(gteGFC - (gteG0 << 4))))));
			mac2=A2_32(aux>>12);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"((s32)gteB0 << 16) , "r"(MUL((s32)gteIR0,(limB10(gteBFC - (gteB0 << 4))))));
			mac3=A3_32(aux>>12);
		}
	#endif
		gteMAC1 = mac1;
		gteMAC2 = mac2;
		gteMAC3 = mac3;
	}
}

void _gteDPCT(psxRegisters *regs) {
	pcsx4all_prof_start_with_pause(PCSX4ALL_PROF_GTE,PCSX4ALL_PROF_CPU);
	int v;

#ifdef GTE_LOG
	GTE_LOG("GTE DPCT\n");
#endif
#ifdef USE_GTE_FLAG
	gteFLAG = 0;
#endif

	for (v = 0; v < 3; v++) {
	
		#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
			gteMAC1 = A1((((s64)gteR0 << 16) + ((s64)gteIR0 * (limB10(gteRFC - (gteR0 << 4))))) >> 12);
			gteMAC2 = A2((((s64)gteG0 << 16) + ((s64)gteIR0 * (limB10(gteGFC - (gteG0 << 4))))) >> 12);
			gteMAC3 = A3((((s64)gteB0 << 16) + ((s64)gteIR0 * (limB10(gteBFC - (gteB0 << 4))))) >> 12);
		#else
			int aux;
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"((s32)gteR0 << 16) , "r"(MUL((s32)gteIR0,(limB10(gteRFC - (gteR0 << 4))))));
			gteMAC1=A1_32(aux>>12);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"((s32)gteG0 << 16) , "r"(MUL((s32)gteIR0,(limB10(gteGFC - (gteG0 << 4))))));
			gteMAC2=A2_32(aux>>12);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"((s32)gteB0 << 16) , "r"(MUL((s32)gteIR0,(limB10(gteBFC - (gteB0 << 4))))));
			gteMAC3=A3_32(aux>>12);
		#endif

		gteRGB0 = gteRGB1;
		gteRGB1 = gteRGB2;
		gteCODE2 = gteCODE;
		gteR2 = limC1(gteMAC1 >> 4);
		gteG2 = limC2(gteMAC2 >> 4);
		gteB2 = limC3(gteMAC3 >> 4);
	}
	gteIR1 = limB10(gteMAC1);
	gteIR2 = limB20(gteMAC2);
	gteIR3 = limB30(gteMAC3);
	pcsx4all_prof_end_with_resume(PCSX4ALL_PROF_GTE,PCSX4ALL_PROF_CPU);
}

void gteDPCT(void) {
	_gteDPCT(&psxRegs);
}

void _gteNCS_(psxRegisters *regs) {
	{
		s32 mac1, mac2, mac3;
	#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
		mac1 = A1((((s64)gteL11 * gteVX0) + (gteL12 * gteVY0) + (gteL13 * gteVZ0)) >> 12);
		mac2 = A2((((s64)gteL21 * gteVX0) + (gteL22 * gteVY0) + (gteL23 * gteVZ0)) >> 12);
		mac3 = A3((((s64)gteL31 * gteVX0) + (gteL32 * gteVY0) + (gteL33 * gteVZ0)) >> 12);
	#else
		{
			int aux=MUL((s32)gteL11,gteVX0);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL12,gteVY0)) , "r"(aux));
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL13,gteVZ0)) , "r"(aux));
			aux>>=12;
			mac1=A1_32(aux);
		}
		{
			int aux=MUL((s32)gteL21,gteVX0);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL22,gteVY0)) , "r"(aux));
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL23,gteVZ0)) , "r"(aux));
			aux>>=12;
			mac2=A2_32(aux);
		}
		{
			int aux=MUL((s32)gteL31,gteVX0);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL32,gteVY0)) , "r"(aux));
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL33,gteVZ0)) , "r"(aux));
			aux>>=12;
			mac3=A3_32(aux);
		}
	#endif
	{
		s16 _ir1=limB11(mac1);
		s16 _ir2=limB21(mac2);
		s16 _ir3=limB31(mac3);
#ifdef USE_GTE_FLAG
		gteIR1 = _ir1;
		gteIR2 = _ir2;
		gteIR3 = _ir3;
#endif
	
	#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
		mac1 = A1((((s64)gteRBK << 12) + (gteLR1 * _ir1) + (gteLR2 * _ir2) + (gteLR3 * _ir3)) >> 12);
		mac2 = A2((((s64)gteGBK << 12) + (gteLG1 * _ir1) + (gteLG2 * _ir2) + (gteLG3 * _ir3)) >> 12);
		mac3 = A3((((s64)gteBBK << 12) + (gteLB1 * _ir1) + (gteLB2 * _ir2) + (gteLB3 * _ir3)) >> 12);
	#else
		{
			int aux=MUL(gteLR1,_ir1);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLR2,_ir2)) , "r"(aux));
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLR3,_ir3)) , "r"(aux));
			aux>>=12;
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteRBK) , "r"(aux));
			mac1=A1_32(aux);
		}
		{
			int aux=MUL(gteLG1,_ir1);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLG2,_ir2)) , "r"(aux));
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLG3,_ir3)) , "r"(aux));
			aux>>=12;
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteGBK) , "r"(aux));
			mac2=A2_32(aux);
		}
		{
			int aux=MUL(gteLB1,_ir1);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLB2,_ir2)) , "r"(aux));
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLB3,_ir3)) , "r"(aux));
			aux>>=12;
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteBBK) , "r"(aux));
			mac3=A3_32(aux);
		}
	#endif
	}
		gteMAC1 = mac1;
		gteMAC2 = mac2;
		gteMAC3 = mac3;
	}
}

void _gteNCS(psxRegisters *regs) {
	pcsx4all_prof_start_with_pause(PCSX4ALL_PROF_GTE,PCSX4ALL_PROF_CPU);
#ifdef GTE_LOG
	GTE_LOG("GTE NCS\n");
#endif
#ifdef USE_GTE_FLAG
	gteFLAG = 0;
#endif

	#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
		gteMAC1 = A1((((s64)gteL11 * gteVX0) + (gteL12 * gteVY0) + (gteL13 * gteVZ0)) >> 12);
		gteMAC2 = A2((((s64)gteL21 * gteVX0) + (gteL22 * gteVY0) + (gteL23 * gteVZ0)) >> 12);
		gteMAC3 = A3((((s64)gteL31 * gteVX0) + (gteL32 * gteVY0) + (gteL33 * gteVZ0)) >> 12);
	#else
		int aux=MUL((s32)gteL11,gteVX0);
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL12,gteVY0)) , "r"(aux));
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL13,gteVZ0)) , "r"(aux));
		aux>>=12;
		gteMAC1=A1_32(aux);

		aux=MUL((s32)gteL21,gteVX0);
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL22,gteVY0)) , "r"(aux));
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL23,gteVZ0)) , "r"(aux));
		aux>>=12;
		gteMAC2=A2_32(aux);

		aux=MUL((s32)gteL31,gteVX0);
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL32,gteVY0)) , "r"(aux));
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL33,gteVZ0)) , "r"(aux));
		aux>>=12;
		gteMAC3=A3_32(aux);
	#endif
	{
		s16 _ir1=limB11(gteMAC1);
		s16 _ir2=limB21(gteMAC2);
		s16 _ir3=limB31(gteMAC3);
#ifdef USE_GTE_FLAG
		gteIR1 = _ir1;
		gteIR2 = _ir2;
		gteIR3 = _ir3;
#endif
	
	#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
		gteMAC1 = A1((((s64)gteRBK << 12) + (gteLR1 * _ir1) + (gteLR2 * _ir2) + (gteLR3 * _ir3)) >> 12);
		gteMAC2 = A2((((s64)gteGBK << 12) + (gteLG1 * _ir1) + (gteLG2 * _ir2) + (gteLG3 * _ir3)) >> 12);
		gteMAC3 = A3((((s64)gteBBK << 12) + (gteLB1 * _ir1) + (gteLB2 * _ir2) + (gteLB3 * _ir3)) >> 12);
	#else
		aux=MUL(gteLR1,_ir1);
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLR2,_ir2)) , "r"(aux));
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLR3,_ir3)) , "r"(aux));
		aux>>=12;
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteRBK) , "r"(aux));
		gteMAC1=A1_32(aux);
		
		aux=MUL(gteLG1,_ir1);
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLG2,_ir2)) , "r"(aux));
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLG3,_ir3)) , "r"(aux));
		aux>>=12;
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteGBK) , "r"(aux));
		gteMAC2=A2_32(aux);

		aux=MUL(gteLB1,_ir1);
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLB2,_ir2)) , "r"(aux));
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLB3,_ir3)) , "r"(aux));
		aux>>=12;
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteBBK) , "r"(aux));
		gteMAC3=A3_32(aux);
	#endif
	}

	gteIR1 = limB11(gteMAC1);
	gteIR2 = limB21(gteMAC2);
	gteIR3 = limB31(gteMAC3);

	gteRGB0 = gteRGB1;
	gteRGB1 = gteRGB2;
	gteCODE2 = gteCODE;
	gteR2 = limC1(gteMAC1 >> 4);
	gteG2 = limC2(gteMAC2 >> 4);
	gteB2 = limC3(gteMAC3 >> 4);
	pcsx4all_prof_end_with_resume(PCSX4ALL_PROF_GTE,PCSX4ALL_PROF_CPU);
}

void gteNCS(void) {
	_gteNCS(&psxRegs);
}

void _gteNCT_(psxRegisters *regs) {
	{
		s32 vx = VX_m3(0);
		s32 vy = VY_m3(0);
		s32 vz = VZ_m3(0);
		s32 mac1, mac2, mac3;
	
	#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
		mac1 = A1((((s64)gteL11 * vx) + (gteL12 * vy) + (gteL13 * vz)) >> 12);
		mac2 = A2((((s64)gteL21 * vx) + (gteL22 * vy) + (gteL23 * vz)) >> 12);
		mac3 = A3((((s64)gteL31 * vx) + (gteL32 * vy) + (gteL33 * vz)) >> 12);
	#else
		{
			int aux=MUL((s32)gteL11,vx);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL12,vy)) , "r"(aux));
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL13,vz)) , "r"(aux));
			aux>>=12;
			mac1=A1_32(aux);
		}
		{
			int aux=MUL((s32)gteL21,vx);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL22,vy)) , "r"(aux));
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL23,vz)) , "r"(aux));
			aux>>=12;
			mac2=A2_32(aux);
		}
		{
			int aux=MUL((s32)gteL31,vx);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL32,vy)) , "r"(aux));
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL33,vz)) , "r"(aux));
			aux>>=12;
			mac3=A3_32(aux);
		}
		#endif
		{
			s16 _ir1=limB11(mac1);
			s16 _ir2=limB21(mac2);
			s16 _ir3=limB31(mac3);
#ifdef USE_GTE_FLAG
			gteIR1 = _ir1;
			gteIR2 = _ir2;
			gteIR3 = _ir3;
#endif
		
		#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
			mac1 = A1((((s64)gteRBK << 12) + (gteLR1 * _ir1) + (gteLR2 * _ir2) + (gteLR3 * _ir3)) >> 12);
			mac2 = A2((((s64)gteGBK << 12) + (gteLG1 * _ir1) + (gteLG2 * _ir2) + (gteLG3 * _ir3)) >> 12);
			mac3 = A3((((s64)gteBBK << 12) + (gteLB1 * _ir1) + (gteLB2 * _ir2) + (gteLB3 * _ir3)) >> 12);
		#else
			{
				int aux=MUL(gteLR1,_ir1);
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLR2,_ir2)) , "r"(aux));
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLR3,_ir3)) , "r"(aux));
				aux>>=12;
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteRBK) , "r"(aux));
				mac1=A1_32(aux);
			}
			{
				int aux=MUL(gteLG1,_ir1);
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLG2,_ir2)) , "r"(aux));
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLG3,_ir3)) , "r"(aux));
				aux>>=12;
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteGBK) , "r"(aux));
				mac2=A2_32(aux);
			}
			{
				int aux=MUL(gteLB1,_ir1);
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLB2,_ir2)) , "r"(aux));
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLB3,_ir3)) , "r"(aux));
				aux>>=12;
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteBBK) , "r"(aux));
				mac3=A3_32(aux);
			}
		#endif
		}
		
		gteRGB0 = gteRGB1;
		gteRGB1 = gteRGB2;
		gteCODE2 = gteCODE;
		gteR2 = limC1(mac1 >> 4);
		gteG2 = limC2(mac2 >> 4);
		gteB2 = limC3(mac3 >> 4);
	}
	{
		s32 vx = VX_m3(1);
		s32 vy = VY_m3(1);
		s32 vz = VZ_m3(1);
		s32 mac1, mac2, mac3;
	
	#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
		mac1 = A1((((s64)gteL11 * vx) + (gteL12 * vy) + (gteL13 * vz)) >> 12);
		mac2 = A2((((s64)gteL21 * vx) + (gteL22 * vy) + (gteL23 * vz)) >> 12);
		mac3 = A3((((s64)gteL31 * vx) + (gteL32 * vy) + (gteL33 * vz)) >> 12);
	#else
		{
			int aux=MUL((s32)gteL11,vx);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL12,vy)) , "r"(aux));
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL13,vz)) , "r"(aux));
			aux>>=12;
			mac1=A1_32(aux);
		}
		{
			int aux=MUL((s32)gteL21,vx);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL22,vy)) , "r"(aux));
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL23,vz)) , "r"(aux));
			aux>>=12;
			mac2=A2_32(aux);
		}
		{
			int aux=MUL((s32)gteL31,vx);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL32,vy)) , "r"(aux));
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL33,vz)) , "r"(aux));
			aux>>=12;
			mac3=A3_32(aux);
		}
		#endif
		{
			s16 _ir1=limB11(mac1);
			s16 _ir2=limB21(mac2);
			s16 _ir3=limB31(mac3);
#ifdef USE_GTE_FLAG
			gteIR1 = _ir1;
			gteIR2 = _ir2;
			gteIR3 = _ir3;
#endif
		
		#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
			mac1 = A1((((s64)gteRBK << 12) + (gteLR1 * _ir1) + (gteLR2 * _ir2) + (gteLR3 * _ir3)) >> 12);
			mac2 = A2((((s64)gteGBK << 12) + (gteLG1 * _ir1) + (gteLG2 * _ir2) + (gteLG3 * _ir3)) >> 12);
			mac3 = A3((((s64)gteBBK << 12) + (gteLB1 * _ir1) + (gteLB2 * _ir2) + (gteLB3 * _ir3)) >> 12);
		#else
			{
				int aux=MUL(gteLR1,_ir1);
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLR2,_ir2)) , "r"(aux));
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLR3,_ir3)) , "r"(aux));
				aux>>=12;
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteRBK) , "r"(aux));
				mac1=A1_32(aux);
			}
			{
				int aux=MUL(gteLG1,_ir1);
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLG2,_ir2)) , "r"(aux));
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLG3,_ir3)) , "r"(aux));
				aux>>=12;
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteGBK) , "r"(aux));
				mac2=A2_32(aux);
			}
			{
				int aux=MUL(gteLB1,_ir1);
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLB2,_ir2)) , "r"(aux));
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLB3,_ir3)) , "r"(aux));
				aux>>=12;
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteBBK) , "r"(aux));
				mac3=A3_32(aux);
			}
		#endif
		}
		
		gteRGB0 = gteRGB1;
		gteRGB1 = gteRGB2;
		gteCODE2 = gteCODE;
		gteR2 = limC1(mac1 >> 4);
		gteG2 = limC2(mac2 >> 4);
		gteB2 = limC3(mac3 >> 4);
	}
	{
		s32 vx = VX_m3(2);
		s32 vy = VY_m3(2);
		s32 vz = VZ_m3(2);
		s32 mac1, mac2, mac3;
	
	#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
		mac1 = A1((((s64)gteL11 * vx) + (gteL12 * vy) + (gteL13 * vz)) >> 12);
		mac2 = A2((((s64)gteL21 * vx) + (gteL22 * vy) + (gteL23 * vz)) >> 12);
		mac3 = A3((((s64)gteL31 * vx) + (gteL32 * vy) + (gteL33 * vz)) >> 12);
	#else
		{
			int aux=MUL((s32)gteL11,vx);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL12,vy)) , "r"(aux));
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL13,vz)) , "r"(aux));
			aux>>=12;
			mac1=A1_32(aux);
		}
		{
			int aux=MUL((s32)gteL21,vx);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL22,vy)) , "r"(aux));
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL23,vz)) , "r"(aux));
			aux>>=12;
			mac2=A2_32(aux);
		}
		{
			int aux=MUL((s32)gteL31,vx);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL32,vy)) , "r"(aux));
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL33,vz)) , "r"(aux));
			aux>>=12;
			mac3=A3_32(aux);
		}
		#endif
		{
			s16 _ir1=limB11(mac1);
			s16 _ir2=limB21(mac2);
			s16 _ir3=limB31(mac3);
#ifdef USE_GTE_FLAG
			gteIR1 = _ir1;
			gteIR2 = _ir2;
			gteIR3 = _ir3;
#endif
		
		#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
			mac1 = A1((((s64)gteRBK << 12) + (gteLR1 * _ir1) + (gteLR2 * _ir2) + (gteLR3 * _ir3)) >> 12);
			mac2 = A2((((s64)gteGBK << 12) + (gteLG1 * _ir1) + (gteLG2 * _ir2) + (gteLG3 * _ir3)) >> 12);
			mac3 = A3((((s64)gteBBK << 12) + (gteLB1 * _ir1) + (gteLB2 * _ir2) + (gteLB3 * _ir3)) >> 12);
		#else
			{
				int aux=MUL(gteLR1,_ir1);
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLR2,_ir2)) , "r"(aux));
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLR3,_ir3)) , "r"(aux));
				aux>>=12;
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteRBK) , "r"(aux));
				mac1=A1_32(aux);
			}
			{
				int aux=MUL(gteLG1,_ir1);
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLG2,_ir2)) , "r"(aux));
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLG3,_ir3)) , "r"(aux));
				aux>>=12;
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteGBK) , "r"(aux));
				mac2=A2_32(aux);
			}
			{
				int aux=MUL(gteLB1,_ir1);
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLB2,_ir2)) , "r"(aux));
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLB3,_ir3)) , "r"(aux));
				aux>>=12;
				asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteBBK) , "r"(aux));
				mac3=A3_32(aux);
			}
		#endif
		}
		gteMAC1 = mac1;
		gteMAC2 = mac2;
		gteMAC3 = mac3;
	}
}

void _gteNCT(psxRegisters *regs) {
	pcsx4all_prof_start_with_pause(PCSX4ALL_PROF_GTE,PCSX4ALL_PROF_CPU);
	int v;
	s32 vx, vy, vz;

#ifdef GTE_LOG
	GTE_LOG("GTE NCT\n");
#endif
#ifdef USE_GTE_FLAG
	gteFLAG = 0;
#endif

	for (v = 0; v < 3; v++) {
		vx = VX_m3(v);
		vy = VY_m3(v);
		vz = VZ_m3(v);
		
		#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
			gteMAC1 = A1((((s64)gteL11 * vx) + (gteL12 * vy) + (gteL13 * vz)) >> 12);
			gteMAC2 = A2((((s64)gteL21 * vx) + (gteL22 * vy) + (gteL23 * vz)) >> 12);
			gteMAC3 = A3((((s64)gteL31 * vx) + (gteL32 * vy) + (gteL33 * vz)) >> 12);
		#else
			int aux=MUL((s32)gteL11,vx);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL12,vy)) , "r"(aux));
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL13,vz)) , "r"(aux));
			aux>>=12;
			gteMAC1=A1_32(aux);

			aux=MUL((s32)gteL21,vx);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL22,vy)) , "r"(aux));
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL23,vz)) , "r"(aux));
			aux>>=12;
			gteMAC2=A2_32(aux);

			aux=MUL((s32)gteL31,vx);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL32,vy)) , "r"(aux));
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteL33,vz)) , "r"(aux));
			aux>>=12;
			gteMAC3=A3_32(aux);
		#endif
		{
			s16 _ir1=limB11(gteMAC1);
			s16 _ir2=limB21(gteMAC2);
			s16 _ir3=limB31(gteMAC3);
#ifdef USE_GTE_FLAG
			gteIR1 = _ir1;
			gteIR2 = _ir2;
			gteIR3 = _ir3;
#endif
		
		#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
			gteMAC1 = A1((((s64)gteRBK << 12) + (gteLR1 * _ir1) + (gteLR2 * _ir2) + (gteLR3 * _ir3)) >> 12);
			gteMAC2 = A2((((s64)gteGBK << 12) + (gteLG1 * _ir1) + (gteLG2 * _ir2) + (gteLG3 * _ir3)) >> 12);
			gteMAC3 = A3((((s64)gteBBK << 12) + (gteLB1 * _ir1) + (gteLB2 * _ir2) + (gteLB3 * _ir3)) >> 12);
		#else
			aux=MUL(gteLR1,_ir1);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLR2,_ir2)) , "r"(aux));
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLR3,_ir3)) , "r"(aux));
			aux>>=12;
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteRBK) , "r"(aux));
			gteMAC1=A1_32(aux);
			
			aux=MUL(gteLG1,_ir1);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLG2,_ir2)) , "r"(aux));
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLG3,_ir3)) , "r"(aux));
			aux>>=12;
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteGBK) , "r"(aux));
			gteMAC2=A2_32(aux);

			aux=MUL(gteLB1,_ir1);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLB2,_ir2)) , "r"(aux));
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLB3,_ir3)) , "r"(aux));
			aux>>=12;
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteBBK) , "r"(aux));
			gteMAC3=A3_32(aux);
		#endif
		}
		
		gteRGB0 = gteRGB1;
		gteRGB1 = gteRGB2;
		gteCODE2 = gteCODE;
		gteR2 = limC1(gteMAC1 >> 4);
		gteG2 = limC2(gteMAC2 >> 4);
		gteB2 = limC3(gteMAC3 >> 4);
	}
	gteIR1 = limB11(gteMAC1);
	gteIR2 = limB21(gteMAC2);
	gteIR3 = limB31(gteMAC3);
	pcsx4all_prof_end_with_resume(PCSX4ALL_PROF_GTE,PCSX4ALL_PROF_CPU);
}

void gteNCT(void) {
	_gteNCT(&psxRegs);
}

void _gteCC_(psxRegisters *regs) {
	{
		s32 mac1, mac2, mac3;
	#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
		mac1 = A1((((s64)gteRBK << 12) + (gteLR1 * gteIR1) + (gteLR2 * gteIR2) + (gteLR3 * gteIR3)) >> 12);
		mac2 = A2((((s64)gteGBK << 12) + (gteLG1 * gteIR1) + (gteLG2 * gteIR2) + (gteLG3 * gteIR3)) >> 12);
		mac3 = A3((((s64)gteBBK << 12) + (gteLB1 * gteIR1) + (gteLB2 * gteIR2) + (gteLB3 * gteIR3)) >> 12);
	#else
		{
			int aux=MUL(gteLR1,gteIR1);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLR2,gteIR2)) , "r"(aux));
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLR3,gteIR3)) , "r"(aux));
			aux>>=12;
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteRBK) , "r"(aux));
			mac1=A1_32(aux);
		}
		{
			int aux=MUL(gteLG1,gteIR1);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLG2,gteIR2)) , "r"(aux));
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLG3,gteIR3)) , "r"(aux));
			aux>>=12;
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteGBK) , "r"(aux));
			mac2=A2_32(aux);
		}
		{
			int aux=MUL(gteLB1,gteIR1);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLB2,gteIR2)) , "r"(aux));
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLB3,gteIR3)) , "r"(aux));
			aux>>=12;
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteBBK) , "r"(aux));
			mac3=A3_32(aux);
		}
	#endif
	{
		s16 _ir1=limB11(mac1);
		s16 _ir2=limB21(mac2);
		s16 _ir3=limB31(mac3);
#ifdef USE_GTE_FLAG
		gteIR1 = _ir1;
		gteIR2 = _ir2;
		gteIR3 = _ir3;
#endif
	
	#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
		mac1 = A1(((s64)gteR * _ir1) >> 8);
		mac2 = A2(((s64)gteG * _ir2) >> 8);
		mac3 = A3(((s64)gteB * _ir3) >> 8);
	#else
		mac1 = A1_32((MUL((s32)gteR,_ir1)) >> 8);
		mac2 = A2_32((MUL((s32)gteG,_ir2)) >> 8);
		mac3 = A3_32((MUL((s32)gteB,_ir3)) >> 8);
	#endif
	}
		gteMAC1 = mac1;
		gteMAC2 = mac2;
		gteMAC3 = mac3;
	}
}

void _gteCC(psxRegisters *regs) {
	pcsx4all_prof_start_with_pause(PCSX4ALL_PROF_GTE,PCSX4ALL_PROF_CPU);
#ifdef GTE_LOG
	GTE_LOG("GTE CC\n");
#endif
#ifdef USE_GTE_FLAG
	gteFLAG = 0;
#endif

	#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
		gteMAC1 = A1((((s64)gteRBK << 12) + (gteLR1 * gteIR1) + (gteLR2 * gteIR2) + (gteLR3 * gteIR3)) >> 12);
		gteMAC2 = A2((((s64)gteGBK << 12) + (gteLG1 * gteIR1) + (gteLG2 * gteIR2) + (gteLG3 * gteIR3)) >> 12);
		gteMAC3 = A3((((s64)gteBBK << 12) + (gteLB1 * gteIR1) + (gteLB2 * gteIR2) + (gteLB3 * gteIR3)) >> 12);
	#else
		int aux=MUL(gteLR1,gteIR1);
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLR2,gteIR2)) , "r"(aux));
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLR3,gteIR3)) , "r"(aux));
		aux>>=12;
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteRBK) , "r"(aux));
		gteMAC1=A1_32(aux);
		
		aux=MUL(gteLG1,gteIR1);
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLG2,gteIR2)) , "r"(aux));
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLG3,gteIR3)) , "r"(aux));
		aux>>=12;
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteGBK) , "r"(aux));
		gteMAC2=A2_32(aux);

		aux=MUL(gteLB1,gteIR1);
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLB2,gteIR2)) , "r"(aux));
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLB3,gteIR3)) , "r"(aux));
		aux>>=12;
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteBBK) , "r"(aux));
		gteMAC3=A3_32(aux);
	#endif
	{
		s16 _ir1=limB11(gteMAC1);
		s16 _ir2=limB21(gteMAC2);
		s16 _ir3=limB31(gteMAC3);
#ifdef USE_GTE_FLAG
		gteIR1 = _ir1;
		gteIR2 = _ir2;
		gteIR3 = _ir3;
#endif
	
	#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
		gteMAC1 = A1(((s64)gteR * _ir1) >> 8);
		gteMAC2 = A2(((s64)gteG * _ir2) >> 8);
		gteMAC3 = A3(((s64)gteB * _ir3) >> 8);
	#else
		gteMAC1 = A1_32((MUL((s32)gteR,_ir1)) >> 8);
		gteMAC2 = A2_32((MUL((s32)gteG,_ir2)) >> 8);
		gteMAC3 = A3_32((MUL((s32)gteB,_ir3)) >> 8);
	#endif
	}
	
	gteIR1 = limB11(gteMAC1);
	gteIR2 = limB21(gteMAC2);
	gteIR3 = limB31(gteMAC3);

	gteRGB0 = gteRGB1;
	gteRGB1 = gteRGB2;
	gteCODE2 = gteCODE;
	gteR2 = limC1(gteMAC1 >> 4);
	gteG2 = limC2(gteMAC2 >> 4);
	gteB2 = limC3(gteMAC3 >> 4);
	pcsx4all_prof_end_with_resume(PCSX4ALL_PROF_GTE,PCSX4ALL_PROF_CPU);
}

void gteCC(void) {
	_gteCC(&psxRegs);
}

#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
#define macINTPL(SHIFT) \
{ \
	gteMAC1 = A1(((gteIR1 << 12) + (gteIR0 * limB10(((s64)gteRFC - gteIR1)))) >> SHIFT); \
	gteMAC2 = A2(((gteIR2 << 12) + (gteIR0 * limB20(((s64)gteGFC - gteIR2)))) >> SHIFT); \
	gteMAC3 = A3(((gteIR3 << 12) + (gteIR0 * limB30(((s64)gteBFC - gteIR3)))) >> SHIFT); \
}
#else
#define macINTPL(SHIFT) \
{ \
	gteMAC1 = A1_32(((gteIR1 << 12) + (MUL(gteIR0,limB10(((s64)gteRFC - gteIR1))))) >> SHIFT); \
	gteMAC2 = A2_32(((gteIR2 << 12) + (MUL(gteIR0,limB20(((s64)gteGFC - gteIR2))))) >> SHIFT); \
	gteMAC3 = A3_32(((gteIR3 << 12) + (MUL(gteIR0,limB30(((s64)gteBFC - gteIR3))))) >> SHIFT); \
}
#endif
void _gteINTPL_s12(psxRegisters *regs) { macINTPL(12); }
void _gteINTPL_s0(psxRegisters *regs) { macINTPL(0); }

void _gteINTPL(psxRegisters *regs) {
	pcsx4all_prof_start_with_pause(PCSX4ALL_PROF_GTE,PCSX4ALL_PROF_CPU);
	int shift = (GTE_SF(gteop)?12:0);
	int lm = GTE_LM(gteop);

#ifdef GTE_LOG
	GTE_LOG("GTE INTPL\n");
#endif
#ifdef USE_GTE_FLAG
	gteFLAG = 0;
#endif

	#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
		gteMAC1 = A1(((gteIR1 << 12) + (gteIR0 * limB10(((s64)gteRFC - gteIR1)))) >> shift);
		gteMAC2 = A2(((gteIR2 << 12) + (gteIR0 * limB20(((s64)gteGFC - gteIR2)))) >> shift);
		gteMAC3 = A3(((gteIR3 << 12) + (gteIR0 * limB30(((s64)gteBFC - gteIR3)))) >> shift);
	#else
		gteMAC1 = A1_32(((gteIR1 << 12) + (MUL(gteIR0,limB10(((s64)gteRFC - gteIR1))))) >> shift);
		gteMAC2 = A2_32(((gteIR2 << 12) + (MUL(gteIR0,limB20(((s64)gteGFC - gteIR2))))) >> shift);
		gteMAC3 = A3_32(((gteIR3 << 12) + (MUL(gteIR0,limB30(((s64)gteBFC - gteIR3))))) >> shift);
	#endif
	
	gteIR1 = limB1(gteMAC1, lm);
	gteIR2 = limB2(gteMAC2, lm);
	gteIR3 = limB3(gteMAC3, lm);
	gteRGB0 = gteRGB1;
	gteRGB1 = gteRGB2;
	gteCODE2 = gteCODE;
	gteR2 = limC1(gteMAC1 >> 4);
	gteG2 = limC2(gteMAC2 >> 4);
	gteB2 = limC3(gteMAC3 >> 4);
	pcsx4all_prof_end_with_resume(PCSX4ALL_PROF_GTE,PCSX4ALL_PROF_CPU);
}

void gteINTPL(void) {
	_gteINTPL(&psxRegs);
}

void _gteCDP_(psxRegisters *regs) {
	{
		s32 mac1, mac2, mac3;
	#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
		mac1 = A1((((s64)gteRBK << 12) + (gteLR1 * gteIR1) + (gteLR2 * gteIR2) + (gteLR3 * gteIR3)) >> 12);
		mac2 = A2((((s64)gteGBK << 12) + (gteLG1 * gteIR1) + (gteLG2 * gteIR2) + (gteLG3 * gteIR3)) >> 12);
		mac3 = A3((((s64)gteBBK << 12) + (gteLB1 * gteIR1) + (gteLB2 * gteIR2) + (gteLB3 * gteIR3)) >> 12);
	#else
		{
			int aux=MUL(gteLR1,gteIR1);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLR2,gteIR2)) , "r"(aux));
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLR3,gteIR3)) , "r"(aux));
			aux>>=12;
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteRBK) , "r"(aux));
			mac1=A1_32(aux);
		}
		{
			int aux=MUL(gteLG1,gteIR1);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLG2,gteIR2)) , "r"(aux));
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLG3,gteIR3)) , "r"(aux));
			aux>>=12;
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteGBK) , "r"(aux));
			mac2=A2_32(aux);
		}
		{
			int aux=MUL(gteLB1,gteIR1);
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLB2,gteIR2)) , "r"(aux));
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLB3,gteIR3)) , "r"(aux));
			aux>>=12;
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteBBK) , "r"(aux));
			mac3=A3_32(aux);
		}
	#endif
	{
		s16 _ir1=limB11(mac1);
		s16 _ir2=limB21(mac2);
		s16 _ir3=limB31(mac3);
#ifdef USE_GTE_FLAG
		gteIR1 = _ir1;
		gteIR2 = _ir2;
		gteIR3 = _ir3;
#endif
	
	#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
		mac1 = A1(((((s64)gteR << 4) * _ir1) + (gteIR0 * limB10(gteRFC - ((gteR * _ir1) >> 8)))) >> 12);
		mac2 = A2(((((s64)gteG << 4) * _ir2) + (gteIR0 * limB20(gteGFC - ((gteG * _ir2) >> 8)))) >> 12);
		mac3 = A3(((((s64)gteB << 4) * _ir3) + (gteIR0 * limB30(gteBFC - ((gteB * _ir3) >> 8)))) >> 12);
	#else
		{
			int aux;
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(((s32)gteR << 4),_ir1)) , "r"(MUL(gteIR0,limB10(gteRFC - ((MUL(gteR,_ir1)) >> 8)))));
			mac1=A1_32(aux>>12);
		}
		{
			int aux;
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(((s32)gteG << 4),_ir2)) , "r"(MUL(gteIR0,limB20(gteGFC - ((MUL(gteG,_ir2)) >> 8)))));
			mac2=A2_32(aux>>12);
		}
		{
			int aux;
			asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(((s32)gteB << 4),_ir3)) , "r"(MUL(gteIR0,limB30(gteBFC - ((MUL(gteB,_ir3)) >> 8)))));
			mac3=A3_32(aux>>12);
		}
	#endif
	}
		gteMAC1 = mac1;
		gteMAC2 = mac2;
		gteMAC3 = mac3;
	}
}

void _gteCDP(psxRegisters *regs) {
	pcsx4all_prof_start_with_pause(PCSX4ALL_PROF_GTE,PCSX4ALL_PROF_CPU);
#ifdef GTE_LOG
	GTE_LOG("GTE CDP\n");
#endif
#ifdef USE_GTE_FLAG
	gteFLAG = 0;
#endif

	#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
		gteMAC1 = A1((((s64)gteRBK << 12) + (gteLR1 * gteIR1) + (gteLR2 * gteIR2) + (gteLR3 * gteIR3)) >> 12);
		gteMAC2 = A2((((s64)gteGBK << 12) + (gteLG1 * gteIR1) + (gteLG2 * gteIR2) + (gteLG3 * gteIR3)) >> 12);
		gteMAC3 = A3((((s64)gteBBK << 12) + (gteLB1 * gteIR1) + (gteLB2 * gteIR2) + (gteLB3 * gteIR3)) >> 12);
	#else
		int aux=MUL(gteLR1,gteIR1);
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLR2,gteIR2)) , "r"(aux));
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLR3,gteIR3)) , "r"(aux));
		aux>>=12;
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteRBK) , "r"(aux));
		gteMAC1=A1_32(aux);
		
		aux=MUL(gteLG1,gteIR1);
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLG2,gteIR2)) , "r"(aux));
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLG3,gteIR3)) , "r"(aux));
		aux>>=12;
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteGBK) , "r"(aux));
		gteMAC2=A2_32(aux);

		aux=MUL(gteLB1,gteIR1);
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLB2,gteIR2)) , "r"(aux));
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(gteLB3,gteIR3)) , "r"(aux));
		aux>>=12;
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(gteBBK) , "r"(aux));
		gteMAC3=A3_32(aux);
	#endif
	{
		s16 _ir1=limB11(gteMAC1);
		s16 _ir2=limB21(gteMAC2);
		s16 _ir3=limB31(gteMAC3);
#ifdef USE_GTE_FLAG
		gteIR1 = _ir1;
		gteIR2 = _ir2;
		gteIR3 = _ir3;
#endif
	
	#if !defined(__arm__) || defined(USE_GTE_EXTRA_EXACT)
		gteMAC1 = A1(((((s64)gteR << 4) * _ir1) + (gteIR0 * limB10(gteRFC - ((gteR * _ir1) >> 8)))) >> 12);
		gteMAC2 = A2(((((s64)gteG << 4) * _ir2) + (gteIR0 * limB20(gteGFC - ((gteG * _ir2) >> 8)))) >> 12);
		gteMAC3 = A3(((((s64)gteB << 4) * _ir3) + (gteIR0 * limB30(gteBFC - ((gteB * _ir3) >> 8)))) >> 12);
	#else
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(((s32)gteR << 4),_ir1)) , "r"(MUL(gteIR0,limB10(gteRFC - ((MUL(gteR,_ir1)) >> 8)))));
		gteMAC1=A1_32(aux>>12);
	
		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(((s32)gteG << 4),_ir2)) , "r"(MUL(gteIR0,limB20(gteGFC - ((MUL(gteG,_ir2)) >> 8)))));
		gteMAC2=A2_32(aux>>12);

		asm ("qadd %0, %1, %2" : "=r" (aux) : "r"(MUL(((s32)gteB << 4),_ir3)) , "r"(MUL(gteIR0,limB30(gteBFC - ((MUL(gteB,_ir3)) >> 8)))));
		gteMAC3=A3_32(aux>>12);
	#endif
	}
	
	gteIR1 = limB11(gteMAC1);
	gteIR2 = limB21(gteMAC2);
	gteIR3 = limB31(gteMAC3);

	gteRGB0 = gteRGB1;
	gteRGB1 = gteRGB2;
	gteCODE2 = gteCODE;
	gteR2 = limC1(gteMAC1 >> 4);
	gteG2 = limC2(gteMAC2 >> 4);
	gteB2 = limC3(gteMAC3 >> 4);
	pcsx4all_prof_end_with_resume(PCSX4ALL_PROF_GTE,PCSX4ALL_PROF_CPU);
}

void gteCDP(void) {
	_gteCDP(&psxRegs);
}

