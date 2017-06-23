// If your platform gives the same results as PS1 did in the HI register
//   when dividing by zero, define this to eliminate some fixup code, which
//   MIPS32 jz-series CPUs seem to. (See notes in recDIV()/recDIVU())
#if defined(GCW_ZERO)
	#define OMIT_DIV_BY_ZERO_HI_FIXUP
#endif

// See recDIV()/recDIVU (disable for debugging purposes)
#define OMIT_DIV_BY_ZERO_FIXUP_IF_EXCEPTION_SEQUENCE_FOUND

// See recMULT()/recMULTU (disable for debugging purposes)
#define USE_CONST_MULT_OPTIMIZATIONS

// See recDIV()/recDIVU (disable for debugging purposes)
#define USE_CONST_DIV_OPTIMIZATIONS


static void recMULT()
{
// Lo/Hi = Rs * Rt (signed)

#ifdef USE_CONST_MULT_OPTIMIZATIONS
	// First, check if either or both operands are const values
	bool rs_const = IsConst(_Rs_);
	bool rt_const = IsConst(_Rt_);

	if (rs_const || rt_const)
	{
		// Check rs_const or rt_const before using either value here!
		s32 rs_val = iRegs[_Rs_].r;
		s32 rt_val = iRegs[_Rt_].r;

		bool const_res = false;
		s32 lo_res = 0;
		s32 hi_res = 0;

		if ((rs_const && !rs_val) || (rt_const && !rt_val)) {
			// If either operand is 0, both LO/HI result is 0
			const_res = true;
			lo_res = 0;
			hi_res = 0;
		} else if (rs_const && rt_const) {
			// If both operands are known-const, compute result statically
			s64 res = (s64)rs_val * (s64)rt_val;
			const_res = true;
			lo_res = (s32)res;
			hi_res = (s32)(res >> 32);
		} else if ((rs_const && (abs(rs_val) == 1)) || (rt_const && (abs(rt_val) == 1))) {
			// If one of the operands is known to be const-val '+/-1', result is identity
			//  (with negation if val is -1)

			u32 ident_reg_psx = rs_const ? _Rt_ : _Rs_;
			u32 ident_reg = regMipsToHost(ident_reg_psx, REG_LOAD, REG_REGISTER);

			u32 work_reg = ident_reg;
			bool negate_res = rs_const ? (rs_val < 0) : (rt_val < 0);

			if (negate_res) {
				SUBU(TEMP_1, 0, work_reg);
				work_reg = TEMP_1;
			}

			SW(work_reg, PERM_REG_1, offGPR(32)); // LO
			// Upper word is all 0s or 1s depending on sign of LO result
			SRA(TEMP_1, work_reg, 31);
			SW(TEMP_1, PERM_REG_1, offGPR(33));   // HI

			regUnlock(ident_reg);

			// We're done
			return;
		} else {
			// If one of the operands is a const power-of-two, we can get result by shifting

			// Determine which operand is const power-of-two value, if any
			bool rs_pot = rs_const && (rs_val != 0x80000000) && ((abs(rs_val) & (abs(rs_val) - 1)) == 0);
			bool rt_pot = rt_const && (rt_val != 0x80000000) && ((abs(rt_val) & (abs(rt_val) - 1)) == 0);

			if (rs_pot || rt_pot) {
				u32 npot_reg_psx = rs_pot ? _Rt_ : _Rs_;
				u32 npot_reg = regMipsToHost(npot_reg_psx, REG_LOAD, REG_REGISTER);

				// Count trailing 0s of const power-of-two operand to get left-shift amount
				u32 pot_val = rs_pot ? (u32)abs(rs_val) : (u32)abs(rt_val);
				u32 shift_amt = __builtin_ctz(pot_val);

				u32 work_reg = npot_reg;
				bool negate_res = rs_pot ? (rs_val < 0) : (rt_val < 0);
				if (negate_res) {
					SUBU(TEMP_2, 0, npot_reg);
					work_reg = TEMP_2;
				}

				SLL(TEMP_1, work_reg, shift_amt);
				SW(TEMP_1, PERM_REG_1, offGPR(32)); // LO
				// Sign-extend here when computing upper word of result
				SRA(TEMP_1, work_reg, (32 - shift_amt));
				SW(TEMP_1, PERM_REG_1, offGPR(33)); // HI

				regUnlock(npot_reg);

				// We're done
				return;
			}
		}

		if (const_res) {
			if (lo_res) {
				LI32(TEMP_1, (u32)lo_res);
				SW(TEMP_1, PERM_REG_1, offGPR(32)); // LO
			} else {
				SW(0, PERM_REG_1, offGPR(32)); // LO
			}

			if (hi_res) {
				LI32(TEMP_1, (u32)hi_res);
				SW(TEMP_1, PERM_REG_1, offGPR(33)); // HI
			} else {
				SW(0, PERM_REG_1, offGPR(33)); // HI
			}

			// We're done
			return;
		}

		// We couldn't emit faster code, so fall-through here
	}
#endif // USE_CONST_MULT_OPTIMIZATIONS

	u32 rs = regMipsToHost(_Rs_, REG_LOAD, REG_REGISTER);
	u32 rt = regMipsToHost(_Rt_, REG_LOAD, REG_REGISTER);

	MULT(rs, rt);
	MFLO(TEMP_1);
	MFHI(TEMP_2);
	SW(TEMP_1, PERM_REG_1, offGPR(32)); // LO
	SW(TEMP_2, PERM_REG_1, offGPR(33)); // HI

	regUnlock(rs);
	regUnlock(rt);
}

static void recMULTU()
{
// Lo/Hi = Rs * Rt (unsigned)

	// First, check if either or both operands are const values
#ifdef USE_CONST_MULT_OPTIMIZATIONS
	bool rs_const = IsConst(_Rs_);
	bool rt_const = IsConst(_Rt_);

	if (rs_const || rt_const)
	{
		// Check rs_const or rt_const before using either value here!
		u32 rs_val = iRegs[_Rs_].r;
		u32 rt_val = iRegs[_Rt_].r;

		bool const_res = false;
		u32 lo_res = 0;
		u32 hi_res = 0;

		if ((rs_const && !rs_val) || (rt_const && !rt_val)) {
			// If either operand is 0, both LO/HI result is 0
			const_res = true;
			lo_res = 0;
			hi_res = 0;
		} else if (rs_const && rt_const) {
			// If both operands are known-const, compute result statically
			u64 res = (u64)rs_val * (u64)rt_val;
			const_res = true;
			lo_res = (u32)res;
			hi_res = (u32)(res >> 32);
		} else if ((rs_const && (rs_val == 1)) || (rt_const && (rt_val == 1))) {
			// If one of the operands is known to be const-val '1', result is identity
			u32 ident_reg_psx = rs_const ? _Rt_ : _Rs_;
			u32 ident_reg = regMipsToHost(ident_reg_psx, REG_LOAD, REG_REGISTER);

			SW(0, PERM_REG_1, offGPR(33));         // HI
			SW(ident_reg, PERM_REG_1, offGPR(32)); // LO

			regUnlock(ident_reg);

			// We're done
			return;
		} else {
			// If one of the operands is a const power-of-two, we can get result by shifting

			// Determine which operand is const power-of-two value, if any
			bool rs_pot = rs_const && ((rs_val & (rs_val - 1)) == 0);
			bool rt_pot = rt_const && ((rt_val & (rt_val - 1)) == 0);

			if (rs_pot || rt_pot) {
				u32 npot_reg_psx = rs_pot ? _Rt_ : _Rs_;
				u32 npot_reg = regMipsToHost(npot_reg_psx, REG_LOAD, REG_REGISTER);

				// Count trailing 0s of const power-of-two operand to get left-shift amount
				u32 pot_val = rs_pot ? rs_val : rt_val;
				u32 shift_amt = __builtin_ctz(pot_val);

				SLL(TEMP_1, npot_reg, shift_amt);
				SW(TEMP_1, PERM_REG_1, offGPR(32)); // LO
				SRL(TEMP_1, npot_reg, (32 - shift_amt));
				SW(TEMP_1, PERM_REG_1, offGPR(33)); // HI

				regUnlock(npot_reg);

				// We're done
				return;
			}
		}

		if (const_res) {
			if (lo_res) {
				LI32(TEMP_1, lo_res);
				SW(TEMP_1, PERM_REG_1, offGPR(32)); // LO
			} else {
				SW(0, PERM_REG_1, offGPR(32)); // LO
			}

			if (hi_res) {
				LI32(TEMP_1, hi_res);
				SW(TEMP_1, PERM_REG_1, offGPR(33)); // HI
			} else {
				SW(0, PERM_REG_1, offGPR(33)); // HI
			}

			// We're done
			return;
		}

		// We couldn't emit faster code, so fall-through here
	}
#endif // USE_CONST_MULT_OPTIMIZATIONS

	u32 rs = regMipsToHost(_Rs_, REG_LOAD, REG_REGISTER);
	u32 rt = regMipsToHost(_Rt_, REG_LOAD, REG_REGISTER);

	MULTU(rs, rt);
	MFLO(TEMP_1);
	MFHI(TEMP_2);
	SW(TEMP_1, PERM_REG_1, offGPR(32)); // LO
	SW(TEMP_2, PERM_REG_1, offGPR(33)); // HI

	regUnlock(rs);
	regUnlock(rt);
}

static void recDIV()
{
// Hi, Lo = rs / rt signed

#ifdef USE_CONST_DIV_OPTIMIZATIONS
	bool rs_const = IsConst(_Rs_);
	bool rt_const = IsConst(_Rt_);

	// First, check if divisor operand is const value
	if (rt_const)
	{
		// Check rs_const before using rs_val value here!
		u32 rs_val = iRegs[_Rs_].r;
		u32 rt_val = iRegs[_Rt_].r;

		if (!rt_val) {
			// If divisor operand is const 0:
			//  LO result is -1 if dividend is positive or zero,
			//               +1 if dividend is negative
			//  HI result is Rs val
			u32 rs = regMipsToHost(_Rs_, REG_LOAD, REG_REGISTER);

			ADDIU(TEMP_2, 0, -1);
			SLT(TEMP_1, rs, 0);           // TEMP_1 = dividend < 0
			MOVN(TEMP_1, TEMP_2, TEMP_1); // if (TEMP_1 != 0) TEMP_1 = TEMP_2
			SW(TEMP_1, PERM_REG_1, offGPR(32)); // LO
			SW(rs, PERM_REG_1, offGPR(33));     // HI

			regUnlock(rs);

			// We're done
			return;
		} else if (rt_val == 1) {
			// If divisor is const-val '1', result is identity
			u32 rs = regMipsToHost(_Rs_, REG_LOAD, REG_REGISTER);

			SW(0, PERM_REG_1, offGPR(33));  // HI
			SW(rs, PERM_REG_1, offGPR(32)); // LO

			regUnlock(rs);

			// We're done
			return;
		} else if (rs_const) {
			// If both operands are known-const, compute result statically
			u32 lo_res = rs_val / rt_val;
			u32 hi_res = rs_val % rt_val;

			if (lo_res) {
				LI32(TEMP_1, lo_res);
				SW(TEMP_1, PERM_REG_1, offGPR(32)); // LO
			} else {
				SW(0, PERM_REG_1, offGPR(32)); // LO
			}

			if (hi_res) {
				LI32(TEMP_1, hi_res);
				SW(TEMP_1, PERM_REG_1, offGPR(33)); // HI
			} else {
				SW(0, PERM_REG_1, offGPR(33)); // HI
			}

			// We're done
			return;
		} else {
			// If divisor is a const power-of-two, we can get result by shifting
			bool rt_pot = (rt_val != 0x80000000) && ((abs(rt_val) & (abs(rt_val) - 1)) == 0);

			if (rt_pot) {
				u32 rs = regMipsToHost(_Rs_, REG_LOAD, REG_REGISTER);

				// Count trailing 0s of const power-of-two divisor to get right-shift amount
				u32 pot_val = (u32)abs(rt_val);
				u32 shift_amt = __builtin_ctz(pot_val);

				u32 work_reg = rs;
				bool negate_res = (rt_val < 0);

				if (negate_res) {
					SUBU(TEMP_2, 0, rs);
					work_reg = TEMP_2;
				}

				SRA(TEMP_1, work_reg, shift_amt);
				SW(TEMP_1, PERM_REG_1, offGPR(32)); // LO

				// Subtract one from pot divisor to get remainder modulo mask
				if ((pot_val-1) > 0xffff) {
					LI32(TEMP_1, (pot_val-1));
					AND(TEMP_1, rs, TEMP_1);
				} else {
					ANDI(TEMP_1, rs, (pot_val-1));
				}
				SW(TEMP_1, PERM_REG_1, offGPR(33)); // HI

				regUnlock(rs);

				// We're done
				return;
			}
		}

		// We couldn't emit faster code, so fall-through here
	}
#endif // USE_CONST_DIV_OPTIMIZATIONS

	u32 rs = regMipsToHost(_Rs_, REG_LOAD, REG_REGISTER);
	u32 rt = regMipsToHost(_Rt_, REG_LOAD, REG_REGISTER);

	// Test if divisor is 0, emulating correct results for PS1 CPU.
	// NOTE: we don't bother checking for signed division overflow (the
	//       last entry in this list), as it seems to work the same on
	//       modern MIPS32 CPUs like the jz4770 of GCW Zero. Unfortunately,
	//       division-by-zero results only match PS1 in the 'hi' reg.
	//  Rs              Rt       Hi/Remainder  Lo/Result
	//  0..+7FFFFFFFh    0   -->  Rs           -1
	//  -80000000h..-1   0   -->  Rs           +1
	//  -80000000h      -1   -->  0           -80000000h

	bool omit_div_by_zero_fixup = false;

	if (IsConst(_Rt_) && iRegs[_Rt_].r != 0) {
		// If divisor is known-const val and isn't 0, no need to fixup
		omit_div_by_zero_fixup = true;
	} else if (!branch) {
#ifdef OMIT_DIV_BY_ZERO_FIXUP_IF_EXCEPTION_SEQUENCE_FOUND
		// If we're not inside a branch-delay slot, we scan ahead for a
		//  PS1 GCC auto-generated code sequence that was inserted in most
		//  binaries after a DIV/DIVU instruction. The sequence checked for
		//  div-by-zero (and signed overflow, for DIV) and issued an
		//  exception that went unhandled by PS1 BIOS, which would crash.
		//  If found, don't bother to emulate PS1 div-by-zero result.
		//
		// Sequence is sometimes preceeded by one or two MFHI/MFLO opcodes.
		u32 code_loc = pc;
		code_loc += 4 * rec_scan_for_MFHI_MFLO_sequence(code_loc);
		omit_div_by_zero_fixup = (rec_scan_for_div_by_zero_check_sequence(code_loc) > 0);
#endif
	}

	if (omit_div_by_zero_fixup) {
		DIV(rs, rt);
		MFLO(TEMP_1);              // NOTE: Hi/Lo can't be cached for now, so spill them
		MFHI(TEMP_2);
	} else {
		DIV(rs, rt);
		ADDIU(MIPSREG_A1, 0, -1);
		SLT(TEMP_3, rs, 0);        // TEMP_3 = (rs < 0 ? 1 : 0)
		MFLO(TEMP_1);              // NOTE: Hi/Lo can't be cached for now, so spill them
		MFHI(TEMP_2);

		// If divisor was 0, set LO result (quotient) to 1 if dividend was < 0
		// If divisor was 0, set LO result (quotient) to -1 if dividend was >= 0
		MOVN(MIPSREG_A0, TEMP_3, TEMP_3);      // if (TEMP_3 != 0) then MIPSREG_A1 = TEMP_3
		MOVZ(MIPSREG_A0, MIPSREG_A1, TEMP_3);  // if (TEMP_3 == 0) then MIPSREG_A1 = MIPSREG_A0
		MOVZ(TEMP_1, MIPSREG_A0, rt);          // if (rt == 0) then TEMP_1 = MIPSREG_A0

#ifndef OMIT_DIV_BY_ZERO_HI_FIXUP
		// If divisor was 0, set HI result (remainder) to rs
		MOVZ(TEMP_2, rs, rt);
#endif
	}

	SW(TEMP_1, PERM_REG_1, offGPR(32));
	SW(TEMP_2, PERM_REG_1, offGPR(33));
	regUnlock(rs);
	regUnlock(rt);
}

static void recDIVU()
{
// Hi, Lo = rs / rt unsigned

	// First, check if divisor operand is const value
#ifdef USE_CONST_DIV_OPTIMIZATIONS
	bool rt_const = IsConst(_Rt_);

	if (rt_const)
	{
		bool rs_const = IsConst(_Rs_);

		// Check rs_const before using rs_val value here!
		u32 rs_val = iRegs[_Rs_].r;
		u32 rt_val = iRegs[_Rt_].r;

		if (!rt_val) {
			// If divisor operand is const 0:
			//  LO result is 0xffff_ffff
			//  HI result is Rs val
			u32 rs = regMipsToHost(_Rs_, REG_LOAD, REG_REGISTER);

			ADDIU(TEMP_1, 0, -1);
			SW(TEMP_1, PERM_REG_1, offGPR(32)); // LO
			SW(rs, PERM_REG_1, offGPR(33));     // HI

			regUnlock(rs);

			// We're done
			return;
		} else if (rt_val == 1) {
			// If divisor is const-val '1', result is identity
			u32 rs = regMipsToHost(_Rs_, REG_LOAD, REG_REGISTER);

			SW(0, PERM_REG_1, offGPR(33));  // HI
			SW(rs, PERM_REG_1, offGPR(32)); // LO

			regUnlock(rs);

			// We're done
			return;
		} else if (rs_const) {
			// If both operands are known-const, compute result statically
			u32 lo_res = rs_val / rt_val;
			u32 hi_res = rs_val % rt_val;

			if (lo_res) {
				LI32(TEMP_1, lo_res);
				SW(TEMP_1, PERM_REG_1, offGPR(32)); // LO
			} else {
				SW(0, PERM_REG_1, offGPR(32)); // LO
			}

			if (hi_res) {
				LI32(TEMP_1, hi_res);
				SW(TEMP_1, PERM_REG_1, offGPR(33)); // HI
			} else {
				SW(0, PERM_REG_1, offGPR(33)); // HI
			}

			// We're done
			return;
		} else {
			// If divisor is a const power-of-two, we can get result by shifting
			bool rt_pot = (rt_val & (rt_val - 1)) == 0;

			if (rt_pot) {
				u32 rs = regMipsToHost(_Rs_, REG_LOAD, REG_REGISTER);

				// Count trailing 0s of const power-of-two divisor to get right-shift amount
				u32 pot_val = rt_val;
				u32 shift_amt = __builtin_ctz(pot_val);

				SRL(TEMP_1, rs, shift_amt);
				SW(TEMP_1, PERM_REG_1, offGPR(32)); // LO

				// Subtract one from pot divisor to get remainder modulo mask
				if ((pot_val-1) > 0xffff) {
					LI32(TEMP_1, (pot_val-1));
					AND(TEMP_1, rs, TEMP_1);
				} else {
					ANDI(TEMP_1, rs, (pot_val-1));
				}
				SW(TEMP_1, PERM_REG_1, offGPR(33)); // HI

				regUnlock(rs);

				// We're done
				return;
			}
		}

		// We couldn't emit faster code, so fall-through here
	}
#endif // USE_CONST_DIV_OPTIMIZATIONS

	u32 rs = regMipsToHost(_Rs_, REG_LOAD, REG_REGISTER);
	u32 rt = regMipsToHost(_Rt_, REG_LOAD, REG_REGISTER);

	// Test if divisor is 0, emulating correct results for PS1 CPU.
	//  Rs              Rt       Hi/Remainder  Lo/Result
	//  0..FFFFFFFFh    0   -->  Rs            FFFFFFFFh

	bool omit_div_by_zero_fixup = false;

	if (IsConst(_Rt_) && iRegs[_Rt_].r != 0) {
		// If divisor is known-const val and isn't 0, no need to fixup
		omit_div_by_zero_fixup = true;
	} else if (!branch) {
#ifdef OMIT_DIV_BY_ZERO_FIXUP_IF_EXCEPTION_SEQUENCE_FOUND
		// See notes in recDIV() emitter
		u32 code_loc = pc;
		code_loc += 4 * rec_scan_for_MFHI_MFLO_sequence(code_loc);
		omit_div_by_zero_fixup = (rec_scan_for_div_by_zero_check_sequence(code_loc) > 0);
#endif
	}

	if (omit_div_by_zero_fixup) {
		DIVU(rs, rt);
		MFLO(TEMP_1);              // NOTE: Hi/Lo can't be cached for now, so spill them
		MFHI(TEMP_2);
	} else {
		DIVU(rs, rt);
		ADDIU(TEMP_3, 0, -1);
		MFLO(TEMP_1);              // NOTE: Hi/Lo can't be cached for now, so spill them
		MFHI(TEMP_2);

		// If divisor was 0, set LO result (quotient) to 0xffff_ffff
		MOVZ(TEMP_1, TEMP_3, rt);  // if (rt == 0) then TEMP_1 = TEMP_3

#ifndef OMIT_DIV_BY_ZERO_HI_FIXUP
		// If divisor was 0, set HI result (remainder) to rs
		MOVZ(TEMP_2, rs, rt);
#endif
	}

	SW(TEMP_1, PERM_REG_1, offGPR(32));
	SW(TEMP_2, PERM_REG_1, offGPR(33));
	regUnlock(rs);
	regUnlock(rt);
}

static void recMFHI() {
// Rd = Hi
	if (!_Rd_) return;
	SetUndef(_Rd_);
	u32 rd = regMipsToHost(_Rd_, REG_FIND, REG_REGISTER);

	LW(rd, PERM_REG_1, offGPR(33));
	regMipsChanged(_Rd_);
	regUnlock(rd);
}

static void recMTHI() {
// Hi = Rs
	u32 rs = regMipsToHost(_Rs_, REG_LOAD, REG_REGISTER);
	SW(rs, PERM_REG_1, offGPR(33));
	regUnlock(rs);
}

static void recMFLO() {
// Rd = Lo
	if (!_Rd_) return;
	SetUndef(_Rd_);
	u32 rd = regMipsToHost(_Rd_, REG_FIND, REG_REGISTER);

	LW(rd, PERM_REG_1, offGPR(32));
	regMipsChanged(_Rd_);
	regUnlock(rd);
}

static void recMTLO() {
// Lo = Rs
	u32 rs = regMipsToHost(_Rs_, REG_LOAD, REG_REGISTER);
	SW(rs, PERM_REG_1, offGPR(32));
	regUnlock(rs);
}
