/***************************************************************************
 *   Copyright (C) 2007 Ryan Schultz, PCSX-df Team, PCSX team              *
 *   schultz.ryan@gmail.com, http://rschultz.ath.cx/code.php               *
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
 *   51 Franklin Steet, Fifth Floor, Boston, MA 02111-1307 USA.            *
 ***************************************************************************/

#ifndef __MISC_H__
#define __MISC_H__

#include "psxcommon.h"
#include "plugins.h"
#include "r3000a.h"
#include "psxmem.h"

#undef s_addr

typedef struct {
	unsigned char id[8];
    u32 text;                   
    u32 data;                    
    u32 pc0;
    u32 gp0;                     
    u32 t_addr;
    u32 t_size;
    u32 d_addr;                  
    u32 d_size;                  
    u32 b_addr;                  
    u32 b_size;                  
    u32 s_addr;
    u32 s_size;
    u32 SavedSP;
    u32 SavedFP;
    u32 SavedGP;
    u32 SavedRA;
    u32 SavedS0;
} EXE_HEADER;

struct external_filehdr {
	unsigned short f_magic;		/* magic number			*/
	unsigned short f_nscns;		/* number of sections		*/
	unsigned long f_timdat;	/* time & date stamp		*/
	unsigned long f_symptr;	/* file pointer to symtab	*/
	unsigned long f_nsyms;		/* number of symtab entries	*/
	unsigned short f_opthdr;	/* sizeof(optional hdr)		*/
	unsigned short f_flags;		/* flags			*/
};

#define	FILHDR	struct external_filehdr

extern char CdromId[10];
extern char CdromLabel[33];

extern int LoadCdrom(void);
extern int LoadCdromFile(const char *filename, EXE_HEADER *head);
extern int CheckCdrom(void);
extern int Load(const char *ExePath);

extern int SaveState(const char *file);
extern int LoadState(const char *file);
extern int CheckState(const char *file);
extern int toSaveState;
extern int toLoadState;
extern int toExit;
extern char *SaveState_filename;

#endif /* __MISC_H__ */
