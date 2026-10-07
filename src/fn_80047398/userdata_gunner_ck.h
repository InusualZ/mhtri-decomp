/*
 * Leaf header (docs/plan.md 6.5 rule 2): `userdata_gunner_ck` (0x8004AEC0), defined by
 * `src/fn_80047398.cpp`'s range.  `fn_80047398.h` includes this one, so there is one declaration; it is
 * separate so a lobby unit can call it without the rest of that header, whose `item_pair_copy` and
 * `fn_8004A*` views clash with the lobby band's.
 */
#ifndef MHTRI_FN_80047398_USERDATA_GUNNER_CK_H
#define MHTRI_FN_80047398_USERDATA_GUNNER_CK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8004AEC0 - whether the save block's two equipment records make a gunner set (`Get_pl_type` 4..6).  The
 * callers hold the block under their own view (`get_userdata()`, `lobby_world_block`), so the parameter is
 * the block by address.  GUESS name. */
/* untyped: the save block, seen by each caller through its own record view */
u32 userdata_gunner_ck(void* userdata);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_FN_80047398_USERDATA_GUNNER_CK_H */
