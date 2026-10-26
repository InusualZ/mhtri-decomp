/* The master action predicates and status-bit helpers of `Pl/pl_act.cpp` (0x8026F888-0x8026FEF0), which
 * `Pl/fn_80229ECC.cpp`, `Pl/pl_act_step.cpp`, `Pl/pl_act.cpp` and the ef/enemy/camera consumers drive.  The map names
 * are bare `fn_` stems, so they take C linkage; the header is C-visible.
 */
#ifndef MHTRI_PL_PL_NOW_AREA_CK_H
#define MHTRI_PL_PL_NOW_AREA_CK_H

#include "types.h"
#include "Pl/plw_fwd.h"

#ifdef __cplusplus
extern "C" {
#endif

s32 pl_status_bit_ck(struct _ENEMY_WORK* other, u32 mask);
u32 pl_now_area_ck(struct _PLW* self);
u32 pl_item_page2_ck(struct _PLW* self);
u8 pl_act_param_tier_ck(struct _PLW* self, u32 idx);
void pl_act_set_flag(struct _PLW* self, u32 bits);
u32 pl_model_pos_sync(struct _PLW* self);
void pl_cond_flags_clear(struct _PLW* self, s32 v);
u32 pl_input_active_ck(struct _PLW* self);
u32 pl_stick_dir_ck(struct _PLW* self, u32 mask);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_NOW_AREA_CK_H */
