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

static RChunk* g_code_chunk = NULL;
static RHeap* g_code_heap = NULL;

#define RECMEM_SIZE	(12 * 1024 * 1024)
#define REC_MAX_OPCODES	80
#define MEM_TRANSLATION_CACHE_SIZE (RECMEM_SIZE + (REC_MAX_OPCODES*2) + 0x4000)

char *recMem;/* the recompiled blocks will be here */

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
        if( recMem == NULL)
            qDebug()<<"error alocating rec memory";
        }
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

int modify_function_pointers_symbian( int offset ){
    qDebug()<< "offset = " << offset;
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
