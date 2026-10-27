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
struct _PL_ROOT;
struct TaskSlot;
/* The scene driver: the root work inits, the three game-mode tasks `mh3_pad.cpp` hands `Tsk_Change`, and the
 * per-mode steps they dispatch on the root work's mode. */
void quest_scene_work_init(struct TaskSlot* task, u8 kind);
void vs_scene_work_init(struct TaskSlot* task);
void scene_se_handles_release(struct _PL_ROOT* root);
void net_player_drop_step(struct _PL_ROOT* root, u8 player);
void quest_game_task(struct TaskSlot* task);
void quest_scene_boot(struct _PL_ROOT* root);
void quest_scene_res_load(struct _PL_ROOT* root, u8 kind);
void vs_scene_res_load(struct _PL_ROOT* root);
void quest_scene_load(struct _PL_ROOT* root);
void scene_actors_move(struct _PL_ROOT* root);
void scene_hud_update(void);
void quest_scene_main(struct _PL_ROOT* root);
void scene_area_change_step(u8 kind);
void net_error_dialog_draw(void);
void quest_scene_end(struct _PL_ROOT* root);
void field_game_task(struct TaskSlot* task);
void field_scene_boot(struct _PL_ROOT* root);
void field_scene_load(struct _PL_ROOT* root);
void field_scene_main(struct _PL_ROOT* root);
void field_scene_end(struct _PL_ROOT* root);
void scene_exit_point_ck(struct _PL_ROOT* root);
void vs_game_task(struct TaskSlot* task);
void vs_scene_boot(struct _PL_ROOT* root);
void vs_scene_load(struct _PL_ROOT* root);
void vs_scene_main(struct _PL_ROOT* root);
void vs_scene_end(struct _PL_ROOT* root);
/* 0x8028F24C - the stage map kind of the root work's map (0 when there is no root work). */
u32 pl_map_kind_get(void);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_MOTION_H */
