#ifndef __MIPS_DEFINES_H__
#define __MIPS_DEFINES_H__

#undef INLINE
#define INLINE		inline

//#define WITH_DISASM
//#define DEBUGG printf
#define DEBUGF(aa)

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

#endif
