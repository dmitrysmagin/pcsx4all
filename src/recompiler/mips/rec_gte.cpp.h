#ifdef gte_old

#define CP2_REGCACHE

#define CP2_CALLFunc_Flush(func) \
	CALLFunc(func)

#define CP2_CALLFunc_NoFlush(func) \
	CALLFunc(func)

#define CP2_FUNC(f,n,cycle) \
void gte##f(u32 code, u32 pc); void rec##f() \
{ \
	if (autobias) cycles_pending+=cycle; \
	CP2_REGCACHE \
	{ \
		/* TODO: Remove */regClearJump();	/**/ \
		LI32(MIPSREG_A0, psxRegs.code); \
		LI32(MIPSREG_A1, pc); \
		CP2_CALLFunc_##n((u32)gte##f); \
	} \
} \

#define CP2_FUNC2(f,n,cycle) \
void gte##f(u32 code); void rec##f() \
{ \
	if (autobias) cycles_pending+=cycle; \
	CP2_REGCACHE \
	{ \
		/* TODO: Remove */regClearJump();	/**/ \
		LI32(MIPSREG_A0, psxRegs.code); \
		CP2_CALLFunc_##n((u32)gte##f); \
	} \
}

#define CP2_FUNC3(f,n,cycle) \
void gte##f(); void rec##f() \
{ \
	if (autobias) cycles_pending+=cycle; \
	CP2_REGCACHE \
	{ \
		/* TODO: Remove */regClearJump();	/**/ \
		CP2_CALLFunc_##n((u32)gte##f); \
	} \
}

CP2_FUNC2(MFC2,Flush,2);
CP2_FUNC2(MTC2,Flush,2);
CP2_FUNC(LWC2,Flush,3);
CP2_FUNC(SWC2,Flush,4);
CP2_FUNC3(DCPL,NoFlush,8);
CP2_FUNC3(RTPS,NoFlush,15);
CP2_FUNC2(OP,NoFlush,6);
CP2_FUNC3(NCLIP,NoFlush,8);
CP2_FUNC3(DPCS,NoFlush,8);
CP2_FUNC3(INTPL,NoFlush,8);
CP2_FUNC2(MVMVA,NoFlush,8);
CP2_FUNC3(NCDS,NoFlush,19);
CP2_FUNC3(NCDT,NoFlush,44);
CP2_FUNC3(CDP,NoFlush,13);
CP2_FUNC3(NCCS,NoFlush,17);
CP2_FUNC3(CC,NoFlush,11);
CP2_FUNC3(NCS,NoFlush,14);
CP2_FUNC3(NCT,NoFlush,30);
CP2_FUNC2(SQR,NoFlush,3);
CP2_FUNC3(DPCT,NoFlush,17);
CP2_FUNC3(AVSZ3,NoFlush,5);
CP2_FUNC3(AVSZ4,NoFlush,6);
CP2_FUNC3(RTPT,NoFlush,23);
CP2_FUNC2(GPF,NoFlush,5);
CP2_FUNC2(GPL,NoFlush,5);
CP2_FUNC3(NCCT,NoFlush,39);

static void recCFC2()
{
	if (autobias) cycles_pending += 2;
	if (!_Rt_) return;

	u32 rt = regMipsToHost(_Rt_, REG_FIND, REG_REGISTER);

	LW(rt, PERM_REG_1, offCP2C(_Rd_));
	regMipsChanged(_Rt_);
	regUnlock(rt);
}

static void recCTC2()
{
	if (autobias) cycles_pending += 2;
	u32 rt = regMipsToHost(_Rt_, REG_LOAD, REG_REGISTER);
	SW(rt, PERM_REG_1, offCP2C(_Rd_));
	regUnlock(rt);
}

#elif defined(gte_new) || defined(gte_pcsx)

#define CP2_FUNC(f, cycle) \
extern void gte##f(); \
void rec##f() \
{ \
	if (autobias) cycles_pending+=cycle; \
	regClearJump(); \
	LI32(TEMP_1, pc); \
	SW(TEMP_1, PERM_REG_1, off(pc)); \
	LI32(TEMP_1, psxRegs.code); \
	SW(TEMP_1, PERM_REG_1, off(code)); \
	CALLFunc((u32)gte##f); \
} \

CP2_FUNC(MFC2, 2);
CP2_FUNC(MTC2, 2);
CP2_FUNC(LWC2, 3);
CP2_FUNC(SWC2, 4);
CP2_FUNC(DCPL, 8);
CP2_FUNC(RTPS, 15);
CP2_FUNC(OP, 6);
CP2_FUNC(NCLIP, 8);
CP2_FUNC(DPCS, 8);
CP2_FUNC(INTPL, 8);
CP2_FUNC(MVMVA, 8);
CP2_FUNC(NCDS, 19);
CP2_FUNC(NCDT, 44);
CP2_FUNC(CDP, 13);
CP2_FUNC(NCCS, 17);
CP2_FUNC(CC, 11);
CP2_FUNC(NCS, 14);
CP2_FUNC(NCT, 30);
CP2_FUNC(SQR, 3);
CP2_FUNC(DPCT, 17);
CP2_FUNC(AVSZ3, 5);
CP2_FUNC(AVSZ4, 6);
CP2_FUNC(RTPT, 23);
CP2_FUNC(GPF, 5);
CP2_FUNC(GPL, 5);
CP2_FUNC(NCCT, 39);

static void recCFC2()
{
	if (!_Rt_) return;

	u32 rt = regMipsToHost(_Rt_, REG_FIND, REG_REGISTER);

	LW(rt, PERM_REG_1, offCP2C(_Rd_));
	regMipsChanged(_Rt_);
	regUnlock(rt);
}

static void recCTC2()
{
	u32 rt = regMipsToHost(_Rt_, REG_LOAD, REG_REGISTER);
	SW(rt, PERM_REG_1, offCP2C(_Rd_));
	regUnlock(rt);
}

#endif
