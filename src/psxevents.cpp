/***************************************************************************
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
 * Event / interrupt scheduling
 *
 * Added July 2016 by senquack (Daniel Silsby)
 */

#include "psxevents.h"
#include "r3000a.h"
#include "debug.h"

// To get event-handler functions:
#include "cdrom.h"
#include "plugins.h"
#include "psxdma.h"
#include "mdec.h"

// psxBranchTest() debug statistics
#ifdef DEBUG_EVENTS
unsigned int bt_necessary, bt_unnecessary;
#endif

// When psxRegs.cycle is >= this figure, it gets reset to 0:
static const uint32_t reset_cycle_val_at = 2000000000;

// Function called when PSXINT_RESET_CYCLE_VAL event occurs:
// psxRegs.cycle is now regularly reset to 0 instead of being allowed to
// overflow. This ensures that it can always be compared directly to
// psxRegs.io_cycle_counter to know if psxBranchTest() needs to be called.
static void psxResetCycleValue()
{
#ifdef DEBUG_EVENTS
    /* psxBranchTest() debug statistics */
	printf("\n\nResetting cycle value %u  io_cycle_counter: %u\n", psxRegs.cycle, psxRegs.io_cycle_counter);
	printf("bt_necessary: %u  bt_unnecessary: %u\n", bt_necessary, bt_unnecessary);
	bt_necessary = bt_unnecessary = 0;
	//printf("bt_cp0_irq_enable: %u  bt_cp0_irq_disable: %u\n", bt_cp0_irq_enable, bt_cp0_irq_disable);
	//bt_cp0_irq_enable = bt_cp0_irq_disable = 0;
	psxEventQueue.print_event_queue();
#endif

	// Any events in the queue must be adjusted to match:
	psxEventQueue.adjust_event_timestamps(psxRegs.cycle);

	// All root-counter timestamps must also be adjusted:
	psxRcntAdjustTimestamps(psxRegs.cycle);

	// Reset cycle counter and enqueue new reset event:
	psxRegs.cycle = 0;
	psxEventQueue.enqueue(PSXINT_RESET_CYCLE_VAL, reset_cycle_val_at);

#ifdef DEBUG_EVENTS
	psxEventQueue.print_event_queue();
	printf("io_cycle_counter: %u\n", psxRegs.io_cycle_counter);
	printf("------------------------------------------\n\n");
#endif
}

void EventQueue::schedule_persistent_events()
{
	// Call psxResetCycleValue() once, which will take care of two things:
	//  1.) psxRegs.cycle value is reset to 0, important if loading a savestate
	//      from an older version of emu, and psxRegs.cycle might be huge value
	//  2.) schedule PSXINT_RESET_CYCLE_VAL event, which calls it again
	//      at regular, very infrequent, intervals
	psxResetCycleValue();

	// Must schedule initial SPU update.. it will reschedule itself thereafter
	SCHEDULE_SPU_UPDATE(spu_upd_interval);
}

void EventQueue::enqueue(const psxEventType ev, const uint32_t cycles_after)
{
	// Dequeue event if it already exists, to match original emu behavior
	if (contains(ev))
		queue.remove(ev);

	psxRegs.interrupt |= (uint32_t)1 << ev;
	psxRegs.intCycle[ev].sCycle = psxRegs.cycle;
	psxRegs.intCycle[ev].cycle = cycles_after;
	EventMoreImminent sort_functor; // Functor to sort queue entries by
	queue.insert((uint8_t)ev, sort_functor);
	psxRegs.intCycle[PSXINT_NEXT_EVENT] = psxRegs.intCycle[queue.front()];

	// psxRegs.io_cycle_counter is used by interpreter/recompiler to determine
	//  when next to call psxBranchTest()
	psxRegs.io_cycle_counter = psxRegs.intCycle[PSXINT_NEXT_EVENT].sCycle + psxRegs.intCycle[PSXINT_NEXT_EVENT].cycle;
}

void EventQueue::dequeue(const psxEventType ev)
{
	if (!contains(ev))
		return;

	queue.remove(ev);
	psxRegs.interrupt &= ~((uint32_t)1 << ev);

	// If additional events were also scheduled, copy the next most-imminent
	//  event's timestamp into PSXINT_NEXT_EVENT entry:
	// (At least one event will always remain scheduled, i.e. PSXINT_RCNT
	//  or PSXINT_SPU_UPDATE, so this check for emptiness is not really needed)
	if (!queue.empty()) {
		psxRegs.intCycle[PSXINT_NEXT_EVENT] = psxRegs.intCycle[queue.front()];

		// io_cycle_counter is used by interpreter/recompiler to determine next
		//  time to call psxBranchTest()
		psxRegs.io_cycle_counter = psxRegs.intCycle[PSXINT_NEXT_EVENT].sCycle + psxRegs.intCycle[PSXINT_NEXT_EVENT].cycle;
	} else {
		// Shouldn't happen, but set to 0 just in case:
		psxRegs.io_cycle_counter = 0;
	}
}

void EventQueue::dispatch_and_remove_front()
{
#ifdef DEBUG_EVENTS
	if (queue.empty()) {
		printf("Error: EventQueue::dispatch_and_remove_front() called when queue is empty\n");
		return;
	}
#endif

	uint8_t ev = queue.front();
	queue.remove_front();
	psxRegs.interrupt &= ~((uint32_t)1 << ev);

#ifdef DEBUG_EVENTS
	if (eventFuncs[ev] == event_stub_func) {
		printf("event_stub_func() called in EventQueue for unimplemented event %u\n", ev);
	}
#endif

	eventFuncs[ev]();  // Dispatch event

	// Queue can never be totally empty, as certain persistent events will
	//  always be rescheduled during dispatch above.
#ifdef DEBUG_EVENTS
	if (queue.empty())
		printf("ERROR: empty queue after EventQueue::dispatch_and_remove_front()\n");
#endif

	psxRegs.intCycle[PSXINT_NEXT_EVENT] = psxRegs.intCycle[queue.front()];
}

void EventQueue::init_from_freeze()
{
	queue.clear();
	EventMoreImminent sort_functor; // Functor to sort queue entries by
	for (uint8_t i=0; i < PSXINT_COUNT; ++i) {
		if (psxRegs.interrupt & ((uint32_t)1 << i)) {
			queue.insert(i, sort_functor);
		}
	}

	schedule_persistent_events();

	// Don't trust io_cycle_counter from a freeze (determines next call to
	//  psxBranchTest() from interpreter/recompiler, which also handles
	//  issuing HW IRQ exceptions which might have been pending at save.)
	psxRegs.io_cycle_counter = 0;
}

void EventQueue::event_stub_func()
{
}

#ifdef DEBUG_EVENTS
void EventQueue::print_event_queue()
{
	printf("Queue contains %u events:\n", queue.size());
	for (uint8_t *ev = queue.front_ptr(); ev != queue.end_ptr(); ++ev) {
		printf("EV: %u SCYCLE: %u CYCLE: %u\n", *ev, psxRegs.intCycle[*ev].sCycle, psxRegs.intCycle[*ev].cycle);
	}

	// Consistent ordering check:
	for (uint8_t *ev = queue.front_ptr(); ev != queue.end_ptr(); ++ev) {
		EventMoreImminent compare_func;
		if (ev < (queue.end_ptr() - 1)) {
			if (!compare_func(*ev, *(ev+1))) {
				if ((psxRegs.intCycle[*ev].sCycle + psxRegs.intCycle[*ev].cycle) !=
					(psxRegs.intCycle[*(ev+1)].sCycle + psxRegs.intCycle[*(ev+1)].cycle)) {
					printf("ERROR: EV %u > EV %u\n", *ev, *(ev+1));
				}
			}
		}
	}
}
#endif

EventQueue::EventFunc * const EventQueue::eventFuncs[PSXINT_COUNT] = {
	[PSXINT_SIO]             = sioInterrupt,
	[PSXINT_CDR]             = cdrInterrupt,
	[PSXINT_CDREAD]          = cdrReadInterrupt,
	[PSXINT_GPUDMA]          = gpuInterrupt,
	[PSXINT_MDECOUTDMA]      = mdec1Interrupt,
	[PSXINT_SPUDMA]          = spuInterrupt,
	[PSXINT_GPUBUSY]         = EventQueue::event_stub_func, // UNIMPLEMENTED (TODO?)
	[PSXINT_MDECINDMA]       = mdec0Interrupt,
	[PSXINT_GPUOTCDMA]       = gpuotcInterrupt,
	[PSXINT_CDRDMA]          = cdrDmaInterrupt,
	[PSXINT_NEWDRC_CHECK]    = EventQueue::event_stub_func, // UNIMPLEMENTED (TODO?)
	[PSXINT_RCNT]            = psxRcntUpdate,
	[PSXINT_CDRLID]          = cdrLidSeekInterrupt,
	[PSXINT_CDRPLAY]         = cdrPlayInterrupt,
	[PSXINT_SPUIRQ]          = HandleSPU_IRQ,
	[PSXINT_SPU_UPDATE]      = UpdateSPU,
	[PSXINT_RESET_CYCLE_VAL] = psxResetCycleValue
};

EventQueue psxEventQueue;
