
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