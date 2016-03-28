
// READ DMA (one value)
unsigned short CALLBACK SPU_readDMA(void)
{
 unsigned short s=spuMem[spuAddr>>1];
 spuAddr+=2;
 if(spuAddr>0x7ffff) spuAddr=0;

 iSpuAsyncWait=0;

 return s;
}

// READ DMA (many values)
void CALLBACK SPU_readDMAMem(unsigned short * pusPSXMem,int iSize)
{
 int i;

 for(i=0;i<iSize;i++)
  {
   *pusPSXMem++=spuMem[spuAddr>>1];                    // spu addr got by writeregister
   spuAddr+=2;                                         // inc spu addr
   if(spuAddr>0x7ffff) spuAddr=0;                      // wrap
  }

 iSpuAsyncWait=0;
}

// WRITE DMA (one value)
void CALLBACK SPU_writeDMA(unsigned short val)
{
 spuMem[spuAddr>>1] = val;                             // spu addr got by writeregister

 spuAddr+=2;                                           // inc spu addr
 if(spuAddr>0x7ffff) spuAddr=0;                        // wrap

 iSpuAsyncWait=0;
}

// WRITE DMA (many values)
void CALLBACK SPU_writeDMAMem(unsigned short * pusPSXMem,int iSize)
{
 int i;

 for(i=0;i<iSize;i++)
  {
   spuMem[spuAddr>>1] = *pusPSXMem++;                  // spu addr got by writeregister
   spuAddr+=2;                                         // inc spu addr
   if(spuAddr>0x7ffff) spuAddr=0;                      // wrap
  }
 
 iSpuAsyncWait=0;
}
