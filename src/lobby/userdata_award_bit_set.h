/*
 * lobby/userdata_award_bit_set.h - leaf header (docs/plan.md 6.5 rule 2) for `lobby/fn_80219260.cpp`'s
 *   `userdata_award_bit_set` (0x8021ACCC): the owner's full record view (`LbEquipWork`, a view of the save block)
 *   stays in its source, so the declaration names the record by tag only.
 */
#ifndef MHTRI_LOBBY_USERDATA_AWARD_BIT_SET_H
#define MHTRI_LOBBY_USERDATA_AWARD_BIT_SET_H

#include "types.h"

struct LbEquipWork;

#ifdef __cplusplus
extern "C" {
#endif

/* Sets bit `index` (0..23) of the save's two award halfwords at +0x40F8/+0x40FA.  GUESS name. */
void userdata_award_bit_set(struct LbEquipWork* self, s32 index);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_USERDATA_AWARD_BIT_SET_H */
