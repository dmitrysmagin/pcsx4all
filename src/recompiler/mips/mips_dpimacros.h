/* Macros for DPI ops, manually generated from no template 
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
