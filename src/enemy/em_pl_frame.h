/* The player-act handlers `hud/pl_frame_sync.cpp` defines and its dispatcher calls: each takes the player work and,
 * for all but three, the index of the act variant it serves (the signatures are the call sites').
 */
#ifndef MHTRI_ENEMY_EM_PL_FRAME_H
#define MHTRI_ENEMY_EM_PL_FRAME_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

void pl_act_handler_00(struct _PLW* plw, s32 variant);
void pl_act_handler_01(struct _PLW* plw, s32 variant);
void pl_act_handler_02(struct _PLW* plw, s32 variant);
void pl_act_handler_03(struct _PLW* plw, s32 variant);
void pl_act_handler_04(struct _PLW* plw, s32 variant);
void pl_act_handler_05(struct _PLW* plw, s32 variant);
void pl_act_handler_06(struct _PLW* plw, s32 variant);
void pl_act_handler_07(struct _PLW* plw, s32 variant);
void pl_act_handler_08(struct _PLW* plw, s32 variant);
void pl_act_handler_10(struct _PLW* plw, s32 variant);
void pl_act_handler_11(struct _PLW* plw, s32 variant);
void pl_act_handler_12(struct _PLW* plw, s32 variant);
void pl_act_handler_13(struct _PLW* plw, s32 variant);
void pl_act_handler_14(struct _PLW* plw, s32 variant);
void pl_act_handler_16(struct _PLW* plw, s32 variant);
void pl_act_handler_17(struct _PLW* plw, s32 variant);
void pl_act_handler_18(struct _PLW* plw, s32 variant);
void pl_act_handler_19(struct _PLW* plw, s32 variant);
void pl_act_handler_20(struct _PLW* plw, s32 variant);
void pl_act_handler_21(struct _PLW* plw, s32 variant);
void pl_act_handler_22(struct _PLW* plw, s32 variant);
void pl_act_handler_23(struct _PLW* plw, s32 variant);
void pl_act_handler_24(struct _PLW* plw, s32 variant);
void pl_act_handler_25(struct _PLW* plw, s32 variant);
void pl_act_handler_26(struct _PLW* plw, s32 variant);
void pl_act_handler_27(struct _PLW* plw, s32 variant);
void pl_act_handler_29(struct _PLW* plw, s32 variant);

void pl_act_handler_09(struct _PLW* plw);
void pl_act_handler_15(struct _PLW* plw);
void pl_act_handler_28(struct _PLW* plw);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_EM_PL_FRAME_H */
