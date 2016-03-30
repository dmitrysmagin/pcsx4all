#ifndef __MIPS_EXTERNS_H__
#define __MIPS_EXTERNS_H__

typedef struct {
	u32 	mappedto;
	u32		host_age;
	u32		host_use;
	u32		host_type;
	bool	ismapped;
	int	host_islocked;
} HOST_RecRegister;


typedef struct {
	u32 		mappedto;
	bool		ismapped;
	bool		psx_ischanged;
} PSX_RecRegister;


typedef struct {
	PSX_RecRegister 	psx[32];
	HOST_RecRegister		host[32];
	u32								reglist[32];
	u32								reglist_cnt;
} RecRegisters;


extern psxRegisters 	psxRegs;
extern RecRegisters 	regcache;

extern u32 stores[4];                                                         
extern u32 rotations[4];                                                      

#ifdef WITH_REG_STATS
extern u32 reg_count[32];
extern u32 reg_mapped_count[32];
#endif

#ifdef WITH_DISASM
extern FILE *translation_log_fp;
extern char disasm_buffer[512];
#endif

/* Functions */
extern void rec_test_pc();

extern u32 recIntExecuteBlock(u32 newpc);
extern u32 psxBranchTest_rec(u32 cycles, u32 pc);
extern u32 psx_interrupt(u32 reg);

//extern void regReset();
//extern void regClearJump();
//extern void ClearReg(u32 ra, u32 clearthis);

static u32 recRecompile();

#endif
