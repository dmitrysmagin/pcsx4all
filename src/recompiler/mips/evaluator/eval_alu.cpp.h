#pragma once

/* ALU : Arithmetic and Logical Unit */

extern INLINE u32 eval(LI16, s32 imm16) { return (u32)(s32)(s16)imm16; }

extern INLINE u32 eval(LIU16, u32 imm16) { return (u32)(u16)imm16; }

extern INLINE u32 eval(LI32, u32 imm32) { return imm32; }

extern INLINE u32 eval(SLL, u32 rt, u32 sa) { return ((u32)rt) << sa; }
extern INLINE u32 eval(SRL, u32 rt, u32 sa) { return ((u32)rt) >> sa; }
extern INLINE u32 eval(SRA, u32 rt, u32 sa) { return ((s32)rt) >> sa; }
extern INLINE u32 eval(SLLV, u32 rt, u32 rs) { return eval(SLL, rt, rs); }
extern INLINE u32 eval(SRLV, u32 rt, u32 rs) { return eval(SRL, rt, rs); }
extern INLINE u32 eval(SRAV, u32 rt, u32 rs) { return eval(SRA, rt, rs); }

extern INLINE u32 eval(MOV, u32 rs) { return rs; }

extern INLINE u32 eval(ADD, u32 rs, u32 rt) { return rs + rt; }
extern INLINE u32 eval(SUB, u32 rs, u32 rt) { return rs - rt; }
extern INLINE u32 eval(RSB, u32 rs, u32 rt) { return rt - rs; }

extern INLINE u32 eval(NEG, u32 rs) { return 0 - rs; } 

extern INLINE u32 eval(AND, u32 rs, u32 rt) { return rs & rt; }
extern INLINE u32 eval(OR,  u32 rs, u32 rt) { return rs | rt; }
extern INLINE u32 eval(XOR, u32 rs, u32 rt) { return rs ^ rt; } 
extern INLINE u32 eval(NOR, u32 rs, u32 rt) { return ~(rs | rt); } 

extern INLINE u32 eval(NOT, u32 rs) { return ~rs; } 

extern INLINE u32 eval(SLT,  u32 rs, u32 rt) { return (u32)(((s32)rt) < ((s32)rs)); }
extern INLINE u32 eval(SLTU, u32 rs, u32 rt) { return (u32)(((u32)rt) < ((u32)rs)); }

extern INLINE u32 eval(ADDI16,  u32 rs, s32 imm16) { return rs + (u32)(s32)(s16)imm16; }
extern INLINE u32 eval(ADDUI16, u32 rs, s32 imm16) { return rs + (u32)((imm16<<16)); }
extern INLINE u32 eval(SUBI16,  u32 rs, s32 imm16) { return rs - (u32)(s32)(s16)imm16; }
extern INLINE u32 eval(SUBUI16, u32 rs, s32 imm16) { return rs - (u32)((imm16<<16)); }

extern INLINE u32 eval(ADDI32, u32 rs, s32 imm32) { return rs + (u32)imm32; }
extern INLINE u32 eval(SUBI32, u32 rs, s32 imm32) { return rs - (u32)imm32; }
extern INLINE u32 eval(RSBI32, u32 rs, s32 imm32) { return (u32)imm32 - rs; }

extern INLINE u32 eval(SLTI16,  u32 rs, s32 imm16) { return (u32)(((s32)rs) < ((s32)(s16)imm16)); }
extern INLINE u32 eval(SLTIU16, u32 rs, s32 imm16) { return (u32)(((u32)rs) < ((u32)(s32)(s16)imm16)); }
extern INLINE u32 eval(SLTI32,  u32 rs, s32 imm32) { return (u32)(((s32)rs) < ((s32)imm32)); }
extern INLINE u32 eval(SLTIU32, u32 rs, s32 imm32) { return (u32)(((u32)rs) < ((u32)imm32)); }

extern INLINE u32 eval(ANDI16, u32 rs, u32 imm16) { return rs & (u32)(u16)imm16; }
extern INLINE u32 eval(ANDI32, u32 rs, u32 imm32) { return rs & imm32; }
extern INLINE u32 eval(ORI16,  u32 rs, u32 imm16) { return rs | (u32)(u16)imm16; }
extern INLINE u32 eval(ORI32,  u32 rs, u32 imm32) { return rs | imm32; }
extern INLINE u32 eval(XORI16, u32 rs, u32 imm16) { return rs ^ (u32)(u16)imm16; }
extern INLINE u32 eval(XORI32, u32 rs, u32 imm32) { return rs ^ imm32; }
extern INLINE u32 eval(NORI16, u32 rs, u32 imm16) { return ~(rs | (u32)(u16)imm16); }
extern INLINE u32 eval(NORI32, u32 rs, u32 imm32) { return ~(rs | imm32); }

extern INLINE u32 eval(LUI16, u32 imm16) { return ((u32)imm16)<<16; }
