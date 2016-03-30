static u32 psxBranchTest_rec(u32 cycles, u32 pc)
{
	/* Misc helper */
	psxRegs.pc = pc;
	psxRegs.cycle += cycles;

	psxBranchTest();

	u32 compiledpc = (u32)PC_REC32(psxRegs.pc);
	if (compiledpc) {
		//DEBUGF("returning to 0x%x (t2 0x%x t3 0x%x)\n", compiledpc, psxRegs.GPR.n.t2, psxRegs.GPR.n.t3);
		return compiledpc;
	}

	u32 a = recRecompile();
	//DEBUGF("returning to 0x%x (t2 0x%x t3 0x%x)\n", a, psxRegs.GPR.n.t2, psxRegs.GPR.n.t3);
	return a;
}

static void recSYSCALL()
{
	regClearJump();

	LoadImmediate32(pc - 4, TEMP_1);

	MIPS_STR_IMM(MIPS_POINTER, TEMP_1, PERM_REG_1, offpc);

	MIPS_MOV_REG_IMM8(MIPS_POINTER, MIPSREG_A1, (branch == 1 ? 1 : 0));
	MIPS_MOV_REG_IMM8(MIPS_POINTER, MIPSREG_A0, 0x20);
	CALLFunc((u32)psxException);
	MIPS_LDR_IMM(MIPS_POINTER, MIPSREG_A1, PERM_REG_1, offpc);

	LoadImmediate32(((blockcycles+((pc-oldpc)/4)))*BIAS, MIPSREG_A0);

	CALLFunc((u32)psxBranchTest_rec);

	end_block = 1;
}

/* Set a pending branch */
static INLINE void SetBranch()
{
	branch = 1;
	//psxRegs.code = *(u32*)(psxMemRLUT[pc>>16] + (pc&0xffff));
	psxRegs.code = *(u32 *)((char *)PSXM(pc));
	DISASM_PSX
	pc+=4;

	recBSC[psxRegs.code>>26]();
	branch = 0;
}

static INLINE void iJumpNormal(u32 branchPC)
{
	branch = 1;
	//psxRegs.code = *(u32*)(psxMemRLUT[pc>>16] + (pc&0xffff));
	psxRegs.code = *(u32 *)((char *)PSXM(pc));
	DISASM_PSX
	pc+=4;

	recBSC[psxRegs.code>>26]();

	branch = 0;

	regClearJump();
	LoadImmediate32(branchPC, MIPSREG_A1);
	LoadImmediate32(((blockcycles+((pc-oldpc)/4)))*BIAS, MIPSREG_A0);

	CALLFunc((u32)psxBranchTest_rec);

	end_block = 1;
}

static INLINE void iJumpAL(u32 branchPC, u32 linkpc)
{
	branch = 1;
	//psxRegs.code = *(u32*)(psxMemRLUT[pc>>16] + (pc&0xffff));
	psxRegs.code = *(u32 *)((char *)PSXM(pc));
	DISASM_PSX
	pc+=4;

	recBSC[psxRegs.code>>26]();

	branch = 0;

	regClearJump();
	LoadImmediate32(linkpc, TEMP_1);
	MIPS_STR_IMM(MIPS_POINTER, TEMP_1, PERM_REG_1, offGPR(31));

	LoadImmediate32(branchPC, MIPSREG_A1);
	LoadImmediate32(((blockcycles+((pc-oldpc)/4)))*BIAS, MIPSREG_A0);

	CALLFunc((u32)psxBranchTest_rec);

	end_block = 1;
}

static INLINE void iJump(u32 branchPC)
{
	branch = 1;
	//psxRegs.code = *(u32*)(psxMemRLUT[pc>>16] + (pc&0xffff));
	psxRegs.code = *(u32 *)((char *)PSXM(pc));
	DISASM_PSX
	pc+=4;

	recBSC[psxRegs.code>>26]();

	branch = 0;
	
	if(ibranch > 0)
	{
		regClearJump();
		LoadImmediate32(branchPC, MIPSREG_A1);
		LoadImmediate32(((blockcycles+((pc-oldpc)/4)))*BIAS, MIPSREG_A0);
		CALLFunc((u32)psxBranchTest_rec);
		end_block = 1;
		return;
	}

	ibranch++;
	blockcycles += ((pc-oldpc)/4);
	oldpc = pc = branchPC;
	recClear(branchPC, 1);

}

#if 1
static void recBLTZ()
{
// Branch if Rs < 0
	u32 bpc = _Imm_ * 4 + pc;
	u32 nbpc = pc + 4;
//	iFlushRegs();

	if (bpc == nbpc && psxTestLoadDelay(_Rs_, PSXMu32(bpc)) == 0) {
		return;
	}
	
	if(!(_Rs_))
	{
		SetBranch(); return;
	}

	u32 br1 = regMipsToArm(_Rs_, REG_LOADBRANCH, REG_REGISTERBRANCH);
	SetBranch();
	u32 *backpatch = (u32*)recMem;
	MIPS_EMIT(MIPS_POINTER, 0x04010000 | (br1 << 21)); /* bgez */
	MIPS_EMIT(MIPS_POINTER, 0); /* nop */

	regClearBranch();
	LoadImmediate32(bpc, MIPSREG_A1);
	LoadImmediate32(((blockcycles+((pc-oldpc)/4)))*BIAS, MIPSREG_A0);
	CALLFunc((u32)psxBranchTest_rec);
	rec_recompile_end();

	*backpatch |= mips_relative_offset(backpatch, (u32)recMem, 4);
	regBranchUnlock(br1);
}

static void recBGTZ()
{
// Branch if Rs > 0
	u32 bpc = _Imm_ * 4 + pc;
	u32 nbpc = pc + 4;
//	iFlushRegs();

	if (bpc == nbpc && psxTestLoadDelay(_Rs_, PSXMu32(bpc)) == 0) {
		return;
	}
	
	if(!(_Rs_))
	{
		SetBranch(); return;
	}

	u32 br1 = regMipsToArm(_Rs_, REG_LOADBRANCH, REG_REGISTERBRANCH);
	SetBranch();
	u32 *backpatch = (u32*)recMem;
	MIPS_EMIT(MIPS_POINTER, 0x18000000 | (br1 << 21)); /* blez */
	MIPS_EMIT(MIPS_POINTER, 0); /* nop */

	regClearBranch();
	LoadImmediate32(bpc, MIPSREG_A1);
	LoadImmediate32(((blockcycles+((pc-oldpc)/4)))*BIAS, MIPSREG_A0);
	CALLFunc((u32)psxBranchTest_rec);
	rec_recompile_end();

	*backpatch |= mips_relative_offset(backpatch, (u32)recMem, 4);
	regBranchUnlock(br1);
}

static void recBLTZAL()
{
// Branch if Rs < 0
	u32 bpc = _Imm_ * 4 + pc;
	u32 nbpc = pc + 4;
//	iFlushRegs();

	if (bpc == nbpc && psxTestLoadDelay(_Rs_, PSXMu32(bpc)) == 0) {
		return;
	}
	
	if(!(_Rs_))
	{
		SetBranch(); return;
	}

	u32 br1 = regMipsToArm(_Rs_, REG_LOADBRANCH, REG_REGISTERBRANCH);
	SetBranch();
	u32 *backpatch = (u32*)recMem;
	MIPS_EMIT(MIPS_POINTER, 0x04010000 | (br1 << 21)); /* bgez */
	MIPS_EMIT(MIPS_POINTER, 0); /* nop */

	regClearBranch();
	LoadImmediate32(nbpc, TEMP_1);
	MIPS_STR_IMM(MIPS_POINTER, TEMP_1, PERM_REG_1, offGPR(31));

	LoadImmediate32(bpc, MIPSREG_A1);
	LoadImmediate32(((blockcycles+((pc-oldpc)/4)))*BIAS, MIPSREG_A0);
	CALLFunc((u32)psxBranchTest_rec);
	rec_recompile_end();

	*backpatch |= mips_relative_offset(backpatch, (u32)recMem, 4);
	regBranchUnlock(br1);
}

static void recBGEZAL()
{
// Branch if Rs >= 0
	u32 bpc = _Imm_ * 4 + pc;
	u32 nbpc = pc + 4;
//	iFlushRegs();

	if (bpc == nbpc && psxTestLoadDelay(_Rs_, PSXMu32(bpc)) == 0) {
		return;
	}
	
	if(!(_Rs_))
	{
		iJumpAL(bpc, (pc + 4)); return;
	}

	u32 br1 = regMipsToArm(_Rs_, REG_LOADBRANCH, REG_REGISTERBRANCH);
	SetBranch();
	u32 *backpatch = (u32*)recMem;
	MIPS_EMIT(MIPS_POINTER, 0x04000000 | (br1 << 21)); /* bltz */
	MIPS_EMIT(MIPS_POINTER, 0); /* nop */

	regClearBranch();
	LoadImmediate32(nbpc, TEMP_1);
	MIPS_STR_IMM(MIPS_POINTER, TEMP_1, PERM_REG_1, offGPR(31));

	LoadImmediate32(bpc, MIPSREG_A1);
	LoadImmediate32(((blockcycles+((pc-oldpc)/4)))*BIAS, MIPSREG_A0);
	CALLFunc((u32)psxBranchTest_rec);
	rec_recompile_end();

	*backpatch |= mips_relative_offset(backpatch, (u32)recMem, 4);
	regBranchUnlock(br1);
}
#endif

static void recJ()
{
// j target

	iJumpNormal(_Target_ * 4 + (pc & 0xf0000000));
}

static void recJAL()
{
// jal target

	iJumpAL(_Target_ * 4 + (pc & 0xf0000000), (pc + 4));
}


static void recJR()
{
// jr Rs
	u32 br1 = regMipsToArm(_Rs_, REG_LOADBRANCH, REG_REGISTERBRANCH);
	SetBranch();

	MIPS_MOV_REG_REG(MIPS_POINTER, MIPSREG_A1, br1);
	regBranchUnlock(br1);
	regClearJump();
	LoadImmediate32(((blockcycles+((pc-oldpc)/4)))*BIAS, MIPSREG_A0);
	CALLFunc((u32)psxBranchTest_rec);

	end_block = 1;
}

static void recJALR()
{
// jalr Rs
	u32 br1 = regMipsToArm(_Rs_, REG_LOADBRANCH, REG_REGISTERBRANCH);
	u32 rd = regMipsToArm(_Rd_, REG_FIND, REG_REGISTER);
	LoadImmediate32(pc + 4, rd);
	regMipsChanged(_Rd_);

	SetBranch();
	MIPS_MOV_REG_REG(MIPS_POINTER, MIPSREG_A1, br1);	
	regBranchUnlock(br1);
	regClearJump();
	LoadImmediate32(((blockcycles+((pc-oldpc)/4)))*BIAS, MIPSREG_A0);
	CALLFunc((u32)psxBranchTest_rec);

	end_block = 1;
}

static void recBEQ()
{
// Branch if Rs == Rt
	u32 bpc = _Imm_ * 4 + pc;
	u32 nbpc = pc + 4;
//	iFlushRegs();

	if (bpc == nbpc && psxTestLoadDelay(_Rs_, PSXMu32(bpc)) == 0) {
		return;
	}
	
	if (_Rs_ == _Rt_)
	{
		iJumpNormal(bpc); return;
	}

	u32 br1 = regMipsToArm(_Rs_, REG_LOADBRANCH, REG_REGISTERBRANCH);
	u32 br2 = regMipsToArm(_Rt_, REG_LOADBRANCH, REG_REGISTERBRANCH);
	SetBranch();
	u32 *backpatch = (u32*)recMem;
	MIPS_EMIT(MIPS_POINTER, 0x14000000 | (br1 << 21) | (br2 << 16)); /* bne */
	MIPS_EMIT(MIPS_POINTER, 0); /* nop */

	regClearBranch();
	LoadImmediate32(bpc, MIPSREG_A1);
	LoadImmediate32(((blockcycles+((pc-oldpc)/4)))*BIAS, MIPSREG_A0);
	CALLFunc((u32)psxBranchTest_rec);
	rec_recompile_end();

	*backpatch |= mips_relative_offset(backpatch, (u32)recMem, 4);
	regBranchUnlock(br1);
	regBranchUnlock(br2);
}

static void recBNE()
{
// Branch if Rs != Rt
	u32 bpc = _Imm_ * 4 + pc;
	u32 nbpc = pc + 4;
//	iFlushRegs();

	if (bpc == nbpc && psxTestLoadDelay(_Rs_, PSXMu32(bpc)) == 0) {
		return;
	}
	
	if(!(_Rs_) && !(_Rt_))
	{
		SetBranch(); return;
	}

	u32 br1 = regMipsToArm(_Rs_, REG_LOADBRANCH, REG_REGISTERBRANCH);
	u32 br2 = regMipsToArm(_Rt_, REG_LOADBRANCH, REG_REGISTERBRANCH);
	//DEBUGG("emitting beq %d(%d), %d(%d) (code 0x%x)\n", br1, _Rs_, br2, _Rt_, psxRegs.code);
	SetBranch();
	u32* backpatch = (u32*)recMem;
	//DEBUGG("encore br1 %d br2 %d\n", br1, br2);
	MIPS_EMIT(MIPS_POINTER, 0x10000000 | (br1 << 21) | (br2 << 16)); /* beq */
	MIPS_EMIT(MIPS_POINTER, 0); /* nop */

	regClearBranch();
	LoadImmediate32(bpc, MIPSREG_A1);
	LoadImmediate32(((blockcycles+((pc-oldpc)/4)))*BIAS, MIPSREG_A0);
	CALLFunc((u32)psxBranchTest_rec);
	rec_recompile_end();

	//DEBUGG("backpatching %p rel to %p -> 0x%x\n", backpatch, recMem, mips_relative_offset(backpatch, (u32)recMem, 4));
	*backpatch |= mips_relative_offset(backpatch, (u32)recMem, 4);
	regBranchUnlock(br1);
	regBranchUnlock(br2);
}

#if 1
static void recBLEZ()
{
// Branch if Rs <= 0
	u32 bpc = _Imm_ * 4 + pc;
	u32 nbpc = pc + 4;
//	iFlushRegs();

	if (bpc == nbpc && psxTestLoadDelay(_Rs_, PSXMu32(bpc)) == 0) {
		return;
	}
	
	if(!(_Rs_))
	{
		iJumpNormal(bpc); return;
	}

	u32 br1 = regMipsToArm(_Rs_, REG_LOADBRANCH, REG_REGISTERBRANCH);
	SetBranch();
	u32 *backpatch = (u32*)recMem;
	MIPS_EMIT(MIPS_POINTER, 0x1c000000 | (br1 << 21)); /* bgtz */
	MIPS_EMIT(MIPS_POINTER, 0); /* nop */

	regClearBranch();
	LoadImmediate32(bpc, MIPSREG_A1);
	LoadImmediate32(((blockcycles+((pc-oldpc)/4)))*BIAS, MIPSREG_A0);

	CALLFunc((u32)psxBranchTest_rec);
	rec_recompile_end();

	*backpatch |= mips_relative_offset(backpatch, (u32)recMem, 4);
	regBranchUnlock(br1);
}

static void recBGEZ()
{
// Branch if Rs >= 0
	u32 bpc = _Imm_ * 4 + pc;
	u32 nbpc = pc + 4;
//	iFlushRegs();

	if (bpc == nbpc && psxTestLoadDelay(_Rs_, PSXMu32(bpc)) == 0) {
		return;
	}
	
	if(!(_Rs_))
	{
		iJumpNormal(bpc); return;
	}

	u32 br1 = regMipsToArm(_Rs_, REG_LOADBRANCH, REG_REGISTERBRANCH);
	SetBranch();
	u32 *backpatch = (u32*)recMem;
	MIPS_EMIT(MIPS_POINTER, 0x04000000 | (br1 << 21)); /* bltz */
	MIPS_EMIT(MIPS_POINTER, 0); /* nop */

	regClearBranch();
	LoadImmediate32(bpc, MIPSREG_A1);
	LoadImmediate32(((blockcycles+((pc-oldpc)/4)))*BIAS, MIPSREG_A0);

	CALLFunc((u32)psxBranchTest_rec);
	rec_recompile_end();

	*backpatch |= mips_relative_offset(backpatch, (u32)recMem, 4);
	regBranchUnlock(br1);
}
#endif

static void recBREAK() { }

static void recHLE() 
{
	regClearJump();
	
	/* Needed? */
	LoadImmediate32(pc, TEMP_1);
	MIPS_STR_IMM(MIPS_POINTER, TEMP_1, PERM_REG_1, offpc);

	LoadImmediate32(((blockcycles+((pc-oldpc)/4)))*BIAS, MIPSREG_A0);
	CALLFunc((u32)psxHLEt[psxRegs.code & 0xffff]);

	MIPS_LDR_IMM(MIPS_POINTER, MIPSREG_A1, PERM_REG_1, offpc);
	LoadImmediate32(((blockcycles+((pc-oldpc)/4)))*BIAS, MIPSREG_A0);
	CALLFunc((u32)psxBranchTest_rec);

	end_block = 1;
}
