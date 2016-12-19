/***************************************************************************
 * (C) notaz, 2010-2011                                                    *
 * (C) senquack, PCSX4ALL team 2016                                        *
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

#ifndef PLUGIN_LIB_H
#define PLUGIN_LIB_H

extern int pl_fskip_advice;

void pl_frameskip_prepare(void);
void pl_frame_limit(void);
void pl_init(void);
void pl_pause(void);
void pl_resume(void);

static inline int pl_frameskip_advice(void)
{
	return pl_fskip_advice;
}

#endif // PLUGIN_LIB_H
