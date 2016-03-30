/*
 * mips-codegen.h
 *
 * Copyright (c) 2009 Ulrich Hecht
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */


#ifndef MIPS_CG_H
#define MIPS_CG_H

typedef enum {
	MIPSREG_V0 = 2,
	MIPSREG_V1,

	MIPSREG_A0 = 4,
	MIPSREG_A1,
	MIPSREG_A2,
	MIPSREG_A3,

	MIPSREG_T0 = 8,
	MIPSREG_T1,
	MIPSREG_T2,
	MIPSREG_T3,
	MIPSREG_T4,
	MIPSREG_T5,
	MIPSREG_T6,
	MIPSREG_T7,

	MIPSREG_RA = 0x1f,
	MIPSREG_S8 = 0x1e,
	MIPSREG_S0 = 0x10,
	MIPSREG_S1,
	MIPSREG_S2,
	MIPSREG_S3,
	MIPSREG_S4,
	MIPSREG_S5,
	MIPSREG_S6,
	MIPSREG_S7,
} MIPSReg;

#define MIPS_EMIT(p, i) \
	*recMem++ = (u32)(i);

#define MIPS_PUSH(p, reg) \
do { \
	MIPS_EMIT(p, 0x27bdfffc /* addiu sp, sp, -4 */); \
	MIPS_EMIT(p, 0xafa00000 | (reg << 16)); \
} while (0)

#define MIPS_POP(p, reg) \
do { \
	MIPS_EMIT(p, 0x8fa00000 | (reg << 16)); \
	MIPS_EMIT(p, 0x27bd0004 /* addiu sp, sp, 4 */); \
} while (0)

#define MIPS_LDR_IMM(p, rd, rn, imm) MIPS_EMIT(p, 0x8c000000 | ((rn) << 21) | ((rd) << 16) | ((imm) & 0xffff))
#define MIPS_STR_IMM(p, rd, rn, imm) MIPS_EMIT(p, 0xac000000 | ((rn) << 21) | ((rd) << 16) | ((imm) & 0xffff))

#define MIPS_ADDIU(p, rt, rs, imm) MIPS_EMIT(p, 0x24000000 | ((rs) << 21) | ((rt) << 16) | ((imm) & 0xffff))

#define MIPS_MOV_REG_IMM8(p, reg, imm8) \
	MIPS_EMIT(p, 0x34000000 | ((reg) << 16) | ((short)imm8)) /* ori reg, zero, imm8 */

#define MIPS_MOV_REG_REG(p, rd, rs) \
	MIPS_EMIT(p, 0x00000021 | ((rs) << 21) | ((rd) << 11)); /* move rd, rs */

#define MIPS_AND_REG_REG(p, rd, rn, rm) \
	MIPS_EMIT(p, 0x00000024 | ((rn) << 21) | ((rm) << 16) | ((rd) << 11))

#define MIPS_XOR_REG_IMM(p, rd, rn, imm16) \
	MIPS_EMIT(p, 0x38000000 | ((rn) << 21) | ((rd) << 16) | ((imm16) & 0xffff))

#define MIPS_XOR_REG_REG(p, rd, rn, rm) \
	MIPS_EMIT(p, 0x00000026 | ((rn) << 21) | ((rm) << 16) | ((rd << 11)));

#define MIPS_SUB_REG_REG(p, rd, rn, rm) \
	MIPS_EMIT(p, 0x00000023 | ((rn) << 21) | ((rm) << 16) | ((rd) << 11)) /* subu */

#define MIPS_ADD_REG_REG(p, rd, rn, rm) \
	MIPS_EMIT(p, 0x00000021 | ((rn) << 21) | ((rm) << 16) | ((rd) << 11)) /* addu */

#define MIPS_ORR_REG_REG(p, rd, rn, rm) \
	MIPS_EMIT(p, 0x00000025 | ((rn) << 21) | ((rm) << 16) | ((rd) << 11))

/* start of the recompiled block */
#define rec_recompile_start() \
do { \
	if (loadedpermregs == 0) { \
		LoadImmediate32((u32)&psxRegs, PERM_REG_1); \
		loadedpermregs = 1; \
	} \
} while (0)

/* end of the recompiled block */
#define rec_recompile_end() \
do { \
	MIPS_EMIT(MIPS_POINTER, 0x00000008 | (MIPSREG_V0 << 21)); /* jr v0 */ \
	MIPS_EMIT(MIPS_POINTER, 0); /* nop */ \
} while (0)

/* call func */
#define CALLFunc(func) \
do { \
	MIPS_EMIT(MIPS_POINTER, 0x0c000000 | ((func & 0x0fffffff) >> 2)); /* jal func */ \
	MIPS_EMIT(MIPS_POINTER, 0); /* nop */ \
} while (0)

#define mips_relative_offset(source, offset, next) \
	((((u32)(offset) - ((u32)(source) + (next))) >> 2) & 0xFFFF)

#define LoadImmediate32(imm, ireg) \
do { \
	MIPS_EMIT(0, 0x3c000000 | (ireg << 16) | ((imm) >> 16)); /* lui */ \
	MIPS_EMIT(0, 0x34000000 | (ireg << 21) | (ireg << 16) | ((imm) & 0xffff)); /* ori */ \
} while (0)

#endif /* MIPS_CG_H */

