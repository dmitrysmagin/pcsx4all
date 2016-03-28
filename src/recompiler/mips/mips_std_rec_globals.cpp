#include "../common.h"

psxRegisters 	recRegs;
psxRegisters* 	psxRegs;
RecRegisters 	regcache;

u32 psxRecLUT[0x010000];
u8 recMemBase[RECMEM_SIZE];
u32 *recMem;					/* the recompiled blocks will be here */
s8 recRAM[0x200000];				/* and the ptr to the blocks here */
s8 recROM[0x080000];				/* and here */
u32 pc;						/* recompiler pc */
u32 oldpc;
u32 branch = 0;
u32 rec_count;

#ifdef WITH_REG_STATS
u32 reg_count[32];
u32 reg_mapped_count[32];
#endif

#ifdef WITH_DISASM
FILE *translation_log_fp = NULL;
char disasm_buffer[512];
#endif

u32 stores[4];                                                         
u32 rotations[4];
