/* Leaf header (docs/plan.md 6.5 rule 2): `ef/system_core.cpp`'s `pad_warn_mask_clear`, which `Pl/pl_motion.cpp` calls. */
#ifndef MHTRI_EF_PAD_WARN_MASK_CLEAR_H
#define MHTRI_EF_PAD_WARN_MASK_CLEAR_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800D2D44 (0xC): clears the per-channel pad-disconnect warning mask. GUESS name. */
void pad_warn_mask_clear(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_PAD_WARN_MASK_CLEAR_H */
