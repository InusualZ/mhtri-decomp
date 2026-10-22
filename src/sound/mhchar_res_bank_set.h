/* Leaf header (docs/plan.md 6.5 rule 2): `sound/mhchar.cpp`'s `mhchar_res_bank_set`, the one declaration `ai/ai_npc.cpp` needs.
 * GUESS name, from the body (listed on the NAMES line of ai/ai_npc.cpp). */
#ifndef MHTRI_SOUND_MHCHAR_RES_BANK_SET_H
#define MHTRI_SOUND_MHCHAR_RES_BANK_SET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800E3358 (0x250): assigns a resource file to the bank of the kind (0..5) at the index */
void mhchar_res_bank_set(s32 kind, u8 index, u32* file);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_MHCHAR_RES_BANK_SET_H */
