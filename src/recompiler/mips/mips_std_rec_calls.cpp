#include "common.h"

extern void psxBranchTest();

u32 psxBranchTest_rec(u32 cycles, u32 pc)
{
	/* Misc helper */
	psxRegs.pc = pc;
	psxRegs.cycle += cycles;

	psxBranchTest();

	u32 compiledpc = (u32)PC_REC32(psxRegs.pc);
	if( compiledpc != 0 )
	{
		//DEBUGF("returning to 0x%x (t2 0x%x t3 0x%x)\n", compiledpc, psxRegs.GPR.n.t2, psxRegs.GPR.n.t3);
		return compiledpc;
	}
	u32 a = recRecompile();
	//DEBUGF("returning to 0x%x (t2 0x%x t3 0x%x)\n", a, psxRegs.GPR.n.t2, psxRegs.GPR.n.t3);
	return a;
}

u32 psx_interrupt(u32 reg)
{
	if(reg == 12)
		psxRegs.interrupt |= 0x80000000;
	return 0;
}

#ifdef IPHONE
extern "C" void sys_icache_invalidate(const void* Addr, size_t len);

void clear_insn_cache(u32 start, u32 end, int type)
{
	sys_icache_invalidate((void*)start, end - start);
}
#endif
