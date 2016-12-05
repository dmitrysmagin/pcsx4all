/***************************************************************************
 *   Copyright (C) 2010 by Blade_Arma                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02111-1307 USA.           *
 ***************************************************************************/

/*
 * Internal PSX counters.
 */

///////////////////////////////////////////////////////////////////////////////
//senquack - NOTE: Root counters code here has been updated to match Notaz's
// PCSX Rearmed where possible. Important changes include:
// * Proper handling of counter overflows.
// * VBlank root counter (counter 3) is triggered only as often as needed,
//   not every HSync.
// * Because SPU was originally updated inside VBlank root counter code, the
//   above change necessitated moving SPU update handling to psxBranchTest()
//   in r3000a.cpp. In PCSX Rearmed, Notaz's SPU plugin requires an update
//   only once per emulated frame, but we target slower devices and until
//   we get good auto-frameskip, more-frequent SPU updates are necessary.
//   Older SPU plugins required *much* more frequent updates, making it even
//   less practical to do SPU updates here now that psxRcntUpdate() here is
//   called so much less frequently.
// * Some optimizations, more accurate calculation of timer updates.
//
// TODO : Implement direct rootcounter mem access of Rearmed dynarec?
//        (see https://github.com/notaz/pcsx_rearmed/commit/b1be1eeee94d3547c20719acfa6b0082404897f1 )
//        Seems to make Parasite Eve 2 RCntFix hard to implement, though.
// TODO : Implement Rearmed's auto-frameskip so SPU doesn't need to
//        hackishly be updated twice per emulated frame.
// TODO : Implement Rearmed's frame limiter

#include "psxcounters.h"
#include "psxevents.h"
#include "gpu.h"
#include "profiler.h"
#include "plugin_lib/perfmon.h"

/******************************************************************************/

enum
{
    Rc0Gate           = 0x0001, // 0    not implemented
    Rc1Gate           = 0x0001, // 0    not implemented
    Rc2Disable        = 0x0001, // 0    partially implemented
    RcUnknown1        = 0x0002, // 1    ?
    RcUnknown2        = 0x0004, // 2    ?
    RcCountToTarget   = 0x0008, // 3
    RcIrqOnTarget     = 0x0010, // 4
    RcIrqOnOverflow   = 0x0020, // 5
    RcIrqRegenerate   = 0x0040, // 6
    RcUnknown7        = 0x0080, // 7    ?
    Rc0PixelClock     = 0x0100, // 8    fake implementation
    Rc1HSyncClock     = 0x0100, // 8
    Rc2Unknown8       = 0x0100, // 8    ?
    Rc0Unknown9       = 0x0200, // 9    ?
    Rc1Unknown9       = 0x0200, // 9    ?
    Rc2OneEighthClock = 0x0200, // 9
    RcUnknown10       = 0x0400, // 10   ?
    RcCountEqTarget   = 0x0800, // 11
    RcOverflow        = 0x1000, // 12
    RcUnknown13       = 0x2000, // 13   ? (always zero)
    RcUnknown14       = 0x4000, // 14   ? (always zero)
    RcUnknown15       = 0x8000, // 15   ? (always zero)
};

#define CounterQuantity           ( 4 )
//static const u32 CounterQuantity  = 4;

static const u32 CountToOverflow  = 0;
static const u32 CountToTarget    = 1;

static const u32 FrameRate[]      = { 60, 50 };

//senquack - Originally {262,312}, updated to match Rearmed:
static const u32 HSyncTotal[]     = { 263, 313 };

//senquack - TODO: PCSX Reloaded uses {243,256} here, and Rearmed
// does away with array completely and uses 240 in all cases:
//static const u32 VBlankStart[]    = { 240, 256 };
#define VBlankStart 240

//Cycles between SPU plugin updates
u32 spu_upd_interval;

#ifndef spu_pcsxrearmed
// Older SPU plugins are updated every 23rd HSync, regardless of PAL/NTSC mode
#define SPU_UPD_INTERVAL 23
#endif

/******************************************************************************/

static Rcnt rcnts[ CounterQuantity ];

u32 hSyncCount = 0;
static u32 spuSyncCount = 0;

//senquack - Added two vars from PCSX Rearmed:
u32 frame_counter = 0;
static u32 hsync_steps = 0;
static u32 base_cycle = 0;

//senquack - Originally separate variables, now handled together with
// all other scheduled emu events as new event type PSXINT_RCNT
static u32 &psxNextCounter = psxRegs.intCycle[PSXINT_RCNT].cycle;
static u32 &psxNextsCounter = psxRegs.intCycle[PSXINT_RCNT].sCycle;

/******************************************************************************/

static inline void setIrq( u32 irq )
{
	psxHu32ref(0x1070) |= SWAPu32(irq);
// CHUI: Añado ResetIoCycle para permite que en el proximo salto entre en psxBranchTest
    	ResetIoCycle();
}

//senquack - Added verboseLog & VERBOSE_LEVEL from PCSX Rearmed:
#define VERBOSE_LEVEL 0

#ifdef DEBUG_BIOS    // Original code logged in many places here when DEBUG_BIOS was defined, so do the same
#define VERBOSE_LEVEL 1
#endif

static void verboseLog( u32 level, const char *str, ... )
{
#if VERBOSE_LEVEL > 0
    if( level <= VerboseLevel )
    {
        va_list va;
        char buf[ 4096 ];

        va_start( va, str );
        vsprintf( buf, str, va );
        va_end( va );

        printf( "%s", buf );
        fflush( stdout );
    }
#endif
}

/******************************************************************************/

INLINE void _psxRcntWcount( u32 index, u32 value )
{
    if( value > 0xffff )
    {
        verboseLog( 1, "[RCNT %i] wcount > 0xffff: %x\n", index, value );
        value &= 0xffff;
    }

    rcnts[index].cycleStart  = psxRegs.cycle;
    rcnts[index].cycleStart -= value * rcnts[index].rate;

    // TODO: <=.
    if( value < rcnts[index].target )
    {
        rcnts[index].cycle = rcnts[index].target * rcnts[index].rate;
        rcnts[index].counterState = CountToTarget;
    }
    else
    {
        rcnts[index].cycle = 0x10000 * rcnts[index].rate;
        rcnts[index].counterState = CountToOverflow;
    }
}

static inline u32 _psxRcntRcount( u32 index )
{
    u32 count;

    count  = psxRegs.cycle;
    count -= rcnts[index].cycleStart;
    if (rcnts[index].rate > 1)
        count = UDIV(count,rcnts[index].rate);

    if( count > 0x10000 )
    {
        verboseLog( 1, "[RCNT %i] rcount > 0x10000: %x\n", index, count );
    }
    count &= 0xffff;

    return count;
}

//senquack - Added from PCSX Rearmed:
static void _psxRcntWmode( u32 index, u32 value )
{
    rcnts[index].mode = value;

    switch( index )
    {
        case 0:
            if( value & Rc0PixelClock )
            {
                rcnts[index].rate = 5;
            }
            else
            {
                rcnts[index].rate = 1;
            }
        break;
        case 1:
            if( value & Rc1HSyncClock )
            {
                rcnts[index].rate = (PSXCLK / (FrameRate[Config.PsxType] * HSyncTotal[Config.PsxType]));
            }
            else
            {
                rcnts[index].rate = 1;
            }
        break;
        case 2:
            if( value & Rc2OneEighthClock )
            {
                rcnts[index].rate = 8;
            }
            else
            {
                rcnts[index].rate = 1;
            }

            // TODO: wcount must work.
            if( value & Rc2Disable )
            {
                rcnts[index].rate = 0xffffffff;
            }
        break;
    }
}

/******************************************************************************/

static void psxRcntSet(void)
{
    s32 countToUpdate;
    u32 i;

    psxNextsCounter = psxRegs.cycle;
    psxNextCounter  = 0x7fffffff;

    for( i = 0; i < CounterQuantity; ++i )
    {
        countToUpdate = rcnts[i].cycle - (psxNextsCounter - rcnts[i].cycleStart);

        if( countToUpdate < 0 )
        {
            psxNextCounter = 0;
            break;
        }

        if( countToUpdate < (s32)psxNextCounter )
        {
            psxNextCounter = countToUpdate;
        }
    }

    // Any previously queued PSXINT_RCNT event will be replaced
    psxEventQueue.enqueue(PSXINT_RCNT, psxNextCounter);
}

/******************************************************************************/

static void psxRcntReset( u32 index )
{
    u32 rcycles;

    rcnts[index].mode |= RcUnknown10;

    if( rcnts[index].counterState == CountToTarget )
    {
        rcycles = psxRegs.cycle - rcnts[index].cycleStart;
        if( rcnts[index].mode & RcCountToTarget )
        {
            rcycles -= rcnts[index].target * rcnts[index].rate;
            rcnts[index].cycleStart = psxRegs.cycle - rcycles;
        }
        else
        {
            rcnts[index].cycle = 0x10000 * rcnts[index].rate;
            rcnts[index].counterState = CountToOverflow;
        }

        if( rcnts[index].mode & RcIrqOnTarget )
        {
            if( (rcnts[index].mode & RcIrqRegenerate) || (!rcnts[index].irqState) )
            {
                verboseLog( 3, "[RCNT %i] irq\n", index );
                setIrq( rcnts[index].irq );
                rcnts[index].irqState = 1;
            }
        }

        rcnts[index].mode |= RcCountEqTarget;

        if( rcycles < 0x10000 * rcnts[index].rate )
            return;
    }

    if( rcnts[index].counterState == CountToOverflow )
    {
        rcycles = psxRegs.cycle - rcnts[index].cycleStart;
        rcycles -= 0x10000 * rcnts[index].rate;

        rcnts[index].cycleStart = psxRegs.cycle - rcycles;

        if( rcycles < rcnts[index].target * rcnts[index].rate )
        {
            rcnts[index].cycle = rcnts[index].target * rcnts[index].rate;
            rcnts[index].counterState = CountToTarget;
        }

        if( rcnts[index].mode & RcIrqOnOverflow )
        {
            if( (rcnts[index].mode & RcIrqRegenerate) || (!rcnts[index].irqState) )
            {
                verboseLog( 3, "[RCNT %i] irq\n", index );
                setIrq( rcnts[index].irq );
                rcnts[index].irqState = 1;
            }
        }

        rcnts[index].mode |= RcOverflow;
    }
}

void psxRcntUpdate()
{
#ifdef DEBUG_ANALYSIS
	dbg_anacnt_psxRcntUpdate++;
#endif
    pcsx4all_prof_start_with_pause(PCSX4ALL_PROF_COUNTERS,PCSX4ALL_PROF_CPU);
    u32 cycle;

    cycle = psxRegs.cycle;

    // rcnt 0.
    if( cycle - rcnts[0].cycleStart >= rcnts[0].cycle )
    {
        psxRcntReset( 0 );
    }

    // rcnt 1.
    if( cycle - rcnts[1].cycleStart >= rcnts[1].cycle )
    {
        psxRcntReset( 1 );
    }

    // rcnt 2.
    if( cycle - rcnts[2].cycleStart >= rcnts[2].cycle )
    {
        psxRcntReset( 2 );
    }

    // rcnt base.
    if( cycle - rcnts[3].cycleStart >= rcnts[3].cycle )
    {
        // TODO (senquack): Implement HW_GPU_STATUS code from Rearmed
        u32 leftover_cycles = cycle - rcnts[3].cycleStart - rcnts[3].cycle;
        u32 next_vsync;

        hSyncCount += hsync_steps;

        // VSync irq.
        if( hSyncCount == VBlankStart )
        {
#ifdef DEBUG_BIOS
            dbg("UpdateLace");
#endif
            HW_GPU_STATUS &= ~PSXGPU_LCF;

#ifdef USE_GPULIB
            GPU_vBlank( 1, 0 );
#endif
            setIrq( 0x01 );
            GPU_updateLace();

            pmonUpdate();  // Update and display performance stats

            pad_update();

#ifdef spu_pcsxrearmed
            //senquack - PCSX Rearmed updates its SPU plugin once per emulated
            // frame. However, we target slower platforms like GCW Zero, and
            // lack auto-frameskip, so this would lead to audio dropouts. For
            // now, we update SPU plugin twice per frame in r3000a.cpp
            // psxBranchTest()
            //SPU_async( cycle, 1 );
#endif
        }

        // Update lace. (with InuYasha fix)
        if( hSyncCount >= (Config.VSyncWA ? UDIV(HSyncTotal[Config.PsxType],BIAS) : HSyncTotal[Config.PsxType]) )
        {
            hSyncCount = 0;
            frame_counter++;

            gpuSyncPluginSR();
            if( (HW_GPU_STATUS & PSXGPU_ILACE_BITS) == PSXGPU_ILACE_BITS )
                HW_GPU_STATUS |= frame_counter << 31;

#ifdef USE_GPULIB
            GPU_vBlank( 0, HW_GPU_STATUS >> 31 );
#endif

#ifdef DEBUG_END_FRAME
		{
			static unsigned _endframe_=0;
			static unsigned _frametime_[DEBUG_END_FRAME+1];
			_frametime_[_endframe_]=(get_ticks()
#ifndef TIME_IN_MSEC
					/1000
#endif
					);
			_endframe_++;
			if (_endframe_>DEBUG_END_FRAME) {
				unsigned i;
				for(i=1;i<_endframe_;i++) 
					printf("FRAME %u = %u msec\n",i,_frametime_[i]-_frametime_[i-1]);
				pcsx4all_exit();
			}
		}
#endif

			if ((toSaveState)&&(SaveState_filename)) {
			toSaveState=0;
			SaveState(SaveState_filename);
			if (toExit)
				pcsx4all_exit();
			}
			if ((toLoadState)&&(SaveState_filename))
			{
				toLoadState=0;
				LoadState(SaveState_filename);
				pcsx4all_prof_reset();
#ifdef PROFILER_PCSX4ALL
				_pcsx4all_prof_end(PCSX4ALL_PROF_CPU,0);
#endif
				pcsx4all_prof_start(PCSX4ALL_PROF_CPU);
				psxCpu->Execute();
				pcsx4all_prof_end(PCSX4ALL_PROF_CPU);
				pcsx4all_exit();
			}
        }

        // Schedule next call, in hsyncs
        hsync_steps = HSyncTotal[Config.PsxType] - hSyncCount;
        next_vsync = VBlankStart - hSyncCount; // ok to overflow
        if( next_vsync && next_vsync < hsync_steps )
            hsync_steps = next_vsync;

        rcnts[3].cycleStart = cycle - leftover_cycles;
        if (Config.PsxType)
                // 20.12 precision, clk / 50 / 313 ~= 2164.14
                base_cycle += hsync_steps * 8864320;
        else
                // clk / 60 / 263 ~= 2146.31
                base_cycle += hsync_steps * 8791293;
        rcnts[3].cycle = base_cycle >> 12;
        base_cycle &= 0xfff;
    }

    psxRcntSet();

    pcsx4all_prof_end_with_resume(PCSX4ALL_PROF_COUNTERS,PCSX4ALL_PROF_CPU);
}

/******************************************************************************/

void psxRcntWcount( u32 index, u32 value )
{
    verboseLog( 2, "[RCNT %i] wcount: %x\n", index, value );

    _psxRcntWcount( index, value );
    psxRcntSet();
}

void psxRcntWmode( u32 index, u32 value )
{
    verboseLog( 1, "[RCNT %i] wmode: %x\n", index, value );

    _psxRcntWmode( index, value );
    _psxRcntWcount( index, 0 );

    rcnts[index].irqState = 0;
    psxRcntSet();
}

void psxRcntWtarget( u32 index, u32 value )
{
    verboseLog( 1, "[RCNT %i] wtarget: %x\n", index, value );

    rcnts[index].target = value;

    _psxRcntWcount( index, _psxRcntRcount( index ) );
    psxRcntSet();
}

/******************************************************************************/

u32 psxRcntRcount( u32 index )
{
#ifdef DEBUG_ANALYSIS
	dbg_anacnt_psxRcntRcount++;
#endif
    pcsx4all_prof_start_with_pause(PCSX4ALL_PROF_COUNTERS,PCSX4ALL_PROF_CPU);
    u32 count;

	//senquack - TODO: Do we still need this?
#ifndef USE_NO_IDLE_LOOP
    if (autobias) {
	u32 cycle_next=rcnts[index].cycle+rcnts[index].cycleStart;
	if (cycle_next>psxRegs.cycle) {
		if (psxRegs.io_cycle_counter>(psxRegs.cycle+32) && psxRegs.io_cycle_counter<cycle_next) {
			cycle_next = psxRegs.io_cycle_counter - psxRegs.cycle - 24;
		} else {
			cycle_next = 12+((cycle_next - psxRegs.cycle)&0x1FF);
		}
	    	if (rcnts[index].target) {
			unsigned value=(((rcnts[index].target)*(rcnts[index].rate+1))/4)*BIAS;
			if (value<cycle_next)
				psxRegs.cycle+=value;
			else
				psxRegs.cycle+=cycle_next;
		} else {
			psxRegs.cycle+=cycle_next;
		}
    	}
    }
#endif

    count = _psxRcntRcount( index );

    // Parasite Eve 2 fix.
    if( Config.RCntFix ) {
        if( index == 2 ) {
            if( rcnts[index].counterState == CountToTarget )
                count=UDIV(count,BIAS);
        }
    }

    verboseLog( 2, "[RCNT %i] rcount: %x\n", index, count );

    pcsx4all_prof_end_with_resume(PCSX4ALL_PROF_COUNTERS,PCSX4ALL_PROF_CPU);
    return count;
}

u32 psxRcntRmode( u32 index )
{
#ifdef DEBUG_ANALYSIS
	dbg_anacnt_psxRcntRmode++;
#endif
    pcsx4all_prof_start_with_pause(PCSX4ALL_PROF_COUNTERS,PCSX4ALL_PROF_CPU);
    u16 mode;

    mode = rcnts[index].mode;
    rcnts[index].mode &= 0xe7ff;

    verboseLog( 2, "[RCNT %i] rmode: %x\n", index, mode );

    pcsx4all_prof_end_with_resume(PCSX4ALL_PROF_COUNTERS,PCSX4ALL_PROF_CPU);
    return mode;
}

u32 psxRcntRtarget( u32 index )
{
    verboseLog( 2, "[RCNT %i] rtarget: %x\n", index, rcnts[index].target );

    return rcnts[index].target;
}

/******************************************************************************/

void psxRcntInit(void)
{
    s32 i;

    // rcnt 0.
    rcnts[0].rate   = 1;
    rcnts[0].irq    = 0x10;

    // rcnt 1.
    rcnts[1].rate   = 1;
    rcnts[1].irq    = 0x20;

    // rcnt 2.
    rcnts[2].rate   = 1;
    rcnts[2].irq    = 0x40;

    // rcnt base.
    rcnts[3].rate   = 1;
    rcnts[3].mode   = RcCountToTarget;
    rcnts[3].target = (PSXCLK / (FrameRate[Config.PsxType] * HSyncTotal[Config.PsxType]));

    for( i = 0; i < CounterQuantity; ++i )
    {
        _psxRcntWcount( i, 0 );
    }

    hSyncCount = 0;
    hsync_steps = 1;

    // SPU interval handling:
#ifdef spu_pcsxrearmed
    // PCSX Rearmed only updates SPU once per frame, but we target platforms like
    //  GCW Zero that are too slow to run many games at 60FPS. Until we have good
    //  auto-frameskip, we update SPU plugin twice per frame to avoid dropouts.
	//  If this interval is changed, be sure to check cutscenes in Metal Gear
	//  Solid or the intro to Chrono Cross, as they use SPU HW IRQ and are highly
	//  sensitive to timing.
    spu_upd_interval = PSXCLK / (FrameRate[Config.PsxType] * 2);
#else
    // Older SPU plugins are updated much more often than above
    spu_upd_interval = (SPU_UPD_INTERVAL * PSXCLK) / (FrameRate[Config.PsxType] * HSyncTotal[Config.PsxType]);
#endif
    // Schedule first SPU update. Future ones will be scheduled automatically
    SCHEDULE_SPU_UPDATE(spu_upd_interval);

    psxRcntSet();
}

/******************************************************************************/

int psxRcntFreeze(void *f, FreezeMode mode)
{
    u32 spuSyncCount = 0;
    u32 count;
    s32 i;

    if (    freeze_rw(f, mode, &rcnts, sizeof(rcnts))
         || freeze_rw(f, mode, &hSyncCount, sizeof(hSyncCount))
         || freeze_rw(f, mode, &spuSyncCount, sizeof(spuSyncCount))
         || freeze_rw(f, mode, &psxNextCounter, sizeof(psxNextCounter))
         || freeze_rw(f, mode, &psxNextsCounter, sizeof(psxNextsCounter)) )
    return -1;

    if (mode == FREEZE_LOAD)
    {
        // don't trust things from a savestate
        for( i = 0; i < CounterQuantity; ++i )
        {
            _psxRcntWmode( i, rcnts[i].mode );
            count = (psxRegs.cycle - rcnts[i].cycleStart) / rcnts[i].rate;
            _psxRcntWcount( i, count );
        }
        hsync_steps = (psxRegs.cycle - rcnts[3].cycleStart) / rcnts[3].target;
        psxRcntSet();

        base_cycle = 0;
    }

    return 0;
}

unsigned psxGetSpuSync(void){
	return spuSyncCount;
}

//senquack - Called before psxRegs.cycle is adjusted back to zero
// by psxResetCycleValue() in psxevents.cpp
void psxRcntAdjustTimestamps(const uint32_t prev_cycle_val)
{
	for (int i=0; i < CounterQuantity; ++i) {
		rcnts[i].cycleStart -= prev_cycle_val;
	}
}

/******************************************************************************/
