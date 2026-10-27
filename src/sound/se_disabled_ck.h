/* Leaf header (docs/plan.md 6.5 rule 2): `sound/fn_800D7F54.cpp`'s SE-disabled predicate (registry name; GUESS). */
#ifndef MHTRI_SOUND_SE_DISABLED_CK_H
#define MHTRI_SOUND_SE_DISABLED_CK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800D843C - 1 while sound effects are disabled (the request layer skips its work). */
u32 se_disabled_ck(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_SE_DISABLED_CK_H */
