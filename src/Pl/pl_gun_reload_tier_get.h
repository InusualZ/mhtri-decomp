/* Leaf header (docs/plan.md 6.5 rule 2): `Pl/pl_act.cpp`'s `pl_gun_reload_tier_get`, which `Pl/fn_802840DC.cpp` calls. */
#ifndef MHTRI_PL_PL_GUN_RELOAD_TIER_GET_H
#define MHTRI_PL_PL_GUN_RELOAD_TIER_GET_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802791FC - the reload speed tier (0-2) of an ammo kind after the reload skills (registry name); the callers store and extend the result as a byte (`stb r3` direct, `clrlwi` before a compare), so the call view returns `u8`. */
u8 pl_gun_reload_tier_get(struct _PLW* self, u8 ammo);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_GUN_RELOAD_TIER_GET_H */
