/***************************************************************************
*   Copyright (C) 2010 PCSX4ALL Team                                      *
*   Copyright (C) 2010 Franxis                                            *
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

// READ DMA (one value)
unsigned short SPU_readDMA(void)
{
#ifdef DEBUG_ANALYSIS
	dbg_anacnt_SPU_readDMA++;
#endif
	unsigned short s=spuMem[spuAddr>>1];
	spuAddr+=2;
	if(spuAddr>0x7ffff) spuAddr=0;
	return s;
}

// READ DMA (many values)
void SPU_readDMAMem(unsigned short * pusPSXMem,int iSize)
{
#ifdef DEBUG_ANALYSIS
	dbg_anacnt_SPU_readDMAMem++;
#endif
	int i;
	for(i=0;i<iSize;i++)
	{
		*pusPSXMem++=spuMem[spuAddr>>1];                    // spu addr got by writeregister
		spuAddr+=2;                                         // inc spu addr
		if(spuAddr>0x7ffff) spuAddr=0;                      // wrap
	}
}

// WRITE DMA (one value)
void SPU_writeDMA(unsigned short val)
{
#ifdef DEBUG_ANALYSIS
	dbg_anacnt_SPU_readDMA++;
#endif
	spuMem[spuAddr>>1] = val;                             // spu addr got by writeregister
	spuAddr+=2;                                           // inc spu addr
	if(spuAddr>0x7ffff) spuAddr=0;                        // wrap
}

// WRITE DMA (many values)
void SPU_writeDMAMem(unsigned short * pusPSXMem,int iSize)
{
#ifdef DEBUG_ANALYSIS
	dbg_anacnt_SPU_writeDMAMem++;
#endif
	int i;
	for(i=0;i<iSize;i++)
	{
		spuMem[spuAddr>>1] = *pusPSXMem++;                  // spu addr got by writeregister
		spuAddr+=2;                                         // inc spu addr
		if(spuAddr>0x7ffff) spuAddr=0;                      // wrap
	}
}
