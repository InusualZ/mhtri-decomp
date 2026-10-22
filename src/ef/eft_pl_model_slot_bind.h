/* Leaf header (docs/plan.md 6.5 rule 2): `ef/eft_model_slot.cpp`'s `eft_pl_model_slot_bind`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_EF_EFT_PL_MODEL_SLOT_BIND_H
#define MHTRI_EF_EFT_PL_MODEL_SLOT_BIND_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800FACAC - takes effect model slot 0 and binds it to player `self` (+0x30), clearing its +3 byte */
void eft_pl_model_slot_bind(struct _PLW* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EFT_PL_MODEL_SLOT_BIND_H */
