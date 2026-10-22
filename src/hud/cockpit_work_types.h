/* hud/cockpit_work_types.h - the cockpit work record type `hud/cockpit_quest.cpp` owns and the two record types it
 *   embeds, for a consumer that cannot include `hud/cockpit_quest.h` beside its own headers (`menu/menu_item.cpp`; that
 *   header's `get_move_work_*` spellings clash with `unsplit/ef.h`'s).  `hud/cockpit_quest.h` includes this one (rule 1:
 *   one definition); the storage is declared in the leaf header `hud/cockpit_work.h`.
 */
#ifndef MHTRI_HUD_COCKPIT_WORK_TYPES_H
#define MHTRI_HUD_COCKPIT_WORK_TYPES_H

#include "types.h"
#include "pl.h"
#include "hud/layout_types.h"

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
 * `src/menu/fn_802E4978.cpp` (`menu/fn_802E4978.h`, which cannot be included here - it
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
    /* +0x07C */ s8 transfer_mode_0x07C[4]; /* each player slot's transfer-mode icon (`setCockpitTransferMode`) */
    /* +0x080 */ u8 unused_0x080[0x088 - 0x080];
} CockpitState;

#endif /* MHTRI_HUD_COCKPIT_WORK_TYPES_H */
