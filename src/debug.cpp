/***************************************************************************
*   Copyright (C) 2010 PCSX4ALL Team                                      *
*   Copyright (C) 2010 Chui                                               *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   51 Franklin Street, Fifth Floor, Boston, MA 02111-1307 USA.           *
***************************************************************************/

#include "psxcommon.h"
#include "psxmem.h"
#include "r3000a.h"
#include "debug.h"

#ifdef DEBUG_PCSX4ALL

#ifdef DEBUG_START
#if (DEBUG_START==0) && !defined(DEBUG_FRAME)
int dbg_now=1;
#else
int dbg_now=0;
#endif
#else
#if defined(DEBUG_FRAME) || defined(DEBUG_ANALYSIS)
int dbg_now=0;
#else
int dbg_now=1;
#endif
#endif

int dbg_frame=0;
#ifdef DEBUG_BIOS
unsigned dbg_biosunset();
#endif

void dbgsum(char *str, void *buff, unsigned len) {
	if (dbg_now) {
		unsigned char *p=(unsigned char *)buff;
		unsigned i,ret=0;
		for(i=0;i<len;i++,p++)
			ret+=(*p)*i;
		fprintf(DEBUG_STR_FILE,"%s : 0x%X\n",str,ret);
#ifdef DEBUG_PCSX4ALL_FFLUSH
		fflush(DEBUG_STR_FILE);
#endif
	}
}

void dbgregs(void *r) {
	if (dbg_now) {
		int i;
		psxRegisters *regs=(psxRegisters *)r;
		for(i=0;i<34;i++) {
			fprintf(DEBUG_STR_FILE," R%.2i=%.8X",i,regs->GPR.r[i]);
			if (!((i+1)&3)) fprintf(DEBUG_STR_FILE,"\n");
		}
#ifdef DEBUG_BIOS
		fprintf(DEBUG_STR_FILE," INT=%.8X CYL=%i",regs->interrupt,regs->cycle);
#endif
		fprintf(DEBUG_STR_FILE," IOC=%u\n",regs->io_cycle_counter);
#ifdef DEBUG_PCSX4ALL_FFLUSH
		fflush(DEBUG_STR_FILE);
#endif
	}
}

void dbgregsCop(void *r) {
	if (dbg_now) {
		int i;
		psxRegisters *regs=(psxRegisters *)r;
		for(i=0;i<32;i++) {
			fprintf(DEBUG_STR_FILE," CP0 R%.2i=%.8X",i,regs->CP0.r[i]);
			if (!((i+1)&3)) fprintf(DEBUG_STR_FILE,"\n");
		}
		fprintf(DEBUG_STR_FILE,"\n");
		for(i=0;i<32;i++) {
			fprintf(DEBUG_STR_FILE," CP2D R%.2i=%.8X",i,regs->CP2D.r[i]);
			if (!((i+1)&3)) fprintf(DEBUG_STR_FILE,"\n");
		}
		fprintf(DEBUG_STR_FILE,"\n");
		for(i=0;i<32;i++) {
			fprintf(DEBUG_STR_FILE," CP2C R%.2i=%.8X",i,regs->CP2C.r[i]);
			if (!((i+1)&3)) fprintf(DEBUG_STR_FILE,"\n");
		}
		fprintf(DEBUG_STR_FILE,"\n");
		for(i=0;i<32;i++) {
			fprintf(DEBUG_STR_FILE," INT %.2i=%.8X",i,regs->intCycle[i]);
			if (!((i+1)&3)) fprintf(DEBUG_STR_FILE,"\n");
		}
		fprintf(DEBUG_STR_FILE,"\n");
#ifdef DEBUG_PCSX4ALL_FFLUSH
		fflush(DEBUG_STR_FILE);
#endif
	}
}

void dbgpsxregs(void){
	dbgregs((void *)&psxRegs);
}

void dbgpsxregsCop(void){
	dbgregsCop((void *)&psxRegs);
}

static const char *strBSC[64]={
	"SPECIAL","REGIMM","J","JAL","BEQ","BNE","BLEZ","BGTZ",
	"ADDI","ADDIU","SLTI","SLTIU","ANDI","ORI","XORI","LUI",
	"COP0","NULL","COP2","NULL","NULL","NULL","NULL","NULL",
	"NULL","NULL","NULL","NULL","NULL","NULL","NULL","NULL",
	"LB","LH","LWL","LW","LBU","LHU","LWR","NULL",
	"SB","SH","SWL","SW","NULL","NULL","SWR","NULL",
	"NULL","NULL","LWC2","NULL","NULL","NULL","NULL","NULL",
	"NULL","NULL","SWC2","HLE","NULL","NULL","NULL","NULL"
};

static unsigned dbg_anacntBSC[64];

static const char *strSPC[64]={
	"SLL","NULL","SRL","SRA","SLLV","NULL","SRLV","SRAV",
	"JR","JALR","NULL","NULL","SYSCALL","BREAK","NULL","NULL",
	"MFHI","MTHI","MFLO","MTLO","NULL","NULL","NULL","NULL",
	"MULT","MULTU","DIV","DIVU","NULL","NULL","NULL","NULL",
	"ADD","ADDU","SUB","SUBU","AND","OR","XOR","NOR",
	"NULL","NULL","SLT","SLTU","NULL","NULL","NULL","NULL",
	"NULL","NULL","NULL","NULL","NULL","NULL","NULL","NULL",
	"NULL","NULL","NULL","NULL","NULL","NULL","NULL","NULL"
};

static unsigned dbg_anacntSPC[64];

static const char *strREG[32]={
	"BLTZ","BGEZ","NULL","NULL","NULL","NULL","NULL","NULL",
	"NULL","NULL","NULL","NULL","NULL","NULL","NULL","NULL",
	"BLTZAL","BGEZAL","NULL","NULL","NULL","NULL","NULL","NULL",
	"NULL","NULL","NULL","NULL","NULL","NULL","NULL","NULL"
};

static unsigned dbg_anacntREG[32];

static const char *strCP0[32]={
	"MFC0","NULL","CFC0","NULL","MTC0","NULL","CTC0","NULL",
	"NULL","NULL","NULL","NULL","NULL","NULL","NULL","NULL",
	"RFE","NULL","NULL","NULL","NULL","NULL","NULL","NULL",
	"NULL","NULL","NULL","NULL","NULL","NULL","NULL","NULL"
};

static unsigned dbg_anacntCP0[32];

static const char *strCP2[64]={
	"BASIC","RTPS","NULL","NULL","NULL","NULL","NCLIP","NULL",
	"NULL","NULL","NULL","NULL","OP","NULL","NULL","NULL",
	"DPCS","INTPL","MVMVA","NCDS","CDP","NULL","NCDT","NULL",
	"NULL","NULL","NULL","NCCS","CC","NULL","NCS","NULL",
	"NCT","NULL","NULL","NULL","NULL","NULL","NULL","NULL",
	"SQR","DCPL","DPCT","NULL","NULL","AVSZ3","AVSZ4","NULL",
	"RTPT","NULL","NULL","NULL","NULL","NULL","NULL","NULL",
	"NULL","NULL","NULL","NULL","NULL","GPF","GPL","NCCT"
};

static unsigned dbg_anacntCP2[64];

static const char *strCP2BSC[32]={
	"MFC2","NULL","CFC2","NULL","MTC2","NULL","CTC2","NULL",
	"NULL","NULL","NULL","NULL","NULL","NULL","NULL","NULL",
	"NULL","NULL","NULL","NULL","NULL","NULL","NULL","NULL",
	"NULL","NULL","NULL","NULL","NULL","NULL","NULL","NULL"
};

static unsigned dbg_anacntCP2BSC[32];
static unsigned dbg_anacnt_total_opcodes=0;
static unsigned dbg_anacnt_total_opcodes_block=0;
unsigned dbg_anacnt_softCall=0;
unsigned dbg_anacnt_softCall2=0;
unsigned dbg_anacnt_psxInterrupt=0;
unsigned dbg_anacnt_psxBiosException=0;
unsigned dbg_anacnt_psxRcntUpdate=0;
unsigned dbg_anacnt_psxRcntRcount=0;
unsigned dbg_anacnt_psxRcntRmode=0;
unsigned dbg_anacnt_spuInterrupt=0;
unsigned dbg_anacnt_psxDma4=0;
unsigned dbg_anacnt_psxDma2=0;
unsigned dbg_anacnt_gpuInterrupt=0;
unsigned dbg_anacnt_psxDma6=0;
unsigned dbg_anacnt_CDR_playthread=0;
unsigned dbg_anacnt_CDR_stopCDDA=0;
unsigned dbg_anacnt_CDR_startCDDA=0;
unsigned dbg_anacnt_CDR_playCDDA=0;
unsigned dbg_anacnt_CDR_getTN=0;
unsigned dbg_anacnt_CDR_getTD=0;
unsigned dbg_anacnt_CDR_readTrack=0;
unsigned dbg_anacnt_CDR_play=0;
unsigned dbg_anacnt_psxHwRead8=0;
unsigned dbg_anacnt_psxHwRead16=0;
unsigned dbg_anacnt_psxHwRead32=0;
unsigned dbg_anacnt_psxHwWrite8=0;
unsigned dbg_anacnt_psxHwWrite16=0;
unsigned dbg_anacnt_psxHwWrite32=0;
unsigned dbg_anacnt_PAD1_startPoll=0;
unsigned dbg_anacnt_PAD2_startPoll=0;
unsigned dbg_anacnt_PAD1_poll=0;
unsigned dbg_anacnt_PAD2_poll=0;
unsigned dbg_anacnt_hleDummy=0;
unsigned dbg_anacnt_hleA0=0;
unsigned dbg_anacnt_hleB0=0;
unsigned dbg_anacnt_hleC0=0;
unsigned dbg_anacnt_hleBootstrap=0;
unsigned dbg_anacnt_hleExecRet=0;
unsigned dbg_anacnt_cdrInterrupt=0;
unsigned dbg_anacnt_cdrReadInterrupt=0;
unsigned dbg_anacnt_cdrRead0=0;
unsigned dbg_anacnt_cdrWrite0=0;
unsigned dbg_anacnt_cdrRead1=0;
unsigned dbg_anacnt_cdrWrite1=0;
unsigned dbg_anacnt_cdrRead2=0;
unsigned dbg_anacnt_cdrWrite2=0;
unsigned dbg_anacnt_cdrRead3=0;
unsigned dbg_anacnt_cdrWrite3=0;
unsigned dbg_anacnt_psxDma3=0;
unsigned dbg_anacnt_psxException=0;
unsigned dbg_anacnt_psxBranchTest=0;
unsigned dbg_anacnt_psxExecuteBios=0;
unsigned dbg_anacnt_GPU_sendPacket=0;
unsigned dbg_anacnt_GPU_updateLace=0;
unsigned dbg_anacnt_GPU_writeStatus=0;
unsigned dbg_anacnt_GPU_readStatus=0;
unsigned dbg_anacnt_GPU_writeData=0;
unsigned dbg_anacnt_GPU_writeDataMem=0;
unsigned dbg_anacnt_GPU_readData=0;
unsigned dbg_anacnt_GPU_readDataMem=0;
unsigned dbg_anacnt_GPU_dmaChain=0;
unsigned dbg_anacnt_SPU_writeRegister=0;
unsigned dbg_anacnt_SPU_readRegister=0;
unsigned dbg_anacnt_SPU_readDMA=0;
unsigned dbg_anacnt_SPU_writeDMA=0;
unsigned dbg_anacnt_SPU_writeDMAMem=0;
unsigned dbg_anacnt_SPU_readDMAMem=0;
unsigned dbg_anacnt_SPU_playADPCM=0;
unsigned dbg_anacnt_SPU_async=0;
unsigned dbg_anacnt_SPU_playCDDA=0;
unsigned dbg_anacnt_SIO_Int=0;
unsigned dbg_anacnt_sioWrite8=0;
unsigned dbg_anacnt_sioWriteStat16=0;
unsigned dbg_anacnt_sioWriteMode16=0;
unsigned dbg_anacnt_sioWriteCtrl16=0;
unsigned dbg_anacnt_sioWriteBaud16=0;
unsigned dbg_anacnt_sioRead8=0;
unsigned dbg_anacnt_sioReadStat16=0;
unsigned dbg_anacnt_sioReadMode16=0;
unsigned dbg_anacnt_sioReadCtrl16=0;
unsigned dbg_anacnt_sioReadBaud16=0;
unsigned dbg_anacnt_sioInterrupt=0;
unsigned dbg_anacnt_mdecWrite0=0;
unsigned dbg_anacnt_mdecRead0=0;
unsigned dbg_anacnt_mdecWrite1=0;
unsigned dbg_anacnt_mdecRead1=0;
unsigned dbg_anacnt_psxDma0=0;
unsigned dbg_anacnt_psxDma1=0;
unsigned dbg_anacnt_mdec1Interrupt=0;
unsigned dbg_anacnt_psxMemRead8=0;
unsigned dbg_anacnt_psxMemRead8_direct=0;
unsigned dbg_anacnt_psxMemRead16=0;
unsigned dbg_anacnt_psxMemRead16_direct=0;
unsigned dbg_anacnt_psxMemRead32=0;
unsigned dbg_anacnt_psxMemRead32_direct=0;
unsigned dbg_anacnt_psxMemWrite8=0;
unsigned dbg_anacnt_psxMemWrite8_direct=0;
unsigned dbg_anacnt_psxMemWrite16=0;
unsigned dbg_anacnt_psxMemWrite16_direct=0;
unsigned dbg_anacnt_psxMemWrite32_error=0;
unsigned dbg_anacnt_psxMemWrite32=0;
unsigned dbg_anacnt_psxMemWrite32_direct=0;
unsigned dbg_anacnt_LoadCdrom=0;
unsigned dbg_anacnt_LoadCdromFile=0;
unsigned dbg_anacnt_CheckCdrom=0;
unsigned dbg_anacnt_Load=0;

void dbg_opcode(unsigned _pc, unsigned _opcode, unsigned _cycle_add, unsigned block) {
#ifdef DEBUG_CPU_OPCODES
//if (_pc-4==0x800966a0) { dbg_enable(); }
//if (_pc-4==0x80094ab0 && isdbg()) { exit(0); }
	if (!isdbg()) {
#ifdef DEBUG_MIN_OPCODES
#ifdef DEBUG_BIOS
		static unsigned nloop=0;
		static unsigned sentinel=0;
		if (!dbg_biosin())
#endif
		{
#ifdef DEBUG_BIOS
			if (sentinel){
				sentinel=0;
				nloop++;
			}
#endif
			if (!block) {
				static unsigned long long min=0;
				min++;
				if (min>((unsigned long long)DEBUG_MIN_OPCODES)) {
					dbg_enable();
				}
			}
		}
#ifdef DEBUG_BIOS
		else {
//const char *str="ERROR";
//switch(_opcode>>26) {
//case 0: str=strSPC[_opcode&0x3f]; break;
//case 1: str=strREG[(_opcode>>16)&0x1F]; break;
//case 16: str=strCP0[(_opcode>>21)&0x1F]; break;
//case 18: if (!(_opcode&0x3f)) str=strCP2BSC[(_opcode>>21)&0x1F];
// else str=strCP2[_opcode&0x3f]; break;
//default: str=strBSC[_opcode>>26];
//}
//printf("%p [%s] %u%s\n",_pc-4,str,_cycle_add+psxRegs.cycle,block?" (BLK)":"");
			sentinel++;
			if (sentinel>50000) {
				printf("Infinite loop %i, PC=%p!\n",nloop,dbg_biosptr);
				pcsx4all_exit();
			}
		}
#endif
#endif
#ifdef DEBUG_BIOS
		dbg_bioscheckopcode(_pc-4);
		if (!isdbg())
#endif
		return;
	}
//if (!PC_REC(0x8005ad20)) dbg("AHORA VACIO");
//dbgsum("-SUM ",(void *)&psxM[0],0x200000);
	const char *str="ERROR";
	switch(_opcode>>26) {
		case 0: str=strSPC[_opcode&0x3f]; break;
		case 1: str=strREG[(_opcode>>16)&0x1F]; break;
		case 16: str=strCP0[(_opcode>>21)&0x1F]; break;
		case 18: if (!(_opcode&0x3f)) str=strCP2BSC[(_opcode>>21)&0x1F];
			 else str=strCP2[_opcode&0x3f]; break;
		default: str=strBSC[_opcode>>26];
	}
//	dbgf("%p [%s] %u%s\n",_pc-4,str,_cycle_add+psxRegs.cycle,block?" (BLK)":"");
//dbgf("%p [%s] %u%s (R2=%p,R3=%p,R5=%p) %p\n",_pc-4,str,_cycle_add+psxRegs.cycle,block?" (BLK)":"",psxRegs.GPR.r[2],psxRegs.GPR.r[3],psxRegs.GPR.r[5],(SWAP16(*(u16*)&psxH[(0x1074) & 0xffff])));
dbgf("%p [%s] %u%s INT2=%u\n",_pc-4,str,_cycle_add+psxRegs.cycle,block?" (BLK)":"",psxRegs.intCycle[2]);
#endif
//if (isdbg()) fflush(stdout);
#ifdef DEBUG_ANALYSIS
	static int yet_initted=0;
	if (!yet_initted) {
		yet_initted=1;
		memset(dbg_anacntBSC,0,sizeof(dbg_anacntBSC));
		memset(dbg_anacntSPC,0,sizeof(dbg_anacntSPC));
		memset(dbg_anacntREG,0,sizeof(dbg_anacntREG));
		memset(dbg_anacntCP0,0,sizeof(dbg_anacntCP0));
		memset(dbg_anacntCP2,0,sizeof(dbg_anacntCP2));
		memset(dbg_anacntCP2BSC,0,sizeof(dbg_anacntCP2BSC));
		dbg_anacnt_total_opcodes=0;
	}
	switch(_opcode>>26) {
		case 0: dbg_anacntSPC[_opcode&0x3f]++; break;
		case 1: dbg_anacntREG[(_opcode>>16)&0x1F]++; break;
		case 16: dbg_anacntCP0[(_opcode>>21)&0x1F]++; break;
		case 18: if (!(_opcode&0x3f)) dbg_anacntCP2BSC[(_opcode>>21)&0x1F]++;
			 else dbg_anacntCP2[_opcode&0x3f]++; break;
		default: dbg_anacntBSC[_opcode>>26]++;
	}
	dbg_anacnt_total_opcodes++;
	if (block)
		dbg_anacnt_total_opcodes_block++;
#endif
#ifdef DEBUG_MAX_OPCODES
	static unsigned long long total_opcodes=0;
	total_opcodes++;
	if (total_opcodes>((unsigned long long)DEBUG_MAX_OPCODES)){
		pcsx4all_exit();
	}
#endif
}

void dbg_print_analysis(void) {
#ifdef DEBUG_ANALYSIS
	int i;
	for(i=0;i<64;i++)
		if (dbg_anacntBSC[i]) printf("%u [%s]\n",dbg_anacntBSC[i],strBSC[i]);
	for(i=0;i<64;i++)
		if (dbg_anacntSPC[i]) printf("%u [%s]\n",dbg_anacntSPC[i],strSPC[i]);
	for(i=0;i<32;i++)
		if (dbg_anacntREG[i]) printf("%u [%s]\n",dbg_anacntREG[i],strREG[i]);
	for(i=0;i<32;i++)
		if (dbg_anacntCP0[i]) printf("%u [%s]\n",dbg_anacntCP0[i],strCP0[i]);
	for(i=0;i<64;i++)
		if (dbg_anacntCP2[i]) printf("%u [%s]\n",dbg_anacntCP2[i],strCP2[i]);
	for(i=0;i<32;i++)
		if (dbg_anacntCP2BSC[i]) printf("%u [%s]\n",dbg_anacntCP2BSC[i],strCP2BSC[i]);
	printf("TOTAL OPCODES: %u, %u block\n",dbg_anacnt_total_opcodes,dbg_anacnt_total_opcodes_block);
	printf("dbg_anacnt_softCall: %u\n",dbg_anacnt_softCall);
	printf("dbg_anacnt_softCall2: %u\n",dbg_anacnt_softCall2);
	printf("dbg_anacnt_psxInterrupt: %u\n",dbg_anacnt_psxInterrupt);
	printf("dbg_anacnt_psxBiosException: %u\n",dbg_anacnt_psxBiosException);
	printf("dbg_anacnt_psxRcntUpdate: %u\n",dbg_anacnt_psxRcntUpdate);
	printf("dbg_anacnt_psxRcntRcount: %u\n",dbg_anacnt_psxRcntRcount);
	printf("dbg_anacnt_psxRcntRmode: %u\n",dbg_anacnt_psxRcntRmode);
	printf("dbg_anacnt_spuInterrupt: %u\n",dbg_anacnt_spuInterrupt);
	printf("dbg_anacnt_psxDma4: %u\n",dbg_anacnt_psxDma4);
	printf("dbg_anacnt_psxDma2: %u\n",dbg_anacnt_psxDma2);
	printf("dbg_anacnt_gpuInterrupt: %u\n",dbg_anacnt_gpuInterrupt);
	printf("dbg_anacnt_psxDma6: %u\n",dbg_anacnt_psxDma6);
	printf("dbg_anacnt_CDR_playthread: %u\n",dbg_anacnt_CDR_playthread);
	printf("dbg_anacnt_CDR_stopCDDA: %u\n",dbg_anacnt_CDR_stopCDDA);
	printf("dbg_anacnt_CDR_startCDDA: %u\n",dbg_anacnt_CDR_startCDDA);
	printf("dbg_anacnt_CDR_playCDDA: %u\n",dbg_anacnt_CDR_playCDDA);
	printf("dbg_anacnt_CDR_getTN: %u\n",dbg_anacnt_CDR_getTN);
	printf("dbg_anacnt_CDR_getTD: %u\n",dbg_anacnt_CDR_getTD);
	printf("dbg_anacnt_CDR_readTrack: %u\n",dbg_anacnt_CDR_readTrack);
	printf("dbg_anacnt_CDR_play: %u\n",dbg_anacnt_CDR_play);
	printf("dbg_anacnt_psxHwRead8: %u\n",dbg_anacnt_psxHwRead8);
	printf("dbg_anacnt_psxHwRead16: %u\n",dbg_anacnt_psxHwRead16);
	printf("dbg_anacnt_psxHwRead32: %u\n",dbg_anacnt_psxHwRead32);
	printf("dbg_anacnt_psxHwWrite8: %u\n",dbg_anacnt_psxHwWrite8);
	printf("dbg_anacnt_psxHwWrite16: %u\n",dbg_anacnt_psxHwWrite16);
	printf("dbg_anacnt_psxHwWrite32: %u\n",dbg_anacnt_psxHwWrite32);
	printf("dbg_anacnt_PAD1_startPoll: %u\n",dbg_anacnt_PAD1_startPoll);
	printf("dbg_anacnt_PAD2_startPoll: %u\n",dbg_anacnt_PAD2_startPoll);
	printf("dbg_anacnt_PAD1_poll: %u\n",dbg_anacnt_PAD1_poll);
	printf("dbg_anacnt_PAD2_poll: %u\n",dbg_anacnt_PAD2_poll);
	printf("dbg_anacnt_hleDummy: %u\n",dbg_anacnt_hleDummy);
	printf("dbg_anacnt_hleA0: %u\n",dbg_anacnt_hleA0);
	printf("dbg_anacnt_hleB0: %u\n",dbg_anacnt_hleB0);
	printf("dbg_anacnt_hleC0: %u\n",dbg_anacnt_hleC0);
	printf("dbg_anacnt_hleBootstrap: %u\n",dbg_anacnt_hleBootstrap);
	printf("dbg_anacnt_hleExecRet: %u\n",dbg_anacnt_hleExecRet);
	printf("dbg_anacnt_cdrInterrupt: %u\n",dbg_anacnt_cdrInterrupt);
	printf("dbg_anacnt_cdrReadInterrupt: %u\n",dbg_anacnt_cdrReadInterrupt);
	printf("dbg_anacnt_cdrRead0: %u\n",dbg_anacnt_cdrRead0);
	printf("dbg_anacnt_cdrWrite0: %u\n",dbg_anacnt_cdrWrite0);
	printf("dbg_anacnt_cdrRead1: %u\n",dbg_anacnt_cdrRead1);
	printf("dbg_anacnt_cdrWrite1: %u\n",dbg_anacnt_cdrWrite1);
	printf("dbg_anacnt_cdrRead2: %u\n",dbg_anacnt_cdrRead2);
	printf("dbg_anacnt_cdrWrite2: %u\n",dbg_anacnt_cdrWrite2);
	printf("dbg_anacnt_cdrRead3: %u\n",dbg_anacnt_cdrRead3);
	printf("dbg_anacnt_cdrWrite3: %u\n",dbg_anacnt_cdrWrite3);
	printf("dbg_anacnt_psxDma3: %u\n",dbg_anacnt_psxDma3);
	printf("dbg_anacnt_psxException: %u\n",dbg_anacnt_psxException);
	printf("dbg_anacnt_psxBranchTest: %u\n",dbg_anacnt_psxBranchTest);
	printf("dbg_anacnt_psxExecuteBios: %u\n",dbg_anacnt_psxExecuteBios);
	printf("dbg_anacnt_GPU_sendPacket: %u\n",dbg_anacnt_GPU_sendPacket);
	printf("dbg_anacnt_GPU_updateLace: %u\n",dbg_anacnt_GPU_updateLace);
	printf("dbg_anacnt_GPU_writeStatus: %u\n",dbg_anacnt_GPU_writeStatus);
	printf("dbg_anacnt_GPU_readStatus: %u\n",dbg_anacnt_GPU_readStatus);
	printf("dbg_anacnt_GPU_writeData: %u\n",dbg_anacnt_GPU_writeData);
	printf("dbg_anacnt_GPU_writeDataMem: %u\n",dbg_anacnt_GPU_writeDataMem);
	printf("dbg_anacnt_GPU_readData: %u\n",dbg_anacnt_GPU_readData);
	printf("dbg_anacnt_GPU_readDataMem: %u\n",dbg_anacnt_GPU_readDataMem);
	printf("dbg_anacnt_GPU_dmaChain: %u\n",dbg_anacnt_GPU_dmaChain);
	printf("dbg_anacnt_SPU_writeRegister: %u\n",dbg_anacnt_SPU_writeRegister);
	printf("dbg_anacnt_SPU_readRegister: %u\n",dbg_anacnt_SPU_readRegister);
	printf("dbg_anacnt_SPU_readDMA: %u\n",dbg_anacnt_SPU_readDMA);
	printf("dbg_anacnt_SPU_writeDMA: %u\n",dbg_anacnt_SPU_writeDMA);
	printf("dbg_anacnt_SPU_writeDMAMem: %u\n",dbg_anacnt_SPU_writeDMAMem);
	printf("dbg_anacnt_SPU_readDMAMem: %u\n",dbg_anacnt_SPU_readDMAMem);
	printf("dbg_anacnt_SPU_playADPCM: %u\n",dbg_anacnt_SPU_playADPCM);
	printf("dbg_anacnt_SPU_async: %u\n",dbg_anacnt_SPU_async);
	printf("dbg_anacnt_SPU_playCDDA: %u\n",dbg_anacnt_SPU_playCDDA);
	printf("dbg_anacnt_SIO_Int: %u\n",dbg_anacnt_SIO_Int);
	printf("dbg_anacnt_sioWrite8: %u\n",dbg_anacnt_sioWrite8);
	printf("dbg_anacnt_sioWriteStat16: %u\n",dbg_anacnt_sioWriteStat16);
	printf("dbg_anacnt_sioWriteMode16: %u\n",dbg_anacnt_sioWriteMode16);
	printf("dbg_anacnt_sioWriteCtrl16: %u\n",dbg_anacnt_sioWriteCtrl16);
	printf("dbg_anacnt_sioWriteBaud16: %u\n",dbg_anacnt_sioWriteBaud16);
	printf("dbg_anacnt_sioRead8: %u\n",dbg_anacnt_sioRead8);
	printf("dbg_anacnt_sioReadStat16: %u\n",dbg_anacnt_sioReadStat16);
	printf("dbg_anacnt_sioReadMode16: %u\n",dbg_anacnt_sioReadMode16);
	printf("dbg_anacnt_sioReadCtrl16: %u\n",dbg_anacnt_sioReadCtrl16);
	printf("dbg_anacnt_sioReadBaud16: %u\n",dbg_anacnt_sioReadBaud16);
	printf("dbg_anacnt_sioInterrupt: %u\n",dbg_anacnt_sioInterrupt);
	printf("dbg_anacnt_mdecWrite0: %u\n",dbg_anacnt_mdecWrite0);
	printf("dbg_anacnt_mdecRead0: %u\n",dbg_anacnt_mdecRead0);
	printf("dbg_anacnt_mdecWrite1: %u\n",dbg_anacnt_mdecWrite1);
	printf("dbg_anacnt_mdecRead1: %u\n",dbg_anacnt_mdecRead1);
	printf("dbg_anacnt_psxDma0: %u\n",dbg_anacnt_psxDma0);
	printf("dbg_anacnt_psxDma1: %u\n",dbg_anacnt_psxDma1);
	printf("dbg_anacnt_mdec1Interrupt: %u\n",dbg_anacnt_mdec1Interrupt);
	printf("dbg_anacnt_psxMemRead8: %u\n",dbg_anacnt_psxMemRead8);
	printf("dbg_anacnt_psxMemRead8_direct: %u\n",dbg_anacnt_psxMemRead8_direct);
	printf("dbg_anacnt_psxMemRead16: %u\n",dbg_anacnt_psxMemRead16);
	printf("dbg_anacnt_psxMemRead16_direct: %u\n",dbg_anacnt_psxMemRead16_direct);
	printf("dbg_anacnt_psxMemRead32: %u\n",dbg_anacnt_psxMemRead32);
	printf("dbg_anacnt_psxMemRead32_direct: %u\n",dbg_anacnt_psxMemRead32_direct);
	printf("dbg_anacnt_psxMemWrite8: %u\n",dbg_anacnt_psxMemWrite8);
	printf("dbg_anacnt_psxMemWrite8_direct: %u\n",dbg_anacnt_psxMemWrite8_direct);
	printf("dbg_anacnt_psxMemWrite16: %u\n",dbg_anacnt_psxMemWrite16);
	printf("dbg_anacnt_psxMemWrite16_direct: %u\n",dbg_anacnt_psxMemWrite16_direct);
	printf("dbg_anacnt_psxMemWrite32_error: %u\n",dbg_anacnt_psxMemWrite32_error);
	printf("dbg_anacnt_psxMemWrite32: %u\n",dbg_anacnt_psxMemWrite32);
	printf("dbg_anacnt_psxMemWrite32_direct: %u\n",dbg_anacnt_psxMemWrite32_direct);
	printf("dbg_anacnt_LoadCdrom: %u\n",dbg_anacnt_LoadCdrom);
	printf("dbg_anacnt_LoadCdromFile: %u\n",dbg_anacnt_LoadCdromFile);
	printf("dbg_anacnt_CheckCdrom: %u\n",dbg_anacnt_CheckCdrom);
	printf("dbg_anacnt_Load: %u\n",dbg_anacnt_Load);
#endif
}

#ifdef DEBUG_BIOS
void dbg_bioscall(unsigned branchPC){
	dbgpsxregs();
	if (!dbg_biosin() && isdbg()){
		dbg_biosset(psxRegs.GPR.n.ra);
		if (branchPC==0xA0 && (psxRegs.GPR.n.t1 & 0xff)==0x43)
			dbg_biosptr=*((unsigned*)PSXM(psxRegs.GPR.n.a0));
		dbgf("BIOS CALL 0x%.2X:0x%.2X, Return %p\n",branchPC,psxRegs.GPR.n.t1 & 0xff,dbg_biosptr);
		fflush(stdout);
//if (!(branchPC==0xA0 && psxRegs.GPR.n.t1==0x70))
		dbg_disable();
	}
}

void dbg_biosreturn(void){
	if (dbg_biosin()){
		dbg_enable();
		dbg("RETURN BIOS");
		dbgpsxregs();
		fflush(stdout);
		if (Config.HLE && autobias)
			psxRegs.cycle+=1;
		dbg_biosunset();
	}
}
#endif

#endif
