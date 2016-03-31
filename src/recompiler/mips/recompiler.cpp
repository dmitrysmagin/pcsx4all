//#define NO_ZERO_REGISTER_OPTIMISATION	1

#include "psxcommon.h"
#include "psxhle.h"
#include "psxmem.h"
#include "r3000a.h"
#include "gte.h"

#include "defines.h"
#include "mips_codegen.h"
#include "disasm.h"

/* Regcache data */
typedef struct {
	u32	mappedto;
	u32	host_age;
	u32	host_use;
	u32	host_type;
	bool	ismapped;
	int	host_islocked;
} HOST_RecRegister;

typedef struct {
	u32	mappedto;
	bool	ismapped;
	bool	psx_ischanged;
} PSX_RecRegister;

typedef struct {
	PSX_RecRegister		psx[32];
	HOST_RecRegister	host[32];
	u32			reglist[32];
	u32			reglist_cnt;
} RecRegisters;

RecRegisters	regcache;
static u32 iRegs[32]; /* used for imm caching and back up of regs in dynarec */

static u32 psxRecLUT[0x010000];

#undef PC_REC
#undef PC_REC8
#undef PC_REC16
#undef PC_REC32
#define PC_REC(x)	(psxRecLUT[(x) >> 16] + ((x) & 0xffff))
#define PC_REC8(x)	(*(u8 *)PC_REC(x))
#define PC_REC16(x)	(*(u16*)PC_REC(x))
#define PC_REC32(x)	(*(u32*)PC_REC(x))

static u8 recMemBase[RECMEM_SIZE + (REC_MAX_OPCODES*2) + 0x4000] __attribute__ ((__aligned__ (32)));
static u32 *recMem;				/* the recompiled blocks will be here */
static s8 recRAM[0x200000];			/* and the ptr to the blocks here */
static s8 recROM[0x080000];			/* and here */
static u32 pc;					/* recompiler pc */
static u32 oldpc;
static u32 branch = 0;

#ifdef WITH_DISASM
FILE *translation_log_fp = NULL;
char disasm_buffer[512];
#endif

#include "regcache.h"
#include "interpreter_old.cpp"

static void recReset();
static u32 recRecompile();
static void recClear(u32 Addr, u32 Size);

extern void (*recBSC[64])();
extern void (*recSPC[64])();
extern void (*recREG[32])();
extern void (*recCP0[32])();
extern void (*recCP2[64])();
extern void (*recCP2BSC[32])();

extern int skCount;

u8	*current_translation_ptr;
u32 	opcode;
u32	*recMemStart;
u32	isInBios = 0;
u32 	loadedpermregs = 0;
u32	end_block = 0;
int ibranch;
u32 blockcycles = 0;

#ifdef WITH_DISASM

#define make_stub_label(name)                                                 \
 { (void *)name, (char*)#name }                                                      \

disasm_label stub_labels[] =
{
  make_stub_label(gteMFC2),
  make_stub_label(gteMTC2),
  make_stub_label(gteLWC2),
  make_stub_label(gteSWC2),
  make_stub_label(gteRTPS),
  make_stub_label(gteOP),
  make_stub_label(gteNCLIP),
  make_stub_label(gteDPCS),
  make_stub_label(gteINTPL),
  make_stub_label(gteMVMVA),
  make_stub_label(gteNCDS),
  make_stub_label(gteNCDT),
  make_stub_label(gteCDP),
  make_stub_label(gteNCCS),
  make_stub_label(gteCC),
  make_stub_label(gteNCS),
  make_stub_label(gteNCT),
  make_stub_label(gteSQR),
  make_stub_label(gteDCPL),
  make_stub_label(gteDPCT),
  make_stub_label(gteAVSZ3),
  make_stub_label(gteAVSZ4),
  make_stub_label(gteRTPT),
  make_stub_label(gteGPF),
  make_stub_label(gteGPL),
  make_stub_label(gteNCCT),
//  make_stub_label(psxBranchTest_full),
  make_stub_label(psxMemRead8),
  make_stub_label(psxMemRead16),
  make_stub_label(psxMemRead32),
  make_stub_label(psxMemWrite8),
  make_stub_label(psxMemWrite16),
  make_stub_label(psxMemWrite32),
  make_stub_label(psxException),
  make_stub_label(psxBranchTest_rec),
  make_stub_label(psx_interrupt)
};

const u32 num_stub_labels = sizeof(stub_labels) / sizeof(disasm_label);

#define DISASM_INIT								\
  	/*translation_log_fp = fopen("/mnt/sd/translation_log.txt", "a+");*/     	\
  	printf(/*translation_log_fp, */"Block PC %x (MIPS) -> %p\n", pc,  \
   		recMemStart);                                          	        \

#define DISASM_PSX								\
		disasm_mips_instruction(psxRegs.code,disasm_buffer,pc, 0, 0);	\
    		DEBUGG(/*translation_log_fp, */"%08x: %08x %s\n", pc, 		\
			psxRegs.code, disasm_buffer);   			\

#define DISASM_HOST								\
	DEBUGG(/*translation_log_fp, */"\n");                                      \
  	for(	current_translation_ptr = (u8*)recMemStart;            	    	\
   		(u32)current_translation_ptr < (u32)recMem; 			\
		current_translation_ptr += 4)  					\
  	{                                                                       \
    		opcode = *(u32*)current_translation_ptr;			\
		disasm_mips_instruction(opcode, disasm_buffer,                   \
     			(u32)current_translation_ptr, stub_labels,		\
			num_stub_labels);     			        	\
    		DEBUGG(/*translation_log_fp, */"%08x: %s\t(0x%08x)\n", 			\
			current_translation_ptr, disasm_buffer, opcode);	        \
  	}                                                                       \
                                                                              	\
  	DEBUGG(/*translation_log_fp, */"\n");                                      \
 	/*fflush(translation_log_fp);*/                                             \
	/*gp2x_sync();*/								\
  	/*fclose(translation_log_fp);*/						\

#else

#define DISASM_PSX								\

#define DISASM_HOST								\

#define DISASM_INIT								\

#endif

#define REC_FUNC_TEST(f) 														\
extern void psx##f(); 															\
void rec##f() 																	\
{ 																				\
	regClearJump();																\
	LoadImmediate32(pc, TEMP_1); 												\
	MIPS_STR_IMM(MIPS_POINTER, TEMP_1, PERM_REG_1, offpc); 							\
	LoadImmediate32(psxRegs.code, TEMP_1); 									\
	MIPS_STR_IMM(MIPS_POINTER, TEMP_1, PERM_REG_1, offcode); 							\
	CALLFunc((u32)psx##f); 														\
}

#include "gen_alu.h"     // Helpers for generating ALU opcodes
#include "rec_lsu.cpp.h" // Load Store Unit
#include "rec_gte.cpp.h" // Geometry Transformation Engine
#include "rec_alu.cpp.h" // Arithmetic Logical Unit
#include "rec_mdu.cpp.h" // Multiple Divide Unit
#include "rec_cp0.cpp.h" // Coprocessor 0
#include "rec_bcu.cpp.h" // Branch Control Unit

#include <sys/cachectl.h>

void clear_insn_cache(void *start, void *end, int flags)
{
	cacheflush(start, (char *)end - (char *)start, ICACHE);
}

static u32 recRecompile()
{
	if ( (u32)recMem - (u32)recMemBase >= RECMEM_SIZE_MAX )
		recReset();

	recMem = (u32*)(((u32)recMem + 64) & ~(63));
	recMemStart = recMem;

	regReset();

	blockcycles = 0;

	PC_REC32(psxRegs.pc) = (u32)recMem;
	oldpc = pc = psxRegs.pc;
	ibranch = 0;

	DISASM_INIT

	if( isInBios )
	{
		if( isInBios == 1 )
		{
			isInBios = 2;
			MIPS_PUSH(MIPS_POINTER, MIPSREG_RA);
			MIPS_PUSH(MIPS_POINTER, MIPSREG_S8);
			MIPS_PUSH(MIPS_POINTER, MIPSREG_S7);
			MIPS_PUSH(MIPS_POINTER, MIPSREG_S6);
			MIPS_PUSH(MIPS_POINTER, MIPSREG_S5);
			MIPS_PUSH(MIPS_POINTER, MIPSREG_S4);
			MIPS_PUSH(MIPS_POINTER, MIPSREG_S3);
			MIPS_PUSH(MIPS_POINTER, MIPSREG_S2);
			MIPS_PUSH(MIPS_POINTER, MIPSREG_S1);
			MIPS_PUSH(MIPS_POINTER, MIPSREG_S0);
		}
		else if( isInBios == 2 && psxRegs.pc == 0x80030000 )
		{
			PC_REC32(psxRegs.pc) = 0;
			isInBios = 0;
			MIPS_POP(MIPS_POINTER, MIPSREG_S0);
			MIPS_POP(MIPS_POINTER, MIPSREG_S1);
			MIPS_POP(MIPS_POINTER, MIPSREG_S2);
			MIPS_POP(MIPS_POINTER, MIPSREG_S3);
			MIPS_POP(MIPS_POINTER, MIPSREG_S4);
			MIPS_POP(MIPS_POINTER, MIPSREG_S5);
			MIPS_POP(MIPS_POINTER, MIPSREG_S6);
			MIPS_POP(MIPS_POINTER, MIPSREG_S7);
			MIPS_POP(MIPS_POINTER, MIPSREG_S8);
			MIPS_POP(MIPS_POINTER, MIPSREG_RA);
			MIPS_EMIT(MIPS_POINTER, 0x00000008 | (MIPSREG_RA << 21)); /* jr ra */
			clear_insn_cache(recMemStart, recMem, 0);
			return (u32)recMemStart;
		}
	}

	rec_recompile_start();
	memset(iRegs, 0xff, 32*4);

	for (;;)
	{
		//psxRegs.code = *(u32*)((psxMemRLUT[pc>>16] + (pc&0xffff)));
		psxRegs.code = *(u32 *)((char *)PSXM(pc));
		DISASM_PSX
		pc+=4;
		recBSC[psxRegs.code>>26]();
		int ilock;
		for(ilock = REG_CACHE_START; ilock < REG_CACHE_END; ilock++)
		{					
			if( regcache.host[ilock].ismapped )
			{
				regcache.host[ilock].host_age++;
				regcache.host[ilock].host_islocked = 0;
			}
		}
		branch = 0;
		if (end_block)
		{
			end_block = 0;
			rec_recompile_end();
			DISASM_HOST
			clear_insn_cache(recMemStart, recMem, 0);
			return (u32)recMemStart;
		}
	}

	return (u32)recMemStart;
}

static void recNULL() { }

static int recInit()
{
	int i;

	recMem = (u32*)recMemBase;
	memset(recMem, 0, RECMEM_SIZE + (REC_MAX_OPCODES*2) + 0x4000);
	loadedpermregs = 0;
	recReset();

	//recRAM = (char*) malloc(0x200000);
	//recROM = (char*) malloc(0x080000);
	if (recRAM == NULL || recROM == NULL || recMemBase == NULL || psxRecLUT == NULL) {
		printf("Error allocating memory\n"); return -1;
	}

	for (i=0; i<0x80; i++) psxRecLUT[i + 0x0000] = (u32)&recRAM[(i & 0x1f) << 16];
	memcpy(psxRecLUT + 0x8000, psxRecLUT, 0x80 * 4);
	memcpy(psxRecLUT + 0xa000, psxRecLUT, 0x80 * 4);

	for (i=0; i<0x08; i++) psxRecLUT[i + 0xbfc0] = (u32)&recROM[i << 16];

	return 0;
}

static void recShutdown() { }

static void recSPECIAL()
{
	recSPC[_Funct_]();
}

static void recREGIMM()
{
	recREG[_Rt_]();
}

static void recCOP0()
{
	recCP0[_Rs_]();
}

static void recCOP2()
{
	recCP2[_Funct_]();
}

static void recBASIC()
{
	recCP2BSC[_Rs_]();
}

void (*recBSC[64])() =
{
	recSPECIAL, recREGIMM, recJ   , recJAL  , recBEQ , recBNE , recBLEZ, recBGTZ,
	recADDI   , recADDIU , recSLTI, recSLTIU, recANDI, recORI , recXORI, recLUI ,
	recCOP0   , recNULL  , recCOP2, recNULL , recNULL, recNULL, recNULL, recNULL,
	recNULL   , recNULL  , recNULL, recNULL , recNULL, recNULL, recNULL, recNULL,
	recLB     , recLH    , recLWL , recLW   , recLBU , recLHU , recLWR , recNULL,
	recSB     , recSH    , recSWL , recSW   , recNULL, recNULL, recSWR , recNULL,
	recNULL   , recNULL  , recLWC2, recNULL , recNULL, recNULL, recNULL, recNULL,
	recNULL   , recNULL  , recSWC2, recHLE  , recNULL, recNULL, recNULL, recNULL
};

void (*recSPC[64])() =
{
	recSLL , recNULL, recSRL , recSRA , recSLLV   , recNULL , recSRLV, recSRAV,
	recJR  , recJALR, recNULL, recNULL, recSYSCALL, recBREAK, recNULL, recNULL,
	recMFHI, recMTHI, recMFLO, recMTLO, recNULL   , recNULL , recNULL, recNULL,
	recMULT, recMULTU, recDIV, recDIVU, recNULL   , recNULL , recNULL, recNULL,
	recADD , recADDU, recSUB , recSUBU, recAND    , recOR   , recXOR , recNOR ,
	recNULL, recNULL, recSLT , recSLTU, recNULL   , recNULL , recNULL, recNULL,
	recNULL, recNULL, recNULL, recNULL, recNULL   , recNULL , recNULL, recNULL,
	recNULL, recNULL, recNULL, recNULL, recNULL   , recNULL , recNULL, recNULL
};

void (*recREG[32])() =
{
	recBLTZ  , recBGEZ  , recNULL, recNULL, recNULL, recNULL, recNULL, recNULL,
	recNULL  , recNULL  , recNULL, recNULL, recNULL, recNULL, recNULL, recNULL,
	recBLTZAL, recBGEZAL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL,
	recNULL  , recNULL  , recNULL, recNULL, recNULL, recNULL, recNULL, recNULL
};

void (*recCP0[32])() =
{
	recMFC0, recNULL, recCFC0, recNULL, recMTC0, recNULL, recCTC0, recNULL,
	recNULL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL,
	recRFE , recNULL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL,
	recNULL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL
};

void (*recCP2[64])() =
{
	recBASIC, recRTPS , recNULL , recNULL, recNULL, recNULL , recNCLIP, recNULL, // 00
	recNULL , recNULL , recNULL , recNULL, recOP  , recNULL , recNULL , recNULL, // 08
	recDPCS , recINTPL, recMVMVA, recNCDS, recCDP , recNULL , recNCDT , recNULL, // 10
	recNULL , recNULL , recNULL , recNCCS, recCC  , recNULL , recNCS  , recNULL, // 18
	recNCT  , recNULL , recNULL , recNULL, recNULL, recNULL , recNULL , recNULL, // 20
	recSQR  , recDCPL , recDPCT , recNULL, recNULL, recAVSZ3, recAVSZ4, recNULL, // 28 
	recRTPT , recNULL , recNULL , recNULL, recNULL, recNULL , recNULL , recNULL, // 30
	recNULL , recNULL , recNULL , recNULL, recNULL, recGPF  , recGPL  , recNCCT  // 38
};

void (*recCP2BSC[32])() =
{
	recMFC2, recNULL, recCFC2, recNULL, recMTC2, recNULL, recCTC2, recNULL,
	recNULL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL,
	recNULL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL,
	recNULL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL
};

static void recExecute()
{
	loadedpermregs = 0;

	void (**recFunc)() = (void (**)()) (u32)PC_REC(psxRegs.pc);

	if (*recFunc == 0) 
		recRecompile();

	(*recFunc)();
}

static void recExecuteBlock(unsigned target_pc)
{
	isInBios = 1;
	loadedpermregs = 0;	

	void (**recFunc)() = (void (**)()) (u32)PC_REC(psxRegs.pc);

	if (*recFunc == 0) 
		recRecompile();

	(*recFunc)();
}

static void recClear(u32 Addr, u32 Size)
{
	//memset((u32*)PC_REC(Addr), 0, (Size * 4));
	//memset(recRAM, 0, 0x200000);
	//printf("addr %x\n", Addr);

	memset((u32*)PC_REC(Addr), 0, (Size * 4));
	//memset(recRAM+0x0, 0, 0x200000);

	if( Addr == 0x8003d000 )
	{
		// temp fix for Buster Bros Collection and etc.
		memset(recRAM+0x4d88, 0, 0x8);
		//recReset();
		//memset(recRAM, 0, 0x200000);
		//memset(&recRAM[0x1362<<1], 0, (0x4));
	}
}

static void recReset()
{
	memset(recRAM, 0, 0x200000);
	memset(recROM, 0, 0x080000);

	//memset((void*)recMemBase, 0, RECMEM_SIZE);

	recMem = (u32*)recMemBase;

	regReset();

	branch = 0;	
	end_block = 0;
}

R3000Acpu psxRec =
{
	recInit,
	recReset,
	recExecute,
	recExecuteBlock,
	recClear,
	recShutdown
};
