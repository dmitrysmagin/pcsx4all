/* gpSP4Symbian
 *
 * Copyright (C) 2009 Summeli <summeli@summeli.fi>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */
#ifdef __SYMBIAN32__

#include <e32std.h>
#include <QDebug>
#include "symbian_memory_handler.h"
#include "psxcommon.h"
#include "symbols.h"


static RChunk* g_code_chunk = NULL;
static RHeap* g_code_heap = NULL;

#define RECMEM_SIZE	(12 * 1024 * 1024)
#define REC_MAX_OPCODES	80
#define MEM_TRANSLATION_CACHE_SIZE (RECMEM_SIZE + (REC_MAX_OPCODES*2) + 0x4000)

char *recMem;/* the recompiled blocks will be here */
//char *recRAM;[0x200000] __attribute__ ((__aligned__ (32)));	/* and the ptr to the blocks here */
//char *recROM;[0x080000] __attribute__ ((__aligned__ (32)));	/* and here */
//u32 *psxRecLUT;[0x010000];

#define KDistanceFromCodeSection 0x500000


//in Symbian we have to tell to the OS that these memoryblocks contains codes,
//so we can invalidate and execute the codeblock in the future.
typedef unsigned char byte; 

TInt user_counter =0;

TInt CreateChunkAt(TUint32 addr,TInt minsize, TInt maxsize )
{
  TInt err = g_code_chunk->CreateLocalCode( minsize, maxsize );

  if( err )
	  return err;
  if ((TUint32)g_code_chunk->Base() != addr)
      {
      TUint offset = (TInt)g_code_chunk->Base();
      offset = addr-offset;
      g_code_chunk->Close();
      RChunk temp;
      if( offset > 0x7FFFFFFF )
    	  {
    	  //shit, offset too big :(
    	  return KErrNoMemory;
    	  }
      TInt chunkoffset = (TInt) offset;

      err = temp.CreateLocal(0,chunkoffset);
      if( err )
    	  {
    	  temp.Close();
    	  return err;
    	  }
      err = g_code_chunk->CreateLocalCode(minsize,maxsize);        
      temp.Close();
      }
  return err;
}

int create_all_translation_caches()
	{

    TInt minsize = MEM_TRANSLATION_CACHE_SIZE;
    TInt maxsize = MEM_TRANSLATION_CACHE_SIZE + 4096;
	
	RProcess process;
	TModuleMemoryInfo  info;
	
	TInt error = process.GetMemoryInfo( info );
	if( error )
		return error;
	
    TUint32 programAddr = 0x10000000;//(TUint32) info.iCodeBase;
	programAddr += info.iCodeSize;
	
    TUint32 destAddr = programAddr - KDistanceFromCodeSection;
	
	g_code_chunk = new RChunk();

	TInt err = CreateChunkAt(destAddr,minsize, maxsize );
    if(err)qDebug() <<"alloc error" << err;

    TUint32 dynamiaddr = (TUint32) g_code_chunk->Base();

	g_code_heap = UserHeap::ChunkHeap(*g_code_chunk, minsize, 1, maxsize );
    if( g_code_heap != NULL )
	    {
        recMem = (char*) g_code_heap->Alloc( MEM_TRANSLATION_CACHE_SIZE );
        //recRAM = (char*) g_code_heap->Alloc( 0x200000 );
        //recROM = (char*) g_code_heap->Alloc( 0x080000 );
        //psxRecLUT = (u32*) g_code_heap->Alloc( 0x040000 );
        if( recMem == NULL)
            qDebug()<<"error alocating rec memory";
	    }
    qDebug()<< "recmen = " << (int)&recMem << ">" << (int)(&recMem + MEM_TRANSLATION_CACHE_SIZE);
    return 0;
}


void CLEAR_INSN_CACHE(u32 *start,u32 *end)
{	
    TAny* a = reinterpret_cast<TAny*>( start );
    TAny* b = reinterpret_cast<TAny*>( end );
    User::IMB_Range( a, b );
}

void SymbianPackHeap()
{
    User::CompressAllHeaps();
    User::Heap().Compress();
/*
    RProcess myProcess;
    myProcess.SetPriority(EPriorityForeground);
    myProcess.Close();*/
}

void close_all_caches()
{
    g_code_heap->Free( recMem );
    //g_code_heap->Free( recRAM );
    //g_code_heap->Free( recROM );

    g_code_heap->Close();

    g_code_chunk->Close();
}


void (*_gteMVMVA_cv0_mx0_s12_)(psxRegisters *regs, u32 vxy, s32 vz);
void (*_gteMVMVA_cv0_mx0_s0_)(psxRegisters *regs, u32 vxy, s32 vz);
void (*_gteMVMVA_cv0_mx1_s12_)(psxRegisters *regs, u32 vxy, s32 vz);
void (*_gteMVMVA_cv0_mx1_s0_)(psxRegisters *regs, u32 vxy, s32 vz);
void (*_gteMVMVA_cv0_mx2_s12_)(psxRegisters *regs, u32 vxy, s32 vz);
void (*_gteMVMVA_cv0_mx2_s0_)(psxRegisters *regs, u32 vxy, s32 vz);
void (*_gteMVMVA_cv0_mx3_s12_)(psxRegisters *regs);
void (*_gteMVMVA_cv0_mx3_s0_)(psxRegisters *regs);
void (*_gteMVMVA_cv1_mx0_s12_)(psxRegisters *regs, u32 vxy, s32 vz);
void (*_gteMVMVA_cv1_mx0_s0_)(psxRegisters *regs, u32 vxy, s32 vz);
void (*_gteMVMVA_cv1_mx1_s12_)(psxRegisters *regs, u32 vxy, s32 vz);
void (*_gteMVMVA_cv1_mx1_s0_)(psxRegisters *regs, u32 vxy, s32 vz);
void (*_gteMVMVA_cv1_mx2_s12_)(psxRegisters *regs, u32 vxy, s32 vz);
void (*_gteMVMVA_cv1_mx2_s0_)(psxRegisters *regs, u32 vxy, s32 vz);
void (*_gteMVMVA_cv1_mx3_s12_)(psxRegisters *regs);
void (*_gteMVMVA_cv1_mx3_s0_)(psxRegisters *regs);
void (*_gteMVMVA_cv2_mx0_s12_)(psxRegisters *regs, u32 vxy, s32 vz);
void (*_gteMVMVA_cv2_mx0_s0_)(psxRegisters *regs, u32 vxy, s32 vz);
void (*_gteMVMVA_cv2_mx1_s12_)(psxRegisters *regs, u32 vxy, s32 vz);
void (*_gteMVMVA_cv2_mx1_s0_)(psxRegisters *regs, u32 vxy, s32 vz);
void (*_gteMVMVA_cv2_mx2_s12_)(psxRegisters *regs, u32 vxy, s32 vz);
void (*_gteMVMVA_cv2_mx2_s0_)(psxRegisters *regs, u32 vxy, s32 vz);
void (*_gteMVMVA_cv2_mx3_s12_)(psxRegisters *regs);
void (*_gteMVMVA_cv2_mx3_s0_)(psxRegisters *regs);
void (*_gteMVMVA_cv3_mx0_s12_)(psxRegisters *regs, u32 vxy, s32 vz);
void (*_gteMVMVA_cv3_mx0_s0_)(psxRegisters *regs, u32 vxy, s32 vz);
void (*_gteMVMVA_cv3_mx1_s12_)(psxRegisters *regs, u32 vxy, s32 vz);
void (*_gteMVMVA_cv3_mx1_s0_)(psxRegisters *regs, u32 vxy, s32 vz);
void (*_gteMVMVA_cv3_mx2_s12_)(psxRegisters *regs, u32 vxy, s32 vz);
void (*_gteMVMVA_cv3_mx2_s0_)(psxRegisters *regs, u32 vxy, s32 vz);
void (*_gteMVMVA_cv3_mx3_s12_)(psxRegisters *regs);
void (*_gteMVMVA_cv3_mx3_s0_)(psxRegisters *regs);
void (*recRun)(unsigned int ptr, unsigned int regs);
void (*_gteOP_s12_)(void);
void (*_gteOP_s0_)(void);
void (*_gteDPCS_s12_)(void);
void (*_gteDPCS_s0_)(void);
void (*_gteGPF_s12_)(void);
void (*_gteGPF_s0_)(void);
void (*_gteSQR_s12_)(void);
void (*_gteSQR_s0_)(void);
void (*_gteINTPL_s0_)(void);
void (*_gteINTPL_s12_)(void);
void (*_gteINTPL_block_)(void);
void (*_gteDCPL__)(void);
void (*_gteRTPS__)(void);
void (*_gteNCDS__)(void);
void (*_gteNCDT__)(void);
void (*_gteCDP__)(void);
void (*_gteNCCS__)(void);
void (*_gteCC__)(void);
void (*_gteNCS__)(void);
void (*_gteNCT__)(void);
void (*_gteDPCT__)(void);
void (*_gteNCCT__)(void);
void (*_gteRTPT__)(void);
void (*_gteNCLIP_)(void);
void (*_gteAVSZ3__)(void);
void (*_gteAVSZ4__)(void);
void (*_gteGPL_s12_)(void);
void (*_gteGPL_s0_)(void);
int modify_function_pointers_symbian( int offset )
    {
    qDebug()<< "offset = " << offset << "adress1 = " << (int)&recRunSYM;

    //generate the symbs into the memory
    recRun = &recRunSYM + offset;
    qDebug()<< "offset = " << (int)&recRun;
    _gteINTPL_s0_ = &_gteINTPL_s0_SYM + offset;
    _gteMVMVA_cv2_mx0_s12_ = &_gteMVMVA_cv2_mx0_s12_SYM + offset;
    _gteMVMVA_cv1_mx3_s0_=&_gteMVMVA_cv1_mx3_s0_SYM + offset;
    _gteMVMVA_cv1_mx3_s12_= &_gteMVMVA_cv1_mx3_s12_SYM  + offset;
    _gteMVMVA_cv1_mx2_s0_ = &_gteMVMVA_cv1_mx2_s0_SYM + offset;
    _gteMVMVA_cv1_mx2_s12_= &_gteMVMVA_cv1_mx2_s12_SYM + offset;
    _gteMVMVA_cv1_mx1_s0_= &_gteMVMVA_cv1_mx1_s0_SYM + offset;
    _gteMVMVA_cv1_mx1_s12_= &_gteMVMVA_cv1_mx1_s12_SYM + offset;
    _gteMVMVA_cv1_mx0_s0_= &_gteMVMVA_cv1_mx0_s0_SYM + offset;
    _gteMVMVA_cv1_mx0_s12_= &_gteMVMVA_cv1_mx0_s12_SYM + offset;
    _gteMVMVA_cv0_mx3_s0_= &_gteMVMVA_cv0_mx3_s0_SYM + offset;
    _gteMVMVA_cv0_mx3_s12_= &_gteMVMVA_cv0_mx3_s12_SYM + offset;
    _gteMVMVA_cv0_mx2_s0_=  &_gteMVMVA_cv0_mx2_s0_SYM + offset;
    _gteMVMVA_cv0_mx2_s12_= &_gteMVMVA_cv0_mx2_s12_SYM + offset;
    _gteMVMVA_cv0_mx1_s0_ =&_gteMVMVA_cv0_mx1_s0_SYM + offset;
    _gteMVMVA_cv0_mx1_s12_= &_gteMVMVA_cv0_mx1_s12_SYM + offset;
    _gteMVMVA_cv0_mx0_s0_=  &_gteMVMVA_cv0_mx0_s0_SYM + offset;
    _gteMVMVA_cv0_mx0_s12_ = &_gteMVMVA_cv0_mx0_s12_SYM + offset;
    _gteNCLIP_ = &_gteNCLIP_SYM + offset;
    _gteINTPL_block_ = &_gteINTPL_block_SYM + offset;
    _gteINTPL_s12_ = &_gteINTPL_s12_SYM + offset;
    _gteMVMVA_cv3_mx2_s0_ = &_gteMVMVA_cv3_mx2_s0_SYM + offset;
    _gteMVMVA_cv3_mx2_s12_ =  &_gteMVMVA_cv3_mx2_s12_SYM + offset;
    _gteMVMVA_cv3_mx1_s0_ = &_gteMVMVA_cv3_mx1_s0_SYM + offset;
    _gteMVMVA_cv3_mx1_s12_ =  &_gteMVMVA_cv3_mx1_s12_SYM + offset;
    _gteMVMVA_cv3_mx0_s0_ =  &_gteMVMVA_cv3_mx0_s0_SYM + offset;
    _gteMVMVA_cv3_mx0_s12_ =  &_gteMVMVA_cv3_mx0_s12_SYM + offset;
    _gteMVMVA_cv2_mx3_s0_ =  &_gteMVMVA_cv2_mx3_s0_SYM + offset;
    _gteMVMVA_cv2_mx3_s12_ =  &_gteMVMVA_cv2_mx3_s12_SYM + offset;
    _gteMVMVA_cv2_mx2_s0_ =  &_gteMVMVA_cv2_mx2_s0_SYM + offset;
    _gteMVMVA_cv2_mx2_s12_ =  &_gteMVMVA_cv2_mx2_s12_SYM + offset;
    _gteMVMVA_cv2_mx1_s0_ =  &_gteMVMVA_cv2_mx1_s0_SYM + offset;
    _gteMVMVA_cv2_mx1_s12_ =  &_gteMVMVA_cv2_mx1_s12_SYM + offset;
    _gteMVMVA_cv2_mx0_s0_ =  &_gteMVMVA_cv2_mx0_s0_SYM + offset;
    _gteGPL_s0_ =  &_gteGPL_s0_SYM + offset;
    _gteGPL_s12_ =   &_gteGPL_s12_SYM + offset;
    _gteDCPL__ =  &_gteDCPL__SYM + offset;
    _gteSQR_s0_ =  &_gteSQR_s0_SYM + offset;
    _gteSQR_s12_ = &_gteSQR_s12_SYM + offset;
    _gteGPF_s0_ = &_gteGPF_s0_SYM + offset;
    _gteGPF_s12_ = &_gteGPF_s12_SYM + offset;
    _gteOP_s0_ = &_gteOP_s0_SYM + offset;
    _gteDPCS_s0_ = &_gteDPCS_s0_SYM + offset;
    _gteMVMVA_cv3_mx3_s0_ = &_gteMVMVA_cv3_mx3_s0_SYM + offset;
    _gteMVMVA_cv3_mx3_s12_ = &_gteMVMVA_cv3_mx3_s12_SYM + offset;
    _gteAVSZ4__ = &_gteAVSZ4__SYM + offset;
    _gteAVSZ3__ = &_gteAVSZ3__SYM + offset;
    _gteRTPT__ = &_gteRTPT__SYM + offset;
    _gteNCCT__ = &_gteNCCT__SYM + offset;
    _gteDPCT__ =   &_gteDPCT__SYM + offset;
    _gteNCT__ = &_gteNCT__SYM + offset;
    _gteNCS__ = &_gteNCS__SYM + offset;
    _gteCC__ = &_gteCC__SYM + offset;
    _gteNCCS__ = &_gteNCCS__SYM + offset;
    _gteCDP__ = &_gteCDP__SYM + offset;
    _gteNCDT__ =  &_gteNCDT__SYM + offset;
    _gteNCDS__ = &_gteNCDS__SYM + offset;
    _gteRTPS__ = &_gteRTPS__SYM + offset;
}

void keepBacklightOn()
{
    User::ResetInactivityTime();
}

void symb_usleep(int aValue)
{
    User::AfterHighRes(aValue);
}


#endif
