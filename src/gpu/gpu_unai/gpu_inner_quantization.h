#ifndef _OP_DITHER_H_
#define _OP_DITHER_H_

static void SetupDitheringConstants()
{
	// Initialize Dithering Constants
	// The screen is divided into 8x8 chunks and sub-unitary noise is applied
	// using the following matrix. This ensures that data lost in color
	// quantization will be added back to the image 'by chance' in predictable
	// patterns that are naturally 'smoothed' by your sight when viewed from a
	// certain distance.
	//
	// http://caca.zoy.org/study/index.html
	//
	// Shading colors are encoded in 4.5, and then are quantitized to 5.0,
	// DitherMatrix constants reflect that.

	static const u8 DitherMatrix[] = {
		 0, 32,  8, 40,  2, 34, 10, 42,
		48, 16, 56, 24, 50, 18, 58, 26,
		12, 44,  4, 36, 14, 46,  6, 38,
		60, 28, 52, 20, 62, 30, 54, 22,
		 3, 35, 11, 43,  1, 33,  9, 41,
		51, 19, 59, 27, 49, 17, 57, 25,
		15, 47,  7, 39, 13, 45,  5, 37,
		63, 31, 55, 23, 61, 29, 53, 21
	};

	int i, j;
	for (i = 0; i < 8; i++)
	{
		for (j = 0; j < 8; j++)
		{
			u16 offset = (i << 3) | j;

			u32 component = ((DitherMatrix[offset] + 1) << 4) / 65; //[5.5] -> [5]

			gpu_unai.DitherMatrix[offset] = (component)
			                              | (component << 10)
			                              | (component << 20);
		}
	}
}

template <const int DITHER>
INLINE void gpuColorQuantization(u32 &uSrc, u16 *&pDst)
{
	if (DITHER)
	{
		u16 fbpos  = (u32)(pDst - gpu_unai.vram);
		u16 offset = ((fbpos & (0x7 << 10)) >> 7) | (fbpos & 0x7);

		//clean overflow flags and add
		uSrc = (uSrc & 0x1FF7FDFF) + gpu_unai.DitherMatrix[offset];

		if (uSrc & (1<< 9)) uSrc |= (0x1FF    );
		if (uSrc & (1<<19)) uSrc |= (0x1FF<<10);
		if (uSrc & (1<<29)) uSrc |= (0x1FF<<20);
	}

	uSrc = ((uSrc>> 4) & (0x1F    ))
		 | ((uSrc>> 9) & (0x1F<<5 ))
		 | ((uSrc>>14) & (0x1F<<10));
}

#endif //_OP_DITHER_H_