
#define EMULATE_READ(insn, _rt_, _rs_, imm16) \


#define LOAD(insn, _rt_, _rs_, imm16) \
({ \
	gen(ADDI16, T0, _rs_, imm16); \
	TEST	T0, #(1<<12) \
	MOV		T1, T0, LSR #28 \
	ORRNE	T1, T1, #(1<<2)
	ADD		T1, T1, #(256>>2) \
	LDR		T1, [SP, T1, LSL #2] \
	TEST	T1, #0 \
	insn	rt, [T1, +T0] \
	BLEQ	emulate_read_##size##_##_rt_ \
})

#define STORE(insn, _rt_, _rs_, imm16) \
({ \
	gen(ADDI16, T0, _rs_, imm16); \
	TEST	T0, #(1<<12) \
	MOV		T1, T0, LSR #28 \
	ORRNE	T1, T1, #(1<<2)
	ADD		T1, T1, #(256>>2) \
	LDR		T1, [SP, T1, LSL #2] \
	TEST	T1, #0 \
	insn	rt, [T1, +T0] \
	BLEQ	emulate_write_##size##_##_rt_ \
})

static void gen(LB, u32 rt, u32 rs, s32 imm16)
{
	LOAD(LDRNESB, rt, rs, imm16, s8);
}

static void gen(LBU, u32 rt, u32 rs, s32 imm16)
{
	LOAD(LDRNEB, rt, rs, imm16, u8);
}
		
static void gen(LBH, u32 rt, u32 rs, s32 imm16)
{
	LOAD(LDRNESH, rt, rs, imm16, s8);
}
		
static void gen(LBHU, u32 rt, u32 rs, s32 imm16)
{
	LOAD(LDRNEH, rt, rs, imm16, u16);
}
		
static void gen(LW, u32 rt, u32 rs, s32 imm16)
{
	LOAD(LDRNE, rt, rs, imm16, u32);
}
		
static void gen(SB, u32 rt, u32 rs, s32 imm16)
{
	STORE(STRNEB, rt, rs, imm16, s8);
}

static void gen(SBH, u32 rt, u32 rs, s32 imm16)
{
	STORE(STRNEH, rt, rs, imm16, s8);
}
		
static void gen(SW, u32 rt, u32 rs, s32 imm16)
{
	STORE(STRNE, rt, rs, imm16, u32);
}
		

static void recLB()
	iPushOfB();
	/* TODO: Remove */iFlushRegs(false);	/**/
	/* TODO: Remove */regClearJump(false);	/**/
	CALLFunc_NoFlush((u32)psxMemReadS8);
	if (_Rt_)
	{
		u32 rt = AllocateRegister(_Rt_);
		ARM_MOV_REG_REG(ARM_POINTER, rt, ARMREG_R0);
		armRegs[rt].changed = true;
	}
}

static void recLBU()
{
// Rt = mem[Rs + Im] (unsigned)

	iPushOfB();
	/* TODO: Remove */iFlushRegs(false);		/**/				
	/* TODO: Remove */regClearJump(false);	/**/				
	CALLFunc_NoFlush((u32)psxMemRead8);
	if (_Rt_)
	{		
		u32 rt = AllocateRegister(_Rt_);
		ARM_MOV_REG_REG(ARM_POINTER, rt, ARMREG_R0);
		armRegs[rt].changed = true;
	}
}

static void recLH()
{
// Rt = mem[Rs + Im] (signed)

	iPushOfB();
	/* TODO: Remove */iFlushRegs(false);		/**/				
	/* TODO: Remove */regClearJump(false);	/**/				
	CALLFunc_NoFlush((u32)psxMemReadS16);
	if (_Rt_)
	{		
		u32 rt = AllocateRegister(_Rt_);
		ARM_MOV_REG_REG(ARM_POINTER, rt, ARMREG_R0);
		armRegs[rt].changed = true;
	}
}

static void recLHU() {
// Rt = mem[Rs + Im] (unsigned)

	iPushOfB();
	/* TODO: Remove */iFlushRegs(false);		/**/				
	/* TODO: Remove */regClearJump(false);	/**/				

	CALLFunc_NoFlush((u32)psxMemRead16);
	if (_Rt_) {		
		u32 rt = AllocateRegister(_Rt_);
		ARM_MOV_REG_REG(ARM_POINTER, rt, ARMREG_R0);
		armRegs[rt].changed = true;
	}
}

static void recLW() {
// Rt = mem[Rs + Im] (unsigned)

	iPushOfB();
	/* TODO: Remove */iFlushRegs(false);		/**/				
	/* TODO: Remove */regClearJump(false);	/**/				

	CALLFunc_NoFlush((u32)psxMemRead32);
	if (_Rt_) {
		u32 rt = AllocateRegister(_Rt_);
		ARM_MOV_REG_REG(ARM_POINTER, rt, ARMREG_R0);
		armRegs[rt].changed = true;
	}
}

REC_FUNC_TEST(LWL);
REC_FUNC_TEST(LWR);

static void recSB() {
// mem[Rs + Im] = Rt

	iPushOfB();	
	if (IsConst(_Rt_)) {
		LoadImmediate32(iRegs[_Rt_].k, ARMREG_R1);
	} else {
		u32 rt = LoadMipsReg(_Rt_);
		ARM_MOV_REG_REG(ARM_POINTER, ARMREG_R1, rt);
		//ARM_LDR_IMM(ARM_POINTER, ARMREG_R1, PERM_REG_1, CalcDisp(_Rt_));
	}
	/* TODO: Remove */iFlushRegs(false);
	/* TODO: Remove */regClearJump(false);
	CALLFunc_NoFlush((u32)psxMemWrite8);
}

static void recSH() {
// mem[Rs + Im] = Rt

	iPushOfB();	
	if (IsConst(_Rt_)) {
		LoadImmediate32(iRegs[_Rt_].k, ARMREG_R1);
	} else {
		u32 rt = LoadMipsReg(_Rt_);
		ARM_MOV_REG_REG(ARM_POINTER, ARMREG_R1, rt);
		//ARM_LDR_IMM(ARM_POINTER, ARMREG_R1, PERM_REG_1, CalcDisp(_Rt_));
	}
	/* TODO: Remove */iFlushRegs(false);		/**/				
	/* TODO: Remove */regClearJump(false);	/**/				

	CALLFunc_NoFlush((u32)psxMemWrite16);
}

static void recSW() {
// mem[Rs + Im] = Rt

	iPushOfB();	
	if (IsConst(_Rt_)) {
		LoadImmediate32(iRegs[_Rt_].k, ARMREG_R1);
	} else {
		u32 rt = LoadMipsReg(_Rt_);
		ARM_MOV_REG_REG(ARM_POINTER, ARMREG_R1, rt);
		//ARM_LDR_IMM(ARM_POINTER, ARMREG_R1, PERM_REG_1, CalcDisp(_Rt_));
	}
	/* TODO: Remove */iFlushRegs(false);		/**/				
	/* TODO: Remove */regClearJump(false);	/**/				

	CALLFunc_NoFlush((u32)psxMemWrite32);
}

REC_FUNC_TEST(SWL);
REC_FUNC_TEST(SWR);
