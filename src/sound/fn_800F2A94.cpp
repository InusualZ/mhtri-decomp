/* sound/fn_800F2A94.cpp - the game's BGM controller (the three stream channels, the large-monster tracks, the area
 * tracks and the lobby tracks).
 * RANGE. .text 0x800F2A94..0x800F6520 (45 functions), extab 0x8000BA6C..0x8000BB6C, extabindex 0x8002568C..0x8002580C,
 *   .data 0x8059B170..0x8059B3B0 (the per-map area track tables, the pointer table, the demo channel table, the
 *   lobby stream command), .bss 0x806A11B8 (the control work), .sdata 0x80791650..0x80791698 (the small track
 *   tables and the work pointer), .sdata2 0x807965A0..0x807965D0 (the float pool, emitted from literals).
 * FLAGS. `#pragma peephole off` and `#pragma optimization_level 4` for the whole file (playbook 108: level 4 keeps
 *   `work + i` in a saved register across the per-slot loops' calls and lowers switch ranges to `subi; cmplwi`;
 *   unit 98.53 -> 99.04 %, 36 -> 38 rows at 100). With peephole on, 22 rows lose their clrlwi/extsh forms.
 * NAMES. GUESS (from each body and its callers; no __FILE__ string, no dump name): bgm_ctrl_init, bgm_ctrl_frame,
 *   bgm_ch_req, bgm_ch_status_update, bgm_quest_clear_frame, bgm_em_slots_phase11_set, bgm_em_slots_phase6_set,
 *   bgm_em_watch_update, bgm_em_track_get, bgm_em_slot_alloc, bgm_em_slot_select, bgm_em_slots_stop,
 *   bgm_em_alert_ck, bgm_ch_playing_ck, bgm_quest_pending_frame, bgm_map_track_play, bgm_ch_fade_out, bgm_main_stop,
 *   bgm_mode_jingle_play, bgm_lobby_ch1_stop, bgm_lobby_depart_set, bgm_arena_track_start, bgm_arena_track_stop,
 *   bgm_lobby_area_update, bgm_env_ch_stop, bgm_main_fade_out, snd_bgm_hold_set, snd_hunt_stream_start,
 *   snd_quest_scene_set, snd_quest_frame_begin, snd_quest_frame_end, the tables (bgm_env_*_tbl, bgm_env_tbl,
 *   bgm_em_map_track_tbl, bgm_map_ch1_track_tbl, bgm_demo_ch_tbl, bgm_lobby_cmd) and the work (bgm_ctrl_work,
 *   bgm_ctrl); every BgmCtrl field name is a GUESS from the code that reads it. Foreign registry names (GUESS):
 *   snd_stream_bgm_flag_get, snd_bgm_loading_get, snd_bgm_fade_level_get, stream_cmd_play_locked, lb_special_bgm_ck,
 *   lb_item_list_done_ck, lb_item_list_done_clear, se_volume_curve_tbl.
 * RESIDUALS. bgm_ctrl_frame: the lobby channel-2 block (retail's `switch (area)` tests 5..7 as a u8
 *   `addi 0xFB; clrlwi; cmplwi` and case 2 with `cmplwi`, keeps that compare in cr0 for the default case and
 *   threads the case-2 idle path past the default's first test; ours lowers it signed - a (u8)/(u32) cast, case
 *   orders, an if chain and an inline helper all measured lower). snd_quest_frame_end: case 1's `req == 0` compare
 *   in cr1 (retail computes it once, ours twice) and one extra saved GPR. bgm_em_slot_select,
 *   bgm_lobby_area_update, snd_quest_frame_begin, demo_stream_stop_ck, bgm_quest_clear_frame: register colouring
 *   and one `clrlwi` before a store. .bss/.sdata end 4 bytes short of the claims (0x244/0x44 emitted,
 *   0x248/0x48 claimed: the alignment pad before the next unit). Not flippable yet: the registry names above
 *   must land in the map first.
 * SHAPES. The work is one static record reached through the `.sdata` pointer `bgm_ctrl` (re-read after every store
 *   in bgm_ctrl_init). Channel 0 is the field/area track, channels 1 and 2 carry the large-monster tracks (monster
 *   slots 0/1) or the lobby tracks. Retail's u8 range tests are `(u8)(x + 0xFB) <= 2`-style (addi of the
 *   complement): `(u8)(x - 5)` compiles to `subi` and loses the row. A loop that ends after three hits is
 *   `for (i < 6) { if (n >= 3) break; }`. `cond ? 1 : 2` gives retail's cntlzw/extrwi/neg/addi select.
 */
#pragma peephole off
#pragma optimization_level 4
#include "types.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "nw4r/math.h"
#include "sound/fn_800F2A94.h"
#include "sound/fn_800E46E8.h"
#include "sound/GetStreamStatus.h"
#include "sound/stream_mgr_get.h"
#include "sound/fn_800EF7D8.h"
#include "sound/snd_stream_mgr_types.h"
#include "sound/stream_cmd_copy.h"
#include "sound/stream_cmd_play_locked.h"
#include "sound/snd_bgm_fade_level_get.h"
#include "sound/snd_stream_bgm_flag_get.h"
#include "sound/snd_bgm_loading_get.h"
#include "sound/se_volume_curve_tbl.h"
#include "sound/fn_800D7F54.h"
#include "sound/demo_stream_play.h"
#include "sound/demo_stream_stop.h"
#include "sound/demo_stream_stop_ck.h"
#include "sound/em_se_stop_all.h"
#include "sound/bgm_event_flag_set.h"
#include "sound/bgm_lobby_return_req.h"
#include "sound/bgm_track15_req.h"
#include "ef/fn_800CDB2C.h"
#include "ef/get_move_work_adrs.h"
#include "quest/quest_types.h"
#include "quest/quest_entry.h"
#include "quest/quest_item_slot.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/enemy_control.h"
#include "enemy/fn_8012BDF4.h"
#include "enemy/em_sight_player_ck2.h"
#include "enemy/em_any_player_seen_ck.h"
#include "enemy/em_pop.h"
#include "stage/stg_w.h"
#include "stage/get_now_mapno.h"
#include "Pl/pl_map_kind_get.h"
#include "Pl/fn_80288CEC.h"
#include "menu/demo_play_ck.h"
#include "menu/get_pop_dat_ptr.h"
#include "lobby/lb_server_sel_trans.h"
#include "lobby/event_demo_running_ck.h"
#include "lobby/demo_flag_ck.h"
#include "lobby/lb_quest_screen.h"
#include "lobby/lb_quest_work_active_ck.h"
#include "lobby/kujira_event_over_ck.h"
#include "lobby/lb_menu_pos_tbl.h"
#include "lobby/lb_item_list_done_ck.h"
#include "lobby/lb_special_bgm_ck.h"
#include "lobby/lobby_w.h"
#include "mh3_pad/system_w.h"
#include "mh3_pad/vec3.h"
#include "fn_8004CAD8.h"
#include "camera/camera_angle_x_get.h"
#include "Network/network_pat_control.h"

/* The BGM control work: the three stream channels, the two large-monster track slots (channels 1 and 2),
 * the three watched monsters, the area/environment track state and the lobby track state.
 * size: 0x244 */
struct BgmCtrl {
    /* +0x000 */ u32 status[3];          /* each channel's stream status this frame */
    /* +0x00C */ u32 prev_status[3];     /* and the frame before */
    /* +0x018 */ u8 ch_state[3];         /* 0 idle, 1 loading, 2 playing, 3 ended, 4 finished */
    /* +0x01B */ u8 pad_0x01B;
    /* +0x01C */ f32 field_0x1C[3];      /* cleared by bgm_ctrl_init only */
    /* +0x028 */ f32 field_0x28[3];      /* cleared by bgm_ctrl_init only */
    /* +0x034 */ u32 field_0x34[3];      /* cleared by bgm_ctrl_init only */
    /* +0x040 */ u8 ch_track[3];         /* the track (id >= 0x32) each channel was last asked for */
    /* +0x043 */ u8 hold_state;          /* the field BGM hold step (snd_bgm_hold_set) */
    /* +0x044 */ u8 quest_end;           /* set once the quest is over */
    /* +0x045 */ u8 hunt_state;          /* the post-kill stream step */
    /* +0x046 */ s16 hunt_timer;         /* its countdown */
    /* +0x048 */ u8 quest_pending;       /* the quest scene asked for the BGM switch */
    /* +0x049 */ u8 env_state;           /* the area track step: 0 idle, 1 started, 2 playing, 3 silent */
    /* +0x04A */ u8 env_track[2];        /* the area track per environment (0 normal, 1 alternate) */
    /* +0x04C */ u8 clear_phase;         /* the quest clear jingle step (bgm_quest_clear_frame) */
    /* +0x04D */ u8 em_state[2];         /* per monster slot: 0 idle, 1 playing, 2 fading, 3 ending */
    /* +0x04F */ u8 pad_0x04F;
    /* +0x050 */ s16 em_end_timer[2];    /* the end countdown of em_state 3 */
    /* +0x054 */ s16 em_delay[2];        /* the delay before a slot's track starts */
    /* +0x058 */ u8 em_cur_track[2];     /* the track a slot plays */
    /* +0x05A */ u16 em_unique[2];       /* the monster a slot belongs to, 0xFFFF = none */
    /* +0x05E */ u8 em_req_track[2];     /* the track a slot asks for */
    /* +0x060 */ u8 em_flag_0x60[2];     /* cleared with the slot */
    /* +0x062 */ u8 em_prio[2];          /* the requested track's priority */
    /* +0x064 */ u8 em_yield[2];         /* the slot gives way to the other one */
    /* +0x066 */ u8 field_0x66;          /* reset to 1 by bgm_em_slots_stop when set */
    /* +0x067 */ u8 pad_0x067[0x170 - 0x067];
    /* +0x170 */ u32 field_0x170;        /* set to 80 when channel 1/2 starts track 0x32 */
    /* +0x174 */ f32 field_0x174;        /* cleared by bgm_ctrl_init only */
    /* +0x178 */ u8 env_area;            /* the environment the area track follows (0 normal, 1 alternate) */
    /* +0x179 */ u8 env_area_prev;
    /* +0x17A */ u8 env_alt_on;          /* the alternate tracks of map 0x16 are up */
    /* +0x17B */ u8 pad_0x17B[0x1BB - 0x17B];
    /* +0x1BB */ u8 lobby_rand;          /* the last random lobby ambience */
    /* +0x1BC */ u8 behind_flag;         /* cleared by bgm_behind_flag_clear */
    /* +0x1BD */ u8 pad_0x1BD[3];
    /* +0x1C0 */ s32 lobby_timer;        /* frames to the next random lobby ambience */
    /* +0x1C4 */ u8 volume[6];           /* 0x7F each; [5] is the lobby ambience volume */
    /* +0x1CA */ u8 ch_cur_track[3];     /* the track each channel plays */
    /* +0x1CD */ u8 pad_0x1CD[3];
    /* +0x1D0 */ s32 env_timer;          /* frames since the area track started */
    /* +0x1D4 */ u8 area;                /* this frame's area */
    /* +0x1D5 */ u8 area_prev;           /* the previous frame's area */
    /* +0x1D6 */ u8 lobby_state;         /* the lobby track step */
    /* +0x1D7 */ u8 player_mask;         /* one bit per player (snd_player_mask_set/_clear) */
    /* +0x1D8 */ u8 event_flag;          /* bgm_event_flag_set */
    /* +0x1D9 */ u8 watch_active[3];     /* per watched monster: the record is valid */
    /* +0x1DC */ u16 watch_unique[3];    /* its unique id, 0xFFFF = none */
    /* +0x1E2 */ u8 watch_kind[3];       /* its kind (the enemy record's +0x03) */
    /* +0x1E5 */ u8 watch_seen[3];       /* it has a player in sight */
    /* +0x1E8 */ u8 watch_angry[3];      /* its status byte is 2 */
    /* +0x1EB */ u8 watch_alert[3];      /* its status byte is 1 */
    /* +0x1EE */ u8 option_bgm;          /* option 17, stored twice */
    /* +0x1EF */ u8 option_bgm_saved;
    /* +0x1F0 */ u8 option_se;           /* option 18, stored twice */
    /* +0x1F1 */ u8 option_se_saved;
    /* +0x1F2 */ u8 pad_0x1F2[2];
    /* +0x1F4 */ u32 ch_word_0x1F4[3];   /* cleared by bgm_ch_req for its channel */
    /* +0x200 */ u8 pad_0x200[0x234 - 0x200];
    /* +0x234 */ s16 event_timer;        /* bgm_event_flag_set's 36-frame count */
    /* +0x236 */ s16 hold_timer;         /* frames snd_quest_frame_begin waits */
    /* +0x238 */ u8 item_sel;            /* the item list selection the lobby track was picked for */
    /* +0x239 */ u8 depart_flag;         /* bgm_lobby_depart_set */
    /* +0x23A */ u8 arena_flag;          /* bgm_arena_track_start */
    /* +0x23B */ u8 ch_end_count[3];     /* per channel: frames in state 4 */
    /* +0x23E */ s16 depart_timer;
    /* +0x240 */ s16 lobby_delay;
    /* +0x242 */ u8 pad_0x242[2];
};

/* The per-map area track tables: one {normal, alternate} pair per area. */
/* .data 0x8059B170 */
static u8 bgm_env_map01_tbl[16][2] = {
    {0x32, 0x40}, {0x33, 0x39}, {0x34, 0x39}, {0x35, 0x39}, {0x36, 0x39}, {0x37, 0x39},
    {0x38, 0x39}, {0x3B, 0x39}, {0x3C, 0x39}, {0x3D, 0x39}, {0x33, 0x39}, {0x3F, 0x3E},
    {0x3B, 0x39}, {0x00, 0x00}, {0x00, 0x00}, {0x00, 0x00},
};
/* .data 0x8059B190 */
static u8 bgm_env_map02_tbl[12][2] = {
    {0x32, 0x00}, {0x3A, 0x00}, {0x3A, 0x00}, {0x33, 0x00}, {0x34, 0x00}, {0x35, 0x00},
    {0x32, 0x00}, {0x36, 0x00}, {0x36, 0x00}, {0x37, 0x00}, {0x38, 0x00}, {0x39, 0x00},
};
/* .data 0x8059B1A8 */
static u8 bgm_env_map03_tbl[12][2] = {
    {0x32, 0x34}, {0x33, 0x34}, {0x33, 0x34}, {0x33, 0x34}, {0x33, 0x34}, {0x35, 0x34},
    {0x37, 0x36}, {0x32, 0x34}, {0x33, 0x34}, {0x39, 0x34}, {0x38, 0x34}, {0x00, 0x00},
};
/* .data 0x8059B1C0 */
static u8 bgm_env_map04_tbl[10][2] = {
    {0x32, 0x00}, {0x33, 0x00}, {0x34, 0x00}, {0x35, 0x00}, {0x36, 0x00}, {0x36, 0x00},
    {0x37, 0x00}, {0x38, 0x00}, {0x38, 0x00}, {0x39, 0x00},
};
/* .data 0x8059B1D4 */
static u8 bgm_env_map15_tbl[10][2] = {
    {0x32, 0x00}, {0x33, 0x00}, {0x38, 0x00}, {0x38, 0x00}, {0x36, 0x00}, {0x36, 0x00},
    {0x37, 0x00}, {0x33, 0x00}, {0x38, 0x00}, {0x39, 0x00},
};
/* .data 0x8059B1E8 */
static u8 bgm_env_map05_tbl[12][2] = {
    {0x32, 0x00}, {0x33, 0x00}, {0x34, 0x00}, {0x32, 0x00}, {0x35, 0x00}, {0x36, 0x00},
    {0x37, 0x00}, {0x38, 0x00}, {0x39, 0x00}, {0x39, 0x00}, {0x3A, 0x00}, {0x00, 0x00},
};
/* .data 0x8059B200 */
static u8 bgm_env_map16_tbl[12][2] = {
    {0x3B, 0x00}, {0x3C, 0x00}, {0x3D, 0x00}, {0x3B, 0x00}, {0x35, 0x00}, {0x36, 0x00},
    {0x37, 0x00}, {0x38, 0x00}, {0x39, 0x00}, {0x39, 0x00}, {0x3A, 0x00}, {0x00, 0x00},
};
/* .data 0x8059B218 */
static u8 bgm_env_map12_tbl[16][2] = {
    {0x32, 0x40}, {0x33, 0x39}, {0x34, 0x39}, {0x35, 0x39}, {0x36, 0x39}, {0x37, 0x39},
    {0x38, 0x39}, {0x3B, 0x39}, {0x3C, 0x39}, {0x3D, 0x39}, {0x33, 0x39}, {0x3F, 0x3E},
    {0x3B, 0x39}, {0x00, 0x00}, {0x00, 0x00}, {0x00, 0x00},
};
/* .data 0x8059B238 */
static u8 bgm_env_map13_tbl[12][2] = {
    {0x32, 0x00}, {0x3B, 0x00}, {0x32, 0x00}, {0x33, 0x00}, {0x34, 0x00}, {0x35, 0x00},
    {0x3A, 0x00}, {0x36, 0x00}, {0x36, 0x00}, {0x37, 0x00}, {0x38, 0x00}, {0x39, 0x00},
};
/* .data 0x8059B250 */
static u8 bgm_env_map14_tbl[12][2] = {
    {0x32, 0x36}, {0x33, 0x36}, {0x34, 0x36}, {0x3A, 0x36}, {0x34, 0x36}, {0x37, 0x36},
    {0x38, 0x39}, {0x3A, 0x36}, {0x3B, 0x35}, {0x3C, 0x36}, {0x3C, 0x36}, {0x00, 0x00},
};
/* .data 0x8059B268 */
static u8 bgm_em_map_track_tbl[24] = {
    0x50, 0x50, 0x51, 0x52, 0x53, 0x54, 0x50, 0x50, 0x55, 0x55, 0x50, 0x55,
    0x50, 0x51, 0x52, 0x53, 0x54, 0x50, 0x50, 0x55, 0x55, 0x00, 0x00, 0x00,
};
/* .data 0x8059B280 */
static u8 bgm_map_ch1_track_tbl[24] = {
    0x00, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x46, 0x4B, 0x4C, 0x4C, 0x46, 0x4C,
    0x46, 0x47, 0x48, 0x49, 0x4A, 0x46, 0x4B, 0x4C, 0x4C, 0x00, 0x00, 0x00,
};
/* .data 0x8059B298 */
static u8 bgm_env_map21_tbl[10][2] = {
    {0x32, 0x3C}, {0x33, 0x3D}, {0x34, 0x34}, {0x35, 0x3F}, {0x36, 0x36}, {0x37, 0x41},
    {0x38, 0x42}, {0x39, 0x39}, {0x00, 0x00}, {0x00, 0x00},
};
/* .data 0x8059B2AC */
static u8 bgm_env_lobby_alt_tbl[10][2] = {
    {0x50, 0x50}, {0x50, 0x50}, {0x51, 0x51}, {0x50, 0x50}, {0x36, 0x36}, {0x52, 0x52},
    {0x52, 0x52}, {0x52, 0x52}, {0x00, 0x00}, {0x00, 0x00},
};
/* .sdata 0x80791650 */
static u8 bgm_env_map10_tbl[2][2] = {
    {0x38, 0x00}, {0x39, 0x00},
};
/* .sdata 0x80791654 */
static u8 bgm_env_map06_tbl[3][2] = {
    {0x32, 0x00}, {0x33, 0x00}, {0x34, 0x00},
};
/* .sdata 0x80791660 */
static u8 bgm_env_map07_tbl[4][2] = {
    {0x32, 0x34}, {0x32, 0x34}, {0x32, 0x34}, {0x32, 0x35},
};
/* .sdata 0x80791668 */
static u8 bgm_env_map18_tbl[4][2] = {
    {0x33, 0x34}, {0x33, 0x34}, {0x33, 0x34}, {0x33, 0x35},
};
/* .sdata 0x80791670 */
static u8 bgm_env_map08_tbl[2][2] = {
    {0x32, 0x35}, {0x33, 0x35},
};
/* .sdata 0x80791674 */
static u8 bgm_env_map11_tbl[2][2] = {
    {0x32, 0x35}, {0x34, 0x35},
};
/* .sdata 0x80791678 */
static u8 bgm_env_map19_tbl[2][2] = {
    {0x36, 0x35}, {0x33, 0x35},
};
/* .sdata 0x8079167C */
static u8 bgm_env_map20_tbl[2][2] = {
    {0x36, 0x35}, {0x37, 0x35},
};
/* .sdata 0x80791680 */
static u8 bgm_env_map09_tbl[4][2] = {
    {0x32, 0x35}, {0x33, 0x35}, {0x34, 0x35}, {0x00, 0x00},
};
/* .sdata 0x80791688 */
static u8 bgm_env_map22_tbl[4][2] = {
    {0x32, 0x33}, {0x36, 0x37}, {0x34, 0x35}, {0x00, 0x00},
};

/* The unit's own functions that are called before their definition (C linkage, as the map names them). */
extern "C" {
void bgm_ch_status_update(void);
void bgm_em_watch_update(struct BgmCtrl* work);
void bgm_em_track_get(u8 kind, u16 unique, u32* track, u32* prio);
s8 bgm_em_slot_alloc(struct BgmCtrl* work, u16 unique, u32 prio);
void bgm_em_slot_select(struct BgmCtrl* work);
void bgm_em_slots_stop(void);
u32 bgm_em_alert_ck(void);
u32 bgm_ch_playing_ck(s32 channel);
void bgm_map_track_play(void);
void bgm_lobby_area_update(u8 map, u8 area, u8 prev);
void bgm_arena_track_start(void);
void bgm_arena_track_stop(void);
void bgm_quest_clear_frame(void);
void bgm_em_slots_phase11_set(struct BgmCtrl* work, struct _PLW* player);
void bgm_em_slots_phase6_set(struct BgmCtrl* work, struct _PLW* player);
void bgm_quest_pending_frame(void);
void bgm_ch_fade_out(s32 channel);
void bgm_main_stop(void);
void bgm_mode_jingle_play(u8 mode);
void bgm_lobby_ch1_stop(void);
void bgm_lobby_depart_set(void);
void bgm_env_ch_stop(void);
void bgm_main_fade_out(void);
}

/* .data 0x8059B2C0: the area track table of each map (NULL: the map has none). */
static u8 (*bgm_env_tbl[23])[2] = {
    NULL,              bgm_env_map01_tbl, bgm_env_map02_tbl, bgm_env_map03_tbl, bgm_env_map04_tbl, bgm_env_map05_tbl,
    bgm_env_map06_tbl, bgm_env_map07_tbl, bgm_env_map08_tbl, bgm_env_map09_tbl, bgm_env_map10_tbl, bgm_env_map11_tbl,
    bgm_env_map12_tbl, bgm_env_map13_tbl, bgm_env_map14_tbl, bgm_env_map15_tbl, bgm_env_map16_tbl, bgm_env_map06_tbl,
    bgm_env_map18_tbl, bgm_env_map19_tbl, bgm_env_map20_tbl, bgm_env_map21_tbl, bgm_env_map22_tbl,
};

/* .data 0x8059B31C: per demo, whether it uses channel 2 and channel 0. */
static u8 bgm_demo_ch_tbl[100] = {
    0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x00,
};

/* .data 0x8059B380: the lobby ambience stream command. */
static StreamCmd bgm_lobby_cmd = {
    8, 0, 0, 100, 0, 0, 0, 0, 0x40, 6, 0, 0x7F, 0, 0x32, 0, 0, 0, 0, 30, 0, 0, 0, 0, {{0, 0}},
};

/* .bss 0x806A11B8 / .sdata 0x80791690 */
static BgmCtrl bgm_ctrl_work;
static BgmCtrl* bgm_ctrl = &bgm_ctrl_work;

/* 0x800F2A94 (0x244): clears the control work and sets its defaults. */
void bgm_ctrl_init(void) {
    memset(bgm_ctrl, 0, sizeof(BgmCtrl));
    for (s32 i = 0; i < 3; i++) {
        bgm_ctrl->field_0x1C[i] = 0.0f;
        bgm_ctrl->field_0x28[i] = 0.0f;
        bgm_ctrl->field_0x34[i] = 0;
        bgm_ctrl->status[i] = 0;
        bgm_ctrl->prev_status[i] = 0;
        bgm_ctrl->ch_state[i] = 0;
    }
    bgm_ctrl->hold_state = 0;
    bgm_ctrl->quest_end = 0;
    bgm_ctrl->hunt_state = 0;
    bgm_ctrl->hunt_timer = 0;
    bgm_ctrl->env_state = 0;
    bgm_ctrl->env_track[0] = 0xFF;
    bgm_ctrl->env_track[1] = 0xFF;
    bgm_ctrl->field_0x174 = 0.0f;
    bgm_ctrl->env_area = 0xFF;
    bgm_ctrl->env_area_prev = 0xFF;
    bgm_ctrl->env_alt_on = 0;
    bgm_ctrl->lobby_rand = 0xFF;
    bgm_ctrl->behind_flag = 0;
    bgm_ctrl->clear_phase = 0;
    for (s32 i = 0; i < 6; i++) {
        bgm_ctrl->volume[i] = 0x7F;
    }
    for (s32 i = 0; i < 3; i++) {
        bgm_ctrl->ch_cur_track[i] = 0;
    }
    for (s32 i = 0; i < 2; i++) {
        bgm_ctrl->em_unique[i] = 0xFFFF;
    }
    bgm_ctrl->area = 0xFF;
    bgm_ctrl->area_prev = 0xFF;
    bgm_ctrl->lobby_state = 0;
    bgm_ctrl->player_mask = 0;
    bgm_ctrl->event_flag = 0;
    bgm_ctrl->arena_flag = 0;
    bgm_ctrl->event_timer = 0;
    bgm_ctrl->hold_timer = 0;
    bgm_ctrl->item_sel = 0;
    bgm_ctrl->depart_flag = 0;
    bgm_ctrl->depart_timer = 0;
    bgm_ctrl->lobby_delay = 0;
    for (s32 i = 0; i < 3; i++) {
        bgm_ctrl->ch_end_count[i] = 0;
    }
    bgm_ctrl->option_bgm = bgm_ctrl->option_bgm_saved = get_option_cfg(17);
    bgm_ctrl->option_se = bgm_ctrl->option_se_saved = get_option_cfg(18);
}

/* 0x800F2CD8 (0x160): stops the field channels for demo `id` and, for the gathering demo, restarts the monster track. */
void demo_stream_play(u8 id) {
    BgmCtrl* work = bgm_ctrl;
    u8 ch2 = bgm_demo_ch_tbl[id * 2];
    if (bgm_demo_ch_tbl[id * 2 + 1] != 0 && (u32)srt_ready_ck(0) == 1) {
        PlayStream(0, 0x28);
    }
    if (ch2 != 0 && (u32)srt_ready_ck(2) == 1) {
        PlayStream(2, 0x28);
    }
    if (demo_play_ck() == 1) {
        if (id == 0x2C || id == 0x2F || id == 0x2B || id == 0x0E) {
            PlayStream(2, 0x28);
        }
    } else if (id == 0x2C) {
        bgm_em_watch_update(work);
        if (work->watch_active[0] != 0 && work->watch_unique[0] != 0xFFFF) {
            u32 track;
            u32 prio;
            bgm_em_track_get(work->watch_kind[0], work->watch_unique[0], &track, &prio);
            work->em_unique[0] = work->watch_unique[0];
            work->em_delay[0] = 0;
            work->em_req_track[0] = track;
            work->em_prio[0] = prio;
            work->em_state[0] = 1;
            bgm_ch_req(1, work->em_req_track[0]);
            work->em_cur_track[0] = work->em_req_track[0];
        }
    }
}

/* 0x800F2E38 (0x13C): whether the channels demo `id` uses have run to their end (pausing them on the way). */
u32 demo_stream_stop_ck(u8 id) {
    u8 up = 0;
    u8 ch2 = bgm_demo_ch_tbl[id * 2];
    u8 ch0 = bgm_demo_ch_tbl[id * 2 + 1];
    if (demo_play_ck() == 1) {
        if (id == 0x2C || id == 0x2F || id == 0x2B || id == 0x0E) {
            ch2 = 1;
        }
    }
    if (ch2 == 0 && ch0 == 0) {
        return 1;
    }
    if (ch0 != 0) {
        s32 status = GetStreamStatus(0);
        if ((u32)(status - 1) <= 1) {
            PlayStream(0, 2);
        }
        if (status == 3) {
            up = 1;
        }
    } else {
        up = 1;
    }
    if (ch2 != 0) {
        s32 status = GetStreamStatus(2);
        if ((u32)(status - 1) <= 1) {
            PlayStream(2, 2);
        }
        if (status == 3) {
            up++;
        }
    } else {
        up++;
    }
    return up == 2;
}

/* 0x800F2F74 (0xE0): stops the channels demo `id` uses once they have run to their end. */
void demo_stream_stop(u8 id) {
    u8 ch2 = bgm_demo_ch_tbl[id * 2];
    u8 ch0 = bgm_demo_ch_tbl[id * 2 + 1];
    if (demo_play_ck() == 1) {
        if (id == 0x2C || id == 0x2F || id == 0x2B || id == 0x0E) {
            ch2 = 1;
        }
    }
    if (ch2 == 0 && ch0 == 0) {
        return;
    }
    if (ch0 != 0) {
        if (GetStreamStatus(0) == 3) {
            PlayStream(0, 3);
        }
    }
    if (ch2 != 0) {
        if (GetStreamStatus(2) == 3) {
            PlayStream(2, 3);
        }
    }
}

/* 0x800F3054 (0x1C4): refreshes every channel's state and requests `id` on `channel`. */
void bgm_ch_req(s32 channel, u32 id) {
    BgmCtrl* work = bgm_ctrl;
    if (work == NULL) {
        return;
    }
    u8 is_bgm = snd_stream_bgm_flag_get(channel, id);
    if (channel == 0 && id == 0x47) {
        set_stream_main_vol_flag(channel, 1);
    } else {
        set_stream_main_vol_flag(channel, is_bgm);
    }
    stream_bus_volume_get(channel);
    if (srt_ready_ck(channel) == 0) {
        return;
    }
    for (s32 i = 0; i < 3; i++) {
        switch (GetStreamStatus(i)) {
        case 0:
            work->ch_state[i] = 0;
            work->ch_track[i] = 0;
            work->ch_cur_track[i] = 0;
            break;
        case 1:
            work->ch_state[i] = 1;
            break;
        case 2:
            work->ch_state[i] = 2;
            break;
        case 3:
            work->ch_state[i] = 3;
            break;
        }
    }
    if (id >= 0x32) {
        if (id == work->ch_track[channel]) {
            return;
        }
        if (work->ch_state[channel] == 1) {
            return;
        }
        if ((u8)(work->ch_state[channel] + 0xFE) <= 1) {
            PlayStream(channel, 1);
            work->ch_track[channel] = 0;
            work->ch_cur_track[channel] = 0;
        }
        work->ch_track[channel] = id;
    } else if (id == 1) {
        work->ch_track[channel] = id;
    }
    PlayStream(channel, id);
    work->ch_word_0x1F4[channel] = 0;
    if (channel != 0 && id == 0x32) {
        work->field_0x170 = 80;
    }
    work->ch_cur_track[channel] = id;
    work->ch_end_count[channel] = 0;
}

/* 0x800F3218 (0xD4): polls each channel's stream status; a channel that stops after playing is marked finished. */
void bgm_ch_status_update(void) {
    BgmCtrl* work = bgm_ctrl;
    if (work == NULL) {
        return;
    }
    if (event_demo_ck() == 1) {
        return;
    }
    for (s32 i = 0; i < 3; i++) {
        work->prev_status[i] = work->status[i];
        if ((u32)srt_ready_ck(i) == 1) {
            work->status[i] = GetStreamStatus(i);
        } else {
            work->status[i] = 0;
        }
        if (work->ch_state[i] != 4 || work->status[i] != 0) {
            work->ch_state[i] = (u8)work->status[i];
            if (work->ch_state[i] == 0 && work->prev_status[i] == 2) {
                work->ch_state[i] = 4;
            }
        }
    }
}

/* 0x800F32EC (0x200): the quest clear jingle steps, then the field BGM hold steps. */
void bgm_quest_clear_frame(void) {
    BgmCtrl* work = bgm_ctrl;
    if (work == NULL) {
        return;
    }
    bgm_ch_status_update();
    if (PlayMode_ck() != 3 && PlayMode_ck() != 2) {
        return;
    }
    switch (work->clear_phase) {
    case 1:
        resumeSoundEngine();
        work->clear_phase++;
        break;
    case 2: {
        s32 stopped = 0;
        for (s32 i = 0; i < 3; i++) {
            if (GetStreamStatus(i) == 0) {
                stopped++;
            }
        }
        if (stopped == 3) {
            work->clear_phase++;
        }
        break;
    }
    case 3:
        if (quest_sub_state_end_ck(1) == 1 && (u32)srt_ready_ck(1) == 1) {
            bgm_ch_req(1, 0x35);
            bgm_ch_req(1, 4);
        }
        if (quest_sub_state_end_ck(1) == -1 && (u32)srt_ready_ck(1) == 1) {
            bgm_ch_req(1, 0x34);
            bgm_ch_req(1, 4);
        }
        work->clear_phase++;
        break;
    }
    if (PlayMode_ck() == 2) {
        return;
    }
    if (work->clear_phase != 0) {
        return;
    }
    switch (work->hold_state) {
    case 1:
        if (work->ch_state[1] == 2) {
            return;
        }
        work->hold_state++;
    case 2:
        if ((u8)event_demo_running_ck() != 0) {
            work->hold_state++;
            return;
        }
        if (work->ch_state[1] == 0 || (quest_phase_get() == 7 && get_now_areano() != 0)) {
            bgm_ch_req(0, 0x23);
            work->hold_state++;
        }
        break;
    }
}

/* 0x800F34EC (0x68): the monster slots of quest phase 11 (the arena track on slot 0). */
void bgm_em_slots_phase11_set(BgmCtrl* work, _PLW* player) {
    work->em_prio[0] = 0;
    work->em_yield[0] = 0;
    work->em_req_track[0] = 0;
    work->em_prio[1] = 0;
    work->em_yield[1] = 0;
    work->em_req_track[1] = 0;
    if (work->quest_pending != 0) {
        return;
    }
    if (player->area_0x16 == 0) {
        return;
    }
    work->em_req_track[0] = 0x61;
    work->em_prio[0] = 2;
    if (work->quest_end != 0 || work->hunt_state != 0) {
        work->em_yield[0] = 1;
    }
}

/* 0x800F3554 (0xB0): the monster slots of quest phase 6 (the siege tracks on slot 0). */
void bgm_em_slots_phase6_set(BgmCtrl* work, _PLW* player) {
    work->em_prio[0] = 0;
    work->em_yield[0] = 0;
    work->em_req_track[0] = 0;
    work->em_prio[1] = 0;
    work->em_yield[1] = 0;
    work->em_req_track[1] = 0;
    if (work->quest_pending != 0) {
        return;
    }
    u8 area = player->area_0x16;
    if (area == 0) {
        return;
    }
    if (area != 2) {
        work->em_req_track[0] = 0x5E;
    } else {
        work->em_req_track[0] = 0x5F;
    }
    if (work->event_flag != 0) {
        if (work->event_timer != 0) {
            work->event_timer--;
            work->em_yield[0] = 1;
        } else {
            work->em_req_track[0] = 0x60;
        }
    }
    work->em_prio[0] = 2;
    if (work->quest_end != 0 || work->hunt_state != 0) {
        work->em_yield[0] = 1;
    }
}

/* 0x800F3604 (0x320): refreshes the three watched monsters from the quest's spawn records. */
void bgm_em_watch_update(BgmCtrl* work) {
    _PLW* player;
    _PLW* other;
    s32 i;
    Q_MoveWork* quest;
    s32 n;
    QuestSpawnRec* rec;
    for (i = 0; i < 3; i++) {
        work->watch_active[i] = 0;
        work->watch_unique[i] = 0xFFFF;
        work->watch_kind[i] = 0;
        work->watch_seen[i] = 0;
        work->watch_angry[i] = 0;
        work->watch_alert[i] = 0;
    }
    quest = (Q_MoveWork*)get_move_work_adrs(0);
    if (quest == NULL) {
        return;
    }
    rec = quest->spawn_0x2274;
    player = (_PLW*)get_move_work_adrs(2);
    if (player == NULL) {
        return;
    }
    if (PlayMode_ck() != 2) {
        player += (s8)my_player_no();
        other = NULL;
    } else {
        other = player + 1;
    }
    if (quest_phase_get() == 11) {
        bgm_em_slots_phase11_set(work, player);
        return;
    }
    if (quest_phase_get() == 6) {
        bgm_em_slots_phase6_set(work, player);
        return;
    }
    n = 0;
    for (i = 0; i < 6; i++, rec++) {
        _ENEMY_WORK* enemy;
        _ENEMY_MINI_WORK* mini;
        if (n >= 3) {
            break;
        }
        if (rec->monster_0x6 == 0) {
            continue;
        }
        if (rec->count_0x4 == 0) {
            continue;
        }
        if (em_get_unique_work(rec->order_0x0, &enemy, &mini) != 1) {
            continue;
        }
        if (enemy->active == 0) {
            continue;
        }
        if ((enemy->field_0x1C8 & 1) == 0) {
            continue;
        }
        if (em_die_ck(enemy) != 0) {
            continue;
        }
        work->watch_unique[n] = enemy->field_0x01A;
        work->watch_kind[n] = enemy->team;
        u32 seen;
        if (PlayMode_ck() != 2) {
            if ((u32)em_pl_target_ck(enemy, player) == 1) {
                seen = em_sight_player_ck2(enemy, player->chunk_ofs);
            } else {
                seen = 0;
            }
        } else if ((u32)em_pl_target_ck(enemy, player) == 1) {
            seen = em_sight_player_ck2(enemy, player->chunk_ofs);
            if (seen == 0 && (u32)em_pl_target_ck(enemy, other) == 1) {
                seen = em_sight_player_ck2(enemy, other->chunk_ofs);
            }
        } else {
            if ((u32)em_pl_target_ck(enemy, other) == 1) {
                seen = em_sight_player_ck2(enemy, other->chunk_ofs);
            } else {
                seen = 0;
            }
        }
        if (seen == 0) {
            work->watch_seen[n] = 0;
        } else {
            work->watch_seen[n] = 1;
        }
        if (enemy->team == 0x14) {
            if (enemy->field_0x43D == 1 || enemy->field_0x43D == 2 || get_now_areano() == 3) {
                work->watch_alert[n] = 1;
            } else {
                work->watch_alert[n] = 0;
            }
            work->watch_angry[n] = 0;
        } else {
            if (enemy->field_0x43D == 1) {
                work->watch_alert[n] = 1;
            } else {
                work->watch_alert[n] = 0;
            }
            if (enemy->field_0x43D == 2) {
                work->watch_angry[n] = 1;
            } else {
                work->watch_angry[n] = 0;
            }
        }
        work->watch_active[n] = 1;
        n++;
    }
}

/* 0x800F3924 (0x184): the track and its priority for a watched monster of kind `kind`. */
void bgm_em_track_get(u8 kind, u16 unique, u32* track, u32* prio) {
    _ENEMY_WORK* enemy = NULL;
    _ENEMY_MINI_WORK* mini = NULL;
    u8 found = em_get_unique_work(unique, &enemy, &mini);
    *track = 0;
    *prio = 0;
    if (quest_flag_800_ck(NULL) == 1) {
        *track = 0x62;
        *prio = 2;
        return;
    }
    s32 map_track = bgm_em_map_track_tbl[get_now_mapno()];
    if (map_track == 0x55) {
        *track = map_track;
        *prio = 2;
        return;
    }
    switch (kind) {
    case 12:
    case 14:
        *track = 0x5A;
        *prio = 1;
        break;
    case 7:
        *track = 0x5B;
        *prio = 3;
        break;
    case 24:
        *track = 0x61;
        *prio = 3;
        break;
    case 20: {
        u8 area = 1;
        if (found != 0 && enemy != NULL) {
            area = enemy->area_no;
        }
        if ((u8)(area + 0xFF) <= 1) {
            *track = 0x5C;
        } else {
            *track = 0x5D;
        }
        *prio = 3;
        break;
    }
    default:
        *track = bgm_em_map_track_tbl[get_now_mapno()];
        *prio = 2;
        break;
    }
}

/* 0x800F3AA8 (0x1B0): the monster slot `unique` gets (a free one, one whose monster is gone, one whose
 * monster lost sight, or a lower-priority one), -1 for none. */
s8 bgm_em_slot_alloc(BgmCtrl* work, u16 unique, u32 prio) {
    if (unique == work->em_unique[0]) {
        return 0;
    }
    if (unique == work->em_unique[1]) {
        return 1;
    }
    for (s32 i = 0; i < 2; i++) {
        if (work->em_unique[i] == 0xFFFF) {
            work->em_unique[i] = unique;
            return i;
        }
    }
    _ENEMY_WORK* enemy = NULL;
    _ENEMY_MINI_WORK* mini = NULL;
    for (s32 i = 0; i < 2; i++) {
        if (em_get_unique_work(work->em_unique[i], &enemy, &mini) == 0 || enemy == NULL) {
            em_se_stop_all(work->em_unique[i]);
            work->em_unique[i] = unique;
            return i;
        }
    }
    for (s32 i = 0; i < 2; i++) {
        if (em_get_unique_work(work->em_unique[i], &enemy, &mini) != 0 && enemy != NULL &&
            em_any_player_seen_ck(enemy) == 0) {
            em_se_stop_all(work->em_unique[i]);
            work->em_unique[i] = unique;
            return i;
        }
    }
    for (s32 i = 0; i < 2; i++) {
        if (work->em_prio[i] < (u8)prio) {
            work->em_unique[i] = unique;
            return i;
        }
    }
    return -1;
}

/* 0x800F3C58 (0x340): assigns the watched monsters' tracks to the two monster slots and decides which slot yields. */
void bgm_em_slot_select(BgmCtrl* work) {
    _ENEMY_WORK* enemy = NULL;
    _ENEMY_MINI_WORK* mini = NULL;
    s32 i;
    if (quest_phase_get() != 11 && quest_phase_get() != 6) {
        for (i = 0; i < 3; i++) {
            if (work->watch_active[i] != 0 && work->watch_unique[i] != 0xFFFF &&
                em_get_unique_work(work->watch_unique[i], &enemy, &mini) != 0 && enemy != NULL &&
                em_any_player_seen_ck(enemy) == 0) {
                work->watch_seen[i] = 0;
                work->watch_angry[i] = 0;
                work->watch_alert[i] = 0;
            }
        }
        work->em_prio[0] = 0;
        work->em_yield[0] = 0;
        work->em_req_track[0] = 0;
        work->em_prio[1] = 0;
        work->em_yield[1] = 0;
        work->em_req_track[1] = 0;
        for (i = 0; i < 3; i++) {
            if (work->watch_active[i] != 0 && work->watch_unique[i] != 0xFFFF && work->watch_seen[i] != 0 &&
                (work->watch_alert[i] != 0 || work->watch_angry[i] != 0)) {
                u32 track;
                u32 prio;
                bgm_em_track_get(work->watch_kind[i], work->watch_unique[i], &track, &prio);
                s8 slot = bgm_em_slot_alloc(work, work->watch_unique[i], prio);
                if (slot != -1) {
                    if (work->watch_alert[i] != 0) {
                        if (work->em_delay[slot] == 0) {
                            if (work->em_cur_track[slot] == 0x32) {
                                work->em_delay[slot] = 180;
                            } else {
                                work->em_delay[slot] = 0;
                            }
                        }
                        work->em_req_track[slot] = track;
                    } else {
                        work->em_delay[slot] = 0;
                        work->em_req_track[slot] = 0x32;
                    }
                    work->em_prio[slot] = prio;
                }
            }
        }
        if (work->em_prio[0] != 0 && work->em_prio[1] != 0) {
            if (work->em_prio[0] > work->em_prio[1]) {
                work->em_yield[1] = 1;
            } else if (work->em_prio[0] < work->em_prio[1]) {
                work->em_yield[0] = 1;
            } else if (work->em_req_track[0] == 0x32 && work->em_req_track[1] == 0x32) {
                if (work->em_state[0] == 1) {
                    work->em_yield[1] = 1;
                } else if (work->em_state[1] == 1) {
                    work->em_yield[0] = 1;
                } else {
                    work->em_yield[1] = 1;
                }
            } else if (work->em_req_track[0] != 0x32 && work->em_req_track[1] != 0x32) {
                if (work->em_state[0] == 1 && work->em_state[1] == 1) {
                    if (work->em_delay[0] == work->em_delay[1]) {
                        work->em_yield[1] = 1;
                    } else if (work->em_delay[0] > work->em_delay[1]) {
                        work->em_yield[0] = 1;
                    } else if (work->em_delay[0] < work->em_delay[1]) {
                        work->em_yield[1] = 1;
                    }
                } else if (work->em_state[0] == 1) {
                    work->em_yield[1] = 1;
                } else {
                    work->em_yield[0] = 1;
                }
            } else if (work->em_req_track[0] == 0x32) {
                work->em_yield[0] = 1;
            } else {
                work->em_yield[1] = 1;
            }
        }
        if (work->quest_end != 0 || work->hunt_state != 0) {
            work->em_yield[0] = 1;
            work->em_yield[1] = 1;
        }
    }
}

/* 0x800F3F98 (0x3B8): runs the two monster slots: starts, fades and ends their tracks on channels 1 and 2. */
void snd_quest_frame_end(void) {
    BgmCtrl* work = bgm_ctrl;
    if (work == NULL) {
        return;
    }
    if (work->quest_end == 1) {
        return;
    }
    if (quest_phase_get() != 11 && ((u8)event_demo_running_ck() != 0 || (u32)lb_quest_work_active_ck() == 1)) {
        return;
    }
    bgm_em_watch_update(work);
    bgm_em_slot_select(work);
    for (s32 ch = 1; ch < 3; ch++) {
        if (srt_ready_ck(ch) != 0 && bgm_ch_playing_ck(ch) == 1) {
            if (work->ch_state[ch] == 3 && work->quest_end == 0 && work->hunt_state == 0 &&
                work->em_unique[ch - 1] != 0xFFFF && work->em_state[ch - 1] == 1 && work->em_cur_track[ch - 1] != 0) {
                bgm_ch_req(ch, 3);
            }
        }
    }
    for (s32 i = 0; i < 2; i++) {
        if (work->em_delay[i] != 0) {
            work->em_delay[i]--;
        }
        switch (work->em_state[i]) {
        case 0:
            if (work->em_req_track[i] == 0) {
                break;
            }
            if (work->em_yield[i] == 0) {
                if (work->em_delay[i] != 0) {
                    break;
                }
                bgm_ch_req(i + 1, work->em_req_track[i]);
                work->em_state[i] = 1;
            } else {
                work->em_state[i] = 2;
                if (work->em_req_track[i] != 0 && work->em_cur_track[i] != work->em_req_track[i]) {
                    work->em_end_timer[i] = 0;
                    bgm_ch_req(i + 1, 1);
                    work->ch_track[i + 1] = 0;
                    work->em_state[i] = 0;
                    work->em_cur_track[i] = 0;
                    work->em_state[i] = 0;
                    break;
                }
            }
            work->em_cur_track[i] = work->em_req_track[i];
            break;
        case 1:
            work->hold_timer = 60;
            if (work->em_req_track[i] != 0 && work->em_req_track[i] != work->em_cur_track[i]) {
                if (work->em_delay[i] != 0) {
                    bgm_ch_req(i + 1, 0x15);
                    work->ch_track[i + 1] = 0;
                    work->em_state[i] = 0;
                } else {
                    bgm_ch_req(i + 1, work->em_req_track[i]);
                    work->em_state[i] = 1;
                    work->em_cur_track[i] = work->em_req_track[i];
                }
            } else if (work->em_req_track[i] == 0 || work->em_yield[i] == 1) {
                bgm_ch_req(i + 1, 0x15);
                work->em_state[i] = 2;
            } else if (work->em_req_track[i] == 0) {
                bgm_ch_req(i + 1, 0x15);
                work->em_state[i] = 3;
                work->em_end_timer[i] = 90;
            } else if (work->ch_state[i + 1] == 0 || work->ch_state[i + 1] == 4) {
                bgm_ch_req(i + 1, work->em_req_track[i]);
                work->em_state[i] = 1;
                work->em_cur_track[i] = work->em_req_track[i];
            }
            break;
        case 2:
            if (work->em_yield[i] == 0 && work->em_req_track[i] != 0 &&
                work->em_req_track[i] == work->em_cur_track[i]) {
                bgm_ch_req(i + 1, 0x14);
                work->em_state[i] = 1;
            } else if (work->em_delay[i] == 0 && work->em_yield[i] == 0 && work->em_req_track[i] != 0 &&
                       work->em_req_track[i] != work->em_cur_track[i]) {
                bgm_ch_req(i + 1, work->em_req_track[i]);
                work->em_state[i] = 1;
                work->em_cur_track[i] = work->em_req_track[i];
            } else if (work->em_req_track[i] != 0 && work->em_cur_track[i] != work->em_req_track[i]) {
                work->em_end_timer[i] = 0;
                bgm_ch_req(i + 1, 1);
                work->ch_track[i + 1] = 0;
                work->em_state[i] = 0;
                work->em_cur_track[i] = 0;
                work->em_state[i] = 0;
            }
            break;
        case 3:
            work->em_unique[i] = 0xFFFF;
            if (work->em_end_timer[i] == 0) {
                bgm_ch_req(i + 1, 1);
                work->ch_track[i + 1] = 0;
                work->em_state[i] = 0;
                work->em_cur_track[i] = 0;
            } else {
                work->em_end_timer[i]--;
            }
            break;
        }
    }
}

/* 0x800F4350 (0xA0): stops both monster slots' channels and clears their requests. */
void bgm_em_slots_stop(void) {
    BgmCtrl* work = bgm_ctrl;
    if (work == NULL) {
        return;
    }
    for (s32 i = 0; i < 2; i++) {
        if ((u8)(work->ch_state[i] + 0xFE) <= 1) {
            bgm_ch_req(i, 1);
        }
        work->ch_track[i] = 0;
        work->em_state[i] = 0;
        work->em_flag_0x60[i] = 0;
    }
    if (work->field_0x66 != 0) {
        work->field_0x66 = 1;
    }
}

/* 0x800F43F0 (0x108): whether a watched monster is after a player (the battle track should play). */
u32 bgm_em_alert_ck(void) {
    BgmCtrl* work = bgm_ctrl;
    if (work == NULL) {
        return 0;
    }
    if (stage_map_kind_get(get_now_mapno()) == 6) {
        return 0;
    }
    if (PlayMode_ck() != 2 && 0.0f == snd_bgm_fade_level_get()) {
        return 0;
    }
    if (work->watch_seen[0] == 1 && (work->watch_angry[0] == 1 || work->watch_alert[0] == 1)) {
        return 1;
    }
    if (work->watch_seen[1] == 1 && (work->watch_angry[1] == 1 || work->watch_alert[1] == 1)) {
        return 1;
    }
    if (work->watch_seen[2] == 1 && (work->watch_angry[2] == 1 || work->watch_alert[2] == 1)) {
        return 1;
    }
    return 0;
}

/* 0x800F44F8 (0x40): whether `channel` is loading, playing or at its end (state 1..3). */
u32 bgm_ch_playing_ck(s32 channel) {
    if (bgm_ctrl == NULL) {
        return 0;
    }
    return (u8)(bgm_ctrl->ch_state[channel] + 0xFF) <= 2;
}

/* 0x800F4538 (0x28): the quest start: drops the pending flags and starts the clear steps. */
void snd_quest_start_bgm_set(void) {
    BgmCtrl* work = bgm_ctrl;
    if (work == NULL) {
        return;
    }
    work->hunt_state = 0;
    work->quest_pending = 0;
    work->quest_end = 1;
    work->clear_phase = 1;
}

/* 0x800F4560 (0xE4): restarts the hunt streams after a large kill. */
void snd_hunt_stream_start(s32 mode) {
    BgmCtrl* work = bgm_ctrl;
    Q_MoveWork* quest = (Q_MoveWork*)get_move_work_adrs(0);
    if (work == NULL) {
        return;
    }
    if (work->quest_end != 0) {
        return;
    }
    if (work->hunt_state != 0) {
        return;
    }
    if (quest != NULL && quest->field_0xFD == 4) {
        return;
    }
    for (s32 i = 1; i < 3; i++) {
        PlayStream(i, 1);
        work->ch_track[i] = 0;
        work->ch_cur_track[i] = 0;
    }
    if (srt_ready_ck(0) == 0) {
        return;
    }
    bgm_ch_req(0, 1);
    work->ch_state[0] = 0;
    bgm_ch_req(0, 0x46);
    work->hunt_state = 1;
    work->hunt_timer = 60;
    work->env_state = 0;
}

/* 0x800F4644 (0x94): once the pending request has drained, stops channel 1 outside play mode 2. */
void bgm_quest_pending_frame(void) {
    BgmCtrl* work = bgm_ctrl;
    if (work == NULL) {
        return;
    }
    if (work->quest_pending == 0) {
        return;
    }
    if (work->quest_end != 0) {
        return;
    }
    if (work->hunt_state != 0) {
        return;
    }
    work->quest_pending = 0;
    if (PlayMode_ck() == 2) {
        return;
    }
    if (bgm_ch_playing_ck(1) != 1) {
        return;
    }
    bgm_ch_req(1, 1);
    work->ch_track[1] = 0;
}

/* 0x800F46D8 (0x2C): sets bit `id` of the player mask. */
void snd_player_mask_set(u8 id) {
    BgmCtrl* work = bgm_ctrl;
    if (work == NULL) {
        return;
    }
    work->player_mask |= (u8)(1 << id);
}

/* 0x800F4704 (0x30): clears bit `id` of the player mask. */
void snd_player_mask_clear(u8 id) {
    BgmCtrl* work = bgm_ctrl;
    if (work == NULL) {
        return;
    }
    work->player_mask &= (u8) ~(1 << id);
}

/* 0x800F4734 (0x138): switches the BGM for the quest scene (the quest-start track on channel 1, or the
 * network track on channel 0). */
void snd_quest_scene_set(void) {
    BgmCtrl* work = bgm_ctrl;
    if (work == NULL) {
        return;
    }
    if (work->quest_end != 0) {
        return;
    }
    if (quest_play_state_ck() == 0) {
        snd_quest_result_bgm_set();
        return;
    }
    if (PlayMode_ck() != 2) {
        bgm_em_slots_stop();
        work->quest_pending = 1;
        work->env_state = 0;
    } else {
        work->quest_pending = 1;
    }
    if (PlayMode_ck() != 2) {
        for (s32 i = 1; i < 3; i++) {
            if (srt_ready_ck(i) != 0) {
                if (bgm_ch_playing_ck(i) == 1) {
                    bgm_ch_req(i, 1);
                }
                work->ch_track[i] = 0;
            }
        }
        if (srt_ready_ck(1) != 0) {
            work->ch_state[1] = 0;
            bgm_ch_req(1, 0x33);
            bgm_ch_req(1, 4);
        }
    } else if (srt_ready_ck(0) != 0) {
        bgm_ch_req(0, 0x47);
    }
}

/* 0x800F486C (0x50): the quest result: stops the monster slots and raises every pending flag. */
void snd_quest_result_bgm_set(void) {
    BgmCtrl* work = bgm_ctrl;
    if (work == NULL) {
        return;
    }
    bgm_em_slots_stop();
    work->quest_pending = 1;
    work->quest_end = 1;
    work->hunt_state = 1;
    work->hunt_timer = -1;
    work->clear_phase = 1;
}

/* 0x800F48BC (0x38): plays the current map's channel 1 track. */
void bgm_map_track_play(void) {
    bgm_ch_req(1, bgm_map_ch1_track_tbl[get_now_mapno()]);
}

/* 0x800F48F4 (0x174): holds the field BGM for the quest: stops channel 1 and plays the map's track. */
void snd_bgm_hold_set(void) {
    BgmCtrl* work = bgm_ctrl;
    if (work == NULL) {
        return;
    }
    if ((u8)event_demo_running_ck() != 0 && (u8)(get_demo_no() + 0xFF) <= 1) {
        return;
    }
    if (quest_slot_progress_get(NULL) == 1 && quest_phase_get() != 8 && quest_phase_get() != 12) {
        work->hold_state = 1;
        return;
    }
    if (quest_phase_get() == 6) {
        work->hold_state = 1;
        return;
    }
    if (srt_ready_ck(1) == 0) {
        return;
    }
    if (work->hold_state != 0) {
        return;
    }
    if ((u32)srt_ready_ck(0) == 1 && work->ch_state[0] == 2) {
        bgm_ch_req(0, 0x22);
    }
    work->hold_state = 1;
    if (bgm_ch_playing_ck(1) == 1) {
        bgm_ch_req(1, 1);
    }
    if ((move_work_state_ck() != 0 || (quest_phase_get() == 10 && get_now_areano() == 1) || get_now_areano() == 0) &&
        bgm_map_ch1_track_tbl[get_now_mapno()] != 0) {
        bgm_map_track_play();
    }
}

/* 0x800F4A68 (0x28): whether the field BGM is held. */
int snd_bgm_hold_ck(void) {
    BgmCtrl* work = bgm_ctrl;
    if (work == NULL) {
        return 0;
    }
    return work->hold_state != 0;
}

/* 0x800F4A90 (0x728): the area track frame on channel 0 (the area tables, the environment switch, the
 * post-kill steps and the battle check). */
void snd_quest_frame_begin(s32 mode, u8 phase, u8 sub) {
    BgmCtrl* work = bgm_ctrl;
    VEC3 pos;
    u8 tracks[2];
    VEC3_ctor(&pos);
    if (work == NULL) {
        return;
    }
    if (event_demo_ck() == 1) {
        return;
    }
    if (srt_ready_ck(0) == 0) {
        return;
    }
    if (snd_bgm_loading_get() == 1) {
        return;
    }
    Q_MoveWork* quest = (Q_MoveWork*)get_move_work_adrs(0);
    if (quest != NULL && quest->local_0x112 != 0) {
        return;
    }
    if (work->hold_timer != 0) {
        work->hold_timer--;
        return;
    }
    u8 map = get_now_mapno();
    u8 (*tbl)[2];
    if (map == 9) {
        if (sub == 0) {
            tbl = bgm_env_tbl[8];
        } else {
            tbl = bgm_env_tbl[11];
        }
        if (tbl == NULL) {
            return;
        }
        s32 count = system_w.field_0x8af == 0 ? 1 : 2;
        s32 alt = 0;
        for (s32 i = 0; i < count; i++) {
            if (Pl_area_flag_get(i) == 1) {
                alt = 1;
                break;
            }
        }
        tbl = &tbl[alt];
    } else {
        if (map == 0x15 && lobby_w.field_0x15F != 0) {
            tbl = bgm_env_lobby_alt_tbl;
        } else {
            tbl = bgm_env_tbl[map];
            if (tbl == NULL) {
                return;
            }
        }
        tbl = &tbl[sub];
    }
    tracks[0] = (*tbl)[0];
    tracks[1] = (*tbl)[1];
    if ((u8)(map + 0xEB) <= 1) {
        if ((u8)event_demo_running_ck() != 0) {
            return;
        }
        if (demo_flag_ck() == 1) {
            return;
        }
        if (work->env_state == 1) {
            if (work->ch_state[0] == 2) {
                work->env_timer = 0;
                work->env_state = 2;
            }
            work->env_timer++;
            if (work->env_timer > 300) {
                work->env_state = 0;
                work->env_track[0] = 0;
            }
        }
        if (work->env_state == 2 && (work->ch_state[0] == 0 || work->ch_state[0] == 4)) {
            work->env_state = 0;
            work->env_track[0] = 0;
        }
        if ((work->ch_state[0] == 0 || work->ch_state[0] == 4) &&
            (work->env_state == 0 || tracks[mode] == work->env_track[0])) {
            u8 track = tracks[mode];
            if (track != 0) {
                bgm_ch_req(0, track);
                bgm_ch_req(0, 4);
                work->ch_cur_track[0] = track;
                work->env_state = 1;
            } else {
                work->env_state = 3;
            }
            work->env_timer = 0;
            work->env_track[0] = track;
        } else if (work->env_track[0] != tracks[mode]) {
            u8 track = tracks[mode];
            if (track == 0) {
                bgm_ch_req(0, 1);
                work->env_state = 3;
            } else {
                bgm_ch_req(0, track);
                bgm_ch_req(0, 4);
                work->ch_cur_track[0] = track;
                work->env_state = 1;
            }
            work->env_timer = 0;
            work->env_track[0] = track;
        }
        if (map == 0x16) {
            work->env_area = 0;
            _PLW* players = (_PLW*)get_move_work_adrs(2);
            _PLW* player = players + (s8)my_player_no();
            if (player != NULL && sub == 2) {
                VEC3 spot;
                copyVec3(&pos, setVec3(&spot, 1542.0f, 406.5f, 554.0f));
                pos.y = player->motion_pos_0x40;
                if (calcVecDistXZ(&pos, &player->vec_0x03C) <= 420.0f) {
                    work->env_area = 1;
                }
            }
        }
        if (work->env_area != work->env_area_prev) {
            if (work->env_area == 0 && work->env_alt_on == 1) {
                bgm_ch_req(0, 0x23);
                bgm_ch_req(1, 0x23);
                work->env_alt_on = 0;
            }
            if (work->env_area == 1 && work->env_alt_on == 0) {
                bgm_ch_req(0, 0x22);
                bgm_ch_req(1, 0x22);
                work->env_alt_on = 1;
            }
        }
        return;
    }
    switch ((u32)work->hunt_state) {
    case 1:
        if (work->ch_state[0] == 2) {
            work->hunt_state = 2;
        }
    case 2:
        if (work->hunt_state == 2 && work->ch_state[0] == 4) {
            work->hunt_state = 0;
            work->ch_track[0] = 0;
        }
        if (work->ch_state[0] == 4) {
            work->ch_end_count[0]++;
            if (work->ch_end_count[0] >= 150) {
                work->hunt_state = 0;
                work->ch_track[0] = 0;
            }
        }
        if (work->hunt_state != 0 && work->hunt_timer > 0) {
            if (--work->hunt_timer <= 0) {
                work->hunt_state = 0;
                work->hunt_timer = 0;
            }
        }
        break;
    default:
        if (work->quest_end == 1) {
            break;
        }
        if (PlayMode_ck() != 2) {
            if (bgm_em_alert_ck() == 1) {
                if (work->ch_state[0] == 2) {
                    bgm_ch_req(0, 1);
                    work->ch_track[0] = 0;
                    work->env_state = 0;
                    work->env_track[0] = 0;
                }
                break;
            }
        } else if (bgm_em_alert_ck() == 1) {
            break;
        }
        if (work->ch_state[0] == 0 || work->ch_state[0] == 4) {
            if (work->env_state == 1) {
                if (bgm_em_alert_ck() == 0) {
                    work->env_state = 0;
                    work->env_track[0] = 0;
                }
                break;
            }
            work->env_area = 0xFF;
            work->env_timer = 0;
            work->env_track[0] = tracks[0];
        }
        work->env_area_prev = work->env_area;
        if (stage_water_area_ck() == 1) {
            if (camera_underwater_ck() == 1) {
                work->env_area = 1;
            } else {
                work->env_area = 0;
            }
        } else {
            work->env_area = 0;
            _PLW* players = (_PLW*)get_move_work_adrs(2);
            _PLW* player = players + (s8)my_player_no();
            if (player != NULL && (map == 1 || map == 12) && sub == 11) {
                VEC3 spot;
                copyVec3(&pos, setVec3(&spot, 5000.0f, 0.0f, 6000.0f));
                pos.y = player->motion_pos_0x40;
                if (calcVecDistXZ(&pos, &player->vec_0x03C) <= 1000.0f) {
                    work->env_area = 1;
                } else {
                    work->env_area = 0;
                }
            }
        }
        if (work->env_area != work->env_area_prev || tracks[work->env_area] != work->env_track[work->env_area]) {
            if (work->env_area == 0) {
                work->ch_state[0] = 0;
                bgm_ch_req(0, tracks[0]);
                bgm_ch_req(0, 4);
                work->env_track[0] = tracks[0];
                work->ch_cur_track[0] = tracks[0];
            } else {
                work->ch_state[0] = 0;
                bgm_ch_req(0, tracks[1]);
                bgm_ch_req(0, 4);
                work->env_track[1] = tracks[1];
                work->ch_cur_track[0] = tracks[1];
            }
            work->env_state = 1;
        }
        break;
    }
}

/* 0x800F51B8 (0x28): fades `channel` out while it plays. */
void bgm_ch_fade_out(s32 channel) {
    BgmCtrl* work = bgm_ctrl;
    if (work == NULL) {
        return;
    }
    if (work->ch_state[channel] != 2) {
        return;
    }
    bgm_ch_req(channel, 5);
}

/* 0x800F51E0 (0x28): stops channel 0 while it plays. */
void bgm_main_stop(void) {
    BgmCtrl* work = bgm_ctrl;
    if (work == NULL) {
        return;
    }
    if (work->ch_state[0] != 2) {
        return;
    }
    bgm_ch_req(0, 1);
}

/* 0x800F5208 (0x88): plays the jingle of game mode `mode` on channel 2. */
void bgm_mode_jingle_play(u8 mode) {
    if (bgm_ctrl == NULL) {
        return;
    }
    if (srt_ready_ck(2) == 0) {
        return;
    }
    s32 id;
    switch (mode) {
    case 4:
        id = 0x32;
        break;
    case 6:
        id = 0x33;
        break;
    case 7:
        id = 0x34;
        break;
    default:
        return;
    }
    bgm_ch_req(2, id);
}

/* 0x800F5290 (0x67C): the BGM frame: the lobby tracks and ambience in map 0x15, the area tracks elsewhere. */
void bgm_ctrl_frame(void) {
    BgmCtrl* work = bgm_ctrl;
    if (work == NULL) {
        return;
    }
    bgm_ch_status_update();
    u8 map = get_now_mapno();
    u8 area = get_now_areano();
    work->area_prev = work->area;
    work->area = area;
    if (map == 0x15) {
        if ((u32)NetCtrlWk::isServerSelectSubState() == 1) {
            for (s32 i = 0; i < 3; i++) {
                if (work->ch_state[i] != 0) {
                    bgm_ch_req(i, 1);
                }
            }
            return;
        }
        if (work->depart_flag == 0) {
            if (work->ch_state[1] == 0 || work->ch_state[1] == 4) {
                bgm_lobby_area_update(map, work->area, work->area_prev);
            } else {
                if (work->area != work->area_prev) {
                    bgm_lobby_area_update(map, work->area, work->area_prev);
                } else if ((work->ch_state[2] == 0 || work->ch_state[2] == 4) && work->depart_flag != 1 &&
                           (u8)(work->area + 0xFB) <= 2 && work->lobby_delay == 0) {
                    work->ch_cur_track[2] = 0;
                    bgm_lobby_area_update(map, work->area, work->area_prev);
                }
                if (work->depart_timer == 0) {
                    work->depart_flag = 0;
                } else {
                    work->depart_timer--;
                }
                if (work->depart_flag != 0) {
                    return;
                }
                if (work->lobby_delay > 0) {
                    work->lobby_delay--;
                }
            }
            switch (work->area) {
            case 2:
                if (work->ch_state[2] == 0 || work->ch_state[2] == 4) {
                    work->behind_flag = 0;
                } else {
                    if (work->ch_state[2] == 2 && work->area_prev != 2 && work->area == 2) {
                        bgm_ch_req(2, 4);
                    }
                    if (work->ch_state[2] == 2) {
                        StreamCmd cmd = bgm_lobby_cmd;
                        VEC3 spot;
                        setVec3(&spot, 0.0f, 154.0f, -620.0f);
                        f32 range = 2000.0f;
                        _PLW* player = (_PLW*)get_move_work_adrs(2);
                        if (player != NULL) {
                            player += (s8)my_player_no();
                            if (player->slot_active != 0) {
                                u8* curve = se_volume_curve_tbl[0];
                                f32 dist = calcVecDistXZ(&spot, &player->vec_0x03C);
                                if (dist > range) {
                                    dist = range;
                                }
                                s32 idx = 100.0f * (dist / range);
                                if (idx >= 100) {
                                    idx = 99;
                                }
                                work->volume[5] = curve[100 + idx];
                                cmd.volume = work->volume[5];
                                stream_cmd_play_locked(2, &cmd);
                            }
                        }
                    }
                    break;
                }
            default:
                if (work->ch_state[2] == 2) {
                    if (lobby_w.field_0x15F == 1) {
                        if (work->area_prev == 2 && work->area != 2) {
                            if (system_w.field_0x8b2 != 0) {
                                system_w.field_0x8b2 = 0;
                            }
                            bgm_ch_req(2, 1);
                        }
                    } else if (work->area_prev == 2 && work->area != 2) {
                        bgm_ch_req(2, 5);
                    }
                }
                {
                    s32 tries = 0;
                    if (work->ch_state[2] == 0 || work->ch_state[2] == 4) {
                        StreamCmd cmd;
                        stream_cmd_copy(&cmd, &bgm_lobby_cmd);
                        if (work->lobby_timer > 0) {
                            work->lobby_timer--;
                            break;
                        }
                        s32 pick;
                        do {
                            pick = ran_suu(0) % 7;
                            if (work->lobby_rand != (u8)pick) {
                                break;
                            }
                            tries++;
                        } while (tries <= 10);
                        work->lobby_rand = pick;
                        if (lobby_w.field_0x15F != 0) {
                            if (system_w.field_0x8b2 == 2) {
                                system_w.field_0x8b2 = 0;
                                return;
                            }
                            if (system_w.field_0x8b2 != 1) {
                                break;
                            }
                            work->lobby_rand = 7;
                            work->ch_cur_track[2] = 0x57;
                        } else {
                            system_w.field_0x8b2 = 0;
                            work->ch_cur_track[2] = pick + 0x50;
                        }
                        bgm_ch_req(2, work->ch_cur_track[2]);
                        work->lobby_timer = 90;
                        if (work->area != 2) {
                            bgm_ch_req(2, 5);
                            cmd.volume = 0;
                            stream_cmd_play_locked(2, &cmd);
                        }
                    } else if (work->ch_state[2] == 2 && system_w.field_0x8b2 == 1) {
                        system_w.field_0x8b2 = 2;
                    }
                }
                break;
            case 5:
            case 6:
            case 7:
                work->lobby_timer = 0;
                break;
            }
        }
    } else {
        if ((u32)lb_special_bgm_ck() == 1) {
            if (work->arena_flag == 0) {
                bgm_arena_track_start();
                return;
            }
        } else if (work->arena_flag == 1) {
            bgm_arena_track_stop();
            work->arena_flag = 0;
            return;
        }
        if (work->depart_timer > 0) {
            work->depart_timer--;
        }
        if (work->depart_flag != 0 && work->depart_timer == 0) {
            work->depart_flag = 0;
            bgm_lobby_area_update(map, work->area, work->area_prev);
        }
        if (work->ch_state[1] == 0 || work->ch_state[1] == 4) {
            if (work->depart_flag == 0) {
                bgm_lobby_area_update(map, work->area, work->area_prev);
            }
        } else if (work->area != work->area_prev) {
            bgm_lobby_area_update(map, work->area, work->area_prev);
        }
    }
    if (system_w.field_0x8b2 == 2) {
        return;
    }
    if (map == 0x16 && work->lobby_state != 2) {
        return;
    }
    if (map == 0x15 && work->lobby_state != 0) {
        return;
    }
    if (work->depart_timer > 0) {
        work->depart_timer--;
        return;
    }
    if (work->depart_flag != 0) {
        if (work->depart_timer == 0) {
            work->depart_flag = 0;
            bgm_lobby_area_update(map, work->area, work->area_prev);
        }
        return;
    }
    snd_quest_frame_begin(getItemListSelection(), map, work->area);
}

/* 0x800F590C (0x18): clears the "behind the scene" flag. */
void bgm_behind_flag_clear(void) {
    BgmCtrl* work = bgm_ctrl;
    if (work == NULL) {
        return;
    }
    work->behind_flag = 0;
}

/* 0x800F5924 (0x74): stops the lobby track on channel 1 and restarts the lobby track steps. */
void bgm_lobby_ch1_stop(void) {
    BgmCtrl* work = bgm_ctrl;
    if (work == NULL) {
        return;
    }
    if (event_demo_ck() == 1) {
        return;
    }
    if (get_now_mapno() == 0x15 && work->depart_flag != 0) {
        return;
    }
    bgm_ch_req(1, 1);
    work->ch_state[1] = 0;
    work->lobby_state = 1;
}

/* 0x800F5998 (0xBC): the quest departure: stops the lobby channel and waits before the next track. */
void bgm_lobby_depart_set(void) {
    BgmCtrl* work = bgm_ctrl;
    if (work == NULL) {
        return;
    }
    work->depart_timer = 450;
    if (get_now_mapno() == 0x15) {
        if (work->lobby_state != 0) {
            work->lobby_state = 0;
        }
        bgm_ch_req(2, 1);
        work->ch_state[2] = 0;
        work->depart_flag = 1;
        sysSE_stop(0x35);
        work->env_track[0] = 0;
        return;
    }
    work->depart_timer = 570;
    bgm_ch_req(1, 1);
    work->ch_state[1] = 0;
    work->depart_flag = 1;
    sysSE_stop(0x35);
}

/* 0x800F5A54 (0x50): starts the arena track 0x36 on channel 1. */
void bgm_arena_track_start(void) {
    BgmCtrl* work = bgm_ctrl;
    if (work == NULL) {
        return;
    }
    work->arena_flag = 1;
    work->ch_cur_track[1] = 0x36;
    bgm_ch_req(1, work->ch_cur_track[1]);
    bgm_ch_req(1, 4);
}

/* 0x800F5AA4 (0x24): drops the arena flag and stops channel 1. */
void bgm_arena_track_stop(void) {
    BgmCtrl* work = bgm_ctrl;
    if (work == NULL) {
        return;
    }
    work->arena_flag = 0;
    bgm_ch_req(1, 1);
}

/* 0x800F5AC8 (0x48): starts track 0x49 on channel 2. */
void bgm_lobby_return_req(void) {
    BgmCtrl* work = bgm_ctrl;
    if (work == NULL) {
        return;
    }
    work->ch_cur_track[2] = 0x49;
    bgm_ch_req(2, work->ch_cur_track[2]);
    bgm_ch_req(2, 4);
}

/* The lobby track for the current selection: 0x47 for the first item, 0x46 otherwise. */
static inline void bgm_lobby_pick(BgmCtrl* work) {
    if (getItemListSelection() == 0) {
        work->ch_cur_track[1] = 0x47;
    } else {
        work->ch_cur_track[1] = 0x46;
    }
}

/* 0x800F5B10 (0x824): picks the lobby tracks of map `map` when the area changes from `prev` to `area`. */
void bgm_lobby_area_update(u8 map, u8 area, u8 prev) {
    BgmCtrl* work = bgm_ctrl;
    if (work == NULL) {
        return;
    }
    if (srt_ready_ck(1) == 0) {
        return;
    }
    if (srt_ready_ck(2) == 0) {
        return;
    }
    if (system_w.field_0x8b2 != 0) {
        return;
    }
    if (map == 0x15 && (u32)NetCtrlWk::isServerSelectSubState() == 1) {
        for (s32 i = 0; i < 3; i++) {
            if (work->ch_state[i] != 0) {
                bgm_ch_req(i, 1);
            }
        }
        return;
    }
    if ((u8)event_demo_running_ck() != 0) {
        if ((u8)(get_demo_no() + 0xF4) <= 1) {
            if (lb_item_list_done_ck() == 1) {
                work->lobby_state = 2;
                work->item_sel = getItemListSelection();
                lb_item_list_done_clear();
            }
            return;
        }
    } else if (demo_flag_ck() == 1) {
        return;
    }
    if (map == 0x16) {
        if (lb_item_list_done_ck() == 0) {
            work->lobby_state = 2;
        } else {
            if (demo_flag_ck() == 1) {
                work->lobby_state = 2;
                work->item_sel = getItemListSelection();
            } else {
                work->lobby_state = 0;
            }
            lb_item_list_done_clear();
        }
    }
    switch (map) {
    case 0x16:
        switch (area) {
        case 0:
            if (prev == 1) {
                bgm_ch_req(1, 0x23);
                return;
            }
            bgm_ch_req(1, 1);
            if (kujira_event_over_ck() == 1) {
                if (work->lobby_state == 0) {
                    bgm_lobby_pick(work);
                    work->lobby_state = 1;
                    work->item_sel = getItemListSelection();
                } else {
                    work->ch_cur_track[1] = 0x35;
                }
            } else if (work->lobby_state == 0) {
                bgm_lobby_pick(work);
                work->lobby_state = 1;
                work->item_sel = getItemListSelection();
            } else if (getItemListSelection() == 0) {
                work->ch_cur_track[1] = 0x32;
            } else {
                work->ch_cur_track[1] = 0x33;
            }
            bgm_ch_req(1, work->ch_cur_track[1]);
            bgm_ch_req(1, 4);
            return;
        case 1: {
            s32 started = 0;
            if (work->ch_state[1] == 0 || work->ch_state[1] == 4) {
                bgm_ch_req(1, 1);
                if (kujira_event_over_ck() == 1) {
                    if (work->lobby_state == 0) {
                        bgm_lobby_pick(work);
                        work->lobby_state = 1;
                        work->item_sel = getItemListSelection();
                        started = 1;
                    } else {
                        work->ch_cur_track[1] = 0x35;
                    }
                } else if (work->lobby_state == 0) {
                    bgm_lobby_pick(work);
                    work->lobby_state = 1;
                    work->item_sel = getItemListSelection();
                    started = 1;
                } else if (work->item_sel == 0) {
                    work->ch_cur_track[1] = 0x32;
                } else {
                    work->ch_cur_track[1] = 0x33;
                }
                bgm_ch_req(1, work->ch_cur_track[1]);
            }
            if (started == 0) {
                bgm_ch_req(1, 0x22);
                return;
            }
            bgm_ch_req(1, 4);
            return;
        }
        case 2:
            bgm_ch_req(1, 1);
            if (work->lobby_state == 0) {
                bgm_lobby_pick(work);
                work->lobby_state = 1;
                work->item_sel = getItemListSelection();
            } else {
                work->ch_cur_track[1] = 0x34;
            }
            bgm_ch_req(1, work->ch_cur_track[1]);
            bgm_ch_req(1, 4);
            return;
        default:
            bgm_ch_req(1, 1);
            return;
        }
        break;
    case 0x15:
        if (work->lobby_state == 0) {
            switch (area) {
            case 2:
                if (lobby_w.field_0x15F == 0) {
                    bgm_ch_req(1, 2);
                    if (prev == 3) {
                        bgm_ch_req(1, 0x23);
                    }
                    return;
                }
                if (prev == 3) {
                    work->ch_cur_track[1] = 0x42;
                }
                if (prev == 1) {
                    work->ch_cur_track[1] = 0x42;
                }
                break;
            case 3:
                bgm_ch_req(1, 0x22);
                if ((u8)(prev + 0xFB) <= 2) {
                    bgm_ch_req(1, 3);
                }
                {
                    s32 track;
                    if (lobby_w.field_0x15F != 0) {
                        track = 0x42;
                    } else if (lobby_w.field_0x15C != 0) {
                        track = 0x3E;
                    } else {
                        track = getItemListSelection() == 0 ? 0x3C : 0x3D;
                    }
                    work->ch_cur_track[1] = track;
                }
                break;
            case 4:
                bgm_ch_req(1, 2);
                return;
            case 5:
                if (work->ch_track[2] != 0x3F) {
                    bgm_ch_req(1, 2);
                    bgm_ch_req(2, 1);
                    bgm_ch_req(2, 0x3F);
                    bgm_ch_req(2, 4);
                }
                work->ch_cur_track[2] = 0x3F;
                work->lobby_delay = 20;
                return;
            case 6:
                if (work->ch_track[2] != 0x40) {
                    bgm_ch_req(1, 2);
                    bgm_ch_req(2, 1);
                    bgm_ch_req(2, 0x40);
                    bgm_ch_req(2, 4);
                }
                work->ch_cur_track[2] = 0x40;
                work->lobby_delay = 20;
                return;
            case 7:
                if (work->ch_track[2] != 0x41) {
                    bgm_ch_req(1, 2);
                    bgm_ch_req(2, 1);
                    bgm_ch_req(2, 0x41);
                    bgm_ch_req(2, 4);
                }
                work->ch_cur_track[2] = 0x41;
                work->lobby_delay = 20;
                return;
            default:
                break;
            }
        }
        switch (work->lobby_state) {
        case 1:
            bgm_lobby_pick(work);
            work->item_sel = getItemListSelection();
            bgm_ch_req(1, work->ch_cur_track[1]);
            bgm_ch_req(1, 4);
            bgm_ch_req(2, 1);
        case 2:
        case 3:
            work->lobby_state++;
            return;
        case 4:
            if (work->ch_state[1] == 0 || work->ch_state[1] == 4) {
                work->lobby_state = 0;
                return;
            }
            break;
        default: {
            s32 track;
            if (lobby_w.field_0x15F != 0) {
                track = 0x42;
            } else if (lobby_w.field_0x15C != 0) {
                track = 0x3E;
            } else {
                track = getItemListSelection() == 0 ? 0x3C : 0x3D;
            }
            if (work->ch_track[1] != (u8)track) {
                bgm_ch_req(1, 1);
                bgm_ch_req(1, (u8)track);
                if (area == 3) {
                    bgm_ch_req(1, 0x22);
                } else {
                    bgm_ch_req(1, 4);
                }
                work->ch_cur_track[1] = track;
                if ((u8)(prev + 0xFB) <= 2) {
                    bgm_ch_req(2, 1);
                }
            } else {
                if (work->ch_state[1] == 3) {
                    bgm_ch_req(1, 3);
                }
                if ((u8)(prev + 0xFB) <= 2) {
                    bgm_ch_req(2, 1);
                }
                if (prev == 3) {
                    bgm_ch_req(1, 0x23);
                }
            }
            break;
        }
        }
        break;
    }
}

/* 0x800F6334 (0x4): stops every BGM stream. */
void bgm_stop_all(void) {
    resumeSoundEngine();
}

/* 0x800F6338 (0x7C): stops the playing channels 0 and 2. */
void bgm_env_ch_stop(void) {
    BgmCtrl* work = bgm_ctrl;
    if (work == NULL) {
        return;
    }
    for (s32 i = 0; i < 3; i++) {
        if (srt_ready_ck(i) == 0) {
            continue;
        }
        if (work->ch_state[i] != 2) {
            continue;
        }
        if (i == 1) {
            continue;
        }
        bgm_ch_req(i, 1);
    }
}

/* 0x800F63B4 (0x6C): raises the event flag (outside map kind 6 unless in area 2) with a 36-frame count. */
void bgm_event_flag_set(void) {
    BgmCtrl* work = bgm_ctrl;
    if (work == NULL) {
        return;
    }
    if (pl_map_kind_get() == 6 && get_now_areano() != 2) {
        return;
    }
    if (work->event_flag == 1) {
        return;
    }
    work->event_flag = 1;
    work->event_timer = 36;
}

/* 0x800F6420 (0x54): requests track 0x15 on channel 1 (in map kind 6 only in area 1). */
void bgm_track15_req(void) {
    BgmCtrl* work = bgm_ctrl;
    if (work == NULL) {
        return;
    }
    if (pl_map_kind_get() == 6 && get_now_areano() != 1) {
        return;
    }
    bgm_ch_req(1, 0x15);
}

/* 0x800F6474 (0xA0): stops the monster slot that plays for monster `unique`. */
void em_se_stop_all(u16 unique) {
    BgmCtrl* work = bgm_ctrl;
    if (work == NULL) {
        return;
    }
    for (s32 i = 0; i < 2; i++) {
        if (unique == work->em_unique[i]) {
            bgm_ch_req(i + 1, 1);
            work->ch_track[i + 1] = 0;
            work->em_req_track[i] = 0;
            work->em_state[i] = 0;
            work->em_flag_0x60[i] = 0;
            work->em_unique[i] = 0xFFFF;
            return;
        }
    }
}

/* 0x800F6514 (0xC): fades channel 0 out. */
void bgm_main_fade_out(void) {
    bgm_ch_req(0, 5);
}
