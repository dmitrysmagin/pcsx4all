#define CP2_REGCACHE

#define CP2_CALLFunc_Flush(func) \
	CALLFunc(func)

#define CP2_CALLFunc_NoFlush(func) \
	CALLFunc_NoFlush(func)

#define CP2_FUNC(f,n,flush) \
void gte##f(u32 code, u32 pc); void rec##f() \
{ \
	/*if( skCount != 0 ) return;*/ \
	CP2_REGCACHE \
	{ \
		/* TODO: Remove */regClearJump();	/**/ \
		LoadImmediate32(psxRegs->code, MIPSREG_A0); \
		LoadImmediate32(pc, MIPSREG_A1); \
		CP2_CALLFunc_##n((u32)gte##f); \
	} \
} \

#define CP2_FUNC2(f,n,flush) \
void gte##f(u32 code); void rec##f() \
{ \
	/*if( skCount != 0 ) return;*/ \
	CP2_REGCACHE \
	{ \
		/* TODO: Remove */regClearJump();	/**/ \
		LoadImmediate32(psxRegs->code, MIPSREG_A0); \
		CP2_CALLFunc_##n((u32)gte##f); \
	} \
}

#define CP2_FUNC3(f,n,flush) \
void gte##f(); void rec##f() \
{ \
	/*if( skCount != 0 ) return;*/ \
	CP2_REGCACHE \
	{ \
		/* TODO: Remove */regClearJump();	/**/ \
		CP2_CALLFunc_##n((u32)gte##f); \
	} \
}

CP2_FUNC2(MFC2,Flush,true);
CP2_FUNC2(MTC2,Flush,true);
//CP2_FUNC2(CFC2,Flush,true);
//CP2_FUNC2(CTC2,Flush,true);
CP2_FUNC(LWC2,Flush,true);
CP2_FUNC(SWC2,Flush,true);
CP2_FUNC3(DCPL,NoFlush,false);
CP2_FUNC3(RTPS,NoFlush,false);
CP2_FUNC2(OP,NoFlush,false);
CP2_FUNC3(NCLIP,NoFlush,false);
CP2_FUNC3(DPCS,NoFlush,false);
CP2_FUNC3(INTPL,NoFlush,false);
CP2_FUNC2(MVMVA,NoFlush,false);
CP2_FUNC3(NCDS,NoFlush,false);
CP2_FUNC3(NCDT,NoFlush,false);
CP2_FUNC3(CDP,NoFlush,false);
CP2_FUNC3(NCCS,NoFlush,false);
CP2_FUNC3(CC,NoFlush,false);
CP2_FUNC3(NCS,NoFlush,false);
CP2_FUNC3(NCT,NoFlush,false);
CP2_FUNC2(SQR,NoFlush,false);
CP2_FUNC3(DPCT,NoFlush,false);
CP2_FUNC3(AVSZ3,NoFlush,false);
CP2_FUNC3(AVSZ4,NoFlush,false);
CP2_FUNC3(RTPT,NoFlush,false);
CP2_FUNC2(GPF,NoFlush,false);
CP2_FUNC2(GPL,NoFlush,false);
CP2_FUNC3(NCCT,NoFlush,false);

#if 1
static void recCFC2()
{
	/*if( skCount != 0 ) return;*/
	if (!_Rt_) return;

	u32 rt = regMipsToArm(_Rt_, REG_FIND, REG_REGISTER);

	MIPS_LDR_IMM(MIPS_POINTER, rt, PERM_REG_1, CalcDispCP2C(_Rd_));
	regMipsChanged(_Rt_);
	regBranchUnlock(rt);
}

static void recCTC2()
{
	/*if( skCount != 0 ) return;*/
	u32 rt = regMipsToArm(_Rt_, REG_LOAD, REG_REGISTER);
	MIPS_STR_IMM(MIPS_POINTER, rt, PERM_REG_1, CalcDispCP2C(_Rd_));
	regBranchUnlock(rt);
}
#endif
