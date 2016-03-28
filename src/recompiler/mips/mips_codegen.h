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

void write_to_file(u32 val);

#define MIPS_EMIT(p, i) write32(i);

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

#define MIPS_PUSH(p, reg) MIPS_EMIT(p, 0x27bdfffc /* addiu sp, sp, -4 */); MIPS_EMIT(p, 0xafa00000 | (reg << 16));
#define MIPS_POP(p, reg) MIPS_EMIT(p, 0x8fa00000 | (reg << 16)); MIPS_EMIT(p, 0x27bd0004 /* addiu sp, sp, 4 */);

#define MIPS_LDR_IMM(p, rd, rn, imm) MIPS_EMIT(p, 0x8c000000 | ((rn) << 21) | ((rd) << 16) | ((imm) & 0xffff))
#define MIPS_STR_IMM(p, rd, rn, imm) MIPS_EMIT(p, 0xac000000 | ((rn) << 21) | ((rd) << 16) | ((imm) & 0xffff))

#define MIPS_ADDIU(p, rt, rs, imm) MIPS_EMIT(p, 0x24000000 | ((rs) << 21) | ((rt) << 16) | ((imm) & 0xffff))

#endif /* MIPS_CG_H */

