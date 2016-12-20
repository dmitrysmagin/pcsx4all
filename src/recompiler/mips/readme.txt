
This is a mips to mips recompiler originally written by Ulrich Hecht
for psx4all emulator.

https://github.com/uli/psx4all-dingoo/

Modified, cleaned up, reworked and optimized by Dmitry Smagin and
Daniel Silsby to be used with pcsx4all by Chui and Franxis.

https://github.com/dmitrysmagin/pcsx4all/


What's already done:

 - Adapted to pcsx4all 2.3 codebase
 - Cleaned up from dead code and leftovers from arm recompiler
 - Simplified zero register optimizations, since mips has hardware
   zero register unlike arm
 - Implemented code generation for DIV/DIVU/MTHI/MFHI/MTLO/MFLO
 - Implemented consequent LWL/LWR/SWL/SWR optimizations
 - Implemented consequent loads/stores optimizations
 - Improved constant caching, eliminated useless const reloads
 - GTE code is adapted for new gte core
 - Optimized for mips32r2 target (SEB/SEH/EXT/INS), compatibility with
   Dingoo A320 is retained (which is mips32r1)
 - Added GTE code generation for CFC2, CTC2, MFC2, MTC2, LWC2, SWC2 opcodes
 - Block recompilation is reworked to match pcsx4all behavior,
   recExecuteBlock is fixed for HLE
 - Moved to interpreter_pcsx and gte_pcsx (Destruction Derby 2 fixed)
 - Implemented partially load delays in branch delay slots:
   BGTZ: Tekken 2
   JR: Tekken 3, Skullmonkeys
 - Added check if RAM is valid for writing for writes to constant addresses [senquack],
   this fixes R-Types and Cart World Series freezes at start.

 TODO list

* recompiler:
  - Implement proper page-based code invalidation to support self-modifying code
  - Implement branches in branch delay slots (which game uses them?)
  - Test more games from this list:
     https://github.com/libretro-mirrors/mednafen-git/blob/master/src/psx/notes/PROBLEMATIC-GAMES
  - Implement more GTE code generation (if reasonable)

* constants caching
  For now it's very limited, used by ADDIU, ORI, LUI and memory operations
  - Add constants caching for more opcodes

* register allocator
  For now host registers s0-s7 are allocated, s8 is a pointer to psxRegs
  - Add caching of LO/HI regs
  - Maybe allocate more regs like t4-t7 and save them across calls to HLE?

 Problematic games which get stuck with recompiler:
  - Jackie Chan Stuntmaster (USA) (works with int+bios)
  - Next Tetris (gets stuck occasionally at start)
  - Mortal Kombat Trilogy (works with int+hle)
  - Tomb Raider II (U) (works with int+bios/hle)
  - Tomb Raider V Chronicles (J) (works with int+bios)
    with recompiler the graphics go crazy

