/***************************************************************************
*   Copyright (C) 2010 PCSX4ALL Team                                      *
*   Copyright (C) 2010 Chui                                               *
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

#include "port.h"

#ifndef PROFILER_PCSX4ALL_H
#define PROFILER_PCSX4ALL_H

#define PCSX4ALL_PROF_CPU 0
#define PCSX4ALL_PROF_HW_READ 1
#define PCSX4ALL_PROF_HW_WRITE 2
#define PCSX4ALL_PROF_COUNTERS 3
#define PCSX4ALL_PROF_HLE 4
#define PCSX4ALL_PROF_GTE 5
#define PCSX4ALL_PROF_GPU 6
#define PCSX4ALL_PROF_REC 7
#define PCSX4ALL_PROF_TEST 8

#ifndef PROFILER_PCSX4ALL

#define pcsx4all_prof_init()
#define pcsx4all_prof_reset()
#define pcsx4all_prof_start(A)
#define pcsx4all_prof_end(A)
#define pcsx4all_prof_pause(A)
#define pcsx4all_prof_resume(A)
#define pcsx4all_prof_add(MSG) 0
#define pcsx4all_prof_show()
#define pcsx4all_prof_start(A)
#define pcsx4all_prof_end(A)
#define pcsx4all_prof_resume(A)
#define pcsx4all_prof_pause(A)
#define pcsx4all_prof_start_with_pause(A,B)
#define pcsx4all_prof_end_with_resume(A,B)
#define pcsx4all_prof_pause_with_resume(A,B)

#else

extern unsigned pcsx4all_prof_frames;

#define PCSX4ALL_PROFILER_MAX 256

#ifndef PCSX4ALL_PROFILER_ONE
#define pcsx4all_prof_start(A) int __yet_paused__=_pcsx4all_prof_start((A))
#else
#define pcsx4all_prof_start(A) \
int __yet_paused__=2; \
if ((A)==PCSX4ALL_PROFILER_ONE){ \
__yet_paused__=_pcsx4all_prof_start((A)); \
}
#endif
int _pcsx4all_prof_start(unsigned a);

#ifndef PCSX4ALL_PROFILER_ONE
#define pcsx4all_prof_end(A) _pcsx4all_prof_end((A),__yet_paused__)
#else
#define pcsx4all_prof_end(A) \
if ((A)==PCSX4ALL_PROFILER_ONE){ \
_pcsx4all_prof_end((A),__yet_paused__); \
}
#endif
void _pcsx4all_prof_end(unsigned a, int yet);

#ifndef PCSX4ALL_PROFILER_ONE
#define pcsx4all_prof_resume(A) _pcsx4all_prof_resume((A),__yet_paused_)
#else
#define pcsx4all_prof_resume(A) \
if ((A)==PCSX4ALL_PROFILER_ONE){ \
_pcsx4all_prof_resume((A),__yet_paused_); \
}
#endif
void _pcsx4all_prof_resume(unsigned a, int yet);

#ifndef PCSX4ALL_PROFILER_ONE
#define pcsx4all_prof_pause(A) int __yet_paused_=_pcsx4all_prof_pause((A))
#else
#define pcsx4all_prof_pause(A) \
int __yet_paused_=1; \
if ((A)==PCSX4ALL_PROFILER_ONE){ \
__yet_paused_=_pcsx4all_prof_pause((A)); \
}
#endif
int _pcsx4all_prof_pause(unsigned a);

#ifndef PCSX4ALL_PROFILER_ONE
#define pcsx4all_prof_start_with_pause(A,B) int __yet_paused=_pcsx4all_prof_start_with_pause((A),(B))
#else
#define pcsx4all_prof_start_with_pause(A,B) \
int __yet_paused__=2; \
if ((A)==PCSX4ALL_PROFILER_ONE){ \
__yet_paused__=_pcsx4all_prof_start((A)); \
}
#endif
int _pcsx4all_prof_start_with_pause(unsigned a, unsigned b);

#ifndef PCSX4ALL_PROFILER_ONE
#define pcsx4all_prof_end_with_resume(A,B) _pcsx4all_prof_end_with_resume((A),(B),__yet_paused)
#else
#define pcsx4all_prof_end_with_resume(A,B) \
if ((A)==PCSX4ALL_PROFILER_ONE){ \
_pcsx4all_prof_end((A),__yet_paused__); \
}
#endif
void _pcsx4all_prof_end_with_resume(unsigned a, unsigned b, int yet);

#ifndef PCSX4ALL_PROFILER_ONE
#define pcsx4all_prof_pause_with_resume(A,B) _pcsx4all_prof_pause_with_resume(A,B)
#else
#define pcsx4all_prof_pause_with_resume(A,B) \
if ((A)==PCSX4ALL_PROFILER_ONE){ \
_pcsx4all_prof_pause_with_resume(A,B); \
}
#endif
void _pcsx4all_prof_pause_with_resume(unsigned a, unsigned b);

void pcsx4all_prof_init(void);
void pcsx4all_prof_reset(void);
int pcsx4all_prof_add(const char *msg);
void pcsx4all_prof_show(void);

#endif



#endif

