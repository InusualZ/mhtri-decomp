/* Leaf header (docs/plan.md 6.5 rule 2): `fn_80056F24.cpp`'s `fade_done_ck`, which `Pl/pl_motion.cpp` calls. */
#ifndef MHTRI_FADE_DONE_CK_H
#define MHTRI_FADE_DONE_CK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8005811C (0x24): 1 when fade slot `slot` has finished (its +0x10 word is 0). GUESS name. */
s32 fade_done_ck(u8 slot);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_FADE_DONE_CK_H */
