
This is a mips to mips recompiler originally written by Ulrich Hecht
for psx4all emulator.

https://github.com/uli/psx4all-dingoo/

Modified, cleaned up, reworked and optimized by Dmitry Smagin to be used
with pcsx4all by Chui and Franxis.

https://github.com/dmitrysmagin/pcsx4all/


What's already done:

 - Adapted to pcsx4all 2.3 codebase
 - Cleaned up from dead code and leftovers from arm recompiler
 - Simplified zero register optimizations, since mips has hardware
   zero register unlike arm
 - Implemented code generation for DIV/DIVU
 - Implemented consequent LWL/LWR/SWL/SWR optimizations
 - Implemented consequent loads/stores optimizations
 - Improved constant caching, eliminated useless const reloads
 - GTE code is adapted for new gte core
 - Optimized for mips32r2 target (SEB/SEH/EXT/INS), so pay attention when
   backporting to Dingoo A320 which is mips32r1
 - Added autobias support
 - Added GTE code generation for CFC2, CTC2, MFC2, MTC2, LWC2, SWC2 opcodes

 TODO list

* recompiler:
  - Implement more GTE code generation

* constants caching
  For now it's very limited, used by ADDIU, ORI, LUI and memory operations
  - Add constants caching for more opcodes

* register allocator
  For now host registers s0-s7 are allocated, s8 is a pointer to psxRegs
  - Add caching of LO/HI regs
  - Maybe allocate more regs like t4-t7 and save them across calls to HLE?

 KNOWN BUGS

* Tekken 2 has broken graphics (it's correct with interpreter)
* G.Darius hangs if compiled with USE_DIRECT_MEM_ACCESS