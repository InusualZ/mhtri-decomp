/* Leaf header (docs/plan.md 6.5 rule 2): a symbol `sound/snd_bank_loader.cpp` defines, for a consumer that cannot include
 * the owner's full header.  C++ linkage (the map row carries the mangling). */
#ifndef MHTRI_SOUND_DEMO_BGM_LOAD_H
#define MHTRI_SOUND_DEMO_BGM_LOAD_H

#include "types.h"

/* 0x800F1984 - starts loading demo `demo`'s BGM streams; -1 for an id past the table, 0 once the requests are queued. */
s32 demo_bgm_load(u8 demo);

#endif /* MHTRI_SOUND_DEMO_BGM_LOAD_H */
