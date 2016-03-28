#pragma once

#define T0 ARMREG_R0
#define T1 ARMREG_R1
#define T2 ARMREG_R2

#define gen(insn, ...) arm_gen_##insn(__VA_ARGS__)

INLINE u32 gen(dissect_imm16, u32 imm, u32 &stores, u32 &shifts)
{
  u32 store_count = 0, sa = 0;

  stores = 0;
  shifts = 0;

  if (imm == 0)
    return 0;

	while (imm)
	{
		if (!(imm&3)) { imm >>= 2; sa += 2; continue; }

		stores = (stores<<8)|(imm&255);
		shifts = (shifts<<8)|((32-sa)&31);
    store_count++;

		imm >>= 8; sa += 8;
	}

  return store_count;
}

INLINE s32 gen(dissect_imm16_ex, u32 imm1, u32 imm2, u32 &stores, u32 &shifts)
{
  s32 n1, n2;

  n1 = arm_gen_dissect_imm16(imm1, stores, shifts);

  if (n1 > 1)
  {
    u32 stores2, shifts2;
    n2 = gen(dissect_imm16, imm2, stores2, shifts2);
    if (n1 > n2)
    {
      stores = stores2;
      shifts = shifts2;
			return n2 ? -n2 : -1;
    }
  }
  return n1;
}

INLINE u32 gen(dissect_imm32, u32 imm, u32 &stores, u32 &shifts)
{
  u32 store_count = 0;

  u32 value;

  stores = 0;
  shifts = 0;

  if (imm == 0)
    return 0;

  // Find chunks of non-zero data at 2 bit alignments.
  for (u32 left_shift = 0; left_shift < 32; left_shift += 2)
  {
    value = imm >> left_shift;

    if (!(value & 0x03))
      continue;

    // Hit the end, it might wrap back around to the beginning.
    if ((left_shift > 24) && (store_count > 1))
    {
      // Make a mask for the residual bits. IE, if we have
      // 5 bits of data at the end we can wrap around to 3
      // bits of data in the beginning. Thus the first
      // thing, after being shifted left, has to be less
      // than 111b, 0x7, or (1 << 3) - 1.
      u32 top_bits = 32 - left_shift;
      u32 residual_bits = 8 - top_bits;
      u32 residual_mask = (1 << residual_bits) - 1;
      u32 store = stores&255;
      u32 shift = shifts&31;
      if ((store << (32 - shift)) < residual_mask)
      {
        // Then we can throw out the last bit and tack it on
        // to the first bit.
        stores = (stores&-256) | ((stores << ((top_bits + (32 - shifts))&31)) | (value&255));
        shifts = (shifts&-256) | top_bits;

        return store_count;
      }
    }

    stores |= (imm&255)<<(8*store_count);
    shifts |= ((32 - left_shift)&31)<<(8*store_count);

    store_count++;

    left_shift += 8;
  }

  return store_count;
}

INLINE u32 gen(dissect_imm32_ex, u32 imm1, u32 imm2, u32 &stores, u32 &shifts)
{
  s32 n1, n2;

  n1 = gen(dissect_imm32, imm1, stores, shifts);

  if (n1 > 2)
  {
    u32 stores2, shifts2;
    n2 = gen(dissect_imm32, imm2, stores2, shifts2);
    if (n1 > n2)
    {
      stores = stores2;
      shifts = shifts2;
			return n2 ? -n2 : -1;
    }
  }
  return n1;
}
