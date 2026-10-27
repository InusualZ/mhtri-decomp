/* The types and declarations `src/hud/cockpit_quest.cpp` owns (docs/plan.md 6.5, rules 1-5).
 *
 * The unit is the quest half of the cockpit HUD: the two gauges, the quest marker ring with its player/target/pit
 * markers, and the mark table, driven from the player work record (`_PLW`), the quest-target table
 * (`_ENEMY_WORK`) and the two cockpit work records `cockpit_work[2]` (0x194 B each, `menu/cockpit_work_init.cpp`'s
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
void cockpit_work_init(CockpitWork* work, _PLW* plw);                              /* 0x802E4978 */
void cockpit_frame_tick(void);                                                     /* 0x802E4C24 */
void cockpit_item_tag_update(_PLW* plw, CockpitItemTag* tag);                           /* 0x802E4C5C */
void cockpit_npc_digit_update(CockpitWork* work);                           /* 0x802E5220 */
void cockpit_frame_update(void);                                            /* 0x802E5284 */
u32 cockpit_hud_hidden_self_ck(_PLW* plw);                                                 /* 0x802E54A8 */
void cockpit_draw(void);                                                    /* 0x802E555C */
void cockpit_party_elements_draw(CockpitWork* work);                        /* 0x802E56B4 */
void cockpit_name_tags_draw(CockpitWork* work, u8 player);                             /* 0x802E5764 */
void cockpit_supply_box_update(void);                                                     /* 0x802E5A14 */
void cockpit_take_menu_update(void);                                        /* 0x802E5BB4 */
void cockpit_supply_cell_pos_get(_mh_ivec2_* pos, s8 cell);                                 /* 0x802E5C3C */
void cockpit_pointer_target_set(u16 id, s8 cell);                                   /* 0x802E5D68 */
void cockpit_supply_box_draw(void);                                                     /* 0x802E5E90 */
void cockpit_take_menu_draw(void);                                                     /* 0x802E646C */
void cockpit_quest_menu_draw(void);                                                     /* 0x802E6AAC */
void cockpit_flash_gauge_draw(void);                                                     /* 0x802E6EB4 */
void cockpit_timer_hand_draw(u16 id, s32 value, const _mh_ivec2_* pos);                 /* 0x802E704C */
void cockpit_timer_ring_draw(s32 value, const _mh_ivec2_* pos);                         /* 0x802E70F0 */
void cockpit_timer_draw(void);                                                     /* 0x802E71C4 */
u32 cockpit_free_play_ck(void);                                                      /* 0x802E73B4 */
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
void quest_marker_draw_faded(CockpitWork* work, u16 id, u32 mode, _mh_ivec2_ pos); /* 0x802EBBD0 */
u32 quest_mark_allowed_ck(_PLW* plw);                                       /* 0x802EBE2C */
void quest_marker_draw(CockpitWork* work, u16 id, u32 mode, _mh_ivec2_ pos); /* 0x802EBED8 */
void quest_marker_draw_sub(void);                                                     /* 0x802EC200 */
void cockpit_gauge_icons_update(CockpitWork* work);                                        /* 0x802EC4F0 */
u32 cockpit_gauge_low_ck(_PLW* plw);                                                 /* 0x802EC6C4 */
void cockpit_weapon_hud_draw(void);                                                     /* 0x802EC700 */
void cockpit_aim_view_draw(void);                                                     /* 0x802EC8AC */
void cockpit_ammo_gauge_draw(_PLW* plw, s16 x, s16 y, u8 aim);                          /* 0x802ECA20 */
void cockpit_split_plate_draw(_PLW* plw, const _mh_ivec2_* base);                        /* 0x802ECC28 */
void cockpit_party_plate_draw(_PLW* plw, u16 index);                                     /* 0x802ECE28 */
void cockpit_npc_plate_draw(void);                                                     /* 0x802ED20C */
void cockpit_party_list_draw(void);                                                     /* 0x802ED480 */
s32 cockpit_act_icon_mask_get(_PLW* plw, u8 kind);                                        /* 0x802ED588 */
s32 cockpit_npc_icon_mask_get(_AINPC_W* npc, u8 kind);                                     /* 0x802ED620 */
void cockpit_cursor_step(u32 mask, s32 count, Cursor2* cur);                        /* 0x802ED6F4 */
void cockpit_act_cursor_step(_PLW* plw, Cursor2* cur, u8 kind);                         /* 0x802ED7E4 */
void cockpit_npc_cursor_step(_AINPC_W* npc, Cursor2* cur, u8 kind);                      /* 0x802ED834 */
void cockpit_icon_cursors_update(void);                                                     /* 0x802ED88C */
void cockpit_plate_icons_draw(const _mh_ivec2_* pos, _PLW* plw);                         /* 0x802ED970 */
void cockpit_npc_plate_icons_draw(const _mh_ivec2_* pos);                                    /* 0x802EDA24 */
void cockpit_item_ring_reset(_PLW* plw, u8* a, u8* b);                                  /* 0x802EDAF0 */
void cockpit_item_ring_update(_PLW* plw, u8* anim, u8* fade);                            /* 0x802EDB0C */
void cockpit_item_ring_draw(_PLW* plw, u8* anim);                                      /* 0x802EDCE4 */
void cockpit_item_slot_icon_draw(_PLW* plw, u16 id, u8 part, u16 slot, const _mh_ivec2_* pos);/* 0x802EE288 */
void cockpit_act_icon_draw(_PLW* plw, u16 id, u8 anim, u8 mask, const _mh_ivec2_* pos); /* 0x802EE330 */
void cockpit_item_list_entry_draw(_PLW* plw, u16 id, s16 x, s16 y, u8 slot);                 /* 0x802EE3AC */
void cockpit_item_list_draw(_PLW* plw, u8* anim);                                      /* 0x802EE65C */
void cockpit_item_tag_draw(CockpitWork* work);                                        /* 0x802EE82C */
void cockpit_item_tag_draw_at(_PLW* plw, const CockpitItemTag* tag, const _mh_ivec2_* pos, u8 alpha);/* 0x802EE97C */
s32 cockpit_item_slot_live_ck(_PLW* plw, u16 idx);                                        /* 0x802EEC24 */
s16 cockpit_item_slot_live_count(_PLW* plw);                                                 /* 0x802EECA0 */
u8 cockpit_lockon_level_get(_PLW* plw);                                                  /* 0x802EED0C */
void cockpit_aim_marker_draw(VEC3 pos);                                                 /* 0x802EEDE4 */
void cockpit_aim_marker_draw_split(VEC3 pos, u8 player);                                      /* 0x802EEEA0 */
void cockpit_eye_marker_draw(_PLW* plw);                                                /* 0x802EEF7C */
void cockpit_eye_marker_draw_split(_PLW* plw);                                                /* 0x802EF00C */
void cockpit_shell_marker_draw(_PLW* plw);                                                                                     /* 0x802EF0A0 */
void cockpit_npc_digit_draw(CockpitWork* work);                                        /* 0x802EF0FC */
void cockpit_hunt_log_update(void);                                                     /* 0x802EF230 */
void cockpit_hunt_log_entry_copy(HuntLogEntry* dst, const HuntLogEntry* src);               /* 0x802EF400 */
void cockpit_hunt_log_draw(void);                                                     /* 0x802EF424 */
s32 cockpit_prompt_ck(_PLW* plw);                                                 /* 0x802EF62C */
void cockpit_prompt_update(void);                                                     /* 0x802EF6B0 */
void cockpit_prompt_draw(void);                                                     /* 0x802EF730 */
void cockpit_split_frame_update(void);                                                     /* 0x802EF87C */
void cockpit_split_marker_draw(u8 player);                                                /* 0x802EFA30 */
void cockpit_split_draw(void);                                                     /* 0x802EFB80 */
void cockpit_timer_ring_draw_at(u16 id, s32 seconds, const _mh_ivec2_* pos);               /* 0x802EFD98 */
void cockpit_split_timer_draw(u8 kind);                                                  /* 0x802EFF20 */
void cockpit_split_item_tag_draw(CockpitWork* work);                                        /* 0x802F0124 */
void cockpit_split_item_ring_draw(_PLW* plw, u8* anim, u8 player);                           /* 0x802F02EC */
void cockpit_split_item_list_draw(_PLW* plw, u8* anim, u8 player);                           /* 0x802F07E4 */
void cockpit_split_gauge_draw(CockpitWork* work, u8 player);                             /* 0x802F09C4 */
void cockpit_weapon_frame_draw(CockpitWork* work, const _mh_ivec2_* pos, u8 player);      /* 0x802F12A4 */
void cockpit_stagger_gauge_draw(CockpitWork* work, const _mh_ivec2_* pos, u8 mode);        /* 0x802F140C */
void cockpit_charge_gauge_draw(CockpitWork* work, const _mh_ivec2_* pos, u8 mode);        /* 0x802F16C0 */
void cockpit_split_weapon_hud_draw(CockpitWork* work, u8 player);                             /* 0x802F1944 */
void cockpit_split_aim_view_draw(CockpitWork* work, u8 player);                             /* 0x802F1B1C */
void cockpit_split_ammo_gauge_draw(_PLW* plw, s16 x, s16 y, u8 aim);                          /* 0x802F1CF8 */
void cockpit_split_flash_gauge_draw(CockpitWork* work, u8 player);                             /* 0x802F1F34 */
void cockpit_static_init(void);                                                     /* 0x802F20D8 */
CockpitState* cockpit_state_construct(CockpitState* state);                             /* 0x802F2120 */
PitTrap* pit_trap_construct(PitTrap* trap);                                        /* 0x802F2178 */
CockpitWork* cockpit_work_construct(CockpitWork* work);                                /* 0x802F21A8 */
CockpitItemTag* cockpit_item_tag_construct(CockpitItemTag* tag);                           /* 0x802F2208 */

/* One player's weapon-frame sprite ids (`cockpit_weapon_frame_draw`): the glow and the condition marker.  size: 0x8 */
typedef struct CockpitWeaponIds {
    /* +0x0 */ u32 glow_id;
    /* +0x4 */ u32 cond_id;
} CockpitWeaponIds;

/* One view's stagger gauge sprite ids (`cockpit_stagger_gauge_draw`): the level flash, the frame and the bar.  size: 0xC */
typedef struct CockpitStaggerIds {
    /* +0x0 */ u32 flash_id;
    /* +0x4 */ u32 frame_id;
    /* +0x8 */ u32 bar_id;
} CockpitStaggerIds;

/* One item id/count pair `em_work_slot_pair_get` fills (the take-item menu's rows).  size: 0x4 */
typedef struct CockpitSlotPair {
    /* +0x0 */ u16 id;
    /* +0x2 */ s16 count;
} CockpitSlotPair;

/* The `.data` mask tables `cockpit_act_icon_mask_get` walks (4-byte words terminated by 0), defined in the unit. */
extern u32 cockpit_act_mask_a0[];
extern u32 cockpit_act_mask_b0[];
extern u32 cockpit_act_mask_a1[];
extern u32 cockpit_act_mask_b1[];

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
