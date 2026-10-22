/* The types and declarations `src/hud/cockpit_quest.cpp` owns (docs/plan.md 6.5, rules 1-5).
 *
 * The unit is the quest half of the cockpit HUD: the two gauges, the quest marker ring with its player/target/pit
 * markers, and the mark table, driven from the player work record (`_PLW`), the quest-target table
 * (`_ENEMY_WORK`) and the two cockpit work records `cockpit_work[2]` (0x194 B each, `menu/fn_802E4978.cpp`'s
 * storage).
 *
 * One record, two views: `menu/fn_802E4978.h` (the band below) names the fields it reads of
 * `cockpit_work`/`cockpit_state`, this header the ones this band reads; every member keeps its offset, so neither
 * view moves.  Two headers for one record is the rule 1 debt the fold (`merge` item) closes.
 *
 * Callees are declared by their owners (`pl.h`, `Pl/*.h`, `stage/stg_w.h`, `fn_8004CAD8.h`, `hud/layout.h`, ...);
 * the few still declared below have no header that can be included beside this one.
 */
#ifndef MHTRI_HUD_COCKPIT_QUEST_H
#define MHTRI_HUD_COCKPIT_QUEST_H

#include "types.h"
#include "pl.h"
#include "hud/layout.h"
#include "enemy/ENEMY_WORK.h"
#include "hud/cockpit_work.h"   /* QuestMark, HudBlend, CockpitWork, cockpit_work */

/* The screen projection `screen_projection_get` hands back: two scales and the two offsets
 * `quest_screen_project` divides by.  size: 0x10 */
typedef struct QuestScreen {
    /* +0x00 */ f32 scale_x;
    /* +0x04 */ f32 scale_y;
    /* +0x08 */ f32 ofs_x;
    /* +0x0C */ f32 ofs_y;
} QuestScreen;







/* The one-player cockpit state (`.bss` 0x806BDFF0, 0x88 B; see the type's comment). */
extern CockpitState cockpit_state;

#ifdef __cplusplus
extern "C" {
#endif

/* ---- this unit's bodies, in address order (a bare map name is C linkage) ---- */
u16 quest_bar_a_next_id(u16 id, u8 kind, _PLW* plw);                        /* 0x802E7408 */
u16 quest_bar_b_next_id(s32 id, u8 kind, _PLW* plw);                        /* 0x802E7548 */
void quest_blend_lerp(HudBlend* out, const HudBlend* a, const HudBlend* b, u16 t); /* 0x802E7690 */
void quest_gauge_blend_update(CockpitWork* work);                           /* 0x802E7788 */
void quest_gauge_update(CockpitWork* work, u8 mode);                        /* 0x802E796C */
void quest_gauge_bar_draw(u16 id, s32 shown, s32 ghost, s32 capacity, s32 units, const HudBlend* colors,
                          const _mh_ivec2_* offset, s16 width);             /* 0x802E7CFC */
void quest_gauge_draw(void);                                                /* 0x802E7FA0 */
s32 quest_target_visible_ck(CockpitWork* work, _ENEMY_WORK* target);        /* 0x802E884C */
void quest_notice_spawn(s32 unused_ctx, u8 idx);                            /* 0x802E88BC; the first argument is ignored */
void quest_target_notice_spawn(_ENEMY_WORK* target, u8 idx);                /* 0x802E8A20 */
void quest_targets_update_a(CockpitWork* work);                             /* 0x802E8BA4 */
void quest_targets_update_b(CockpitWork* work);                             /* 0x802E8C8C */
s32 quest_mark_visible_ck(CockpitWork* work);                               /* 0x802E8D7C */
u8 quest_mark_toggle(CockpitWork* work);                                    /* 0x802E8E34 */
u8 quest_mark_flag_get(CockpitWork* work);                                  /* 0x802E8E5C */
void quest_marker_arm(void);                                                /* 0x802E8E64 */
void quest_screen_project(f32* out, const f32* pos);                        /* 0x802E8F54 */
f32 quest_mark_dist_sq(CockpitWork* work, const f32* pos);                  /* 0x802E8FBC */
void quest_rot_pair(f32* a, f32* b, u16 t);                                 /* 0x802E9070 */
f32 quest_mark_dist(CockpitWork* work, const f32* pos);                     /* 0x802E90C0 */
s32 quest_mark_on_screen_ck(CockpitWork* work, const f32* pos);             /* 0x802E90FC */
u32 quest_mark_fade_value(CockpitWork* work, u32 color, f32 dist);          /* 0x802E9130 */
void quest_marker_icon_draw(CockpitWork* work, const f32* pos, u8 index, u16 tex_no, u32 color, const f32* pos2,
                            u16 tex_no2, f32 scale, f32 scale2);            /* 0x802E919C */
f32 quest_marker_draw_at(CockpitWork* work, const f32* pos, u8 index, u16 tex_no, u32 color, s32 clip,
                         f32 scale);                                        /* 0x802E932C */
void quest_marker_ring_draw(const f32* offset, const f32* tex_offset, u32 color, f32 scale, f32 tex_scale); /* 0x802E94D0 */
void quest_marker_pie_draw(const f32* offset, const f32* tex_offset, const f32* extent, u32 color, u16 tex_no,
                           f32 scale, f32 tex_scale);                       /* 0x802E962C */
void quest_player_marker_pos(_PLW* plw, VEC3* out);                         /* 0x802E97A8 */
void quest_player_marker_draw(CockpitWork* work, _PLW* plw, s32 clip);      /* 0x802E98A8 */
void quest_marker_draw_all_non_master(CockpitWork* work, s32 clip);         /* 0x802E9B48 */
void quest_marker_draw_others_then_self(CockpitWork* work, s32 clip);       /* 0x802E9C0C */
void quest_marker_draw_with_master(CockpitWork* work, _PLW* plw);           /* 0x802E9CB4 */
void quest_view_frame_task(CockpitWork* work, u32 view);                    /* 0x802E9E9C */
s32 quest_slot_arm_all_a(s8 value);                                         /* 0x802EA050 */
s32 quest_slot_arm_a(CockpitWork* work, s8 slot);                           /* 0x802EA0CC */
s32 quest_slot_arm_all_b(s8 value);                                         /* 0x802EA138 */
s32 quest_slot_arm_b(CockpitWork* work, s8 slot);                           /* 0x802EA1B4 */
void quest_mark_append(CockpitWork* work, const _mh_ivec2_* pos, u8 tex_idx, u32 coord, u16 size); /* 0x802EA204 */
void quest_mark_draw(const _mh_ivec2_* pos, u8 mode, u32 color, u16 frame); /* 0x802EA278 */
void quest_marks_flush(CockpitWork* work);                                  /* 0x802EA33C */
void quest_npc_marker_draw(s32 clip);                                       /* 0x802EA3B4 */
u8 quest_target_rank_get(CockpitWork* work, _PLW* plw);                     /* 0x802EA5F4 */
u8 quest_target_usable0_ck(CockpitWork* work, _ENEMY_WORK* target);         /* 0x802EA7D4 */
u8 quest_target_usable_ck(CockpitWork* work, _ENEMY_WORK* target, s32 flags); /* 0x802EA7DC */
u32 quest_target_color(CockpitWork* work, _ENEMY_WORK* target, u8 kind);    /* 0x802EA908 */
s32 quest_target_icon_get(CockpitWork* work, _ENEMY_WORK* target, u8* icon, f32* scale, u32* color); /* 0x802EAAB4 */
void quest_target_marker_draw(CockpitWork* work, _ENEMY_WORK* target, s32 clip); /* 0x802EAC38 */
void quest_target_markers_draw(CockpitWork* work, s32 clip);                /* 0x802EAD28 */
void quest_target_markers_draw_live(CockpitWork* work);                     /* 0x802EADC4 */
void quest_target_icon_anim_draw(CockpitWork* work, u8 idx, u16 frame);        /* 0x802EAE64 */
void quest_target_icons_anim_draw(void);                     /* 0x802EAF88 */
void quest_icon_rect_draw(CockpitWork* work, u16 id, u16 size, u32 color, s32 clip); /* 0x802EB034 */
void quest_map_area_icons_draw(CockpitWork* work);                          /* 0x802EB268 */
const u16* quest_map_icon_list_get(u8 mapno);                               /* 0x802EB35C */
void quest_map_icons_draw(CockpitWork* work, u16 size);                     /* 0x802EB400 */
void quest_pit_area_icons_draw(CockpitWork* work, u16 size);                /* 0x802EB51C */
void quest_map_icon_marks_draw(CockpitWork* work, u16 size, s32 clip);      /* 0x802EB5C4 */
void quest_pit_area_icon_marks_draw(CockpitWork* work, u16 size, s32 clip); /* 0x802EB714 */
void quest_map_icon_sprites_draw(CockpitWork* work, u16 size, u8 frame, s32 clip); /* 0x802EB7C4 */
s32 quest_marker_screen_pos(CockpitWork* work, _mh_ivec2_* pos, f32* scale, u8 idx); /* 0x802EB8E8 */
f32 quest_pit_trap_marker_draw(CockpitWork* work, PitTrap* trap, s32 clip); /* 0x802EBA74 */
void quest_pit_traps_draw(CockpitWork* work, s32 clip);                     /* 0x802EBAE0 */
void quest_marker_draw_faded(CockpitWork* work, u16 id, u32 mode, const _mh_ivec2_* pos); /* 0x802EBBD0 */
s32 quest_mark_allowed_ck(_PLW* plw);                                       /* 0x802EBE2C */

/* ---- callees with no header of their own yet ---- */
s32 color_lerp(s32 a, s32 b, f32 t);                   /* 0x802E270C, `hud/layout.cpp` */
void math_sincos_idx(f32* a, f32* b, f32 t);            /* 0x80500EF4 */
u16 quest_bar_id_keep(u16 id, _PLW* plw);               /* 0x802E73B8, the band below's last function */

#ifdef __cplusplus
}
#endif

/* 0x802EBB58 - registers a pit trap for this frame (a C++ free function, the map's `PitTrapSet__FPCQ34nw4r4math4VEC3Uc`). */
void PitTrapSet(const VEC3* pos, u8 kind);

/* ---- callees whose map name is a mangling: the front-end reproduces it (rule 9) ---- */
void drawshape_set_texture_array(u16 id, const _mh_tex_uv_* uv); /* 0x80053ACC */
void drawshape_set_vertex_array(const _mh_ivec2_* verts);        /* 0x80053860 */
u8 get_option_cfg(u8 index);                    /* 0x803BEC70 -> get_option_cfg__FUc */
u8 get_arena_cfg(u8 a, u8 b);                   /* 0x803BEBF0 -> get_arena_cfg__FUcUc */
void set_blendmode(u8 a, u8 b, u8 c);           /* 0x800E4574 -> set_blendmode__FUcUcUc */
u8 get_now_mapno(void);                         /* 0x802AFC74 -> get_now_mapno__Fv */
u16 get_move_work_max(u8 index);                /* 0x800CFAD0 -> get_move_work_max__FUc */
u8* get_move_work_adrs(u8 index);               /* 0x800CFA90 -> get_move_work_adrs__FUc */

#endif /* MHTRI_HUD_COCKPIT_QUEST_H */
