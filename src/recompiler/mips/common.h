#ifndef __COMMON_H__
#define __COMMON_H__

#include "../../psxcommon.h"
#include "../../psxhle.h"
#include "../../psxmem.h"
#include "../../r3000a.h"
#include "gte.h"

// From old psxmem.h
#define psxMemReadS8 psxMemRead8
#define psxMemReadS16 psxMemRead16

#define BIAS_CYCLE_INC BIAS

#undef INLINE
#define INLINE inline

#define DEBUGF(aa) 

#endif /* __COMMON_H__ */
