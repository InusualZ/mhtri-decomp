/* Leaf header for `set_zmode` (0x800E4688), defined by `sound/fn_800E3CBC.cpp` whose full header is not
 * includable beside its consumers: the depth-test / compare-function / z-write setter.  The map spells it
 * `set_zmode__FbUcb`, so it is a C++ free function (rule 9). */
#ifndef MHTRI_SOUND_SET_ZMODE_H
#define MHTRI_SOUND_SET_ZMODE_H

#include "types.h"

#ifdef __cplusplus
void set_zmode(bool depth_test, u8 func, bool write);
/* 0x800E4574 - the blend mode setter (`set_blendmode__FUcUcUc`), the same owner's. */
void set_blendmode(u8 src, u8 dst, u8 op);
#endif

#endif /* MHTRI_SOUND_SET_ZMODE_H */
