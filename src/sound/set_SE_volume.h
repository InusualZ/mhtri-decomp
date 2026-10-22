/* Leaf header (docs/plan.md 6.5 rule 2): a symbol `sound/snd_bank_loader.cpp` defines, for a consumer that cannot include
 * the owner's full header.  C++ linkage (the map row carries the mangling). */
#ifndef MHTRI_SOUND_SET_SE_VOLUME_H
#define MHTRI_SOUND_SET_SE_VOLUME_H

#include "types.h"

/* 0x800F29BC - applies option SE volume level `level`. */
void set_SE_volume(u8 level);

#endif /* MHTRI_SOUND_SET_SE_VOLUME_H */
