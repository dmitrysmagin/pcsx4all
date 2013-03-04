
========================================================================================
PCSX4ALL 2.3 (May 03, 2012) by Franxis (franxism@gmail.com) and Chui (sdl_gp32@yahoo.es)
========================================================================================


1. INTRODUCTION
---------------

It is a port of PCSX Reloaded PSX emulator for GPH WIZ/CAANOO portable consoles.
To see GPL license go to the end of this document (chapter 14).

Official web pages for news, additional information and donations:
http://chui.dcemu.co.uk/
http://franxis.zxq.net/


2. CONTROLS
-----------

The emulator controls are the next ones:
- Joystick: Movement in pad.
- B,X,A,Y,L,R,SELECT+L or I+L,SELECT+R or I+R: Buttons Circle,X,Square,Triangle,L,R,L2,R2.
- SELECT or I: SELECT button.
- MENU or II: START button.
- L+R+START or HOME: Exit to selection menu to select another game (then L+R or HOME in the menu to exit).
- L+R+SELECT or L+R+I: Show FPS.
- VolUp and VolDown: Increase / decrease sound volume.
- SELECT+VolDown or I+L: Load save-state.
- SELECT+VolUp or I+R: Save save-state.


3. EMULATION OPTIONS
--------------------

- WIZ/CAANOO Clock:
200 to 900 MHz options are available. Performance of the emulator is better with bigger values.
533 MHz is the standard value. 700 MHz seems to run ok with all consoles (but batteries duration is reduced).
Use greater values at your own risk!.

- RAM Tweaks:
ON: The RAM Tweaks are activated to gain some more performance. Some consoles have problems with them. Recommended in WIZ.
OFF: The RAM Tweaks are disabled to ensure the emulator run on every console. Recommended in CAANOO.

- Frame-Limit:
ON: Frame-limiter is enabled.
OFF: Frame-limiter is disabled.

- Frame-Skip:
Frame-Skip OFF: Frame-skip disabled
Frame-Skip ON (if spd<50%): Frame-skip automatically enabled if speed < 50%
Frame-Skip ON (if spd<60%): Frame-skip automatically enabled if speed < 60%
Frame-Skip ON (if spd<70%): Frame-skip automatically enabled if speed < 70%
Frame-Skip ON (if spd<80%): Frame-skip automatically enabled if speed < 80%
Frame-Skip ON (if spd<90%): Frame-skip automatically enabled if speed < 90%
Frame-Skip ON (minimum): Frame-skip enabled (minimum)
Frame-Skip ON (medium): Frame-skip enabled (medium)
Frame-Skip ON (maximum): Frame-skip enabled (maximum)

- Interlace:
OFF: Video interlace is disabled.
Simple: Simple video interlace (half of lines are always shown).
Progressive: Progressive interlace (odd/even lines are shown consecutively).

- Sound:
OFF: Sound is disabled.
ON (basic): Basic sound is enabled (FM and WAVE).
ON (XA): Additional sound (XA-Audio).
ON (CD-Audio): Additional sound (CD-Audio).
ON (XA+CD-Audio): Additional sound (both XA-Audio and CD-Audio).

- CPU Clock:
The clock of the CPU can be adjusted from 10% to 200%. The nominal value is 100% and the CPU is emulated accurately.
Use lower values to get more performance. Also the clock can be overclocked up to 200%.

- CPU BIAS:
Auto: The CPU cycle multiplier is automatically adjusted.
1..16: The CPU cycle multiplier is manually adjusted.
3 should be ok for the majority of the games, but the best value depends on the game.
4 can be used with some 2D games to gain speed. The higher values are faster as the CPU is underclocked but if the game needs more CPU power the game will be slowed down.
Lower values can be selected for compatibility but the emulator will be very slow.

- CPU Core:
HLE: The fastest mode. The recompiler is enabled with BIOS emulation.
HLE-Secure: Slower but more compatible HLE recompiler (e.g. Castlevania SOTN or Final Fantasy VII need this to run).
BIOS: Slower than HLE but more compatible. It needs a copy of the PSX BIOS (scph1001.bin).
HLE (Interpreter): Slow interpreter with HLE BIOS emulation.
BIOS (Interpreter): Slow interpreter needing a copy of the PSX BIOS (scph1001.bin).
NOTE: BIOS modes need a copy of the PSX BIOS (scph1001.bin) placed in the same directory as the emulator.
It is needed to play games like Cotton 100% or Bubble Bobble 2 with correct sound. It is also needed with other games
due to bugs in the BIOS HLE emulation (Memory Cards, PAL sound timing, etc).

- GPU Type:
Software: Software GPU with all the features enabled.
No Light: Disable lighting to gain some speed. Graphical bugs could appear in games.
No Blend: Disable blending to gain some speed. Graphical bugs could appear in games.
No Light+Blend: Disable both lighting and blending. Graphical bugs could appear in games.

- Auto-Save:
OFF: Auto-save feature is disabled.
ON: The game state is loaded automatically when the game is started and saved when you exit.

- Game Fixes:
Parasite Eve 2, Vandal Hearts 1/2 fix
InuYasha Sengoku Battle fix


4. INSTALLATION
---------------

autorun.gpu         -> Script for auto-run (WIZ)
mcd001.mcr          -> Memory-card #1
mcd002.mcr          -> Memory-card #2
pcsx4all.gpe        -> Main script to autoselect WIZ/CAANOO
pcsx4all_wiz.gpe    -> Frontend to select games (WIZ)
pcsx4all_wiz        -> Emulator binary (WIZ)
pcsx4all_caanoo.gpe -> Frontend to select games (CAANOO)
pcsx4all_caanoo     -> PCSX4ALL emulator (CAANOO)
scph1001.bin        -> PSX BIOS dump (not included)
warm_2.6.24.ko      -> MMU Hack Kernel Module
conf/               -> Frontend configuration files
exec/               -> PSX executables directory
isos/               -> PSX ISOs directory
save/               -> Save-states directory


5. SUPPORTED GAMES
------------------

- The following ISO formats are supported: iso, bin, bin+cue, bin+toc, img+ccd, mdf+mds, img+sub.
- The ISO file extensions should be in lower case!
- Do not delete cue, toc, ccd, mds or sub files, they are needed for CD-Audio support.
- Compressed ISO formats are not supported yet!
- Additionally PSX executables are supported.


6. ORIGINAL CREDITS
-------------------

- http://www.pcsx.net/ : PCSX created by linuzappz, shadow, Nocomp, Pete Bernett, nik3d and AkumaX.
- http://pcsx-df.sourceforge.net/ : PCSX-df by Ryan Schultz, Andrew Burton, Stephen Chao, Marcus Comstedt and Stefan Sikora.
- http://www.pcsx.net/ : PCSX-Reloaded by Wei Mingzhi and edgbla.
- http://mamedev.org/ : GTE divider reverse engineering by pSXAuthor for MAME.
- http://www.dosbox.com/ : ARMv4 backend by M-HT for DOSBox.
- http://github.com/zodttd/psx4all : GPU coded by Unai for PSX4ALL.
- http://www.zlib.net/ : ZLIB by Jean-loup Gailly and Mark Adler.


7. PORT CREDITS
---------------

- Port to GPH WIZ/CAANOO by Franxis (franxism@gmail.com) and Chui (sdl_gp32@yahoo.es).
- ARM recompiler based on PCSX x86 recompiler by Franxis and Chui.
- GTE optimized in ARMv5 assembler by Franxis and Chui.
- SPU and PAD by Franxis (SPU: 22 KHz mono with async CD-Audio, PAD: SCPH-1010 model).
- Beta testing by Anarchy, nintiendo1, Rivroner, buba-ho-tep and Zenzuke.
- Artwork by Zenzuke.


8. DEVELOPMENT
--------------

May 3, 2012:
- Version 2.3. Several internal changes. Improved HLE, video frame-skip, optimizations, etc.

April 10, 2011:
- Version 2.2. Frame limit bug fixed.

April 9, 2011:
- Version 2.1. Added WIZ TV-Out support. HLE, GPU and SPU optimizations and compatibility improvements.

December 29, 2010:
- Version 2.0. Added CAANOO port. Several improvements (e.g. Auto-BIAS and idle-loop detection).

July 31, 2010:
- Version 1.0. First release for the gp32spain.com GP2X WIZ programming competition 2010.

Developed with:
- DevKitGP2X rc2 (http://sourceforge.net/project/showfiles.php?group_id=114505)
- Sourcery G++ Lite 2006q1-6 (http://www.codesourcery.com/sgpp/lite/arm/portal/release293)
- GpBinConv by Aquafish (www.multimania.com/illusionstudio/aquafish/)


9. KNOWN PROBLEMS
-----------------

- Not compatible with several games.
- Slow playability with several games.


10. TO BE IMPROVED
------------------

- OpenGL-ES GPU.
- Faster GTE.
- Faster recompiler.
- Gain compatibility.


11. THANKS TO
-------------

- Gamepark Holdings: Thank you people for releasing GP2X console, as well as providing some GP2X development
  units for programmers some weeks before GP2X official launch. One of them was finally mine through Anarchy
  mediation (gp32spain.com). Also for releasing the WIZ console, as well as for sending me the development
  unit through EvilDragon mediation (gp32x.de). Finally for releasing the CAANOO console as well as for sending us.
- Puck2099: Thank you for all the information and the Pico library for the WIZ console.
- Orkie: Thank you for all the help with the WIZ and his Castor library.
- Notaz: Thanks for the MMU Hack kernel module for the WIZ console.


12. INTERESTING WEBPAGES ABOUT PSX EMULATION
--------------------------------------------

- http://www.pcsx.net/
- http://pcsxr.codeplex.com/
- http://pcsx-df.sourceforge.net/
- http://code.google.com/p/pcsx-revolution/
- http://github.com/smokku/psx4m


13. SOME OTHER INTERESTING WEBPAGES
-----------------------------------

- http://www.gp32spain.com
- http://www.gp32x.com
- http://www.emulatronia.com
- http://www.emulation64.com


14. GPL LICENSE
----------------

Copyright (C) 2010 PCSX4ALL Team
Copyright (C) 2010 Franxis (franxism@gmail.com)
Copyright (C) 2010 Chui (sdl_gp32@yahoo.es)

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the
Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02111-1307 USA.


15. COMPILATION
---------------

Compile executable for WIZ: DevKitGP2X rc2 (http://sourceforge.net/project/showfiles.php?group_id=114505)
make -f Makefile.wiz clean
make -f Makefile.wiz

NOTE: To compile executable for WIZ the following DevKitGP2X files have to be modified manually:
devkitGP2X\sysroot\usr\lib\libm.so:
/* GNU ld script
   Use the shared library, but some functions are only in
   the static library, so try that secondarily.  */
OUTPUT_FORMAT(elf32-littlearm)
GROUP ( /lib/libm.so.6 )
devkitGP2X\sysroot\usr\lib\libpthread.so:
/* GNU ld script
   Use the shared library, but some functions are only in
   the static library, so try that secondarily.  */
OUTPUT_FORMAT(elf32-littlearm)
GROUP ( /lib/libpthread.so.0 )

Compile executable for CAANOO: Sourcery G++ Lite 2006q1-6 (http://www.codesourcery.com/sgpp/lite/arm/portal/release293)
make -f Makefile.caanoo clean
make -f Makefile.caanoo
