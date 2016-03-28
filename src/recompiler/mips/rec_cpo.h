#define REC_ITYPE_RT_RS_IMM(insn, _rt_, _rs_, _imm_, cpo) 			\
do 										\
{ 										\
	u32 rt  = _rt_; 							\
	u32 rs  = _rs_; 							\
	u32 imm = (u32)_imm_; 							\
	if (!rt) return; 							\
	{ 									\
		u32 r1, r2; 							\
		if (rs == rt) 							\
		{ 								\
			r1 = regMipsToArm(rt, REG_LOAD, REG_REGISTER); 		\
			r2 = r1; 						\
		} 								\
		else 								\
		{ 								\
			r1 = regMipsToArm(rt, REG_FIND, REG_REGISTER);		\
			r2 = regMipsToArm(rs, REG_LOAD, REG_REGISTER); 		\
		} 								\
		gen(insn, r1, r2, imm); 					\
		regMipsChanged(rt); 						\
	} 									\
} 										\
while (0)

#define REC_ITYPE_RT_IMM(insn, _rt_, _imm_, cpo) 				\
do 										\
{										\
	u32 rt  = _rt_;	 							\
	u32 imm = _imm_; 							\
	if (!rt) return; 							\
	u32 r1 = regMipsToArm(rt, REG_FIND, REG_REGISTER);			\
	gen(insn, r1, imm); 							\
	regMipsChanged(rt); 							\
} 										\
while (0)

#define REC_RTYPE_RD_RS_RT(insn1, insn2, _rd_, _rs_, _rt_, cpo) 		\
do 										\
{ 										\
	u32 rd  = _rd_; 							\
	u32 rt  = _rt_; 							\
	u32 rs  = _rs_; 							\
	if (!rd) return; 							\
	u32 r1, r2, r3; 							\
	if (rs == rd) 								\
	{ 									\
		r1 = regMipsToArm(rd, REG_LOAD, REG_REGISTER); 			\
		r2 = r1; 							\
		if (rs == rt) 							\
			r3 = r1; 						\
		else 								\
			r3 = regMipsToArm(rt, REG_LOAD, REG_REGISTER); 		\
	} 									\
	else if (rt == rd) 							\
	{ 									\
		r1 = regMipsToArm(rd, REG_LOAD, REG_REGISTER); 			\
		r2 = regMipsToArm(rs, REG_LOAD, REG_REGISTER);			\
		r3 = r1; 							\
	} 									\
	else 									\
	{ 									\
		r1 = regMipsToArm(rd, REG_FIND, REG_REGISTER); 			\
		r2 = regMipsToArm(rs, REG_LOAD, REG_REGISTER);			\
		if (rs == rt) 							\
			r3 = r2; 						\
		else 								\
			r3 = regMipsToArm(rt, REG_LOAD, REG_REGISTER); 		\
	} 									\
	gen(insn1, r1, r2, r3); 						\
	regMipsChanged(rd);  							\
} 										\
while (0)

#define REC_RTYPE_RD_RT_SA(insn, _rd_, _rt_, _sa_, cpo) 			\
do 										\
{ 										\
	u32 rd = _rd_; 								\
	u32 rt = _rt_; 								\
	u32 sa = _sa_; 								\
	if (!rd) return; 							\
										\
	{ 									\
		u32 r1, r2; 							\
		if (rd == rt) 							\
		{ 								\
			r1 = regMipsToArm(rd, REG_LOAD, REG_REGISTER);		\
			r2 = r1; 						\
		} 								\
		else 								\
		{ 								\
			r1 = regMipsToArm(rd, REG_FIND, REG_REGISTER); 		\
			r2 = regMipsToArm(rt, REG_LOAD, REG_REGISTER); 		\
		} 								\
		gen(insn, r1, r2, sa); 						\
		regMipsChanged(rd); 						\
	} 									\
} 										\
while (0)

#define REC_RTYPE_RD_RT_RS(insn1, insn2, _rd_, _rt_, _rs_, cpo) 		\
do 										\
{ 										\
	u32 rd = _rd_; 								\
	u32 rt = _rt_; 								\
	u32 rs = _rs_; 								\
	if (!rd) return; 							\
	u32 r1, r2, r3; 							\
	if (rt == rd) 								\
	{ 									\
		r1 = regMipsToArm(rd, REG_LOAD, REG_REGISTER); 			\
		r2 = r1; 							\
		if (rs == rt) 							\
			r3 = r1; 						\
		else 								\
			r3 = regMipsToArm(rs, REG_LOAD, REG_REGISTER); 		\
	} 									\
	else if (rs == rd) 							\
	{ 									\
		r1 = regMipsToArm(rd, REG_LOAD, REG_REGISTER); 			\
		r2 = regMipsToArm(rt, REG_LOAD, REG_REGISTER); 			\
		r3 = r1; 							\
	} 									\
	else 									\
	{ 									\
		r1 = regMipsToArm(rd, REG_FIND, REG_REGISTER); 			\
		r2 = regMipsToArm(rt, REG_LOAD, REG_REGISTER); 			\
		if (rs == rt) 							\
			r3 = r2; 						\
		else 								\
			r3 = regMipsToArm(rs, REG_LOAD, REG_REGISTER); 		\
	} 									\
	gen(insn1, r1, r2, r3); 						\
	regMipsChanged(rd);							\
} 										\
while (0)

