/* Leaf header (docs/plan.md 6.5 rule 2): `ef/eft_res.cpp`'s `eft_res_slots_release`, which `Pl/pl_motion.cpp` calls. */
#ifndef MHTRI_EF_EFT_RES_SLOTS_RELEASE_H
#define MHTRI_EF_EFT_RES_SLOTS_RELEASE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800F6A24 (0x54): releases the 256 effect resource slots, last first. GUESS name. */
void eft_res_slots_release(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EFT_RES_SLOTS_RELEASE_H */
