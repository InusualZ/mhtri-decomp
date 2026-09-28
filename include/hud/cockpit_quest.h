/* The types and declarations `src/hud/cockpit_quest.cpp` owns (docs/plan.md 6.5, rules 1-5).
 *
 * The unit is the quest half of the cockpit HUD: it drives the quest bar / target nameplate from the
 * player work record (`_PLW`) and the two cockpit work records `lbl_806BDCC8[2]` (0x194 B each,
 * defined by the band below - `menu/fn_802E4978.cpp`).
 *
 * Playbook 56 (one record, two views).  `lbl_806BDCC8`'s 0x194-byte record is read by *both* bands:
 * `include/menu/fn_802E4978.h` (the band 0x802E4978-0x802E7408) names the fields it reads and the
 * band above (this unit) splices six more into its filler - `blend_0x13E`, `blend_0x140`,
 * `from_0x144`/`to_0x154` (the two 0x10-byte blend endpoints `quest_blend_lerp` interpolates) and the
 * four `field_0x134`..`field_0x13C` members the sibling already names.  Every member the sibling
 * names keeps its own offset, name and type here; the byte total is unchanged (0x194), so neither
 * view moves.  The two headers are the duplicate rule 1 records as debt (outbox: `shared-file`
 * request naming both files) - the fold is the merge, not this unit's.
 *
 * The outbound callees whose owners are registered are declared in the owner's header and included
 * from the source (`pl.h`, `Pl/pl_skill.h`, `Pl/pl_act.h`, `Pl/pl_master.h`, `hud/layout.h`,
 * `main.h`, `Network/network_pat_control.h`).  The rest are unsplit (their band has no registered unit), so their
 * declarations live here with the signature the target's call site shows (rule 2's unsplit case,
 * the same debt `include/hud/layout.h` records).
 */
#ifndef MHTRI_HUD_COCKPIT_QUEST_H
#define MHTRI_HUD_COCKPIT_QUEST_H

#include "types.h"
#include "pl.h"
#include "hud/layout.h"

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
    /* +0x00 */ _mh_tex_uv_ uv;   /* the pair copied in by `uv_pair_copy` */
    /* +0x04 */ u32 field_0x04;    /* the coordinates `fn_802EB034` builds from the two entry point */
    /* +0x08 */ u16 field_0x08;    /* the texture rectangle's size */
    /* +0x0A */ u8 field_0x0A;     /* the texture/index selector byte */
} QuestMark;

/* One record of the quest-target table `get_move_work_adrs(3)` walks (stride 0xB18, the count is
 * `get_move_work_max(3)`); only the two fields `quest_targets_update_a` reads are named.  size: 0xB18 */
typedef struct QuestTarget {
    /* +0x000 */ u8 pad_0x000[0x1C8];
    /* +0x1C8 */ u32 flags;      /* bit 0: the entry is live */
    /* +0x1CC */ u8 pad_0x1CC[0x1E1 - 0x1CC];
    /* +0x1E1 */ u8 index;       /* the bit `CockpitWork::field_0x0BC` is raised for */
    /* +0x1E2 */ u8 pad_0x1E2[0xB18 - 0x1E2];
} QuestTarget;

/* One 0x10-byte blend endpoint the quest bar interpolates between: three integer components the
 * `.data` tables at 0x805D5C88.. hold, plus a fourth word copied verbatim (the target stores it
 * with a plain `lwz`/`stw`, never through the interpolator).  size: 0x10 */
typedef struct HudBlend {
    /* +0x00 */ s32 x;
    /* +0x04 */ s32 y;
    /* +0x08 */ s32 z;
    /* +0x0C */ u32 tag;   /* copied, not blended (the `lwz`/`stw` pair in `quest_blend_lerp`) */
} HudBlend;

/* One player's cockpit work record (`lbl_806BDCC8`, two entries, stride 0x194).  This is *this*
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
    /* +0x02C */ u8 unused_0x02C[0x084 - 0x02C];
    /* +0x084 */ s16 field_0x084;     /* the quest window's spot, off `get_lsp_data(0xBE3)` */
    /* +0x086 */ u16 field_0x086;
    /* +0x088 */ f32 field_0x088;
    /* +0x08C */ u8 unused_0x08C[0x090 - 0x08C];
    /* +0x090 */ f32 marker_ofs_x_0x090; /* the offset `fn_802E932C`/`fn_802EB034` slide the marker by */
    /* +0x094 */ f32 marker_ofs_y_0x094;
    /* +0x098 */ f32 marker_ref_x_0x098; /* the reference `quest_mark_dist_sq` measures the marker against */
    /* +0x09C */ f32 marker_ref_y_0x09C;
    /* +0x0A0 */ f32 field_0x0A0;     /* the `.sdata2` trio `quest_marker_arm` arms together */
    /* +0x0A4 */ f32 field_0x0A4;
    /* +0x0A8 */ f32 field_0x0A8;
    /* +0x0AC */ f32 field_0x0AC;     /* the two scales `quest_mark_dist_sq` carries the marker by */
    /* +0x0B0 */ f32 field_0x0B0;
    /* +0x0B4 */ f32 field_0x0B4;     /* the scale `quest_mark_fade_value` squares */
    /* +0x0B8 */ u16 field_0x0B8;     /* the rotation clock `quest_mark_dist_sq` reads */
    /* +0x0BA */ u16 draw_id_0x0BA;   /* the sprite id both arms of `quest_marker_arm` arm (0xBE5) */
    /* +0x0BC */ u16 field_0x0BC;     /* the quest-target bitmask `quest_targets_update_a` fills */
    /* +0x0BE */ s16 field_0x0BE;
    /* +0x0C0 */ s16 slot_timer_0x0C0[4]; /* per-player countdown `quest_slot_arm_a` arms negative */
    /* +0x0C8 */ u8 slot_flags_0x0C8; /* the per-player bit `quest_slot_arm_a` raises */
    /* +0x0C9 */ u8 slot_state_0x0C9[4]; /* the per-player byte `quest_slot_arm_a`/`quest_slot_arm_b` sets */
    /* +0x0CD */ u8 mark_count_0x0CD; /* how many `marks_0x0D4` entries are live (max 8) */
    /* +0x0CE */ u8 field_0x0CE;     /* the gate `quest_mark_toggle` toggles and `quest_mark_visible_ck` tests */
    /* +0x0CF */ u8 field_0x0CF;
    /* +0x0D0 */ u8 field_0x0D0;
    /* +0x0D1 */ u8 field_0x0D1;
    /* +0x0D2 */ u8 unused_0x0D2[0x0D4 - 0x0D2];
    /* +0x0D4 */ QuestMark marks_0x0D4[8]; /* the 0xC-stride mark table `quest_mark_append` appends to */
    /* +0x134 */ u16 field_0x134;     /* the counted-up gauge value (`fn_802E796C` adds 4) */
    /* +0x136 */ u8 field_0x136;      /* the gauge's step latch (0xFF while the act is winding) */
    /* +0x137 */ u8 field_0x137;
    /* +0x138 */ u8 field_0x138;
    /* +0x139 */ u8 field_0x139;
    /* +0x13A */ u8 field_0x13A;      /* the blend clock, counted down from 9 */
    /* +0x13B */ u8 unused_0x13B;
    /* +0x13C */ s16 field_0x13C;     /* the health value the blend clock is restarted on */
    /* +0x13E */ u16 blend_0x13E;     /* the first bar's blend parameter (`quest_bar_a_next_id`) */
    /* +0x140 */ u16 blend_0x140;     /* the second bar's blend parameter (`quest_bar_b_next_id`) */
    /* +0x142 */ u8 unused_0x142[0x144 - 0x142];
    /* +0x144 */ HudBlend bar_a_0x144; /* the first bar's blended value (`quest_blend_lerp`'s out) */
    /* +0x154 */ HudBlend bar_b_0x154; /* the second bar's blended value */
    /* +0x164 */ u8 unused_0x164[0x194 - 0x164];
} CockpitWork;

/* The one-player cockpit state at `lbl_806BDFF0` (the sub-screen selector).  Its owner is
 * `src/menu/fn_802E4978.cpp` (`include/menu/fn_802E4978.h`, which cannot be included here - it
 * redefines `CockpitWork` - so this is the same one-record-two-views debt as above, filed).  This
 * band reads the player count and the flag pair `quest_targets_update_b` shifts.  size: 0x88 */
typedef struct CockpitState {
    /* +0x000 */ u8 field_0x000;   /* the number of live players `quest_slot_arm_all_a` loops over */
    /* +0x001 */ u8 unused_0x001;
    /* +0x002 */ s16 field_0x002;
    /* +0x004 */ s16 field_0x004;
    /* +0x006 */ u8 unused_0x006[0x058 - 0x006];
    /* +0x058 */ u8 field_0x058;    /* the pending flag `quest_targets_update_b` latches into +0x059 */
    /* +0x059 */ u8 field_0x059;
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
extern CockpitWork lbl_806BDCC8[2];

/* The one-player cockpit state (`.bss` 0x806BDFF0, 0x88 B; see the type's comment). */
extern CockpitState lbl_806BDFF0;

#ifdef __cplusplus
extern "C" {
#endif

/* ---- this unit's own bodies, in address order (a bare `fn_` map name is C linkage) ---- */
u16 quest_bar_a_next_id(u16 id, u8 kind, _PLW* plw);
u16 quest_bar_b_next_id(s32 id, u8 kind, _PLW* plw);
void quest_blend_lerp(HudBlend* out, const HudBlend* a, const HudBlend* b, u16 t);
void fn_802E7788(CockpitWork* work);
/* The bodies this pass added, in address order; `fn_802E796C`..`quest_targets_update_a` keep their original
 * declarations below, and the ones already declared there (`quest_mark_visible_ck`, `quest_mark_dist_sq`,
 * `quest_slot_arm_all_b`) are not repeated. */
u8 quest_mark_toggle(CockpitWork* work);
u8 quest_mark_flag_get(CockpitWork* work);
void quest_marker_arm(void);
void quest_targets_update_b(CockpitWork* work);
s32 quest_slot_arm_all_a(s8 value);
s32 quest_slot_arm_a(CockpitWork* work, s8 slot);
s32 quest_slot_arm_b(CockpitWork* work, s8 slot);
void quest_mark_append(CockpitWork* work, const _mh_tex_uv_* uv, u8 tex_idx, u32 coord, u16 size);
u8 quest_target_usable0_ck(CockpitWork* work, QuestTarget* target);
void quest_marker_draw_faded(CockpitWork* work, u16 id, u8 mode, const _mh_ivec2_* pos);

void fn_802E796C(CockpitWork* work, u8 arg1);
void fn_802E7CFC(u16 id, s32 w, s32 h, s32 a3, s32 a4, const void* a5, const _mh_ivec2_* pos, s16 size);
void fn_802E7FA0(CockpitWork* work);
s32 quest_target_visible_ck(CockpitWork* work, QuestTarget* target);
void quest_targets_update_a(CockpitWork* work);
void quest_screen_project(f32* out, const f32* pos);
f32 quest_mark_dist(CockpitWork* work, const f32* pos);
s32 quest_mark_on_screen_ck(CockpitWork* work, const f32* pos);
u8 quest_mark_fade_value(CockpitWork* work, u8 value, f32 dist);
void quest_view_frame_task(CockpitWork* work, s32 arg1);
s32 quest_slot_arm_all_b(s8 value);                      /* 0x802EA138 size 0x7C (rule 2: this band owns it) */

/* ---- unsplit callees whose map name is bare (C linkage) ---- */
u8 Pl_Skill_slot_item_get(_PLW* plw, u32 slot);            /* 0x802715A0 Pl/pl_skill.cpp defines it */
s32 Pl_act_state_ck(_PLW* plw);                     /* 0x8027681C */
u8 pl_act_kind_get(u8 kind);                        /* 0x802B0598 */
f32 anim_tick_cos(u16 t);                         /* 0x800501B4 */
u8 fn_8028EF30(u8 index);                       /* 0x8028EF30 */
void quest_rot_pair(f32* a, f32* b, u16 t);        /* 0x802E9070 this unit's own body */
f32 quest_mark_dist_sq(CockpitWork* work, const f32* pos); /* 0x802E8FBC this unit's own body */
const QuestScreen* screen_projection_get(void);           /* 0x802B0230 the screen projection */
s32 color_lerp(s32 a, s32 b, f32 t);           /* 0x802E270C */
u8 quest_target_usable_ck(CockpitWork* work, QuestTarget* target, s32 flags); /* 0x802EA7DC */
s32 quest_mark_visible_ck(CockpitWork* work);             /* 0x802E8D7C */
u8 quest_target_rank_get(CockpitWork* work, _PLW* plw);                      /* 0x802EA5F4 */
f32 anim_tick_angle(u16 t);                         /* 0x800501E4 */
void math_sincos_idx(f32* a, f32* b, f32 t);        /* 0x80500EF4 */
u16 quest_bar_id_keep(u16 id, _PLW* plw);             /* 0x802E73B8 the band below's last function */

#ifdef __cplusplus
}
#endif

/* ---- unsplit callees whose map name is a mangling: the front-end reproduces it (rule 9) ---- */
void drawshape_set_texture_array(u16 id, const _mh_tex_uv_* uv); /* 0x80053ACC */
void drawshape_set_vertex_array(const _mh_ivec2_* verts);        /* 0x80053860 */
u8 get_option_cfg(u8 index);                    /* 0x803BEC70 -> get_option_cfg__FUc */
u8 get_arena_cfg(u8 a, u8 b);                   /* 0x803BEBF0 -> get_arena_cfg__FUcUc */
void set_blendmode(u8 a, u8 b, u8 c);           /* 0x800E4574 -> set_blendmode__FUcUcUc */
void get_worldworld_pos(nw4r::math::VEC3* out, const nw4r::math::VEC3* pos, u8 area); /* 0x802B0100 */
u8 get_now_areano(void);                        /* 0x802AFC84 -> get_now_areano__Fv */
u8 get_now_mapno(void);                         /* 0x802AFC74 -> get_now_mapno__Fv */
u16 get_move_work_max(u8 index);                /* 0x800CFAD0 -> get_move_work_max__FUc */
u8* get_move_work_adrs(u8 index);               /* 0x800CFA90 -> get_move_work_adrs__FUc */

/* The pooled constants and tables the bodies below read.  Declared, never defined (playbook 29):
 * the unit's object must not rebuild the `.sdata2`/`.data` sections the split already owns.
 *
 * Only the entries a *written* body reads are declared (an `extern` of unowned data is a rule-12
 * finding, so a declaration arrives with the body that consumes it): the nine `HudBlend` tables at
 * 0x805D5C88..0x805D5CFC and the four floats 0x8079A8E8/0x8079A8F0/0x8079A910/0x8079A92C are read
 * by the still-unwritten bodies (`fn_802E7788` and friends) and are re-declared when those land.
 * The remaining labels are generated names (rule 7 debt, recorded in the unit header): renaming a
 * pooled constant means renaming its map row and sweeping every consuming unit. */
extern f32 lbl_8079A8F8;   /* 0.0f */
extern f32 lbl_8079A8FC;   /* 1.0f */
extern f32 lbl_8079A920;   /* the blend gain `quest_blend_lerp` scales by */
extern f32 lbl_8079A928;   /* the rotation gain `quest_rot_pair` scales by */
extern u16 lbl_805D5C74[2][5];
extern char lbl_805D5D08[];   /* "cockpit_quest.cpp" (0x12 B) */
extern char lbl_805D5D20[];   /* the `nw4r::db::Panic` message */

#endif /* MHTRI_HUD_COCKPIT_QUEST_H */
