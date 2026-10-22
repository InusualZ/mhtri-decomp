/* Pl/pl_motion.h - the declarations `Pl/pl_motion.cpp` owns that other units call (docs/plan.md 6.5 rule 2); the
 * root-work record `_PL_ROOT` stays in `Pl/fn_80288CEC.h`. */
#ifndef MHTRI_PL_PL_MOTION_H
#define MHTRI_PL_PL_MOTION_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8028E4F0 - the root work's `+0x22D9` sub-state flag (`quest_move_sub_state_4_set` raises it), or 0 when there
 * is no root work (GUESS name). */
u8 quest_move_sub_flag_get(void);

u32 pl_net_move_handover(void);
void pl_act_class7_dispatch(struct _PLW* self);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_MOTION_H */
