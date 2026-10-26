/* Leaf header (docs/plan.md 6.5 rule 2): `sound/se_req.cpp`'s `pl_se_charge_level_req`, which `Pl/pl_act.cpp` calls. */
#ifndef MHTRI_SOUND_PL_SE_CHARGE_LEVEL_REQ_H
#define MHTRI_SOUND_PL_SE_CHARGE_LEVEL_REQ_H

#include "types.h"
#include "nw4r/math.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800DD770 - requests the player's charge-level sound at `pos` through its sound work (+0xAFC) (registry name). */
void pl_se_charge_level_req(struct _PLW* self, VEC3* pos);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_PL_SE_CHARGE_LEVEL_REQ_H */
