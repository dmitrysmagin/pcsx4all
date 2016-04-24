//senquack - A version of pcsx_rearmed/libpcsxcore/spu.c's SPUschedule()
//			 adapted for PSX4ALL.

#ifndef SPU_PCSXREARMED_SPUSCHEDULE_H
#define SPU_PCSXREARMED_SPUSCHEDULE_H

//senquack - Some of the ReARMed SPU functions take new "cycles" value from
//           psxRegs.cycle, and I've provided a global pointer to it in
//           r3000a.cpp so that it can be found anywhere in the source without
//           awful circular header depenedency issues.
extern const uint32_t* const psxRegs_cycle_valptr;

//In reARMed, the SPU will set an interrupt flag in psxRegs.interrupt along with
// appropriate values for what cycle value to fire it at. PSX4ALL's interrupts are
// not very well documented, so rather than try to add a new interrupt flag, I
// simply added vars in r3000.c to indicate at what cycle value the SPU has requested its
// update, and if an update has been requested. psxBranchTest() will check this flag
// after doing its normal interrupts in r3000.c
extern uint32_t pcsxrearmed_update_pending;
extern uint32_t pcsxrearmed_update_at_cycle;

//senquack - Did my best to adapt this feature from PCSXReARMedto older PCSX4ALL emu.
//           NOTE: the adapted spu_pcsxrearmed now calls this directly
//			       instead of through a registered callback.
// Original function from pcsx_rearmed/libpcsxcore/spu.c here for reference:
// Notaz's original spuUpdate:
//void CALLBACK SPUschedule(unsigned int cycles_after) {
//	psxRegs.interrupt |= (1 << PSXINT_SPU_UPDATE);
//	psxRegs.intCycle[PSXINT_SPU_UPDATE].cycle = cycles_after;
//	psxRegs.intCycle[PSXINT_SPU_UPDATE].sCycle = psxRegs.cycle;
//	new_dyna_set_event(PSXINT_SPU_UPDATE, cycles_after);
//}
//senquack - TODO - Maybe adapt this further , as this code is exlusive to reARMed dynarec,
//                  if dsmagin's MIPS dynarec ends up with ability to schedule events. Will
//				    have to add some sort of equivalent to new_dyna_set_event() above
void SPUschedule(unsigned int cycles_after) {
	//printf("submitting pending SPU update.. cycles after: %d\n", cycles_after);
    pcsxrearmed_update_pending = 1;
    pcsxrearmed_update_at_cycle = *psxRegs_cycle_valptr + cycles_after;
}

//senquack - This is not used yet in our adaptation and I'm not sure if it's useful for us.
//			 (was implemented in pcsx_rearmed/libpcsxcore/spu.[ch]):
//#define H_SPUirqAddr     0x0da4
//#define H_SPUaddr        0x0da6
//#define H_SPUdata        0x0da8
//#define H_SPUctrl        0x0daa
//#define H_SPUstat        0x0dae
//#define H_SPUon1         0x0d88
//#define H_SPUon2         0x0d8a
//#define H_SPUoff1        0x0d8c
//#define H_SPUoff2        0x0d8e
//void CALLBACK SPUirq(void);
//void CALLBACK SPUirq(void) {
//	psxHu32ref(0x1070) |= SWAPu32(0x200);
//}
#endif //SPU_PCSXREARMED_SPUSCHEDULE_H
