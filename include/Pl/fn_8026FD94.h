/* The master action predicates and status-bit helpers of `Pl/pl_act.cpp` (0x8026F888-0x8026FEF0): `Pl/fn_80229ECC.cpp`,
 * `Pl/fn_802489D4.cpp`, `Pl/pl_act_step.cpp`, `Pl/fn_80273B14.cpp` and the ef/enemy/camera consumers drive them.
 * The map names are bare `fn_` stems, so they take C linkage; the header is C-visible.
 */
#ifndef MHTRI_PL_FN_8026FD94_H
#define MHTRI_PL_FN_8026FD94_H

#include "types.h"
#include "Pl/plw_fwd.h"

#ifdef __cplusplus
extern "C" {
#endif

s32 fn_8026FE98(struct _ENEMY_WORK* other, u32 mask);
u32 fn_8026FD94(struct _PLW* self);
u32 fn_8026FE44(struct _PLW* self);
u8 pl_act_param_tier_ck(struct _PLW* self, u32 idx);
void pl_act_set_flag(struct _PLW* self, u32 bits);
u32 fn_8026FD0C(struct _PLW* self);
void fn_8026FEF0(struct _PLW* self, s32 v);
u32 fn_8026F888(struct _PLW* self);
u32 fn_8026FB20(struct _PLW* self, u32 mask);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_FN_8026FD94_H */
