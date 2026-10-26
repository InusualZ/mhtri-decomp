/* lobby/lb_title_owned_ck.h - leaf header (docs/plan.md 6.5 rule 2) for `lobby/fn_80219260.cpp`'s title helpers; the
 *   owner's record view (`LbEquipWork`, a view of the save block) stays in its source, so it is named by tag only. */
#ifndef MHTRI_LOBBY_LB_TITLE_OWNED_CK_H
#define MHTRI_LOBBY_LB_TITLE_OWNED_CK_H

#include "types.h"

struct LbEquipWork;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8021B624 (0x38): whether title `index` (0..117) is set in the save's owned-title bit mask. */
u32 lb_title_owned_ck(struct LbEquipWork* self, s32 index);
/* 0x8021B5AC (0x78): the range-table entry whose interval holds title `value`. */
s16 lb_title_range_find(s32 value);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_TITLE_OWNED_CK_H */
