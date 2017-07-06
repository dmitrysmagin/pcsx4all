/*
 * mips_codegen.cpp
 *
 * Copyright (c) 2017 Dmitry Smagin / Daniel Silsby
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

#include "psxcommon.h"
#include "r3000a.h"

#include "mem_mapping.h"
#include "mips_codegen.h"

/* Emit code that will convert the base reg of a load/store in PS1 code
 *  into a base reg that can be used to access psxM[]. The register
 *  specified by 'tmp_reg' can be overwritten in the process.
 */
void emitAddressConversion(u32 dst_reg, u32 src_reg, u32 tmp_reg)
{
	if (psx_mem_mapped) {
		// METHOD 1 (best): psxM is mmap'd to simple fixed virtual address,
		//                  mirrored, and expansion-ROM/scratchpad/hardware-I/O
		//                  regions (psxP,psxH) are mapped at offset 0x0f00_0000.
		//                  Caller expects access to both RAM and these regions.
		//                  Thus, we need lower 28 bits of original base address.

		LUI(dst_reg, (PSX_MEM_VADDR >> 16));
#ifdef HAVE_MIPS32R2_EXT_INS
		INS(dst_reg, src_reg, 0, 28);   // dst_reg = PSX_MEM_VADDR | (src_reg & 0x0fffffff)
#else
		SLL(tmp_reg, src_reg, 4);
		SRL(tmp_reg, tmp_reg, 4);
		OR(dst_reg, dst_reg, tmp_reg);
#endif
	} else {
		// METHOD 2: Apparently mmap() and/or mirroring is unavailable, so psxM
		//           could be anywhere! RAM isn't mirrored virtually, so we
		//           must mask out all but lower 21 bits of src_reg here
		//           to handle 2MB PS1 RAM mirrors. The caller knows it can
		//           only use converted base reg to access PS1 RAM.

		LW(dst_reg, PERM_REG_1, off(psxM));
#ifdef HAVE_MIPS32R2_EXT_INS
		EXT(tmp_reg, src_reg, 0, 21);  // tmp_reg = src_reg & 0x001f_ffff
#else
		SLL(tmp_reg, src_reg, 11);
		SRL(tmp_reg, tmp_reg, 11);
#endif
		ADDU(dst_reg, dst_reg, tmp_reg);
	}
}


/* Get u32 opcode val at location in PS1 code */
#define OPCODE_AT(loc) (*(u32 *)((char *)PSXM((loc))))

enum {
	DISCARD_TYPE_DIVBYZERO = 0,
	DISCARD_TYPE_DIVBYZERO_AND_OVERFLOW,
	DISCARD_TYPE_COUNT
};

static const char *mipsrec_discard_type_str[] =
{
	"GCC AUTO-GENERATED DIV-BY-ZERO EXCEPTION CHECK",
	"GCC AUTO-GENERATED DIV-BY-ZERO,OVERFLOW EXCEPTION CHECK"
};

/*
 * Scans for compiler auto-generated instruction sequences that PS1 GCC
 *  added immediately after most DIV/DIVU opcodes. The sequences check for
 *  div-by-0 (for DIV/DIVU), followed by check for signed overflow (for DIV).
 * If either condition was found, original code would have executed
 *  a BREAK opcode, causing BIOS handler to crash the PS1 with a
 *  SystemErrorUnresolvedException, so these code sequences merely bloat
 *  the recompiled code. Values written to $at in these compiler-generated
 *  sequences were not propagated to code outside them.
 * Not only can these sequences be omitted when recompiling, but their
 *  presence allows the DIV/DIVU emitters to know when they can avoid adding
 *  their own check-and-emulate-PS1-div-by-zero-result sequences.
 * NOTE: Rarely, a MFHI and/or MFLO will separate the DIV/DIVU from the
 *  check sequence, so recScanForSequentialMFHI_MFLO() is also provided.
 *  Even more rarely, they can be separated by other types of instructions,
 *  which we'll leave handled by some fancier future optimizer (TODO).
 *
 * Returns: # of opcodes found, 0 if sequence not found.
 */
int rec_scan_for_div_by_zero_check_sequence(u32 code_loc)
{
	int instr_cnt = 0;

	// Div-by-zero check always came first in sequence
	if (((OPCODE_AT(code_loc   ) & 0xfc1fffff) == 0x14000002) &&  // bne ???, zero, +8
	     (OPCODE_AT(code_loc+4 )               == 0         ) &&  // nop
	     (OPCODE_AT(code_loc+8 )               == 0x0007000d))    // break 0x1c00
	{ instr_cnt += 3; }

	// If opcode was DIV, it also checked for signed-overflow
	if ( instr_cnt &&
	     (OPCODE_AT(code_loc+12)               == 0x2401ffff) &&  // li at, -1
	    ((OPCODE_AT(code_loc+16) & 0xfc1fffff) == 0x14010004) &&  // bne ???, at, +16
	     (OPCODE_AT(code_loc+20)               == 0x3c018000) &&  // lui at, 0x8000
	    ((OPCODE_AT(code_loc+24) & 0x141fffff) == 0x14010002) &&  // bne ???, at, +8
	     (OPCODE_AT(code_loc+28)               == 0         ) &&  // nop
	     (OPCODE_AT(code_loc+32)               == 0x0006000d))    // break 0x1800
	{ instr_cnt += 6; }

	return instr_cnt;
}

/*
 * Returns: # of sequential MFHI/MFLO opcodes at PS1 code loc, 0 if none found.
 */
int rec_scan_for_MFHI_MFLO_sequence(u32 code_loc)
{
	int instr_cnt = 0;

	while (((OPCODE_AT(code_loc ) & 0xffff07ff) == 0x00000010) ||  // mfhi ???
	       ((OPCODE_AT(code_loc ) & 0xffff07ff) == 0x00000012))    // mflo ???
	{
		instr_cnt++;
		code_loc += 4;
	}

	return instr_cnt;
}

/*
 * Scans for sequential instructions at PS1 code location that can be safely
 *  ignored/discarded when recompiling. Stops when first sequence is found.
 *  Int ptr arg 'discard_type' is set to type of sequence found.
 *
 * Returns: # of sequential opcodes that can be discarded, 0 if none.
 */
int rec_discard_scan(u32 code_loc, int *discard_type)
{
	int instr_cnt;

	instr_cnt = rec_scan_for_div_by_zero_check_sequence(code_loc);
	if (instr_cnt) {
		if (discard_type) {
			if (instr_cnt > 3)
				*discard_type = DISCARD_TYPE_DIVBYZERO_AND_OVERFLOW;
			else
				*discard_type = DISCARD_TYPE_DIVBYZERO;
		}

		return instr_cnt;
	}

	return 0;
}

/*
 * Returns: Char string ptr describing discardable sequence 'discard_type'
 */
const char* rec_discard_type_str(int discard_type)
{
	if (discard_type >= 0 && discard_type < DISCARD_TYPE_COUNT)
		return mipsrec_discard_type_str[discard_type];
	else
		return "ERROR: Discard type out of range";
}
