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

/* The two-byte cursor `cockpit_cursor_step` advances: `index` into the caller's bit table (`cockpit_act_icon_mask_get`'s
 * mask) and `timer` (0x14 = armed, -1 = idle, else a countdown).  size: 0x2 */
typedef struct Cursor2 {
    /* +0x0 */ u8 index;
    /* +0x1 */ s8 timer;
} Cursor2;

/* The floating item tag a cockpit work record draws over its player (`cockpit_item_tag_draw`/`cockpit_item_tag_draw_at`): the world
 * position `chara_icon_pos_get` fills, the animation frames, the colour and the icon state.  size: 0x1C */
typedef struct CockpitItemTag {
    /* +0x00 */ VEC3 pos;            /* the world position the tag is projected from */
    /* +0x0C */ u8 frame_0x0C;       /* the tag's animation frame */
    /* +0x0D */ u8 bob_frame_0x0D;    /* the bob clock: past 90 the tag bobs by `ofs_y_0x0F` */
    /* +0x0E */ u8 open_frame_0x0E;  /* the opening animation frame, capped at 30 when drawn */
    /* +0x0F */ s8 ofs_y_0x0F;       /* added to the projected y */
    /* +0x10 */ u32 color_0x10;      /* the icon colour */
    /* +0x14 */ u8 kind_prev_0x14;    /* the act kind the frames were started for */
    /* +0x15 */ u8 tex_idx_0x15;     /* the icon cell; 0 hides the tag */
    /* +0x16 */ u8 unusable_0x16;    /* set when the item cannot be used (the red cross) */
    /* +0x17 */ u8 open_0x17;        /* set while the item's sub-menu is open */
    /* +0x18 */ u8 flag_0x18;         /* cleared with the icon state every frame (`cockpit_item_tag_update`) */
    /* +0x19 */ u8 count_0x19;       /* the count drawn on the tag (0 = none) */
    /* +0x1A */ u8 unused_0x1A[0x1C - 0x1A];
} CockpitItemTag;

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
 * `menu/cockpit_work_init.cpp` names keep their offsets, and the six this band reads sit in the byte run
 * the sibling leaves unnamed.  size: 0x194 */
typedef struct CockpitWork {
    /* +0x000 */ _PLW* plw;          /* the player work record (sibling: `CockpitMove* move`) */
    /* +0x004 */ u8 unused_0x004[0x008 - 0x004];
    /* +0x008 */ u32 field_0x008;
    /* +0x00C */ u8 unused_0x00C[0x018 - 0x00C];
    /* +0x018 */ Cursor2 act_cursor_0x018[4];   /* per player: the cursor over the act-status icons (`cockpit_act_icon_mask_get`'s mask) */
    /* +0x020 */ Cursor2 skill_cursor_0x020[4]; /* per player: the cursor over the second icon table */
    /* +0x028 */ Cursor2 npc_cursor_0x028[2];   /* the AI NPC's two cursors (`cockpit_npc_icon_mask_get`'s masks) */
    /* +0x02C */ CockpitItemTag item_tag_0x02C; /* the floating item tag over the player (`cockpit_item_tag_draw`) */
    /* +0x048 */ struct _AINPC_W* ai_npc_0x048;   /* the AI NPC record `quest_target_rank_get` asks about its hold state */
    /* +0x04C */ u16 npc_digit_0x04C;       /* the digit drawn over the AI NPC (0 = none, `cockpit_npc_digit_draw`) */
    /* +0x04E */ u8 unused_0x04E[0x050 - 0x04E];
    /* +0x050 */ u32 npc_digit_color_0x050; /* its colour */
    /* +0x054 */ VEC3 name_pos_0x054[4];    /* each player's name-tag world position (`cockpit_name_tags_draw`) */
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
    /* +0x164 */ u8 item_icon_0x164;      /* the selected item's icon kind (`cockpit_gauge_icons_update` restarts the frame on a change) */
    /* +0x165 */ u8 item_count_prev_0x165; /* last frame's `_PLW` +0x269 count; a drop starts the frame */
    /* +0x166 */ u8 item_frame_0x166;     /* the item icon's animation frame (0 = idle) */
    /* +0x167 */ u8 gauge_frame_0x167;    /* the gauge icon's animation frame */
    /* +0x168 */ s16 gauge_value_0x168;   /* the shown gauge value, stepping up to `_PLW` +0x36C */
    /* +0x16A */ u8 gauge_level_0x16A;    /* 0..5, climbing while `cockpit_gauge_low_ck` holds */
    /* +0x16B */ u8 item_ring_anim_0x16B[4]; /* the item ring's state `cockpit_item_ring_update` steps: shell latch, open frame, blink, level */
    /* +0x16F */ u8 item_list_anim_0x16F[2]; /* [1] is the item list's open frame (up by 10 to 150, down by 16; drawn / 10) */
    /* +0x171 */ u8 unused_0x171[0x174 - 0x171];
    /* +0x174 */ u16 pointer_item_0x174;   /* the item flying from the supply box to the pointer (`cockpit_pointer_target_set`) */
    /* +0x176 */ u8 supply_busy_0x176;     /* the supply box took the frame (a pending take runs next frame) */
    /* +0x177 */ s8 pointer_cell_0x177;    /* the supply box cell it flies from */
    /* +0x178 */ u8 pointer_time_0x178;    /* the flight's length in frames (0 = none) */
    /* +0x179 */ u8 pointer_left_0x179;    /* frames of it left */
    /* +0x17A */ u8 supply_notice_0x17A;   /* the byte the supply box notice watches: 0 shows it */
    /* +0x17B */ u8 cursor_frame_0x17B;    /* the supply box cursor's animation frame */
    /* +0x17C */ u8 box_frame_0x17C;       /* the supply box frame's animation frame */
    /* +0x17D */ u8 take_busy_0x17D;       /* the take menu took the frame (its action runs next frame) */
    /* +0x17E */ u8 item_menu_busy_0x17E;  /* the quest menu took the frame */
    /* +0x17F */ u8 flash_hold_0x17F;     /* frames left of the gauge flash effect */
    /* +0x180 */ u8 flash_frame_0x180;    /* the flash effect's current animation frame */
    /* +0x181 */ u8 unused_0x181[0x184 - 0x181];
    /* +0x184 */ s32 blink_clock_0x184;   /* counts the quest-target blink down (refilled from the frame scale) */
    /* +0x188 */ s32 quest_timer_0x188;   /* > 0 while the quest's own countdown runs (`quest_target_usable_ck`) */
    /* +0x18C */ u16 sparkle_a_frame_0x18C; /* the health bar's full-gauge sparkle frame (0..0x60) */
    /* +0x18E */ u16 sparkle_b_frame_0x18E; /* the stamina bar's full-gauge sparkle frame (0..0x60) */
    /* +0x190 */ u32 frame_0x190;        /* the frame counter the target colours blink on (mod 24) */
} CockpitWork;


/* One hunt log line (`cockpit_hunt_log_push`): the monster's name id, its crown, the count and the line's flash
 * frame.  Copied field by field (`cockpit_hunt_log_entry_copy`).  size: 0x6 (the u16 rounds the 5 used bytes up) */
typedef struct HuntLogEntry {
    /* +0x0 */ u8 name_id;      /* the `str_tbl_33_get` id of the monster's name */
    /* +0x1 */ u8 crown;        /* 0 none, else the crown icon's frame + 1 */
    /* +0x2 */ u16 count;       /* how many were hunted */
    /* +0x4 */ u8 frame;        /* the line's flash frame (1..6, 0 = idle) */
} HuntLogEntry;

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
    /* +0x002 */ u16 quest_notice_step_0x002;  /* the next multiple of the 5-minute limit the quest timer notice waits for */
    /* +0x004 */ u16 quest_notice_timer_0x004; /* frames the quest timer notice stays up (0x7FFF = always) */
    /* +0x006 */ u8 unused_0x006[0x018 - 0x006];
    /* +0x018 */ PitTrap pit_traps_0x018[4]; /* the pit traps `PitTrapSet` registers this frame */
    /* +0x058 */ u8 pit_trap_count_0x058;   /* how many `PitTrapSet` has filled (`quest_targets_update_b` latches it into +0x059) */
    /* +0x059 */ u8 pit_trap_shown_0x059;   /* the count the markers draw */
    /* +0x05A */ u8 hunt_log_count_0x05A;   /* how many `hunt_log_0x05E` entries are shown (3 max) */
    /* +0x05B */ u8 hunt_total_frame_0x05B; /* the total's flash frame (1..6, 0 = idle) */
    /* +0x05C */ u16 hunt_log_timer_0x05C;  /* frames the hunt log stays up; 0 hides it */
    /* +0x05E */ HuntLogEntry hunt_log_0x05E[3]; /* the newest kill first (`cockpit_hunt_log_push`) */
    /* +0x070 */ u32 hunt_total_0x070;      /* the running total the log shows (`em_set_work_state_get`/`quest_work_state_get`) */
    /* +0x074 */ u8 blink_timer_0x074;      /* the 10-frame countdown the quest HUD blink runs (`cockpit_prompt_update`) */
    /* +0x075 */ u8 blink_flag_0x075;       /* the `quest_move_sub_flag_get` value latched while the countdown runs */
    /* +0x076 */ u8 unused_0x076[0x078 - 0x076];
    /* +0x078 */ u32 field_0x078;
    /* +0x07C */ s8 transfer_mode_0x07C[4]; /* each player slot's transfer-mode icon (`setCockpitTransferMode`) */
    /* +0x080 */ u8 unused_0x080[0x088 - 0x080];
} CockpitState;

#endif /* MHTRI_HUD_COCKPIT_WORK_TYPES_H */
