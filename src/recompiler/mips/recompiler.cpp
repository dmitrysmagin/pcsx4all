//#define NO_ZERO_REGISTER_OPTIMISATION	1

#include "../common.h"
#include "recompiler.h"


#include "mips_std_rec_calls.cpp"
#include "mips_std_rec_globals.cpp"
#include "mips_std_rec_debug.cpp"
#include "mips_std_rec_regcache.cpp"

#include "../../evaluator/evaluator.cpp.h"
#include "../../generator/mips/generator.cpp.h"

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

u8*	current_translation_ptr;
u32 	opcode;
u32* 	recMemStart;
u32		isInBios = 0;
u32 	loadedpermregs = 0;
u32	end_block = 0;
// Let's hope the number of indirect branches in a block doesnt exceed this large number of elements
u32 recBranches[1024];
//u32 recBranchesPatch[1024];
u32 recBranchesPatchAddr[1024];
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
  make_stub_label(psxMemReadS8),
  make_stub_label(psxMemRead16),
  make_stub_label(psxMemReadS16),
  make_stub_label(psxMemRead32),
  make_stub_label(psxMemWrite8),
  make_stub_label(psxMemWrite16),
  make_stub_label(psxMemWrite32),
  make_stub_label(psxException),
  make_stub_label(psxBranchTest_rec)
};

const u32 num_stub_labels = sizeof(stub_labels) / sizeof(disasm_label);

#define DISASM_INIT								\
  	/*translation_log_fp = fopen("/mnt/sd/translation_log.txt", "a+");*/     	\
  	printf(/*translation_log_fp, */"Block PC %x (MIPS) -> %p\n", pc,  \
   		recMemStart);                                          	        \

#define DISASM_PSX								\
		disasm_mips_instruction(psxRegs->code,disasm_buffer,pc, 0, 0);	\
    		DEBUGG(/*translation_log_fp, */"%08x: %08x %s\n", pc, 		\
			psxRegs->code, disasm_buffer);   			\

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
	MIPS_STR_IMM(MIPS_POINTER, TEMP_1, PERM_REG_1, 648); 							\
	LoadImmediate32(psxRegs->code, TEMP_1); 									\
	MIPS_STR_IMM(MIPS_POINTER, TEMP_1, PERM_REG_1, 652); 							\
	CALLFunc((u32)psx##f); 														\
}

#include "rec_lsu.cpp.h" // Load Store Unit
#include "rec_gte.cpp.h" // Geometry Transformation Engine
#include "rec_alu.cpp.h" // Arithmetic Logical Unit
#include "rec_mdu.cpp.h" // Multiple Divide Unit
#include "rec_cp0.cpp.h" // Coprocessor 0
#include "rec_bcu.cpp.h" // Branch Control Unit

#define rec_recompile_start() 													\
{																				\
		if( loadedpermregs == 0 ) 												\
		{ 																		\
			LoadImmediate32((u32)psxRegs, PERM_REG_1); 							\
			loadedpermregs = 1; 												\
		} 																		\
}

void rec_flush_cache()
{
	clear_insn_cache((u32)recMemBase, ((u32)recMemBase) + RECMEM_SIZE, 0);
}

static INLINE u32 recScanBlock(u32 scanpc, u32 numbranches)
{
	u32 scancode;

	if( (u32)(PC_REC32(recBranches[ibranch])) != 0 ) return numbranches;

	while(1)
	{
		scancode = *(u32*)((psxMemRLUT[scanpc>>16] + (scanpc&0xffff)));
		scanpc += 4;
		switch(scancode >> 26)
		{
		case 0:
			switch(_fFunct_(scancode))
			{
			case 8: return numbranches; //recJR
			case 9: return numbranches; //recJALR
			case 12: return numbranches; //recSYSCALL
			}
		break;
		case 1:
			switch(_fRt_(scancode))
			{
			case 0: //recBLTZ
				if(_fRs_(scancode) == 0)
				{
					recBranches[numbranches] = scanpc + 4;
					numbranches++;
					numbranches = recScanBlock(recBranches[numbranches-1], numbranches);
					return numbranches;
				}
			break;
			case 1: //recBGEZ
				if(_fRs_(scancode) == 0)
				{
					recBranches[numbranches] =  _fImm_(scancode) * 4 + scanpc;
					numbranches++;
					numbranches = recScanBlock(recBranches[numbranches-1], numbranches);
					return numbranches;
				}
			break;
			case 16: //recBLTZAL
				if(_fRs_(scancode) == 0)
				{
					recBranches[numbranches] = scanpc + 4;
					numbranches++;
					numbranches = recScanBlock(recBranches[numbranches-1], numbranches);
					return numbranches;
				}
			break;
			case 17: //recBGEZAL
				if(_fRs_(scancode) == 0)
				{
					recBranches[numbranches] =  _fImm_(scancode) * 4 + scanpc;
					numbranches++;
					numbranches = recScanBlock(recBranches[numbranches-1], numbranches);
					return numbranches;
				}
			break;
			}
		break;
		case 2: //recJ
			recBranches[numbranches] = _fTarget_(scancode) * 4 + (scanpc & 0xf0000000);
			numbranches++;
			numbranches = recScanBlock(recBranches[numbranches-1], numbranches);
			return numbranches;
		break;	
		case 3: //recJAL
			recBranches[numbranches] = _fTarget_(scancode) * 4 + (scanpc & 0xf0000000);
			numbranches++;
			numbranches = recScanBlock(recBranches[numbranches-1], numbranches);
			return numbranches;
		break;
		case 4: //recBEQ
			if( (_fRs_(scancode) == _fRt_(scancode)) )
			{
				recBranches[numbranches] =  ((_fImm_(scancode)) * 4) + scanpc;
				numbranches++;
				numbranches = recScanBlock(recBranches[numbranches-1], numbranches);
				return numbranches;
			}
		break;
		case 5: //recBNE
			if( (_fRs_(scancode) == 0 && _fRt_(scancode) == 0) )
			{
				recBranches[numbranches] = scanpc + 4;
				numbranches++;
				numbranches = recScanBlock(recBranches[numbranches-1], numbranches);
				return numbranches;
			}
		break;
		case 6: //recBLEZ
			if( (_fRs_(scancode) == 0) )
			{
				recBranches[numbranches] =  _fImm_(scancode) * 4 + scanpc;
				numbranches++;
				numbranches = recScanBlock(recBranches[numbranches-1], numbranches);
				return numbranches;
			}
		break;
		case 7: //recBGTZ
			if( (_fRs_(scancode) == 0) )
			{
				recBranches[numbranches] = scanpc + 4;
				numbranches++;
				numbranches = recScanBlock(recBranches[numbranches-1], numbranches);
				return numbranches;
			}
		break;
		case 59: //recHLE
			return numbranches;
		break;
		}
	}
}

static u32 recRecompile()
{
	if ( (u32)recMem - (u32)recMemBase >= RECMEM_SIZE_MAX )
		recReset();

	recMem = (u32*)(((u32)recMem + 64) & ~(63));
	recMemStart = recMem;

	regReset();

	blockcycles = 0;

	PC_REC32(psxRegs->pc) = (u32)recMem;
	oldpc = pc = psxRegs->pc;
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
		else if( isInBios == 2 && psxRegs->pc == 0x80030000 )
		{
			PC_REC32(psxRegs->pc) = 0;
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
			clear_insn_cache((u32)recMemStart, (u32)recMem, 0);
			return (u32)recMemStart;
		}
	}

	rec_recompile_start();
	memset(psxRegs->iRegs, 0xff, 32*4);

	for (;;)
	{
		psxRegs->code = *(u32*)((psxMemRLUT[pc>>16] + (pc&0xffff)));
		DISASM_PSX
		pc+=4;
		recBSC[psxRegs->code>>26]();
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
			recRet();
			DISASM_HOST
			clear_insn_cache((u32)recMemStart, (u32)recMem, 0);
			return (u32)recMemStart;
		}
	}

	return (u32)recMemStart;
}

static void recNULL() { }

static int recInit()
{
	int i;

	psxRegs = &recRegs;

	recMem = (u32*)recMemBase;	
	loadedpermregs = 0;	
	recReset();

	//recRAM = (char*) malloc(0x200000);
	//recROM = (char*) malloc(0x080000);
	if (recRAM == NULL || recROM == NULL || recMemBase == NULL || psxRecLUT == NULL) {
		SysMessage("Error allocating memory"); return -1;
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

	void (**recFunc)() = (void (**)()) (u32)PC_REC(psxRegs->pc);

	if (*recFunc == 0) 
		recRecompile();

	(*recFunc)();
}

static void recExecuteBlock()
{
	isInBios = 1;
	loadedpermregs = 0;	

	void (**recFunc)() = (void (**)()) (u32)PC_REC(psxRegs->pc);

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
	//rec_flush_cache();

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
