/* Leaf header (docs/plan.md 6.5 rule 2): `ef/eft_res.cpp`'s `eft_instances_release`, which `Pl/pl_motion.cpp` calls. */
#ifndef MHTRI_EF_EFT_INSTANCES_RELEASE_H
#define MHTRI_EF_EFT_INSTANCES_RELEASE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800F8DA4 (0xB8): releases every live effect instance and the effect memory. GUESS name. */
void eft_instances_release(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EFT_INSTANCES_RELEASE_H */
