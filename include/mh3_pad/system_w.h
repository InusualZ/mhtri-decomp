/*
 * `system_w` - the one declaration of the object `system_w` (rule 2), defined by `src/mh3_pad.cpp` (its `.bss`
 * 0x806585B8-0x806694E8 is that unit's own).  The type `SystemWork` lives in `mh3_pad/system_work.h` (rule 1); consumers
 * include this header for the object.
 */
#ifndef MHTRI_MH3_PAD_SYSTEM_W_H
#define MHTRI_MH3_PAD_SYSTEM_W_H

#include "mh3_pad/system_work.h"

#ifdef __cplusplus
extern "C" {
#endif

extern SystemWork system_w;

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MH3_PAD_SYSTEM_W_H */
