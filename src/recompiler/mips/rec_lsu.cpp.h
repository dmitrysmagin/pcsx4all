/*
 * Copyright (c) 2009 Ulrich Hecht
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


/***********************************************
 * Options that can be disabled for debugging: *
 ***********************************************/

// NOTE: Also see options 'USE_CONST_ADDRESSES' and 'USE_CONST_FUZZY_ADDRESSES'
//       defined elsewhere.

// Inline access to known-const HW,Scratchpad addresses (see rec_lsu_hw.cpp.h)
#define USE_DIRECT_HW_ACCESS

// Assume that stores using $k0,$k1,$gp,$sp as base registers aren't used
//  to modify code. Code invalidation sequence will not be emitted.
#define SKIP_CODE_INVALIDATION_FOR_SOME_BASE_REGS

// Assume that loads/stores using $zero,$k0,$k1,$gp,$sp as base registers always
//  go to RAM/scratchpad. Address-range-check and indirect-access sequences
//  will not be emitted.
#define SKIP_ADDRESS_RANGE_CHECK_FOR_SOME_BASE_REGS

// Allow series of loads/stores to include a store found in a branch delay slot,
//  as long as it meets the requirements of inclusion (uses same base reg).
#define INCLUDE_STORES_FOUND_IN_BD_SLOTS

// 2MB of PSX RAM (psxM) is now mirrored four times in virtual address
//  space, like a real PS1. This allows skipping mirror-region boundary
//  checks which special-cased loads/stores that crossed the boundary,
//  the 'Einhander' game fix. See notes in mem_mapping.cpp.
#define SKIP_SAME_2MB_REGION_CHECK

// Bypass 'writeok' cache-isolation check before stores. We now backup first
//  64KB of PSX RAM when cache is isolated, and restore it when unisolated.
//  We simply let the cache-flush stores go through to RAM (very temporarily).
//  See comments in psxmem.cpp psxMemWrite32_CacheCtrlPort().
#define SKIP_WRITEOK_CHECK

// If PSX mem is virtually mapped/mirrored, we only need to call C funcs when
//  accessing HW I/O ports, ROM, or cache control port. We've added support to
//  read/write funcs in psxhw.cpp to handle the latter two. Directly calling
//  psxHwRead*/Write* reduces TLB/Icache pressure because psxMemRead*/Write*
//  would only just end up calling those funcs every time.
//  NOTE: Only when the optimizations listed below are enabled are we sure
//        that all indirect accesses must be going to HW I/O, ROM, or cache port.
#if defined(USE_DIRECT_MEM_ACCESS) && defined(USE_DIRECT_HW_ACCESS) && defined(SKIP_SAME_2MB_REGION_CHECK) && defined(SKIP_WRITEOK_CHECK)
	#define USE_HW_FUNCS_FOR_INDIRECT_ACCESS
#endif


#define LSU_OPCODE(insn, rt, rn, imm) \
	write32((insn) | ((rn) << 21) | ((rt) << 16) | ((imm) & 0xffff))

//#define LOG_LOAD_STORE_SERIES

static s16 imm_max, imm_min;

static u32 LWL_MASKSHIFT[8] = { 0xffffff, 0xffff, 0xff, 0,
                                24, 16, 8, 0 };
static u32 LWR_MASKSHIFT[8] = { 0, 0xff000000, 0xffff0000, 0xffffff00,
                                0, 8, 16, 24 };

static u32 SWL_MASKSHIFT[8] = { 0xffffff00, 0xffff0000, 0xff000000, 0,
                                24, 16, 8, 0 };
static u32 SWR_MASKSHIFT[8] = { 0, 0xff, 0xffff, 0xffffff,
                                0, 8, 16, 24 };

#include "rec_lsu_hw.cpp.h"  // Direct HW I/O


/* Emit instruction(s) to convert address in PSX reg 'op_rs' (allocated to host
 *  reg 'rs') to a host address. Caller specifies 'tmp_reg' as a host reg we can
 *  use to help create the converted address. This should always be a different
 *  reg than 'desired_reg'.
 *
 * Returns: The host reg containing the converted address. For now, this is
 *          always 'desired_reg', but if we ever implement conversion caching,
 *          it might be different.
 *
 * IMPORTANT: We promise to emit at least one instruction here, so caller can
 *            always count on us safely filling any BD slots it needs to.
 *            We also promise to never return 'rs': the caller doesn't
 *            have to keep it locked after calling here if it doesn't want to.
 *
 * NOTE: This function is shared with rec_gte.cpp.h
 *
 */
static u32 emitAddressConversion(const u32 op_rs,
                                 const u32 rs,
                                 const u32 desired_reg,
                                 const u32 tmp_reg)
{
	if (psx_mem_mapped)
	{
		// METHOD 1 : psxM is mmap'd to fixed virtual address, RAM is mirrored,
		// (best)      and expansion-ROM/scratchpad/hardware-I/O regions (psxP,
		//             psxH) are mapped at offset 0x0f00_0000. Start of virtual
		//             mapping is an address with lower 28 bits clear.

		LUI(desired_reg, (PSX_MEM_VADDR >> 16));
#ifdef HAVE_MIPS32R2_EXT_INS
		INS(desired_reg, rs, 0, 28);
#else
		SLL(tmp_reg, rs, 4);
		SRL(tmp_reg, tmp_reg, 4);
		OR(desired_reg, desired_reg, tmp_reg);
#endif
	} else
	{
		// METHOD 2: Apparently mmap() and/or mirroring is unavailable, so psxM
		//            could be anywhere! RAM isn't mirrored virtually, so we
		//            must mask out all but lower 21 bits of rs here to
		//            handle the three 2MB PS1 RAM mirrors. Caller knows it can
		//            only use the converted base reg to access PS1 RAM.

		LW(desired_reg, PERM_REG_1, off(psxM));
#ifdef HAVE_MIPS32R2_EXT_INS
		EXT(tmp_reg, rs, 0, 21);  // tmp_reg = rs & 0x001f_ffff
#else
		SLL(tmp_reg, rs, 11);
		SRL(tmp_reg, tmp_reg, 11);
#endif
		ADDU(desired_reg, desired_reg, tmp_reg);
	}

	return desired_reg;
}

/* Emit no code invalidations for PSX base reg 'op_rs'? */
static inline bool LSU_skip_code_invalidation(const u32 op_rs)
{
#ifdef SKIP_CODE_INVALIDATION_FOR_SOME_BASE_REGS
	// Skip code invalidation when base reg is obviously not
	//  involved in code modification ($k0,$k1,$gp,$sp).
	if (op_rs >= 26 && op_rs <= 29)
		return true;
#endif

#ifdef USE_CONST_FUZZY_ADDRESSES
	// Skip code invalidation when base reg value is not known-const,
	//  but at least known to be somewhere outside RAM.
	//  Probably a scratchpad/ROM static array access in original PS1 code.
	if (IsFuzzyNonramAddr(op_rs))
		return true;
#endif

	return false;
}

/* Emit only inlined direct accesses for PSX base reg 'op_rs'? */
static inline bool LSU_use_only_direct_access(const u32 op_rs)
{
	// NOTE: If 'psx_mem_mapped' is true, all valid PS1 addresses between begin
	//       of RAM and end of scratchpad are virtually mapped/mirrored.

#ifdef SKIP_ADDRESS_RANGE_CHECK_FOR_SOME_BASE_REGS
	if (psx_mem_mapped) {
		// Skip address range check when base register in use is obviously
		//  going to access RAM/scratchpad ($zero,$k0,$k1,$gp,$sp).
		//  Zero reg can only access lower 64KB RAM region reserved for BIOS.
		if ((op_rs >= 26 && op_rs <= 29) || (op_rs == 0))
			return true;
	}
#endif

#ifdef USE_CONST_FUZZY_ADDRESSES
	// Skip address range check when base reg value is not known-const,
	//  but at least known to be somewhere in RAM/scratchpad.
	//  Probably a RAM/scratchpad static array access in original PS1 code.
	if (IsFuzzyRamAddr(op_rs))
		return true;

	if (psx_mem_mapped && IsFuzzyScratchpadAddr(op_rs))
		return true;
#endif

	return false;
}

/* Emit only indirect C mem accesses for PSX base reg 'op_rs'? */
static inline bool LSU_use_only_indirect_access(const u32 op_rs)
{
	// NOTE: If 'psx_mem_mapped' is true, all valid PS1 addresses between begin
	//       of RAM and end of scratchpad are virtually mapped/mirrored.

#ifdef USE_CONST_FUZZY_ADDRESSES
	if (psx_mem_mapped) {
		if (IsFuzzyNonramAddr(op_rs) && !IsFuzzyScratchpadAddr(op_rs)) {
			//  Exact value of base reg is unknown, but it's at least known to
			// not be a RAM or scratchpad address. Only indirect accesses
			// should be emitted.

			return true;
		}
	} else {
		if (IsFuzzyNonramAddr(op_rs)) {
			//  Exact value of base reg is unknown, but it's at least known to
			// not be a RAM address. Only indirect accesses should be emitted.

			return true;
		}
	}
#endif

	return false;
}


/* Return count of the number of consecutive loads and/or stores starting at
   current instruction. All loads/stores in the series must share a common
   base register. Any intervening NOPs encountered are included in the count.
   If 'pc_of_last_store_in_series' is not NULL, it will be set to the last store
   found in the series, or set to impossible value of 1 if no stores were found.
 */
static int count_loads_stores(u32 *pc_of_last_store_in_series, bool *series_includes_bd_slot_store)
{
	int count = 0;
	u32 PC = pc - 4;
	const u32 rs = _Rs_;


#ifdef LOG_LOAD_STORE_SERIES
	int num_nops = 0;
	int num_loads = 0;
	int num_stores = 0;
#endif

	imm_min = imm_max = _Imm_;

	bool store_found = false;
	bool bd_slot_store_found = false;
	int nops_at_end = 0;
	for (;;)
	{
		const u32 opcode = OPCODE_AT(PC);
		PC += 4;

		if (opcode == 0) {
#ifdef LOG_LOAD_STORE_SERIES
			num_nops++;
#endif
			nops_at_end++;
			count++;  // Include the NOP in the count
			continue;
		}

		bool is_store = opcodeIsStore(opcode);
		bool is_load  = is_store ? false : opcodeIsLoad(opcode);

#ifdef LOG_LOAD_STORE_SERIES
		num_stores += is_store;
		num_loads  += is_load;
#endif

		if (is_load || is_store) {
			// All loads/stores in series must share a base register
			if (rs != _fRs_(opcode)) {
				// Break out of for-loop, we can't include the current opcode.
				break;
			}
#ifdef INCLUDE_STORES_FOUND_IN_BD_SLOTS
		} else if (opcodeIsBranchOrJump(opcode)) {
			const u32 bd_slot_opcode = OPCODE_AT(PC);

			// We can include a store in the BD-slot if it abides by the usual
			//  rule: it must share the same base reg as other loads/stores. If
			//  store reads $ra reg, don't try to include it: JAL/JALR/BAL/etc
			//  would give it a new value. It'd be rare anyway, so no loss.
			if (opcodeIsStore(bd_slot_opcode) &&
			    (_fRs_(bd_slot_opcode) == rs) && (_fRt_(bd_slot_opcode) != 31) )
			{
				// Include this branch/jump opcode in count. Emitter will skip it.
				count++;
				bd_slot_store_found = true;
				nops_at_end = 0;
				// Loop around again for BD-slot opcode, we'll terminate after that.
				continue;
			} else {
				// Break out of for-loop, we can't include the current opcode.
				break;
			}
#endif // INCLUDE_STORES_FOUND_IN_BD_SLOTS
		} else {
			// Break out of for-loop, we can't include the current opcode.
			break;
		}

		nops_at_end = 0;
		count++;

		// Update min and max immediate values for the series
		if (_fImm_(opcode) > imm_max) imm_max = _fImm_(opcode);
		if (_fImm_(opcode) < imm_min) imm_min = _fImm_(opcode);

		if (is_store) {
			store_found = true;
			if (pc_of_last_store_in_series != NULL)
				*pc_of_last_store_in_series = (PC-4);
		}

		// If in branch delay slot, limit count to 1
		if (branch)
			break;

		// For loads, check if base reg got overwritten. Stop series here if so.
		if (is_load && (_fRt_(opcode) == _fRs_(opcode)))
			break;

		// If we found a BD slot store, at this point it will already
		//  have been included in the series and we stop immediately.
		if (bd_slot_store_found)
			break;
	}

	// If no stores were found, always assign impossible val '1'. If we did find
	//  a store, we've already set this to correct value inside the for-loop.
	if (!store_found && (pc_of_last_store_in_series != NULL))
		*pc_of_last_store_in_series = 1;

	// Notify caller if the last store in the series lied in a BD slot.
	if (series_includes_bd_slot_store != NULL)
		*series_includes_bd_slot_store = bd_slot_store_found;

	// Don't include any NOPs at the end of sequence in the reported count..
	//  only count ones lying between the load/store opcodes
	count -= nops_at_end;

#ifdef LOG_LOAD_STORE_SERIES
	// Only log this series if we found more than one load/store
	if (count > 1) {
		num_nops -= nops_at_end;
		printf("LOAD/STORE SERIES count: %d  num_loads: %d  num_stores: %d  imm_min: %d  imm_max: %d\n",
			count, num_loads, num_stores, imm_min, imm_max);
		if (bd_slot_store_found)
			printf("(SERIES INCLUDES STORE IN BD SLOT BELOW)\n");
	}
#endif

	return count;
}

/* Emit a series of loads/stores to a common base address.
 *  If there are stores, code invalidation might also be emitted.
 *  If 'force_indirect' param is true, only indirect C func calls will
 *  be emitted. Otherwise, we'll decide here what we can get away with.
 *  Worst case scenario is that we emit both direct and indirect accesses.
 */
static void general_loads_stores(const int  count,
                                 const u32  pc_of_last_store_in_series,
                                 const bool force_indirect)
{
	// All loads/stores in series share the same base reg.
	const u32 op_rs = _Rs_;
	const u32 rs = regMipsToHost(op_rs, REG_LOAD, REG_REGISTER);

	// Allocate the first 'rt' reg of the series. This helps reduce load stall
	//  on 'rs' here and on first use of 'rt' later. It also lets the direct and
	//  indirect codepaths share the code emitted for the reg allocation, if any.
	u32 first_rt_allocation = 0;
	if (opcodeIsLoad(OPCODE_AT(pc-4))) {
		// LWL/LWR load is special: needs to load existing contents of reg
		const u32  insn = OPCODE_AT(pc-4) & 0xfc000000;
		const bool is_lwl_lwr = (insn == 0x88000000 || insn == 0x98000000);
		first_rt_allocation = regMipsToHost(_Rt_, (is_lwl_lwr ? REG_LOAD : REG_FIND), REG_REGISTER);
	} else {
		first_rt_allocation = regMipsToHost(_Rt_, REG_LOAD, REG_REGISTER);
	}

	u32 *backpatch_label_exit_1 = 0;
	u32 *backpatch_label_exit_2 = 0;

#ifdef USE_DIRECT_MEM_ACCESS
	const bool emit_direct   = !force_indirect && !LSU_use_only_indirect_access(op_rs);
	const bool emit_indirect = !emit_direct || !LSU_use_only_direct_access(op_rs);
#else
	// NOTE: If you are debugging, #undefine identifier 'USE_DIRECT_MEM_ACCESS'
	//       and recompile dynarec. Just forcing these false/true isn't enough.
	const bool emit_direct   = false;
	const bool emit_indirect = true;
#endif

	/*****************************************************************
	 * Emit direct loads/stores & any appropriate code invalidations *
	 *****************************************************************/
	if (emit_direct)
	{
		const bool contains_store = (pc_of_last_store_in_series != 1);
		const bool emit_code_invalidation   = contains_store && !LSU_skip_code_invalidation(op_rs);
		const bool emit_address_range_check = emit_indirect;

		u32 *backpatch_label_hle_1 = 0;
		u32 *backpatch_label_hle_2 = 0;
		u32 *backpatch_label_hle_3 = 0;

		/*********************************************************************
		 * Emit any checks+branches (if indirect code is also being emitted) *
		 *********************************************************************/

		bool reg_state_pushed = false;

		if (emit_address_range_check)
		{
			/*****************************
			 * Emit address range check: *
			 *****************************/

			// If we are emitting an address-range-check, it means both direct
			//  and indirect code is emitted. Path to take is chosen at runtime.
			//  Reg allocation state at begin/end of both codepaths must be
			//  identical: push the state here and pop it before indirect code.
			regPushState();
			reg_state_pushed = true;

			if (psx_mem_mapped)
			{
				// See comments in mem_mapping.cpp for full explanation.
				//  PSX addresses between [0x0000_0000 .. 0x1f80_ffff] are
				//  mapped virtually to PSX_MEM_VADDR location. The PS1's
				//  four 2MB RAM mirror regions are mirrored virtually,
				//  eliminating any need for masking, and allowing streamlined
				//  access to both RAM + scratchpad.
				//
				// NOTE: Even though 1KB scratchpad region ends at 0x?f80_03ff,
				//       we inline accesses up to 0x?f80_0fff. This makes the
				//       range check faster. This shouldn't affect anything,
				//       because there's no valid PS1 addresses in that small
				//       area beyond 0x03ff anyway.

				if (emit_code_invalidation)
				{
					// Code-invalidation sequence will be emitted, which means
					//  we will leave MIPSREG_A1 holding the max effective
					//  address shifted left by 4. It will be used to determine
					//  if we should skip the code-invalidation sequence coming
					//  directly after the direct code sequence: if MSB is set,
					//  it cannot be a RAM address and couldn't hold modifiable
					//  code, i.e., it is a scratchpad/8MB ROM-expansion
					//  address [0x?f00_0000 .. 0x?f80_03ff]

					LUI(TEMP_1, 0xf801);
					ADDIU(MIPSREG_A0, rs, imm_max);
					SLL(MIPSREG_A1, MIPSREG_A0, 4);
					SLTU(TEMP_1, MIPSREG_A1, TEMP_1);
					backpatch_label_hle_1 = (u32 *)recMem;
					BEQZ(TEMP_1, 0);

					// MIPSREG_A0 contains effective address of opcode with
					//  maximum immediate offset.
					// MIPSREG_A1 contains this same address shifted left 4:
					//  MSB determines if code-invalidation sequence is skipped.
				} else
				{
					// We can emit one less instruction if code invalidation is
					//  skipped: There is no need to keep a value in MIPSREG_A1
					//  that would otherwise be used to determine when the
					//  address is in scratchpad and code invalidation should be
					//  skipped (scratchpad never contains executable code).
					// Note that we mask away bits 27:24 before using SLTIU
					//  for range check: This is because SLTIU instruction sign-
					//  extends its 16-bit immediate, which is a bit quirky.

					ADDIU(MIPSREG_A0, rs, imm_max);
#ifdef HAVE_MIPS32R2_EXT_INS
					EXT(TEMP_1, MIPSREG_A0, 12, 12);
#else
					SRL(TEMP_1, MIPSREG_A0, 12);
					ANDI(TEMP_1, TEMP_1, 0x0fff);
#endif
					// Note the use of 0x0801 here instead of 0xf801.
					SLTIU(TEMP_1, TEMP_1, 0x0801);
					backpatch_label_hle_1 = (u32 *)recMem;
					BEQZ(TEMP_1, 0);

					// MIPSREG_A0 contains effective address of opcode with
					//  maximum immediate offset.
				}
			} else
			{
				// PSX mem is not virtually mapped/mirrored:
				//  Interpret bit 27 of max effective address as a sign bit. Any
				//  valid PS1 address that is not a RAM address has bits 27:24 set.
				//  Any addresses higher than RAM will take indirect path.

				ADDIU(MIPSREG_A0, rs, imm_max);
				SLL(MIPSREG_A1, MIPSREG_A0, 4);
				backpatch_label_hle_1 = (u32 *)recMem;
				BLTZ(MIPSREG_A1, 0);

				// MIPSREG_A0 contains effective address of opcode with
				//  maximum immediate offset.
			}

			// NOTE: Branch delay slot contains next emitted instruction.
			// IMPORTANT: BD slot here should be empty so it can hold the first
			//            instruction of the base reg conversion sequence.
		}

#ifndef SKIP_WRITEOK_CHECK
		/************************************************************
		 * Emit 'psxRegs.writeok' cache-isolation check (obsolete)  *
		 ************************************************************/

		// Check if (psxRegs.writeok != 0) to see if RAM is writeable, i.e.
		//  PS1 cache is not isolated.
		//  NOTE: This check is no longer necessary, see comments in
		//        psxmem.cpp psxMemWrite32_CacheCtrlPort()

		LW(TEMP_1, PERM_REG_1, off(writeok)); // <BD slot>
		backpatch_label_hle_2 = (u32 *)recMem;
		BEQZ(TEMP_1, 0);

		// NOTE: Branch delay slot contains next emitted instruction
#endif

#ifdef SKIP_SAME_2MB_REGION_CHECK
		//  NOTE: Virtual mapping+mirroring made this check unnecessary.
		if (!psx_mem_mapped)
#endif
		{
			/*****************************************
			 * Emit same-2MB-region check (obsolete) *
			 *****************************************/

			// Check if base_reg and base_reg+imm_min are in the same 2MB region.
			//  Some games like 'Einhander' use a base reg near a RAM mirror-region
			//  boundary to access locations in the prior mirror region. If this
			//  is found, use indirect access.

			ADDIU(TEMP_1, rs, imm_min);  // <BD slot>
#ifdef HAVE_MIPS32R2_EXT_INS
			EXT(TEMP_1, TEMP_1, 21, 3);  // TEMP_1 = (TEMP_1 >> 21) & 0x7
			EXT(TEMP_2, rs, 21, 3);      // TEMP_2 = ((rs+imm_min) >> 21) & 0x7
#else
			SRL(TEMP_1, TEMP_1, 21);     // <BD slot>
			ANDI(TEMP_1, TEMP_1, 7);
			SRL(TEMP_2, rs, 21);
			ANDI(TEMP_2, TEMP_2, 7);
#endif
			backpatch_label_hle_3 = (u32 *)recMem;
			BNE(TEMP_1, TEMP_2, 0);      // goto label_hle if not in same 2MB region

			// NOTE: Branch delay slot contains next emitted instruction
		}

		/************************************
		 * Emit base reg address conversion *
		 ************************************/

		// NOTE: Any empty BD slot will get filled here.
		//  TEMP_1 will hold converted base reg, TEMP_2 is overwritten temp reg.
		//  However, in the future, emitAddressConversion() might return a
		//  different reg, if caching of converted addresses is ever implemented.
		const u32 converted_base_reg = emitAddressConversion(op_rs, rs, TEMP_1, TEMP_2);  // <BD slot>

		// Code invalidation needs the original base register value. However,
		//  it sometimes gets overwritten by the last load in a series.
		//  This is a problem because the invalidation sequence comes after
		//  the actual loads/stores in the series. If this situation is
		//  detected, the original base reg value is moved to a dedicated
		//  register (MIPSREG_A2) before the last load is done. Normally, the
		//  invalidation sequence will use the original base reg.
		u32 unmodified_base_reg = rs;

		u32 PC = pc - 4;
		int icount = count;
		do
		{
			const u32 opcode = OPCODE_AT(PC);
			PC += 4;

			// Skip any NOPs in the series
			if (opcode == 0)
				continue;

			const bool is_store = contains_store ? opcodeIsStore(opcode) : false;
			const bool is_load  = is_store ? false : opcodeIsLoad(opcode);

			if (!is_store && !is_load) {
				// Must be a jump/branch whose BD slot contained a store that
				//  was included in the series, so skip it.
				continue;
			}

			const s16 op_imm = _fImm_(opcode);
			const u32 op_rt  = _fRt_(opcode);

			if (is_store)
			{
				// STORE OPCODE (DIRECT)

				u32 rt;
				if (icount == count) {
					// The 'rt' reg for first opcode in series was already allocated
					rt = first_rt_allocation;
				} else {
					rt = regMipsToHost(op_rt, REG_LOAD, REG_REGISTER);
				}

				backpatch_label_exit_1 = 0;
				if (icount == 1 && (emit_code_invalidation || emit_indirect)) {
					// This is the end of the loop
					backpatch_label_exit_1 = (u32 *)recMem;
					if (emit_code_invalidation) {
						// If addresses were in scratchpad, skip both code
						//  invalidation and the indirect code.
						BLTZ(MIPSREG_A1, 0); // bltz label_exit
					} else {
						// Skip indirect code
						B(0); // b label_exit
					}
					// NOTE: Branch delay slot will contain the instruction below
				}
				LSU_OPCODE(opcode & 0xfc000000, rt, converted_base_reg, op_imm);  // <BD> (MAYBE)

				regUnlock(rt);
			} else
			{
				// LOAD OPCODE (DIRECT)

				// If load overwrites base reg, we must backup original base reg
				//  value, as it is needed for code invalidation sequence later.
				if ((op_rt == op_rs) && emit_code_invalidation) {
					MOV(MIPSREG_A2, rs);
					unmodified_base_reg = MIPSREG_A2;
				}

				u32 rt;
				if (icount == count) {
					// The 'rt' reg for first opcode in series was already allocated
					rt = first_rt_allocation;
				} else {
					// LWL/LWR load is special: it needs to load existing contents of reg
					const u32 insn = opcode & 0xfc000000;
					const bool is_lwl_lwr = (insn == 0x88000000 || insn == 0x98000000);

					rt = regMipsToHost(op_rt, (is_lwl_lwr ? REG_LOAD : REG_FIND), REG_REGISTER);
				}

				if (icount == 1 && (emit_code_invalidation || emit_indirect)) {
					// This is the end of the loop
					backpatch_label_exit_1 = (u32 *)recMem;
					if (emit_code_invalidation) {
						// If addresses were in scratchpad, skip both code
						//  invalidation and the indirect code.
						BLTZ(MIPSREG_A1, 0); // bltz label_exit
					} else {
						// Skip indirect code
						B(0); // b label_exit
					}
					// NOTE: Branch delay slot will contain the instruction below
				}
				LSU_OPCODE(opcode & 0xfc000000, rt, converted_base_reg, op_imm);  // <BD> (MAYBE)

				SetUndef(op_rt);
				regMipsChanged(op_rt);
				regUnlock(rt);
			}
		} while (--icount);

		if (emit_code_invalidation)
		{
			/*****************************************************
			 * Invalidate recRAM[addr+imm16] code block pointers *
			 *****************************************************/

			bool first_invalidation_done = false;

			LUI(TEMP_3, ADR_HI(recRAM)); // temp_3 = upper code block ptr array addr

			u32 PC = pc - 4;
			int icount = count;
			do
			{
				u32 opcode = OPCODE_AT(PC);
				PC += 4;

				// Skip any loads, NOPs, or jumps/branches in the series
				if (!opcodeIsStore(opcode))
					continue;

				const s16 op_imm = _fImm_(opcode);

				// See earlier notes at declaration of 'unmodified_base_reg'

				if (first_invalidation_done || !emit_address_range_check) {
					ADDIU(MIPSREG_A0, unmodified_base_reg, op_imm);      // Code invalidation needs eff addr
				} else {
					// We already placed base_reg+imm_max in MIPSREG_A0 during
					//  initial range-checks. No need to load again if first
					//  immediate is same as imm_max.
					first_invalidation_done = true;
					if (op_imm != imm_max)
						ADDIU(MIPSREG_A0, unmodified_base_reg, op_imm);  // Code invalidation needs eff addr
				}

#ifdef HAVE_MIPS32R2_EXT_INS
				EXT(TEMP_1, MIPSREG_A0, 0, 21); // TEMP_1 = MIPSREG_A0 & 0x1fffff
#else
				SLL(TEMP_1, MIPSREG_A0, 11);
				SRL(TEMP_1, TEMP_1, 11);
#endif

				if ((opcode & 0xfc000000) != 0xac000000) {
					// Not a SW, clear lower 2 bits to ensure addr is aligned:
#ifdef HAVE_MIPS32R2_EXT_INS
					INS(TEMP_1, 0, 0, 2);
#else
					SRL(TEMP_1, TEMP_1, 2);
					SLL(TEMP_1, TEMP_1, 2);
#endif
				}
				ADDU(TEMP_1, TEMP_1, TEMP_3);

				if ((PC-4) == pc_of_last_store_in_series && emit_direct) {
					// This is the last store in the series, so the last invalidation
					//  made and the end of all the direct code. Skip past the
					//  indirect code coming after this.

					backpatch_label_exit_2 = (u32 *)recMem;
					B(0); // b label_exit
					// NOTE: Branch delay slot will contain the instruction below
				}
				// Important: this should be the last opcode in the loop (see note above)
				SW(0, TEMP_1, ADR_LO(recRAM));  // set code block ptr to NULL


				// Last store in series? We're done.
				if ((PC-4) == pc_of_last_store_in_series)
					break;

			} while (--icount);
		}

		if (backpatch_label_hle_1)
			fixup_branch(backpatch_label_hle_1);
		if (backpatch_label_hle_2)
			fixup_branch(backpatch_label_hle_2);
		if (backpatch_label_hle_3)
			fixup_branch(backpatch_label_hle_3);

		if (reg_state_pushed)
			regPopState();
	}

	/******************************
	 * Emit indirect loads/stores *
	 ******************************/
	if (emit_indirect)
	{
		enum { WIDTH_8, WIDTH_16, WIDTH_32 };
		const uptr mem_read_func[]  = { (uptr)psxMemRead8,  (uptr)psxMemRead16,  (uptr)psxMemRead32  };
		const uptr mem_write_func[] = { (uptr)psxMemWrite8, (uptr)psxMemWrite16, (uptr)psxMemWrite32 };
		const uptr *read_func  = mem_read_func;
		const uptr *write_func = mem_write_func;
#ifdef USE_HW_FUNCS_FOR_INDIRECT_ACCESS
		// See notes at top of file
		const uptr hw_read_func[]   = { (uptr)psxHwRead8,   (uptr)psxHwRead16,   (uptr)psxHwRead32   };
		const uptr hw_write_func[]  = { (uptr)psxHwWrite8,  (uptr)psxHwWrite16,  (uptr)psxHwWrite32  };
		if (psx_mem_mapped) {
			read_func  = hw_read_func;
			write_func = hw_write_func;
		}
#endif

		u32 PC = pc - 4;
		int icount = count;
		do
		{
			const u32 opcode = OPCODE_AT(PC);
			PC += 4;

			// Skip any NOPs in the series
			if (opcode == 0)
				continue;

			const bool is_store = opcodeIsStore(opcode);
			const bool is_load  = is_store ? false : opcodeIsLoad(opcode);

			if (!is_store && !is_load) {
				// Must be a jump/branch whose BD slot contained a store that
				//  was included in the series, so skip it.
				continue;
			}

			const u32 op_rt  = _fRt_(opcode);
			const s16 op_imm = _fImm_(opcode);

			if (is_store)
			{
				// STORE OPCODE (INDIRECT)

				u32 rt;
				if (icount == count) {
					// The 'rt' reg for first opcode in series was already allocated
					rt = first_rt_allocation;
				} else {
					rt = regMipsToHost(op_rt, REG_LOAD, REG_REGISTER);
				}

				const u32 insn = opcode & 0xfc000000;
				switch (insn)
				{
					case 0xa0000000: // SB
						ADDIU(MIPSREG_A0, rs, op_imm);
						JAL(write_func[WIDTH_8]);
						MOV(MIPSREG_A1, rt); // <BD> Branch delay slot
						break;
					case 0xa4000000: // SH
						ADDIU(MIPSREG_A0, rs, op_imm);
						JAL(write_func[WIDTH_16]);
						MOV(MIPSREG_A1, rt); // <BD> Branch delay slot
						break;
					case 0xac000000: // SW
						ADDIU(MIPSREG_A0, rs, op_imm);
						JAL(write_func[WIDTH_32]);
						MOV(MIPSREG_A1, rt); // <BD> Branch delay slot
						break;
					case 0xa8000000: // SWL
					case 0xb8000000: // SWR
						ADDIU(MIPSREG_A0, rs, op_imm);
#ifdef HAVE_MIPS32R2_EXT_INS
						JAL(read_func[WIDTH_32]);       // result in MIPSREG_V0
						INS(MIPSREG_A0, 0, 0, 2);       // <BD> clear 2 lower bits of $a0
#else
						SRL(MIPSREG_A0, MIPSREG_A0, 2);
						JAL(read_func[WIDTH_32]);       // result in MIPSREG_V0
						SLL(MIPSREG_A0, MIPSREG_A0, 2); // <BD> clear lower 2 bits of $a0
#endif

						ADDIU(MIPSREG_A0, rs, op_imm);

						if (insn == 0xa8000000)   // SWL
							LUI(TEMP_2, ADR_HI(SWL_MASKSHIFT));
						else                      // SWR
							LUI(TEMP_2, ADR_HI(SWR_MASKSHIFT));

						// Lower 2 bits of dst addr are index into u32 mask/shift arrays:
#ifdef HAVE_MIPS32R2_EXT_INS
						INS(TEMP_2, MIPSREG_A0, 2, 2);
						INS(MIPSREG_A0, 0, 0, 2);       // clear 2 lower bits of addr
#else
						ANDI(TEMP_1, MIPSREG_A0, 3);    // temp_1 = addr & 3
						SLL(TEMP_1, TEMP_1, 2);         // temp_1 *= 4
						OR(TEMP_2, TEMP_2, TEMP_1);

						SRL(MIPSREG_A0, MIPSREG_A0, 2); // clear 2 lower bits of addr
						SLL(MIPSREG_A0, MIPSREG_A0, 2);
#endif

						ADDIU(TEMP_3, TEMP_2, 16);      // array entry of shift amount is
						// 16 bytes past mask entry

						if (insn == 0xa8000000) { // SWL
							LW(TEMP_2, TEMP_2, ADR_LO(SWL_MASKSHIFT)); // temp_2 = mask
							LW(TEMP_3, TEMP_3, ADR_LO(SWL_MASKSHIFT)); // temp_3 = shift
						} else {                  // SWR
							LW(TEMP_2, TEMP_2, ADR_LO(SWR_MASKSHIFT)); // temp_2 = mask
							LW(TEMP_3, TEMP_3, ADR_LO(SWR_MASKSHIFT)); // temp_3 = shift
						}

						AND(MIPSREG_A1, MIPSREG_V0, TEMP_2); // $a1 = mem_val & mask

						if (insn == 0xa8000000) // SWL
							SRLV(TEMP_1, rt, TEMP_3);        // temp_1 = new_data >> shift
						else                    // SWR
							SLLV(TEMP_1, rt, TEMP_3);        // temp_1 = new_data << shift

						JAL(write_func[WIDTH_32]);
						OR(MIPSREG_A1, MIPSREG_A1, TEMP_1);  // <BD> $a1 |= temp_1
						break;
					default:
						printf("ERROR: unrecognized store opcode in %s: %x\n", __func__, opcode);
						exit(1);
						break;
				}

				regUnlock(rt);
			} else
			{
				// LOAD OPCODE (INDIRECT)

				const u32 insn = opcode & 0xfc000000;

				u32 rt;
				if (icount == count) {
					// The 'rt' reg for first opcode in series was already allocated
					rt = first_rt_allocation;
				} else {
					if (insn == 0x88000000 || insn == 0x98000000) {
						// LWL/LWR, so we need existing contents of register
						rt = regMipsToHost(op_rt, REG_LOAD, REG_REGISTER);
					} else {
						rt = regMipsToHost(op_rt, REG_FIND, REG_REGISTER);
					}
				}

				switch (insn)
				{
					case 0x80000000: // LB
						JAL(read_func[WIDTH_8]);
						ADDIU(MIPSREG_A0, rs, op_imm); // <BD> Branch delay slot
						if (op_rt) {
#ifdef HAVE_MIPS32R2_SEB_SEH
							SEB(rt, MIPSREG_V0);
#else
							SLL(rt, MIPSREG_V0, 24);
							SRA(rt, rt, 24);
#endif
						}
						break;
					case 0x90000000: // LBU
						JAL(read_func[WIDTH_8]);    // result in MIPSREG_V0
						ADDIU(MIPSREG_A0, rs, op_imm); // <BD> Branch delay slot
						if (op_rt) {
							MOV(rt, MIPSREG_V0);
						}
						break;
					case 0x84000000: // LH
						JAL(read_func[WIDTH_16]);   // result in MIPSREG_V0
						ADDIU(MIPSREG_A0, rs, op_imm); // <BD> Branch delay slot
						if (op_rt) {
#ifdef HAVE_MIPS32R2_SEB_SEH
							SEH(rt, MIPSREG_V0);
#else
							SLL(rt, MIPSREG_V0, 16);
							SRA(rt, rt, 16);
#endif
						}
						break;
					case 0x94000000: // LHU
						JAL(read_func[WIDTH_16]);   // result in MIPSREG_V0
						ADDIU(MIPSREG_A0, rs, op_imm); // <BD> Branch delay slot
						if (op_rt) {
							ANDI(rt, MIPSREG_V0, 0xffff);
						}
						break;
					case 0x8c000000: // LW
						JAL(read_func[WIDTH_32]);   // result in MIPSREG_V0
						ADDIU(MIPSREG_A0, rs, op_imm); // <BD> Branch delay slot
						if (op_rt) {
							MOV(rt, MIPSREG_V0);
						}
						break;
					case 0x88000000: // LWL
					case 0x98000000: // LWR
						ADDIU(MIPSREG_A0, rs, op_imm);
#ifdef HAVE_MIPS32R2_EXT_INS
						JAL(read_func[WIDTH_32]);   // result in MIPSREG_V0
						INS(MIPSREG_A0, 0, 0, 2);   // <BD> clear 2 lower bits of $a0 (using branch delay slot)
#else
						SRL(MIPSREG_A0, MIPSREG_A0, 2);
						JAL(read_func[WIDTH_32]);       // result in MIPSREG_V0
						SLL(MIPSREG_A0, MIPSREG_A0, 2); // <BD> clear lower 2 bits of $a0
#endif

						ADDIU(MIPSREG_A0, rs, op_imm);

						if (insn == 0x88000000) // LWL
							LUI(TEMP_2, ADR_HI(LWL_MASKSHIFT));
						else                    // LWR
							LUI(TEMP_2, ADR_HI(LWR_MASKSHIFT));

						// Lower 2 bits of dst addr are index into u32 mask/shift arrays:
#ifdef HAVE_MIPS32R2_EXT_INS
						INS(TEMP_2, MIPSREG_A0, 2, 2);
#else
						ANDI(TEMP_1, MIPSREG_A0, 3);    // temp_1 = addr & 3
						SLL(TEMP_1, TEMP_1, 2);         // temp_1 *= 4
						OR(TEMP_2, TEMP_2, TEMP_1);
#endif

						ADDIU(TEMP_3, TEMP_2, 16);      // array entry of shift amount is
														// 16 bytes past mask entry
						if (insn == 0x88000000) { // LWL
							LW(TEMP_2, TEMP_2, ADR_LO(LWL_MASKSHIFT)); // temp_2 = mask
							LW(TEMP_3, TEMP_3, ADR_LO(LWL_MASKSHIFT)); // temp_3 = shift
						} else {                  // LWR
							LW(TEMP_2, TEMP_2, ADR_LO(LWR_MASKSHIFT)); // temp_2 = mask
							LW(TEMP_3, TEMP_3, ADR_LO(LWR_MASKSHIFT)); // temp_3 = shift
						}

						AND(rt, rt, TEMP_2);            // mask pre-existing contents of rt

						if (insn == 0x88000000) // LWL
							SLLV(TEMP_3, MIPSREG_V0, TEMP_3);   // temp_3 = data_read << shift
						else                    // LWR
							SRLV(TEMP_3, MIPSREG_V0, TEMP_3);   // temp_3 = data_read >> shift

						OR(rt, rt, TEMP_3);
						break;
					default:
						printf("ERROR: unrecognized load opcode in %s: %x\n", __func__, opcode);
						exit(1);
						break;
				}

				SetUndef(op_rt);
				regMipsChanged(op_rt);
				regUnlock(rt);
			}
		} while (--icount);
	}

	// label_exit:
	if (backpatch_label_exit_1)
		fixup_branch(backpatch_label_exit_1);
	if (backpatch_label_exit_2)
		fixup_branch(backpatch_label_exit_2);

	regUnlock(rs);
}


/* Emits a series of loads/stores to a known-const RAM base address.
 *  NOTE: We don't do code invalidation here: no games seem to need it.
 *        XXX - In fact, doing code invalidation here makes 'Colony Wars'
 *        freeze when returning to main menu after ship self-destruct sequence.
 */
static void const_loads_stores(const int count,
                               const u32 pc_of_last_store_in_series,
                               const u32 base_reg_constval)
{
	const bool contains_store = (pc_of_last_store_in_series != 1);

#ifndef SKIP_WRITEOK_CHECK
	// Do nothing if (psxRegs.writeok == 0).
	u32 *backpatch_label_no_write = 0;
	if (contains_store) {
		// Check if (psxRegs.writeok != 0) to see if RAM is writeable
		LW(TEMP_1, PERM_REG_1, off(writeok));
		backpatch_label_no_write = (u32 *)recMem;
		BEQZ(TEMP_1, 0);  // if (!psxRegs.writeok) goto label_no_write
		NOP(); // <BD slot>
	}
#endif

	// Keep upper half of last effective address in reg, tracking current
	//  value so we can avoid loading same val repeatedly.
	u16 mem_addr_hi = 0;
	bool upper_mem_addr_loaded = false;

	u32 PC = pc - 4;
	int icount = count;
	do {
		const u32 opcode = OPCODE_AT(PC);
		PC += 4;

		// Skip any NOPs in the series
		if (opcode == 0)
			continue;

		const bool is_store = contains_store ? opcodeIsStore(opcode) : false;
		const bool is_load  = is_store ? false : opcodeIsLoad(opcode);

		if (!is_store && !is_load) {
			// Must be a jump/branch whose BD slot contained a store that
			//  was included in the series, so skip it.
			continue;
		}

		const s16 imm = _fImm_(opcode);
		const u32 rt = _fRt_(opcode);

		const u32 r2 = is_store ? regMipsToHost(rt, REG_LOAD, REG_REGISTER)
		                        : regMipsToHost(rt, REG_FIND, REG_REGISTER);

		const u32 mem_addr = (u32)psxM + ((base_reg_constval + imm) & 0x1fffff);

		if (!upper_mem_addr_loaded || (ADR_HI(mem_addr) != mem_addr_hi)) {
			mem_addr_hi = ADR_HI(mem_addr);
			upper_mem_addr_loaded = true;
			LUI(TEMP_2, ADR_HI(mem_addr));
		}

		LSU_OPCODE(opcode & 0xfc000000, r2, TEMP_2, ADR_LO(mem_addr));

		if (!is_store) {
			SetUndef(rt);
			regMipsChanged(rt);
		}
		regUnlock(r2);
	} while (--icount);

#ifndef SKIP_WRITEOK_CHECK
	// label_no_write:
	if (backpatch_label_no_write)
		fixup_branch(backpatch_label_no_write);
#endif
}


/* Main load/store function that calls all the others above */
static void emitLoadStoreSeries()
{
	// See comments below regarding series and BD slots
	static bool next_call_emits_nothing = false;  // Static variable
	if (next_call_emits_nothing) {
		// Skip emitting anything for just this one call
		next_call_emits_nothing = false;
		return;
	}

	u32  pc_of_last_store_in_series = 1;
	bool series_includes_bd_slot_store = false;
	const int count = count_loads_stores(&pc_of_last_store_in_series, &series_includes_bd_slot_store);

	// Series can include a store found in a BD-slot as their last opcode:
	//  The count includes the jump/branch and BD-slot store. We will skip any
	//  jump/branch found, and emit the BD-slot store. When we are next called,
	//  it will be the branch emitter wanting to recompile this store in its
	//  BD-slot. We won't emit anything then, since we've already handled
	//  it during this call.
	if (series_includes_bd_slot_store)
		next_call_emits_nothing = true;  // Static variable

#ifdef WITH_DISASM
	// First opcode in series was already disassembled in recRecompile().
	// Disassemble the additional opcodes we're including in the series.
	for (int i = 0; i < count-1; i++) {
		// If we encounter a branch/jump opcode in the series, that means the
		//  series ended by including a store in a BD-slot. Stop disassembling:
		//  let recRecompile() disassemble the branch/jump & BD-slot store.
		//  Otherwise, we'll upset the order of disassembly.
		if (opcodeIsBranchOrJump(OPCODE_AT(pc + i * 4)))
			break;
		DISASM_PSX(pc + i * 4);
	}
#endif

	bool const_addr = false;
#ifdef USE_CONST_ADDRESSES
	const_addr = IsConst(_Rs_);
#endif

	if (!const_addr)
	{
		/****************************
		 * Handle non-const address *
		 ****************************/

		// Call general-case emitter for non-const address. It will need to
		//  emit address range check and emit both direct & indirect code.
		general_loads_stores(count, pc_of_last_store_in_series, false);
	} else
	{
		/************************
		 * Handle const address *
		 ************************/

		// Is address outside lower 8MB RAM region? (2MB mirrored x4)
		const u32 base_reg_constval = GetConst(_Rs_);
		const u32 addr_max = base_reg_constval + imm_max;
		if ((addr_max & 0x0fffffff) >= 0x800000)
		{
			bool is_hw_address = false;
#ifdef USE_DIRECT_HW_ACCESS
			{
				const u16 upper = addr_max >> 16;
				if (upper == 0x1f80 || upper == 0x9f80 || upper == 0xbf80)
					is_hw_address = true;
			}
#endif

			if (is_hw_address) {
				/**************************************
				 * Handle const scratchpad/HW address *
				 **************************************/

				// Address is in 64KB scratchpad,HW I/O port region
				const_hw_loads_stores(count, pc_of_last_store_in_series, base_reg_constval);
			} else {
				/*********************************
				 * Handle const indirect address *
				 *********************************/

				// Call general-case emitter, but force indirect access since
				//  known-const address is going to ROM or cache-control port.
				general_loads_stores(count, pc_of_last_store_in_series, true);
			}
		} else {
			/****************************
			 * Handle const RAM address *
			 ****************************/

			const_loads_stores(count, pc_of_last_store_in_series, base_reg_constval);
		}
	}

	pc += (count-1)*4;

	// If we included a store in a BD-slot as our last opcode, next opcode to
	//  be recompiled should be the jump/branch before the slot.
	if (series_includes_bd_slot_store)
		pc -= 8;
}


/***********************************************
 * Opcode emitters called during recompilation *
 ***********************************************/
static void recLB()
{
	// Rt = mem[Rs + Im] (signed)
	emitLoadStoreSeries();
}

static void recLBU()
{
	// Rt = mem[Rs + Im] (unsigned)
	emitLoadStoreSeries();
}

static void recLH()
{
	// Rt = mem[Rs + Im] (signed)
	emitLoadStoreSeries();
}

static void recLHU()
{
	// Rt = mem[Rs + Im] (unsigned)
	emitLoadStoreSeries();
}

static void recLW()
{
	// Rt = mem[Rs + Im] (unsigned)
	emitLoadStoreSeries();
}

static void recLWL()
{
	// Unaligned load (unsigned)
	// NOTE: Mem value read is merged with existing Rt contents
	emitLoadStoreSeries();
}

static void recLWR()
{
	// Unaligned load (unsigned)
	// NOTE: Mem value read is merged with existing Rt contents
	emitLoadStoreSeries();
}

static void recSB()
{
	// mem[Rs + Im] = Rt
	emitLoadStoreSeries();
}

static void recSH()
{
	// mem[Rs + Im] = Rt
	emitLoadStoreSeries();
}

static void recSW()
{
	// mem[Rs + Im] = Rt
	emitLoadStoreSeries();
}

static void recSWL()
{
	// Unaligned store
	emitLoadStoreSeries();
}

static void recSWR()
{
	// Unaligned store
	emitLoadStoreSeries();
}
