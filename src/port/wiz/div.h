#ifndef _DIV_ARM

#ifdef __cplusplus
extern "C" {
#endif

extern unsigned udiv_32by32_arm9e(unsigned d, unsigned n);
extern signed sdiv_32by32_arm9e(signed d, signed n);

extern unsigned long long ludiv_32by32_arm9e(unsigned d, unsigned n);
extern signed long long lsdiv_32by32_arm9e(signed d, signed n);

#ifdef __cplusplus
} /* End of extern "C" */
#endif

#define UDIV(n,d) udiv_32by32_arm9e((d),(n))
#define SDIV(n,d) sdiv_32by32_arm9e((d),(n))

#define LUDIV(n,d) ludiv_32by32_arm9e((d),(n))
#define LSDIV(n,d) lsdiv_32by32_arm9e((d),(n))

#endif
