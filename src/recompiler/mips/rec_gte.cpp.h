#ifdef gte_old

#define CP2_REGCACHE

#define CP2_CALLFunc_Flush(func) \
	CALLFunc(func)

#define CP2_CALLFunc_NoFlush(func) \
	CALLFunc(func)

#define CP2_FUNC(f,n,flush) \
void gte##f(u32 code, u32 pc); void rec##f() \
{ \
	CP2_REGCACHE \
	{ \
		/* TODO: Remove */regClearJump();	/**/ \
		LI32(MIPSREG_A0, psxRegs.code); \
		LI32(MIPSREG_A1, pc); \
		CP2_CALLFunc_##n((u32)gte##f); \
	} \
} \

#define CP2_FUNC2(f,n,flush) \
void gte##f(u32 code); void rec##f() \
{ \
	CP2_REGCACHE \
	{ \
		/* TODO: Remove */regClearJump();	/**/ \
		LI32(MIPSREG_A0, psxRegs.code); \
		CP2_CALLFunc_##n((u32)gte##f); \
	} \
}

#define CP2_FUNC3(f,n,flush) \
void gte##f(); void rec##f() \
{ \
	CP2_REGCACHE \
	{ \
		/* TODO: Remove */regClearJump();	/**/ \
		CP2_CALLFunc_##n((u32)gte##f); \
	} \
}

CP2_FUNC2(MFC2,Flush,true);
CP2_FUNC2(MTC2,Flush,true);
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

static void recCFC2()
{
	if (!_Rt_) return;

	u32 rt = regMipsToArm(_Rt_, REG_FIND, REG_REGISTER);

	LW(rt, PERM_REG_1, offCP2C(_Rd_));
	regMipsChanged(_Rt_);
	regBranchUnlock(rt);
}

static void recCTC2()
{
	u32 rt = regMipsToArm(_Rt_, REG_LOAD, REG_REGISTER);
	SW(rt, PERM_REG_1, offCP2C(_Rd_));
	regBranchUnlock(rt);
}

#elif defined(gte_new) || defined(gte_pcsx)

#define CP2_FUNC(f) \
extern void gte##f(); \
void rec##f() \
{ \
	regClearJump(); \
	LI32(TEMP_1, pc); \
	SW(TEMP_1, PERM_REG_1, off(pc)); \
	LI32(TEMP_1, psxRegs.code); \
	SW(TEMP_1, PERM_REG_1, off(code)); \
	CALLFunc((u32)gte##f); \
} \

CP2_FUNC(MFC2);
CP2_FUNC(MTC2);
CP2_FUNC(LWC2);
CP2_FUNC(SWC2);
CP2_FUNC(DCPL);
CP2_FUNC(RTPS);
CP2_FUNC(OP);
CP2_FUNC(NCLIP);
CP2_FUNC(DPCS);
CP2_FUNC(INTPL);
CP2_FUNC(MVMVA);
CP2_FUNC(NCDS);
CP2_FUNC(NCDT);
CP2_FUNC(CDP);
CP2_FUNC(NCCS);
CP2_FUNC(CC);
CP2_FUNC(NCS);
CP2_FUNC(NCT);
CP2_FUNC(SQR);
CP2_FUNC(DPCT);
CP2_FUNC(AVSZ3);
CP2_FUNC(AVSZ4);
CP2_FUNC(RTPT);
CP2_FUNC(GPF);
CP2_FUNC(GPL);
CP2_FUNC(NCCT);

static void recCFC2()
{
	if (!_Rt_) return;

	u32 rt = regMipsToArm(_Rt_, REG_FIND, REG_REGISTER);

	LW(rt, PERM_REG_1, offCP2C(_Rd_));
	regMipsChanged(_Rt_);
	regBranchUnlock(rt);
}

static void recCTC2()
{
	u32 rt = regMipsToArm(_Rt_, REG_LOAD, REG_REGISTER);
	SW(rt, PERM_REG_1, offCP2C(_Rd_));
	regBranchUnlock(rt);
}

#endif
