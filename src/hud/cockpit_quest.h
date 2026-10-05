/* The types and declarations `src/hud/cockpit_quest.cpp` owns (docs/plan.md 6.5, rules 1-5).
 *
 * The unit is the quest half of the cockpit HUD: the two gauges, the quest marker ring with its player/target/pit
 * markers, and the mark table, driven from the player work record (`_PLW`), the quest-target table
 * (`_ENEMY_WORK`) and the two cockpit work records `cockpit_work[2]` (0x194 B each, `menu/fn_802E4978.cpp`'s
 * storage).
 *
 * One record, two views: `include/menu/fn_802E4978.h` (the band below) names the fields it reads of
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

/* The screen projection `screen_projection_get` hands back: two scales and the two offsets
 * `quest_screen_project` divides by.  size: 0x10 */
typedef struct QuestScreen {
    /* +0x00 */ f32 scale_x;
    /* +0x04 */ f32 scale_y;
    /* +0x08 */ f32 ofs_x;
    /* +0x0C */ f32 ofs_y;
} QuestScreen;

/* One entry of the mark table `CockpitWork::marks_0x0D4`: a texture coordinate pair plus the three
 * values `quest_mark_append` appends after it (0xCD counts the live ones, 8 max).  size: 0xC */
typedef struct QuestMark {
    /* +0x00 */ _mh_ivec2_ pos;   /* the screen position copied in by `uv_pair_copy` */
    /* +0x04 */ u32 field_0x04;    /* the coordinates `quest_icon_rect_draw` builds from the two entry point */
    /* +0x08 */ u16 field_0x08;    /* the texture rectangle's size */
    /* +0x0A */ u8 field_0x0A;     /* the texture/index selector byte */
} QuestMark;


/* One 0x10-byte blend endpoint the quest bar interpolates between: three integer components the
 * `.data` tables at 0x805D5C88.. hold, plus a fourth word copied verbatim (the target stores it
 * with a plain `lwz`/`stw`, never through the interpolator).  size: 0x10 */
typedef struct HudBlend {
    /* +0x00 */ s32 x;
    /* +0x04 */ s32 y;
    /* +0x08 */ s32 z;
    /* +0x0C */ u32 tag;   /* copied, not blended (the `lwz`/`stw` pair in `quest_blend_lerp`) */
} HudBlend;

/* One player's cockpit work record (`cockpit_work`, two entries, stride 0x194).  This is *this*
 * band's view of the shared record (see the header comment): the members the sibling
 * `menu/fn_802E4978.cpp` names keep their offsets, and the six this band reads sit in the byte run
 * the sibling leaves unnamed.  size: 0x194 */
typedef struct CockpitWork {
    /* +0x000 */ _PLW* plw;          /* the player work record (sibling: `CockpitMove* move`) */
    /* +0x004 */ u8 unused_0x004[0x008 - 0x004];
    /* +0x008 */ u32 field_0x008;
    /* +0x00C */ u8 unused_0x00C[0x019 - 0x00C];
    /* +0x019 */ u8 field_0x019;
    /* +0x01A */ u8 unused_0x01A;
    /* +0x01B */ u8 field_0x01B;
    /* +0x01C */ u8 unused_0x01C;
    /* +0x01D */ u8 field_0x01D;
    /* +0x01E */ u8 unused_0x01E;
    /* +0x01F */ u8 field_0x01F;
    /* +0x020 */ u8 unused_0x020;
    /* +0x021 */ u8 field_0x021;
    /* +0x022 */ u8 unused_0x022;
    /* +0x023 */ u8 field_0x023;
    /* +0x024 */ u8 unused_0x024;
    /* +0x025 */ u8 field_0x025;
    /* +0x026 */ u8 unused_0x026;
    /* +0x027 */ u8 field_0x027;
    /* +0x028 */ u8 unused_0x028;
    /* +0x029 */ u8 field_0x029;
    /* +0x02A */ u8 unused_0x02A;
    /* +0x02B */ u8 field_0x02B;
    /* +0x02C */ u8 unused_0x02C[0x048 - 0x02C];
    /* +0x048 */ struct _AINPC_W* ai_npc_0x048;   /* the AI NPC record `quest_target_rank_get` asks about its hold state */
    /* +0x04C */ u8 unused_0x04C[0x084 - 0x04C];
    /* +0x084 */ s16 window_x_0x084;     /* the quest window's spot, off `get_lsp_data(0xBE3)`/`0xBE4` */
    /* +0x086 */ s16 window_y_0x086;
    /* +0x088 */ f32 window_scale_0x088;  /* the window record's width, the scale the marker positions use */
    /* +0x08C */ f32 window_icon_scale_0x08C; /* the window marker icon's scale factor */
    /* +0x090 */ f32 marker_ofs_x_0x090; /* the offset `quest_marker_draw_at`/`quest_icon_rect_draw` slide the marker by */
    /* +0x094 */ f32 marker_ofs_y_0x094;
    /* +0x098 */ f32 marker_ref_x_0x098; /* the reference `quest_mark_dist_sq` measures the marker against */
    /* +0x09C */ f32 marker_ref_y_0x09C;
    /* +0x0A0 */ f32 ring_scale_0x0A0;     /* the `.sdata2` trio `quest_marker_arm` arms together; +0xA0 is the icon scale divisor */
    /* +0x0A4 */ f32 ring_extent_x_0x0A4;
    /* +0x0A8 */ f32 ring_extent_y_0x0A8;
    /* +0x0AC */ f32 ring_inv_x_0x0AC;     /* the two scales `quest_mark_dist_sq` carries the marker by */
    /* +0x0B0 */ f32 ring_inv_y_0x0B0;
    /* +0x0B4 */ f32 ring_radius_0x0B4;     /* the scale `quest_mark_fade_value` squares */
    /* +0x0B8 */ u16 ring_angle_0x0B8;     /* the rotation clock `quest_mark_dist_sq` reads */
    /* +0x0BA */ u16 draw_id_0x0BA;   /* the sprite id both arms of `quest_marker_arm` arm (0xBE5) */
    /* +0x0BC */ u16 target_mask_0x0BC;     /* the quest-target bitmask `quest_targets_update_a` fills */
    /* +0x0BE */ s16 view_timer_0x0BE;   /* the view's own icon timer (`quest_view_frame_task`, view 1) */
    /* +0x0C0 */ s16 slot_timer_0x0C0[4]; /* per-player countdown `quest_slot_arm_a` arms negative */
    /* +0x0C8 */ u8 slot_flags_0x0C8; /* the per-player bit `quest_slot_arm_a` raises */
    /* +0x0C9 */ u8 slot_state_0x0C9[4]; /* the per-player byte `quest_slot_arm_a`/`quest_slot_arm_b` sets */
    /* +0x0CD */ u8 mark_count_0x0CD; /* how many `marks_0x0D4` entries are live (max 8) */
    /* +0x0CE */ u8 mark_gate_0x0CE;     /* the gate `quest_mark_toggle` toggles and `quest_mark_visible_ck` tests */
    /* +0x0CF */ u8 notice_flag_0x0CF;   /* the view state the marker notices point at (`hud_notice_set_flag_ptr`) */
    /* +0x0D0 */ u8 target_rank_0x0D0;   /* `quest_target_rank_get`'s rank of the player, refreshed with the target bitmask */
    /* +0x0D1 */ u8 field_0x0D1;
    /* +0x0D2 */ u8 unused_0x0D2[0x0D4 - 0x0D2];
    /* +0x0D4 */ QuestMark marks_0x0D4[8]; /* the 0xC-stride mark table `quest_mark_append` appends to */
    /* +0x134 */ u16 stagger_shown_0x134; /* the stagger gauge value on screen (`quest_gauge_update` eases it up by 4) */
    /* +0x136 */ u8 stagger_flash_0x136;  /* the gauge's flash alpha (0xFF while the act is winding, then decays by 8) */
    /* +0x137 */ u8 stagger_shake_idx_0x137; /* the low-stagger shake step, wraps at 0x19/0x32 */
    /* +0x138 */ u8 stagger_notice_clock_0x138; /* counts up to the low-stagger notice (0x1E/0x3C frames) */
    /* +0x139 */ u8 notice_flag_0x139;    /* the byte the spawned notice watches: 4 visible, 0 hidden */
    /* +0x13A */ u8 health_hit_clock_0x13A; /* the health gauge's shake clock, restarted at 9 by a hit */
    /* +0x13B */ u8 unused_0x13B;
    /* +0x13C */ s16 health_prev_0x13C;   /* last frame's health, for the hit test */
    /* +0x13E */ u16 blend_0x13E;     /* the first bar's blend parameter (`quest_bar_a_next_id`) */
    /* +0x140 */ u16 blend_0x140;     /* the second bar's blend parameter (`quest_bar_b_next_id`) */
    /* +0x142 */ u8 unused_0x142[0x144 - 0x142];
    /* +0x144 */ HudBlend bar_a_0x144; /* the first bar's blended value (`quest_blend_lerp`'s out) */
    /* +0x154 */ HudBlend bar_b_0x154; /* the second bar's blended value */
    /* +0x164 */ u8 unused_0x164[0x17F - 0x164];
    /* +0x17F */ u8 flash_hold_0x17F;     /* frames left of the gauge flash effect */
    /* +0x180 */ u8 flash_frame_0x180;    /* the flash effect's current animation frame */
    /* +0x181 */ u8 unused_0x181[0x184 - 0x181];
    /* +0x184 */ s32 blink_clock_0x184;   /* counts the quest-target blink down (refilled from the frame scale) */
    /* +0x188 */ s32 quest_timer_0x188;   /* > 0 while the quest's own countdown runs (`quest_target_usable_ck`) */
    /* +0x18C */ u16 sparkle_a_frame_0x18C; /* the health bar's full-gauge sparkle frame (0..0x60) */
    /* +0x18E */ u16 sparkle_b_frame_0x18E; /* the stamina bar's full-gauge sparkle frame (0..0x60) */
    /* +0x190 */ u32 frame_0x190;        /* the frame counter the target colours blink on (mod 24) */
} CockpitWork;

/* One pit trap: its world position and kind byte (`PitTrapSet`).  size: 0x10 */
typedef struct PitTrap {
    /* +0x00 */ VEC3 pos;
    /* +0x0C */ u8 kind_0x0C;      /* 0 is drawn in the first colour, anything else in the second */
    /* +0x0D */ u8 unused_0x0D[0x10 - 0x0D];
} PitTrap;

/* The one-player cockpit state at `cockpit_state` (the sub-screen selector).  Its owner is
 * `src/menu/fn_802E4978.cpp` (`include/menu/fn_802E4978.h`, which cannot be included here - it
 * redefines `CockpitWork` - so this is the same one-record-two-views debt as above, filed).  This
 * band reads the player count and the flag pair `quest_targets_update_b` shifts.  size: 0x88 */
typedef struct CockpitState {
    /* +0x000 */ u8 field_0x000;   /* the number of live players `quest_slot_arm_all_a` loops over */
    /* +0x001 */ u8 unused_0x001;
    /* +0x002 */ s16 field_0x002;
    /* +0x004 */ s16 field_0x004;
    /* +0x006 */ u8 unused_0x006[0x018 - 0x006];
    /* +0x018 */ PitTrap pit_traps_0x018[4]; /* the pit traps `PitTrapSet` registers this frame */
    /* +0x058 */ u8 pit_trap_count_0x058;   /* how many `PitTrapSet` has filled (`quest_targets_update_b` latches it into +0x059) */
    /* +0x059 */ u8 pit_trap_shown_0x059;   /* the count the markers draw */
    /* +0x05A */ u8 field_0x05A;
    /* +0x05B */ u8 unused_0x05B;
    /* +0x05C */ s16 field_0x05C;
    /* +0x05E */ u8 unused_0x05E[0x070 - 0x05E];
    /* +0x070 */ u32 field_0x070;
    /* +0x074 */ u8 field_0x074;
    /* +0x075 */ u8 unused_0x075[0x078 - 0x075];
    /* +0x078 */ u32 field_0x078;
} CockpitState;

/* The two cockpit work records (`.bss` 0x806BDCC8, 0x328 B = 2 x 0x194).  Defined by the band below,
 * which initialises them (`fn_802E4978` memsets one); declared here, never defined (playbook 29's
 * rule for another unit's storage). */
extern CockpitWork cockpit_work[2];

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
