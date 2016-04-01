#ifndef _GEN_ALU_H_
#define _GEN_ALU_H_

#define gen(insn, ...)	mips_gen_##insn(__VA_ARGS__)

/* ALU : Arithmetic and Logical Unit */

extern INLINE u32 gen(MOVI16, u32 rt, s32 imm16)
{
  write32(0x24000000 | (rt << 16) | (imm16 & 0xffff)); /* li (aka addiu zero,) */
  return 1;
}

extern INLINE u32 gen(MOVU16, u32 rt, u32 imm16)
{
  write32(0x34000000 | (rt << 16) | (imm16 & 0xffff)); /* ori zero, */
  return 1;
}

extern INLINE u32 gen(MOVI32, u32 rt, u32 imm32)
{
  write32(0x3c000000 | (rt << 16) | (imm32 >> 16)); /* lui */
  write32(0x34000000 | (rt << 21) | (rt << 16) | (imm32 & 0xffff)); /* ori */
  return 2;
}

extern INLINE u32 gen(MOV, u32 rd, u32 rs) { MOV(rd, rs); return 1; }
extern INLINE u32 gen(CLR, u32 rd)         { LI16(rd, 0); return 1; }
extern INLINE u32 gen(NEG, u32 rd, u32 rs) { DEBUGF("fuckup"); abort(); /* ARM_RSB_REG_IMM8(0, rd, rs, 0); */ return 1; }
extern INLINE u32 gen(NOT, u32 rd, u32 rs) { DEBUGF("fuckup"); abort(); /* ARM_MVN_REG_REG(0, rd, rs); */ return 1; }

extern INLINE u32 gen(LUI, u32 rt, u16 imm16)
{
  write32(0x3c000000 | (rt << 16) | (imm16)); /* lui */
  return 0;
}

extern INLINE u32 gen(SLL, u32 rd, u32 rt, u32 sa) { write32((rt << 16) | (rd << 11) | ((sa & 31) << 6)); return 1; }
extern INLINE u32 gen(SRL, u32 rd, u32 rt, u32 sa) { write32(0x00000002 | (rt << 16) | (rd << 11) | ((sa & 31) << 6)); return 1; }
extern INLINE u32 gen(SRA, u32 rd, u32 rt, u32 sa) { write32(0x00000003 | (rt << 16) | (rd << 11) | ((sa & 31) << 6)); return 1; }

extern INLINE u32 gen(SLL_RT0, u32 rd, u32 rt, u32 sa) { return gen(CLR, rd); }
extern INLINE u32 gen(SRL_RT0, u32 rd, u32 rt, u32 sa) { return gen(CLR, rd); }
extern INLINE u32 gen(SRA_RT0, u32 rd, u32 rt, u32 sa) { return gen(CLR, rd); }

extern INLINE u32 gen(SLLV, u32 rd, u32 rt, u32 rs) { write32(0x00000004 | (rs << 21) | (rt << 16) | (rd << 11)); return 1; }
extern INLINE u32 gen(SLLV_RT0, u32 rd, u32 rt, u32 rs) { write32(0x00000004 | (rs << 21) | (rt << 16) | (rd << 11)); return 1; }
extern INLINE u32 gen(SLLV_RS0, u32 rd, u32 rt, u32 rs) { write32(0x00000004 | (rs << 21) | (rt << 16) | (rd << 11)); return 1; }
extern INLINE u32 gen(SLLV_RT0_RS0, u32 rd, u32 rt, u32 rs) { write32(0x00000004 | (rs << 21) | (rt << 16) | (rd << 11)); return 1; }
extern INLINE u32 gen(SRLV, u32 rd, u32 rt, u32 rs) { write32(0x00000006 | (rs << 21) | (rt << 16) | (rd << 11)); return 1; }
extern INLINE u32 gen(SRLV_RT0, u32 rd, u32 rt, u32 rs) { write32(0x00000006 | (rs << 21) | (rt << 16) | (rd << 11)); return 1; }
extern INLINE u32 gen(SRLV_RS0, u32 rd, u32 rt, u32 rs) { write32(0x00000006 | (rs << 21) | (rt << 16) | (rd << 11)); return 1; }
extern INLINE u32 gen(SRLV_RT0_RS0, u32 rd, u32 rt, u32 rs) { write32(0x00000006 | (rs << 21) | (rt << 16) | (rd << 11)); return 1; }
extern INLINE u32 gen(SRAV, u32 rd, u32 rt, u32 rs) { write32(0x00000007 | (rs << 21) | (rt << 16) | (rd << 11)); return 1; }
extern INLINE u32 gen(SRAV_RT0, u32 rd, u32 rt, u32 rs) { write32(0x00000007 | (rs << 21) | (rt << 16) | (rd << 11)); return 1; }
extern INLINE u32 gen(SRAV_RS0, u32 rd, u32 rt, u32 rs) { write32(0x00000007 | (rs << 21) | (rt << 16) | (rd << 11)); return 1; }
extern INLINE u32 gen(SRAV_RT0_RS0, u32 rd, u32 rt, u32 rs) { write32(0x00000007 | (rs << 21) | (rt << 16) | (rd << 11)); return 1; }

extern INLINE u32 gen(ADD, u32 rd, u32 rs, u32 rt) { MIPS_ADD_REG_REG(0, rd, rs, rt); return 1; }
extern INLINE u32 gen(ADD_RS0, u32 rd, u32 rs, u32 rt) { return gen(MOV, rd, rt); }
extern INLINE u32 gen(ADD_RT0, u32 rd, u32 rs, u32 rt) { return gen(MOV, rd, rs); }
extern INLINE u32 gen(ADD_RS0_RT0, u32 rd, u32 rs, u32 rt) { return gen(CLR, rd); }

extern INLINE u32 gen(SUB, u32 rd, u32 rs, u32 rt) { MIPS_SUB_REG_REG(0, rd, rs, rt); return 1; }
extern INLINE u32 gen(SUB_RS0, u32 rd, u32 rs, u32 rt) { MIPS_SUB_REG_REG(0, rd, rs, rt); return 1; }
extern INLINE u32 gen(SUB_RT0, u32 rd, u32 rs, u32 rt) { MIPS_SUB_REG_REG(0, rd, rs, rt); return 1; }
extern INLINE u32 gen(SUB_RS0_RT0, u32 rd, u32 rs, u32 rt) { MIPS_SUB_REG_REG(0, rd, rs, rt); return 1; }

extern INLINE u32 gen(AND, u32 rd, u32 rs, u32 rt) { AND(rd, rs, rt); return 1; }
extern INLINE u32 gen(AND_RS0, u32 rd, u32 rs, u32 rt) { return gen(CLR, rd); }
extern INLINE u32 gen(AND_RT0, u32 rd, u32 rs, u32 rt) { return gen(CLR, rd); }
extern INLINE u32 gen(AND_RS0_RT0, u32 rd, u32 rs, u32 rt) { return gen(CLR, rd); }

extern INLINE u32 gen(OR, u32 rd, u32 rs, u32 rt) { MIPS_ORR_REG_REG(0, rd, rs, rt); return 1; }
extern INLINE u32 gen(OR_RS0, u32 rd, u32 rs, u32 rt) { return gen(MOV, rd, rt); }
extern INLINE u32 gen(OR_RT0, u32 rd, u32 rs, u32 rt) { return gen(MOV, rd, rs); }
extern INLINE u32 gen(OR_RS0_RT0, u32 rd, u32 rs, u32 rt) { return gen(CLR, rd); }

extern INLINE u32 gen(XOR, u32 rd, u32 rs, u32 rt) { MIPS_XOR_REG_REG(0, rd, rs, rt); return 1; }
extern INLINE u32 gen(XOR_RS0, u32 rd, u32 rs, u32 rt) { return gen(MOV, rd, rt); }
extern INLINE u32 gen(XOR_RT0, u32 rd, u32 rs, u32 rt) { return gen(MOV, rd, rs); }
extern INLINE u32 gen(XOR_RS0_RT0, u32 rd, u32 rs, u32 rt) { return gen(CLR, rd); }

extern INLINE u32 gen(NOR, u32 rd, u32 rs, u32 rt) { write32(0x00000027 | (rs << 21) | (rt << 16) | (rd << 11)); return 1; }
extern INLINE u32 gen(NOR_RS0, u32 rd, u32 rs, u32 rt) { write32(0x00000027 | (rs << 21) | (rt << 16) | (rd << 11)); return 1; }
extern INLINE u32 gen(NOR_RT0, u32 rd, u32 rs, u32 rt) { write32(0x00000027 | (rs << 21) | (rt << 16) | (rd << 11)); return 1; }
extern INLINE u32 gen(NOR_RS0_RT0, u32 rd, u32 rs, u32 rt) { write32(0x00000027 | (rs << 21) | (rt << 16) | (rd << 11)); return 1; }

extern INLINE u32 gen(SLT, u32 rd, u32 rs, u32 rt) { write32(0x0000002a | (rs << 21) | (rt << 16) | (rd << 11)); return 1; }
#define mips_gen_SLT_RS0	mips_gen_SLT
#define mips_gen_SLT_RT0	mips_gen_SLT
#define mips_gen_SLT_RS0_RT0	mips_gen_SLT

extern INLINE u32 gen(SLTU, u32 rd, u32 rs, u32 rt)
{
  write32(0x0000002b | (rs << 21) | (rt << 16) | (rd << 11)); /* sltu */
  return 1;
}

#define mips_gen_SLTU_RS0	mips_gen_SLTU
#define mips_gen_SLTU_RT0	mips_gen_SLTU
#define mips_gen_SLTU_RS0_RT0	mips_gen_SLTU

extern INLINE u32 gen(ADDI, u32 rt, u32 rs, s32 imm16)
{
  ADDIU(rt, rs, imm16);
  return 1;
}
extern INLINE u32 gen(ADDI_RS0, u32 rt, u32 rs, s32 imm16) { return gen(MOVI16, rt, imm16); } 


extern INLINE u32 gen(SUBI, u32 rt, u32 rs, s32 imm16) { return gen(ADDI, rt, rs, -imm16); }
extern INLINE u32 gen(SUBI_RS0, u32 rt, u32 rs, s32 imm16) { return gen(MOVI16, rt, -imm16); }

extern INLINE u32 gen(SLTI, u32 rt, u32 rs, s32 imm16) { write32(0x28000000 | (rs << 21) | (rt << 16) | (imm16 & 0xffff)); return 1; }
#define mips_gen_SLTI_RS0	mips_gen_SLTI

extern INLINE u32 gen(SLTIU, u32 rt, u32 rs, s32 imm16)
{
  write32(0x2c000000 | (rs << 21) | (rt << 16) | (imm16 & 0xffff));
  return 1;
}
#define mips_gen_SLTIU_RS0	mips_gen_SLTIU

extern INLINE u32 gen(ANDI, u32 rt, u32 rs, u32 imm16)
{
  write32(0x30000000 | (rs << 21) | (rt << 16) | (imm16 & 0xffff));
  return 1;
}
extern INLINE u32 gen(ANDI_RS0, u32 rt, u32 rs, u32 imm16) { return gen(CLR, rt); }

extern INLINE u32 gen(ORI, u32 rt, u32 rs, u32 imm16)
{
  write32(0x34000000 | (rs << 21) | (rt << 16) | (imm16 & 0xffff));
  return 0;
}
extern INLINE u32 gen(ORI_RS0, u32 rt, u32 rs, u32 imm16) { return gen(MOVU16, rt, imm16); }

extern INLINE u32 gen(XORI, u32 rt, u32 rs, u32 imm16)
{
  XORI(rt, rs, imm16);
  return 1;
}
extern INLINE u32 gen(XORI_RS0, u32 rt, u32 rs, u32 imm16) { return gen(MOVU16, rt, imm16); }

#endif
