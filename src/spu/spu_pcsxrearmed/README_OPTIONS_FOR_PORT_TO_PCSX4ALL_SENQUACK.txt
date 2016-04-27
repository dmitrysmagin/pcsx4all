Recommended settings:
-xa -cdda -bios

Until we add a -h switch to show help, I'll refer people to this
TXT file listing the various options of the spu_pcsxrearmed SPU
plugin I ported from PCSX ReARMed (credit goes to Notaz), which is
a heavily modified version of the P.E.O.P.S dfsound plugin by Pete.
-senquack Apr 24 2016

**************
- REVISIONS: -
**************
Apr 25 2016:
  * Audio syncing is now on by default. Use -nosyncaudio to disable.
  * New option '-use_old_audio_mutex' enables old method of syncing audio,
    using mutex and condition variable). Default is now to use non-locking
    atomic shared var to syncronize SDL audio thread with emu.

**********************************
- Optional settings you can try: -
**********************************
-nosyncaudio    (Emu won't wait and instead drop samples if output buffer full.)
-interpolation none,simple,gaussian,cubic  (upsamples audio, using more CPU)
-reverb         (enables reverb)
-xapitch	    (enable support for XA music/sfx speed pitch changes)
-notempo        (uses slower, more-compatible SPU cycle estimations, which
                 Pandora/Pyra/Android PCSX-ReARMed builds use as default.
                 Helps Final Fantasy games and Valkyrie Profile compatibility,
                 perhaps others, but can cause more audio stuttering.)
-nofixedupdates (don't output a fixed number of samples per frame.. I haven't
				 yet played with this much, but it is a configurable option in
				 PCSX-ReARMed for a reason, I guess)
-threaded_spu   (enable doing some sound processing in a thread. Almost
                 certainly won't help us in any way, but I did add command-line
                 switch to enable using Notaz's thread code. The Pandora builds
                 run an optimized version of the thread on their DSP chip.)
-volume 0..1024  (1024 is max volume, 0 will mute sound but keep the SPU plugin
                  running for max compatibility & allowing -syncaudio option
                  to have an effect. Use -silent flag to disable SPU plugin.)
-use_old_audio_mutex  (Don't use newer mutex-free SDL audio output code.
                       Only use this flag for debugging/verification, as the
                       newer code uses more efficient method)
