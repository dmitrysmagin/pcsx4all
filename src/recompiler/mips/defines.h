#ifndef __MIPS_DEFINES_H__
#define __MIPS_DEFINES_H__

// From old psxmem.h
#define psxMemReadS8	psxMemRead8
#define psxMemReadS16	psxMemRead16

#undef INLINE
#define INLINE		inline

//#define WITH_DISASM
//#define DEBUGG printf
#define DEBUGF(aa)

#define REG_CACHE_START		MIPSREG_S0
#define REG_CACHE_END		(MIPSREG_S7+1)

#define REG_LOAD		0
#define REG_FIND		1
#define REG_LOADBRANCH		2

#define REG_EMPTY		0
#define REG_REGISTER		1
#define REG_TEMPORARY		2
#define REG_RESERVED		3
#define REG_REGISTERBRANCH	4

/* general defines */
#define MIPS_POINTER		0

#define RECMEM_SIZE		(12 * 1024 * 1024)
#define RECMEM_SIZE_MAX 	(RECMEM_SIZE-(512*1024))
#define REC_MAX_OPCODES		80

#define TEMP_1 				MIPSREG_T0
#define TEMP_2 				MIPSREG_T1
#define TEMP_3 				MIPSREG_T2

/* PERM_REG_1 is used to store psxRegs struct address */
#define PERM_REG_1 			MIPSREG_S8

/* Crazy macro to calculate offset of the field in the structure */
#ifndef offsetof
#define offsetof(T,F) ((unsigned int)((char *)&((T *)0L)->F - (char *)0L))
#endif

/* GPR offset */
#define offGPR(rx) offsetof(psxRegisters, GPR.r[rx])

/* CP0 offset */
#define offCP0(rx) offsetof(psxRegisters,  CP0.r[rx])

/* CP2C offset */
#define offCP2C(rx) offsetof(psxRegisters,  CP2C.r[rx])

/* pc offset */
#define offpc		offsetof(psxRegisters,  pc)

/* code offset */
#define offcode		offsetof(psxRegisters,  code)

/* call func */
#define CALLFunc(func)						 															\
	{																															\
		MIPS_EMIT(MIPS_POINTER, 0x0c000000 | ((func & 0x0fffffff) >> 2)); /* jal func */ \
		MIPS_EMIT(MIPS_POINTER, 0); /* nop */ \
	}

#define CALLFunc_NoFlush(func)		 											\
	CALLFunc(func)						 										\

#define CALLFunc_Branch(func)		 											\
	MIPS_EMIT(MIPS_POINTER, 0x0c000000 | ((func & 0x0fffffff) >> 2)); /* jal func */ \
	MIPS_EMIT(MIPS_POINTER, 0); /* nop */ \
	rec_recompile_end();												\

#define gp2x_sync() sync()

#define _oB_ (_u32(_rRs_) + _Imm_)

#define write32(val) 															\
	*recMem++ = (u32)(val);			 											\

#define mips_relative_offset(source, offset, next) \
  ((((u32)(offset) - ((u32)(source) + (next))) >> 2) & 0xFFFF) \

#define LoadImmediate32(imm32, reg)												\
	mips_load_imm32((imm32), reg)													\

#define mips_load_imm32(imm, ireg) \
{ \
  MIPS_EMIT(0, 0x3c000000 | (ireg << 16) | ((imm) >> 16)); /* lui */ \
  MIPS_EMIT(0, 0x34000000 | (ireg << 21) | (ireg << 16) | ((imm) & 0xffff)); /* ori */ \
}

#endif
