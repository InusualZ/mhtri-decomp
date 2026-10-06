/* enemy/em009_act.h - `enemy/em009_act.cpp`'s C++-scope copy of `ef/system_core.cpp`'s two move-work
 * accessors (the owner's header cannot carry it beside `unsplit/ef.h`'s C spelling: MWCC 10505), and its
 * action-table head. */
#ifndef MHTRI_ENEMY_FN_80387844_H
#define MHTRI_ENEMY_FN_80387844_H

#include "types.h"
#include "nw4r/math.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif
/* The first four `state_sub` handlers `fn_80388144` dispatches to; `fn_803874C8` takes the work record, the
 * other three forward r3. */
void fn_803874C8(struct _ENEMY_WORK* self);
void fn_80387528(void);
void fn_803875A4(void);
void fn_80387620(void);
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
void* get_move_work_adrs(u8 kind);
u32 get_move_work_max(u8 kind);
#endif

#endif /* MHTRI_ENEMY_FN_80387844_H */
