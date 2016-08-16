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
 *
 * Class SortedArray is based largely on OpenMSX's SchedulerQueue class, found
 *  here: https://github.com/openMSX/openMSX/blob/master/src/SchedulerQueue.hh
 *  (Which was primarily written by Wouter Vermaelen a.k.a. m9710797)
 */

#ifndef PSXEVENTS_H
#define PSXEVENTS_H

#include <stdio.h>
#include <stdint.h>
#include "r3000a.h"

enum psxEventType {
	PSXINT_SIO = 0,
	PSXINT_CDR,
	PSXINT_CDREAD,
	PSXINT_GPUDMA,
	PSXINT_MDECOUTDMA,
	PSXINT_SPUDMA,
	PSXINT_GPUBUSY,        //From PCSX Rearmed, but not implemented there nor here
	PSXINT_MDECINDMA,
	PSXINT_GPUOTCDMA,
	PSXINT_CDRDMA,
	PSXINT_NEWDRC_CHECK,   //From PCSX Rearmed, but not implemented here (TODO?)
	PSXINT_RCNT,
	PSXINT_CDRLID,
	PSXINT_CDRPLAY,
	PSXINT_SPUIRQ,         //Check for upcoming SPU HW interrupts
	PSXINT_SPU_UPDATE,     //senquack - update and feed SPU (note that this usage
	                       // differs from Rearmed: Rearmed uses this for checking
	                       // for SPU HW interrupts and lacks a flexibly-scheduled
	                       // interval for SPU update-and-feed)
	PSXINT_RESET_CYCLE_VAL,          // Reset psxRegs.cycle value to 0 to ensure
	                                 //  it can never overflow
	PSXINT_COUNT,
	PSXINT_NEXT_EVENT = PSXINT_COUNT //The most imminent event's entry is
	                                 // always copied to this slot in
	                                 // psxRegs.intCycles[] for fast checking
};

// This is similar to a sorted vector<T>, though this container can have spare
//  capacity both at the front and at the end (vector only at the end). This
//  means that when you remove the 'smallest' element and insert a new element
//  that's only slightly 'bigger' than the smallest one, there's a very good
//  chance this insert runs in O(1) (for vector it's O(N), with N the size of
//  the vector). This is a scenario that occurs very often in the scheduler
//  code.
template<typename T, typename T_IDX_TYPE, size_t T_CAPACITY, size_t T_SPARE_FRONT>
class SortedArray {
public:
	SortedArray() { clear(); }

	void clear()
	{
		useBeginIdx = T_SPARE_FRONT;
		useEndIdx = useBeginIdx;
	}

	size_t capacity() const { return T_CAPACITY; }
	size_t spare_front() const { return useBeginIdx; }
	size_t spare_back() const { return T_CAPACITY - useEndIdx; }
	size_t size() const { return useEndIdx - useBeginIdx; }
	bool empty() const { return useEndIdx == useBeginIdx; }

	// Returns reference to the first element in use
	T& front() { return storage[useBeginIdx]; }
	const T& front() const { return storage[useBeginIdx]; }

	// Returns pointer to the first element in use
	T* front_ptr() { return storage + useBeginIdx; }
	const T* front_ptr() const { return storage + useBeginIdx; }

	// Returns pointer to one past the last element in use,
	//  but when empty, points to same element as front_ptr()
	T* end_ptr() { return storage + useEndIdx; }
	const T* end_ptr() const { return storage + useEndIdx; }

	bool contains(const T& t) const
	{
		const T *it = front_ptr();
		while ((it != end_ptr()) && (*it != t))
			++it;
		return it != end_ptr();
	}

	// Insert new element.
	//  Elements are sorted according to the given predicate. Predicate
	//  will return 'true' for sort_pred(ELEMENT_BEFORE, ELEMENT_AFTER).
	//  Important: two elements that are equivalent according to predicate
	//  keep their relative order, i.e., newly-inserted elements are inserted
	//  after existing equivalent elements.
	template<typename T_SORT_PREDICATE>
	void insert(const T& t, T_SORT_PREDICATE sort_pred)
	{
		T* it = front_ptr();

		while ((it != end_ptr()) && !sort_pred(t, *it))
			++it;

		// Determine if insertion point is closer to beginning of used space,
		//  so the minimum amount of data must be rearranged
		if ((it - front_ptr()) <= (end_ptr() - it)) {
			// Closer to beginning, so moving preceding elements backwards is best
			if (useBeginIdx != 0) {
				// Insert at position 'it-1', moving backward all elements between
				//  that position and the front
				insert_front(it, t);
			} else if (useEndIdx != T_CAPACITY) {
				// Insert at position 'it', moving forward all elements between that
				//  position and the end
				insert_back(it, t);
			} else {
#ifdef DEBUG_EVENTS
				printf("ERROR: SortedArray::insert() could not find space in its array (1)\n");
#endif
			}
		} else {
			// Closer to end, so moving succeeding elements forwards is best
			if (useEndIdx != T_CAPACITY) {
				// Insert at position 'it', moving forward all elements between that
				//  position and the end
				insert_back(it, t);
			} else if (useBeginIdx != 0) {
				// Insert at position 'it-1', moving backward all elements between
				//  that position and the front
				insert_front(it, t);
			} else {
#ifdef DEBUG_EVENTS
				printf("ERROR: SortedArray::insert() could not find space in its array (2)\n");
#endif
			}
		}
	}

	// Remove the head of the sorted array
	void remove_front()
	{
		assert(!empty());
		++useBeginIdx;
	}

	// Remove the first matching element, returning false if not found
	bool remove(const T& t)
	{
		T *it = front_ptr();
		while ((it != end_ptr()) && (*it != t))
			++it;

		if (it == end_ptr())
			return false;

		if ((it - front_ptr()) < (end_ptr() - it - 1)) {
			// Element is closer to the beginning of the array
			move_forward(front_ptr(), it - 1);
			++useBeginIdx;
		} else {
			// Element is closer to the end of the array
			--useEndIdx;
			move_backward(it + 1, end_ptr());
		}
		return true;
	}

private:
	// Copy all elements between [start,end] to [start+1,end+1].
	//  If start address is greater than end, no copy will occur.
	void move_forward(T* const start, T* const end)
	{
		for (T* ptr = end; ptr >= start; --ptr)
			*(ptr + 1) = *ptr;
	}

	// Copy all elements between [start,end] to [start-1,end-1].
	//  If start address is greater than end, no copy will occur.
	void move_backward(T* const start, T* const end)
	{
		for (T* ptr = start; ptr <= end; ++ptr)
			*(ptr - 1) = *ptr;
	}

	// Insert new entry 't' before position 'it'. Array contents before the
	//  position will be moved backward before insertion.
	void insert_front(T* it, const T& t)
	{
		--it;
		move_backward(front_ptr(), it);
		--useBeginIdx;
		*it = t;
	}

	// Insert new entry 't' at position 'it'. Array contents between that
	//  position and the end will be moved forwards before the insertion.
	void insert_back(T* it, const T& t)
	{
		move_forward(it, end_ptr() - 1);
		++useEndIdx;
		*it = t;
	}

private:
	T_IDX_TYPE useBeginIdx;    // Index of first used element of storage[] 
	T_IDX_TYPE useEndIdx;      // One past last used index of storage[], or same
	                           //  value as useBeginIdx when queue is empty.
	T storage[T_CAPACITY];
};

class EventQueue {
public:
	// Schedule an event to occur after 'cycles_after' have passed from now.
	//  If it's already enqueued, it will be rescheduled, without
	//  calling its handler function.
	void enqueue(const psxEventType ev, const uint32_t cycles_after);

	// Unschedule an event, without calling its handler function.
	void dequeue(const psxEventType ev);

	// Remove the front of the queue and call its handler function:
	void dispatch_and_remove_front();

	uint8_t front() const { return queue.front(); }
	bool    empty() const { return queue.empty(); }
	size_t  size()  const { return queue.size(); }
	void    clear_internal_queue() { return queue.clear(); }

	bool contains(const psxEventType ev) const
	{
		// If an event is present in the queue, it must also be set in
		//  psxRegs.interrupt, and is faster to just check that:
		return psxRegs.interrupt & ((uint32_t)1 << ev);
	}

	// Certain events should always be scheduled when either starting
	//  the emulator for the first time, restarting a game, or
	//  loading a save-state. By ensuring these events are always loaded
	//  on save-state load, compatibility with savestates from before
	//  event-system overhaul is ensured.
	void schedule_persistent_events();

	// Build the event queue afresh from psxRegs.interrupt and psxRegs.intCycle[]
	void init_from_freeze();

	// At very intermittent intervals, psxRegs.cycle is reset to 0 to prevent
	//  it from ever overflowing (simplifies checks against it elsewhere).
	//  This function fixes up timestamps of all queued events when this occurs.
	void adjust_event_timestamps(const uint32_t prev_cycle_val)
	{
		for (uint8_t *ev = queue.front_ptr(); ev != queue.end_ptr(); ++ev) {
			psxRegs.intCycle[*ev].sCycle -= prev_cycle_val;
		}

		psxRegs.intCycle[PSXINT_NEXT_EVENT].sCycle -= prev_cycle_val;
	}

	// Functor specifying sort criteria to SortedArray member class
	struct EventMoreImminent {
		inline bool operator()(const uint8_t &lh, const uint8_t &rh) const
		{
			// Compare the two event timestamps, interpreting the result as a signed
			//  integer in case one or both cross psxRegs.cycle overflow boundary.
			int lh_tmp = psxRegs.intCycle[lh].sCycle + psxRegs.intCycle[lh].cycle - psxRegs.cycle;
			int rh_tmp = psxRegs.intCycle[rh].sCycle + psxRegs.intCycle[rh].cycle - psxRegs.cycle;
			return lh_tmp < rh_tmp;
		}
	};

#ifdef DEBUG_EVENTS
	void print_event_queue();
#endif

private:
	SortedArray<uint8_t, uint8_t, PSXINT_COUNT, 2> queue;

	// If a PSXINT_* event number is not implemented, call this stub
	//  (should never happen)
	static void event_stub_func();

	// Event handler function table:
	typedef void (EventFunc)();
	static EventFunc * const eventFuncs[PSXINT_COUNT];
};

extern EventQueue psxEventQueue;

/* psxBranchTest() debug statistics */
#ifdef DEBUG_EVENTS
extern unsigned int bt_necessary, bt_unnecessary;
#endif

#endif //PSXEVENTS_H
