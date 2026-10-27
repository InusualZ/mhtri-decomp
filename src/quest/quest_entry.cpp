/*
 * quest/quest_entry.cpp - the quest system's core: the quest list loaders and `quest_init`, the lot picks and the
 *   pair/reward rolls, the monster spawns, the quest entry, the per-frame quest step and its intruder and successive
 *   hunts, and the result screen's record accessors and text getters.
 * RANGE. .text 0x803AB3BC-0x803B465C (102 functions); extab 0x80018B0C-0x80018DC4, extabindex 0x80038EF8-0x8003930C,
 *   .data 0x805F2A98-0x805F7C68, .bss 0x806C5558-0x806CC420, .sdata 0x807935D8-0x80793684, .sbss 0x80794C18-0x80794C48,
 *   .sdata2 0x8079C510-0x8079C560.  The head 0x803AA4A4-0x803AB3BC is `quest/quest_item_slot.cpp` (the `.data`
 *   emission-order seam of its static initialiser).  `enemy/em_pop.cpp`'s head 0x803B465C-0x803B6F2C reads this
 *   unit's private `.data` tables and pooled `.sdata2` constants, so it is this TU's tail (seam request quest-q1#27).
 * DATA. Emitted in retail order and byte-identical: `.data` (the tables above the bodies, then the bodies' literals
 *   and jump tables), `.bss` (the picked-pair tables, `quest_work`, `quest_text_buffer`, `nora_set_proc`), `.sbss`
 *   and `.sdata`.  Not emitted: the named `.sdata2` constants (playbook 29; part of the pool is read only by
 *   `enemy/em_pop.cpp`'s head, the seam above).  `quest_name_unavailable_text` (.data 0x8060E888, unclaimed, between
 *   `menu/menu_sysmsg.cpp` and `Gecko_ExceptionPPC.cp`) is declared in `unsplit/menu.h` (rule 12, claim request).
 * FLAGS. the `menu` lib's `cflags_menu` (`-O3 -inline noauto -opt nopeephole -Cpp_exceptions on`, Wii/1.3), plus
 *   `#pragma pool_data off` (retail addresses each table through its own symbol although they precede the bodies)
 *   and a file-wide `#pragma optimization_level 4` (idea 108: quest_init, quest_enter_load, quest_arena_data_step,
 *   quest_item_count_sum, quest_monsters_release, quest_monsters_spawn and quest_part_reward_roll rise under it);
 *   per function `opt_propagation off` (quest_enter_load, quest_arena_summary_step, quest_arena_time_text_get: the
 *   arena id's `- 9000` is then computed before the `system_w` column bool, as retail does) and `opt_lifetimes off`
 *   (quest_work_start_reset's callee-saved colours).
 * NAMES. Module `quest` from the dump's globals (`q_result_msg_adrs`, `em_bui_tbl`, `em_hokaku_rem_l/h`,
 *   `nora_set_proc`) and its one function name `quest_init(unsigned char)`.  GUESS: the file name and every function
 *   and field name derived from a body, among them quest_list_load_hunt/_arena, quest_record_find,
 *   quest_part_reward_roll (bui = part breaks), quest_capture_reward_roll (hokaku = captures), quest_monster_setup,
 *   quest_pair_apply, quest_monster_spawn_area, quest_monsters_spawn, quest_enter_load, quest_entry_setup (supply
 *   fields), quest_main_step, quest_intruder_step, quest_successive_step, quest_item_deposit, quest_intro_step,
 *   quest_entry_point_warp; the tables quest_id_threshold_tbl, quest_monster_variant_tbl, quest_area_monster_tbl,
 *   quest_time_class_tbl, quest_supply_drop_tbl, quest_bonus_pick_tbl; the constants quest_timer_zero_d/start_d,
 *   quest_intruder_wait_3f; the types QuestListBlock, QuestEntrySlot, QuestSpawnRec, QuestBossSpawn, NoraSetProc.
 *   GUESS (from each body and its callers): quest_part_reward_roll, quest_capture_reward_roll, quest_monster_setup
 *   GUESS: quest_list_load_hunt, quest_list_load_arena, quest_monsters_spawn_now, quest_monster_spawn_area
 *   GUESS: quest_monsters_spawn, quest_pair_apply, quest_element_copy, quest_pair_copy, quest_area_list_init
 *   GUESS: quest_area_list_refill, quest_start_load, quest_enter_load, quest_main_step, quest_entry_setup
 *   GUESS: quest_entry_active_ck, quest_entry_ready_ck, quest_move_sub_state_ck, quest_finish_step
 *   GUESS: quest_intruder_step, quest_successive_step, quest_record_find, quest_intro_step
 *   GUESS: quest_element_supply_state_get, quest_element_item_use, quest_item_deposit, quest_entry_point_warp
 *   GUESS: quest_result_keep_items, quest_result_deliver_items, quest_result_fill, quest_arena_summary_step
 *   GUESS: quest_element_value_set, quest_gallery_cell_step
 *   GUESS (data, by the table that points at them): quest_area_monsters_NN (quest_area_monster_tbl[NN]),
 *   quest_monster_variants_NN (quest_monster_variant_tbl[NN]), quest_variant_slots_NN_K (entry K of variant list NN),
 *   quest_supply_drop_NN (quest_supply_drop_tbl[NN]), quest_supply_drop_empty (its no-drop entries),
 *   quest_bonus_pick_NN (quest_bonus_pick_tbl[NN], first index), quest_bonus_items_N (the bonus lists' item rows).
 * RESIDUALS. Every row is written.
 *  - quest_main_step: the target shares its fail and clear tails between branches through backward jumps (a goto
 *    shape, rule 8).  The first two fail checks share one tail through one `||` condition (3068 B against 2832); the
 *    goal fail and the two clear sites still inline their tails (a merged clear condition re-tests `left` and scores
 *    lower, a flag-threaded loop keeps its flag tests - MWCC does not thread jumps).  Retail jumps backwards into
 *    blocks no loop heads, so only `goto` reaches them (playbook 213).
 *  - quest_intruder_step, quest_successive_step: retail re-tests the variant rank after its scan as `clrlwi` of
 *    the kept byte; ours reloads the byte (`lbz`); a u8/u16/s32/u32 `rank`, a `(u8)` re-test and a separate found
 *    byte do not reproduce it.
 *  - quest_part_reward_roll, quest_capture_reward_roll, quest_monsters_spawn, quest_init, quest_enter_load,
 *    quest_start_load: register colouring (callee-saved permutations) that declaration order, per-function
 *    `opt_*` pragmas and fresh loop variables do not reach.
 *  - quest_enter_load: retail keeps `stage * 8` across `stage_dcm_path_get`; ours recomputes it.
 *  - flipcheck: .sdata2 short of the claim (the em_pop seam); quest_main_step is not 100 %.
 * SHAPES. Declaration order decides the callee-saved colours (playbook 63): see quest_list_load_*, quest_pair_apply,
 *   quest_monster_setup, quest_item_deposit, quest_capture_reward_roll.  Loop counters the target keeps dead are
 *   written as their own variables (`u16 j` in quest_item_deposit, the threshold scan in quest_init).  The bodies are
 *   in address order; the `.data` tables sit above them, the `.sdata` rows too so the `"%s"` object is last; the switch
 *   arms of quest_result_field_text_get follow its jump table's order.  Out-params of `em_get_unique_work` are
 *   block-scoped per case (retail gives each call site its own stack pair).
 */

#include "quest/quest_entry.h"
#include "quest/quest_item_slot.h"
#include "ef/fn_800CDB2C.h"    /* `move_work_state_ck` - owned by ef/system_core.cpp (rule 2) */
#include "unsplit/menu.h"       /* `get_str_tbl`, `Screen_w` - the band's unowned callees and the screen block */
#include "hud/cockpit.h"
#include "unsplit/unknown.h"    /* `system_w` - the system block no registered unit claims */
#include "Network/network_pat_control.h"   /* isServerSelectState (owner header, rule 2) */
#include "enemy/em_pop.h"       /* `quest_flag_*_ck` - owned by enemy/em_pop.cpp (rule 2) */
#include "enemy/fn_801251D0.h"  /* `enemy_kind_same_ck` - owned by enemy/fn_801251D0.cpp (rule 2) */
#include "fn_8004CAD8.h"    /* get_qResult_work, userdata_record_*_add, userdata_quest_stat_set - owned by fn_8004CAD8.cpp (rule 2) */
#include "lobby/lb_quest_screen.h"  /* quest_rand_seed_set, quest_rand_next - owned by lobby/lb_quest_screen.cpp (rule 2) */
#include "pl.h"                 /* _PLW - the player work (rule 1) */
#include "lobby/lb_entry_notify_send.h" /* lb_entry_notify_send - owned by lobby/lb_companion_ui.cpp (rule 2) */
#include "unsplit/Runtime.PPCEABI.H.h" /* sprintf, strcat (the MSL band has no registered owner) */
#include "ai/fn_802D44F4.h"    /* ai_slots_clear - owned by ai/fn_802D44F4.cpp (rule 2) */
#include "fn_80056F24.h"        /* system_copy_filter_request - owned by fn_80056F24.cpp (rule 2) */
#include "sound/fn_800F2A94.h" /* snd_quest_scene_set - owned by sound/fn_800F2A94.cpp (rule 2) */
#include "lobby/lb_sub18_send.h" /* lb_sub18_send - owned by lobby/lb_companion_ui.cpp (rule 2) */
#include "ef/eft052.h"          /* hud_item_msg_push - owned by ef/eft052.cpp (rule 2) */
#include "enemy/enemy_control.h" /* em_area_entry_make, em_spawn_request - owned by enemy/enemy_control.cpp (rule 2) */
#include "enemy/fn_8013F764.h"   /* em_kind_release - owned by enemy/fn_8013F764.cpp (rule 2) */
#include "Pl/pl_act_stage_latch_set.h"  /* pl_act_stage_latch_set, my_player_work_get - owned by Pl/fn_80273B14.cpp (rule 2) */
#include "Pl/fn_80288CEC.h"   /* pl_warp_start - owned by Pl/fn_80288CEC.cpp (rule 2) */
#include "mh3_pad.h"          /* setVec3 (rule 2) */
#include "stage/stg_w.h"        /* stage_map_area_count_get - owned by stage/stg_w.cpp (rule 2) */
#include "sound/fn_800D7F54.h"  /* snd_item_fail_play - owned by sound/fn_800D7F54.cpp (rule 2) */
#include "Pl/Pl_master_ck.h"    /* Pl_master_ck - owned by Pl/pl_master.cpp (rule 2) */
#include "Pl/pl_skill.h"    /* Pl_Skill_ck, Pl_cat_skill_ck - owned by Pl/pl_skill.cpp (rule 2) */
#include "Runtime.PPCEABI.H/memset.h"  /* memset (owner: the Runtime.PPCEABI.H lib) */
#include "Runtime.PPCEABI.H/memcpy.h"  /* memcpy (owner: the Runtime.PPCEABI.H lib) */
#include "types.h"
#include "font/flfnt.h"                    /* msg_str_gen / flfntStrLen / flKnjMsgNumPtr (rule 2) */
#include "Pl/fn_80273B14.h"               /* `Pl_item_id_usable_ck`, the id-usable predicate (rule 2) */
#include "quest/quest_file_table.h"        /* quest_file_table (rule 2) */
#include "ef/system_core.h"                /* work_mem_alloc (rule 2) */
#include "Pl/pl_act.h"                     /* Pl_motion_input_ck (rule 2) */
#include "Pl/pl_item_add.h"                /* pl_item_add (rule 2) */
#include "lobby/lb_sub13_send.h"           /* lb_sub13_send (rule 2) */
#include "enemy/ENEMY_WORK.h"              /* EmGroundRec (rule 1) */
#include "enemy/fn_801251D0.h"             /* fn_80125F54 (rule 2) */
#include "unsplit/stage.h"                /* StageMapView, the view of stage_w (rule 1) */
#include "stage/stg_w.h"                   /* stage_w (rule 2) */
#include "unsplit/lobby.h"                 /* stage_dcm_buffer_tbl (camera/camera_main.cpp's, declared in the band) */
#include "lobby/lb_entry_start_default.h"  /* lb_entry_start_default (rule 2) */
#include "lobby/lb_entry_start_send.h"     /* lb_entry_start_send (rule 2) */
#include "lobby/lb_sub14_send.h"           /* lb_sub14_send (rule 2) */
#include "lobby/lb_sub16_send.h"           /* lb_sub16_send (rule 2) */
#include "lobby/lb_quest_work_active_ck.h" /* lb_quest_work_active_ck (rule 2) */
#include "NAND/nand.h"                     /* OSGetTime (rule 2) */
#include "lobby/lb_sub15_send.h"           /* lb_sub15_send (rule 2) */
#include "lobby/lb_sub1c_send.h"           /* lb_sub1c_send (rule 2) */
#include "lobby/quest_element_failed_ck.h" /* the lobby quest-screen element tests (rule 2) */
#include "lobby/event_demo_running_ck.h"   /* event_demo_running_ck (rule 2) */
#include "lobby/lb_event_request.h"        /* lb_event_request (rule 2) */
#include "menu/demo_play_ck.h"             /* demo_play_ck (rule 2) */
#include "pad_hold_ck.h"                   /* pad_hold_ck (rule 2) */
#include "ef/loading_caption_set.h"   /* loading_caption_set (rule 2) */
#include "camera/stage_dcm_path_get.h"     /* stage_dcm_path_get (rule 2) */
#include "ai/result_hunt_rank_get.h"       /* the result rank helpers (rule 2) */
#include "stage_map_set.h"                 /* stage_map_set (rule 2) */
#include "lobby/lb_quest_work_init.h"      /* lb_quest_work_init (rule 2) */
#include "sound/snd_bank_loader.h"         /* snd_quest_bgm_set/_load (rule 2) */
#include "mh3_pad/lb_param_w.h"            /* lb_param_w (rule 2) */
#include "enemy/em_quest_element_set.h"     /* em_quest_element_set (rule 2) */
#include "enemy/em_mot_finished_ck.h"      /* em_mot_finished_ck (rule 2) */


/* The band's quest-work pointer in this unit's own view of the record.  `quest_work_ptr` itself is
 * `.sbss` band data `quest/quest_entry.h` declares, and that header cannot take this unit's
 * offsets, so the two views are cast rather than merged. */
#define QUEST_WORK ((Q_ItemWork*)quest_work_ptr)

/* Retail addresses every table of this unit's `.data` through its own symbol (lis/addi), never through the
 * section's pooled base, although the tables are defined above the bodies (playbook 194). */
#pragma pool_data off

/* Retail is scheduled at optimization level 4: several rows only match under it, and none differs otherwise. */
#pragma optimization_level 4

/* This unit's own `.sdata` (`splits.txt` `.sdata 0x807935D0..0x8079367C`, the rows tile it in address order;
 * the twelve `quest_grade_time_*` rows and the two `quest_rank_weight_*` rows are separate objects because
 * `.sdata` only takes objects of at most 8 bytes).
 * Every row is referenced by this unit's code only, except the three bytes tables at 0x807935F0..0x80793600
 * and the 0x80793604 table, which no symbol reads (they sit inside the claim as unreferenced data). */

/* The zero-terminated resource id lists the entry state loads for quest kinds 7 (arena; the list starts one
 * entry in when the stat block is live), 0xB and 6, and the per-kind id table (indexed 1..5) it loads for a
 * quick quest. */
u8 quest_res_ids_arena[5] = { 0x2B, 0x2C, 0x2D, 0x2E, 0 };
u8 quest_res_ids_kind_b[4] = { 0x2F, 0x30, 0x31, 0 };
u8 quest_res_ids_kind_6[4] = { 0x28, 0x29, 0x2A, 0 };
u8 quest_res_id_by_kind[8] = { 0, 2, 0x0F, 0x10, 0x11, 0x12, 0, 0 };

/* Unreferenced rows (GUESS names: pairs of small thresholds, sole readers unknown). */
u8 quest_sdata_tbl_a[8] = { 0x08, 0x20, 0x06, 0x30, 0x04, 0x10, 0, 0 };
u8 quest_sdata_tbl_b[4] = { 0x64, 0, 0xFF, 0 };
u8 quest_sdata_tbl_c[4] = { 0x64, 1, 0xFF, 0 };

/* The four-byte chance table `quest_pair_roll_all` copies over the head of its 16-byte roll buffer before
 * the last two picks.  The row's sole referrer is that function. */
u8 quest_pair_chance_tbl_d[0x4] = { 32, 22, 22, 22 };

/* Twelve separate 8-byte rows (clear time, par time, 3000, 0) - one object each, which is what keeps them in
 * `.sdata`; no symbol reads them (GUESS names). */
u16 quest_grade_time_00[3] = { 0x00B4, 0x00F0, 0x0BB8 };
u16 quest_grade_time_01[3] = { 0x00D2, 0x012C, 0x0BB8 };
u16 quest_grade_time_02[3] = { 0x010E, 0x0168, 0x0BB8 };
u16 quest_grade_time_03[3] = { 0x010E, 0x0168, 0x0BB8 };
u16 quest_grade_time_04[3] = { 0x01A4, 0x021C, 0x0BB8 };
u16 quest_grade_time_05[3] = { 0x02D0, 0x0384, 0x0BB8 };
u16 quest_grade_time_06[3] = { 0x0258, 0x02D0, 0x0BB8 };
u16 quest_grade_time_07[3] = { 0x021C, 0x0294, 0x0BB8 };
u16 quest_grade_time_08[3] = { 0x0258, 0x02D0, 0x0BB8 };
u16 quest_grade_time_09[3] = { 0x02D0, 0x0348, 0x0BB8 };
u16 quest_grade_time_0A[3] = { 0x0258, 0x02D0, 0x0BB8 };
u16 quest_grade_time_0B[3] = { 0x03C0, 0x0474, 0x0BB8 };

/* The rank weights `quest_start_load` indexes by the quest's rank byte (0..6), and the four zero bytes after
 * them. */
u8 quest_rank_weight_tbl[7] = { 0, 100, 80, 60, 40, 20, 10 };

/* The quest grade each bonus-element mask maps to (`quest_grade_set`; sole referrer). */
u8 quest_grade_table[8] = { 0, 1, 1, 3, 1, 6, 5, 3 };

/* The `"%s"` format `quest_reward_faint_penalty` prints a player name through. */
char quest_entry_fmt_str[3] = "%s";

/* This unit's own `.data` (`splits.txt` `.data 0x805F2A98..0x805F7C68`), defined in the target's order above every
 * function so the string literals and jump tables the bodies emit follow it: the quest-id thresholds, the area
 * monster lists and the per-monster variant lists (each `QuestEntrySlot` row: monster, count, the element block
 * - its +0x08..+0x10 words are the spawn position's f32 bits, +0x14..+0x1C the rotation - the element index and
 * the element flags), the two area tables, the enemy-set area tables `enemy/em_pop.cpp` reads, the pair/byte/
 * time-class tables, the supply-drop and bonus-pick lists and the reward-roll tables. */
u16 quest_id_threshold_tbl[20] = {
    1, 100, 1000, 2000, 10000, 14000, 15000, 19000, 20000, 24000, 50000, 55000, 59000, 60000, 61000, 65500, 0, 0, 0, 0,
};
QuestEntrySlot quest_area_monsters_01[5] = {
    { 15, 1, { 0xFF, 0x07, 0, 0, 0, 0, 0, 0, 0xC5561000, 0xC53B8000, 0x4544E000, 0, 0x00002500, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 12, 1, { 0x10, 0x09, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 1, { 0, 0, 0 }, 0x101 },
    { 10, 255, { 0xFF, 0x09, 0x03, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
    { 10, 255, { 0xFF, 0x09, 0x03, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestEntrySlot quest_area_monsters_02[5] = {
    { 1, 1, { 0xFF, 0x06, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 12, 1, { 0x10, 0x05, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x0000036E, 0 }, 1, { 0, 0, 0 }, 0x101 },
    { 10, 255, { 0x03, 0x05, 0x03, 0, 0, 0, 0, 0, 0x43960000, 0x40800000, 0xC3530000, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
    { 10, 255, { 0x03, 0x05, 0x03, 0, 0, 0, 0, 0, 0x43E50000, 0x3F800000, 0xC465C000, 0, 0x00002400, 0 }, -1, { 0, 0, 0 }, 0 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestEntrySlot quest_area_monsters_03[13] = {
    { 1, 1, { 0xFF, 0x0A, 0, 0, 0, 0, 0, 0, 0xC4FA0000, 0xC3890000, 0x44C58000, 0, 0, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 12, 1, { 0x10, 0x09, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 1, { 0, 0, 0 }, 0x101 },
    { 10, 255, { 0x03, 0x03, 0x03, 0x01, 0, 0, 0, 0, 0xC4DAC000, 0x42600000, 0xC4904000, 0, 0x00002800, 0 }, -1, { 0, 0, 0 }, 0 },
    { 10, 255, { 0x03, 0x03, 0x03, 0x01, 0, 0, 0, 0, 0xC512E000, 0x42AA0000, 0xC4896000, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
    { 31, 255, { 0xFF, 0x05, 0x03, 0, 0, 0, 0, 0, 0x44948000, 0x41800000, 0x43F98000, 0, 0x00004000, 0 }, -1, { 0, 0, 0 }, 0 },
    { 31, 255, { 0xFF, 0x05, 0x03, 0, 0, 0, 0, 0, 0x4481A000, 0x41800000, 0x43978000, 0, 0x0000BC00, 0 }, -1, { 0, 0, 0 }, 0 },
    { 17, 255, { 0x06, 0x06, 0x03, 0, 0, 0, 0, 0, 0x43000000, 0xC23C0000, 0x452DB000, 0, 0x0000E000, 0 }, -1, { 0, 0, 0 }, 0 },
    { 17, 255, { 0x06, 0x06, 0x03, 0, 0, 0, 0, 0, 0xC44FC000, 0xC23C0000, 0x44100000, 0, 0x00001800, 0 }, -1, { 0, 0, 0 }, 0 },
    { 10, 255, { 0x09, 0x09, 0x03, 0, 0, 0, 0, 0, 0xC3828000, 0x40E00000, 0x4487E000, 0, 0x0000B400, 0 }, -1, { 0, 0, 0 }, 0 },
    { 10, 255, { 0x09, 0x09, 0x03, 0, 0, 0, 0, 0, 0x43C68000, 0x41100000, 0x441E0000, 0, 0x00009800, 0 }, -1, { 0, 0, 0 }, 0 },
    { 31, 255, { 0xFF, 0x0A, 0x03, 0, 0, 0, 0, 0, 0xC4C22000, 0xC33A0000, 0x44BC0000, 0, 0x0000EC00, 0 }, -1, { 0, 0, 0 }, 0 },
    { 31, 255, { 0xFF, 0x0A, 0x03, 0, 0, 0, 0, 0, 0xC4A90000, 0xC32F0000, 0x44B44000, 0, 0x00007800, 0 }, -1, { 0, 0, 0 }, 0 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestEntrySlot quest_area_monsters_04[5] = {
    { 6, 1, { 0xFF, 0x06, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 12, 1, { 0x10, 0x05, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x0000036E, 0 }, 1, { 0, 0, 0 }, 0x101 },
    { 10, 255, { 0x03, 0x05, 0x03, 0, 0, 0, 0, 0, 0x43960000, 0x40800000, 0xC3530000, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
    { 10, 255, { 0x03, 0x05, 0x03, 0, 0, 0, 0, 0, 0x43E50000, 0x3F800000, 0xC465C000, 0, 0x00002400, 0 }, -1, { 0, 0, 0 }, 0 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestEntrySlot quest_area_monsters_05[13] = {
    { 2, 1, { 0xFF, 0x0A, 0, 0, 0, 0, 0, 0, 0xC4FA0000, 0xC3890000, 0x44C58000, 0, 0, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 12, 1, { 0x10, 0x09, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 1, { 0, 0, 0 }, 0x101 },
    { 10, 255, { 0x03, 0x03, 0x03, 0x01, 0, 0, 0, 0, 0xC4DAC000, 0x42600000, 0xC4904000, 0, 0x00002800, 0 }, -1, { 0, 0, 0 }, 0 },
    { 10, 255, { 0x03, 0x03, 0x03, 0x01, 0, 0, 0, 0, 0xC512E000, 0x42AA0000, 0xC4896000, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
    { 31, 255, { 0xFF, 0x05, 0x03, 0, 0, 0, 0, 0, 0x44948000, 0x41800000, 0x43F98000, 0, 0x00004000, 0 }, -1, { 0, 0, 0 }, 0 },
    { 31, 255, { 0xFF, 0x05, 0x03, 0, 0, 0, 0, 0, 0x4481A000, 0x41800000, 0x43978000, 0, 0x0000BC00, 0 }, -1, { 0, 0, 0 }, 0 },
    { 17, 255, { 0x06, 0x06, 0x03, 0, 0, 0, 0, 0, 0x43000000, 0xC23C0000, 0x452DB000, 0, 0x0000E000, 0 }, -1, { 0, 0, 0 }, 0 },
    { 17, 255, { 0x06, 0x06, 0x03, 0, 0, 0, 0, 0, 0xC44FC000, 0xC23C0000, 0x44100000, 0, 0x00001800, 0 }, -1, { 0, 0, 0 }, 0 },
    { 10, 255, { 0x09, 0x09, 0x03, 0, 0, 0, 0, 0, 0xC3828000, 0x40E00000, 0x4487E000, 0, 0x0000B400, 0 }, -1, { 0, 0, 0 }, 0 },
    { 10, 255, { 0x09, 0x09, 0x03, 0, 0, 0, 0, 0, 0x43C68000, 0x41100000, 0x441E0000, 0, 0x00009800, 0 }, -1, { 0, 0, 0 }, 0 },
    { 31, 255, { 0xFF, 0x0A, 0x03, 0, 0, 0, 0, 0, 0xC4C22000, 0xC33A0000, 0x44BC0000, 0, 0x0000EC00, 0 }, -1, { 0, 0, 0 }, 0 },
    { 31, 255, { 0xFF, 0x0A, 0x03, 0, 0, 0, 0, 0, 0xC4A90000, 0xC32F0000, 0x44B44000, 0, 0x00007800, 0 }, -1, { 0, 0, 0 }, 0 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestEntrySlot quest_area_monsters_06[14] = {
    { 3, 1, { 0xFF, 0x05, 0, 0, 0, 0, 0, 0, 0x43480000, 0x40800000, 0x43BE0000, 0, 0, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 1, 1, { 0xFF, 0x0A, 0x03, 0, 0, 0, 0, 0, 0xC4FA0000, 0xC3890000, 0x44C58000, 0, 0, 0 }, 1, { 0, 0, 0 }, 0x101 },
    { 10, 255, { 0xFF, 0x02, 0x03, 0x01, 0, 0, 0, 0, 0xC4BB6000, 0xC1E00000, 0xC4D94000, 0, 0x0000F000, 0 }, -1, { 0, 0, 0 }, 0 },
    { 10, 255, { 0xFF, 0x02, 0x03, 0x01, 0, 0, 0, 0, 0xC4900000, 0xC1E00000, 0xC4EF2000, 0, 0x00002C00, 0 }, -1, { 0, 0, 0 }, 0 },
    { 10, 255, { 0xFF, 0x03, 0x03, 0x01, 0, 0, 0, 0, 0xC4FF8000, 0x42920000, 0xC4984000, 0, 0x0000FC00, 0 }, -1, { 0, 0, 0 }, 0 },
    { 10, 255, { 0xFF, 0x03, 0x03, 0x01, 0, 0, 0, 0, 0xC4B66000, 0x42000000, 0xC463C000, 0, 0x00002400, 0 }, -1, { 0, 0, 0 }, 0 },
    { 31, 255, { 0xFF, 0x05, 0x03, 0, 0, 0, 0, 0, 0x448E4000, 0x41800000, 0x44050000, 0, 0x0000E000, 0 }, -1, { 0, 0, 0 }, 0 },
    { 31, 255, { 0xFF, 0x05, 0x03, 0, 0, 0, 0, 0, 0x44844000, 0x41800000, 0x439C8000, 0, 0x00009C00, 0 }, -1, { 0, 0, 0 }, 0 },
    { 10, 255, { 0xFF, 0x09, 0x03, 0, 0, 0, 0, 0, 0x43900000, 0x41C80000, 0x44D36000, 0, 0x00007C00, 0 }, -1, { 0, 0, 0 }, 0 },
    { 10, 255, { 0xFF, 0x09, 0x03, 0, 0, 0, 0, 0, 0x429C0000, 0x41C00000, 0x44D36000, 0, 0x00009C00, 0 }, -1, { 0, 0, 0 }, 0 },
    { 31, 255, { 0xFF, 0x0A, 0x03, 0, 0, 0, 0, 0, 0xC4BEA000, 0xC3500000, 0x44988000, 0, 0x00008C00, 0 }, -1, { 0, 0, 0 }, 0 },
    { 31, 255, { 0xFF, 0x0A, 0x03, 0, 0, 0, 0, 0, 0xC4C44000, 0xC3430000, 0x44AD2000, 0, 0x0000EC00, 0 }, -1, { 0, 0, 0 }, 0 },
    { 7, 1, { 0xFF, 0x05, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 1, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestEntrySlot quest_area_monsters_07[14] = {
    { 3, 1, { 0xFF, 0x06, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 8, 1, { 0xFF, 0x03, 0x03, 0, 0, 0, 0, 0, 0xC4484000, 0xC1A00000, 0x44D94000, 0, 0, 0 }, 1, { 0, 0, 0 }, 0x101 },
    { 31, 255, { 0xFF, 0x02, 0x03, 0, 0, 0, 0, 0, 0xC4A16000, 0xC1700000, 0x43DF8000, 0, 0x00005C00, 0 }, -1, { 0, 0, 0 }, 0 },
    { 31, 255, { 0xFF, 0x02, 0x03, 0, 0, 0, 0, 0, 0xC4BE4000, 0x40400000, 0x43B50000, 0, 0x0000D800, 0 }, -1, { 0, 0, 0 }, 0 },
    { 10, 255, { 0xFF, 0x03, 0x03, 0x01, 0, 0, 0, 0, 0x45095000, 0x42500000, 0xC4A6A000, 0, 0x0000C000, 0 }, -1, { 0, 0, 0 }, 0 },
    { 10, 255, { 0xFF, 0x03, 0x03, 0x01, 0, 0, 0, 0, 0x44F92000, 0x42AE0000, 0xC4494000, 0, 0x0000E000, 0 }, -1, { 0, 0, 0 }, 0 },
    { 10, 255, { 0x03, 0x05, 0x03, 0, 0, 0, 0, 0, 0xC38D0000, 0x40000000, 0xC4A66000, 0, 0x0000EC00, 0 }, -1, { 0, 0, 0 }, 0 },
    { 10, 255, { 0x03, 0x05, 0x03, 0, 0, 0, 0, 0, 0x437D0000, 0x40000000, 0xC47F8000, 0, 0x00001800, 0 }, -1, { 0, 0, 0 }, 0 },
    { 31, 255, { 0xFF, 0x06, 0x03, 0, 0, 0, 0, 0, 0xC52FC000, 0xC2B40000, 0x444D8000, 0, 0x00008400, 0 }, -1, { 0, 0, 0 }, 0 },
    { 31, 255, { 0xFF, 0x06, 0x03, 0, 0, 0, 0, 0, 0xC53DB000, 0xC2CC0000, 0x44504000, 0, 0x0000C800, 0 }, -1, { 0, 0, 0 }, 0 },
    { 31, 255, { 0xFF, 0x0A, 0x03, 0, 0, 0, 0, 0, 0xC34E0000, 0xC2440000, 0x44A90000, 0, 0x00005400, 0 }, -1, { 0, 0, 0 }, 0 },
    { 31, 255, { 0xFF, 0x0A, 0x03, 0, 0, 0, 0, 0, 0xC3DE0000, 0xC1E80000, 0x44B14000, 0, 0x00009400, 0 }, -1, { 0, 0, 0 }, 0 },
    { 12, 1, { 0x10, 0x05, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x0000036E, 0 }, 1, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestEntrySlot quest_area_monsters_08[5] = {
    { 8, 1, { 0xFF, 0x03, 0, 0, 0, 0, 0, 0, 0xC4484000, 0xC1A00000, 0x44D94000, 0, 0, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 12, 1, { 0x10, 0x05, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x0000036E, 0 }, 1, { 0, 0, 0 }, 0x101 },
    { 10, 255, { 0x03, 0x05, 0x03, 0, 0, 0, 0, 0, 0x43960000, 0x40800000, 0xC3530000, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
    { 10, 255, { 0x03, 0x05, 0x03, 0, 0, 0, 0, 0, 0x43E50000, 0x3F800000, 0xC465C000, 0, 0x00002400, 0 }, -1, { 0, 0, 0 }, 0 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestEntrySlot quest_area_monsters_09[2] = {
    { 16, 1, { 0xFF, 0x06, 0, 0, 0, 0, 0, 0, 0xC343999A, 0xC23C0000, 0xC4F00000, 0, 0x0000D000, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestEntrySlot quest_area_monsters_13[28] = {
    { 4, 1, { 0xFF, 0x04, 0, 0, 0, 0, 0, 0, 0x45449333, 0x411B3333, 0x43A7A666, 0, 0x0000B000, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 14, 1, { 0x10, 0x05, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 1, { 0, 0, 0 }, 0x101 },
    { 13, 255, { 0x01, 0x01, 0x03, 0, 0, 0, 0, 0, 0xC4A98000, 0x42000000, 0xC4B74000, 0, 0x0000F000, 0 }, -1, { 0, 0, 0 }, 0 },
    { 13, 255, { 0x01, 0x01, 0x03, 0, 0, 0, 0, 0, 0xC4816000, 0x42300000, 0xC4EDE000, 0, 0x00001C00, 0 }, -1, { 0, 0, 0 }, 0 },
    { 13, 255, { 0x02, 0x02, 0x03, 0, 0, 0, 0, 0, 0xC5380000, 0x40C00000, 0xC4104000, 0, 0x00002C00, 0 }, -1, { 0, 0, 0 }, 0 },
    { 13, 255, { 0x02, 0x02, 0x03, 0, 0, 0, 0, 0, 0xC5287000, 0x40000000, 0xC48FC000, 0, 0x00005800, 0 }, -1, { 0, 0, 0 }, 0 },
    { 13, 255, { 0x03, 0x03, 0x03, 0, 0, 0, 0, 0, 0x4597E000, 0x40A00000, 0xC427C000, 0, 0x0000E400, 0 }, -1, { 0, 0, 0 }, 0 },
    { 13, 255, { 0x03, 0x03, 0x03, 0, 0, 0, 0, 0, 0x4589B800, 0x40A00000, 0xC4A4C000, 0, 0x0000B000, 0 }, -1, { 0, 0, 0 }, 0 },
    { 26, 255, { 0xFF, 0x04, 0x03, 0, 0, 0, 0, 0, 0x4579A000, 0x80000000, 0xC4A4A000, 0, 0x00004400, 0 }, -1, { 0, 0, 0 }, 0 },
    { 26, 255, { 0xFF, 0x04, 0x03, 0, 0, 0, 0, 0, 0x4579E000, 0x80000000, 0xC48FC000, 0, 0x00002800, 0 }, -1, { 0, 0, 0 }, 0 },
    { 26, 255, { 0xFF, 0x04, 0x03, 0, 0, 0, 0, 0, 0x456D2000, 0, 0xC47CC000, 0, 0x00000800, 0 }, -1, { 0, 0, 0 }, 0 },
    { 35, 255, { 0xFF, 0x04, 0x03, 0x01, 0, 0, 0, 0, 0xC5413000, 0xBF800000, 0x4486E000, 0, 0x0000E400, 0 }, -1, { 0, 0, 0 }, 0 },
    { 35, 255, { 0xFF, 0x04, 0x03, 0x01, 0, 0, 0, 0, 0x456BC000, 0x40A00000, 0xC49F2000, 0, 0x0000E400, 0 }, -1, { 0, 0, 0 }, 0 },
    { 13, 255, { 0x05, 0x05, 0x03, 0, 0, 0, 0, 0, 0x44004000, 0x42000000, 0xC4BAA000, 0, 0x0000AC00, 0 }, -1, { 0, 0, 0 }, 0 },
    { 13, 255, { 0x05, 0x05, 0x03, 0, 0, 0, 0, 0, 0x43EA0000, 0x41A80000, 0xC45A0000, 0, 0x0000E800, 0 }, -1, { 0, 0, 0 }, 0 },
    { 26, 255, { 0xFF, 0x05, 0x03, 0, 0, 0, 0, 0, 0xC49E6000, 0x40800000, 0xC53C8000, 0, 0x00001400, 0 }, -1, { 0, 0, 0 }, 0 },
    { 26, 255, { 0xFF, 0x05, 0x03, 0, 0, 0, 0, 0, 0x4427C000, 0x42100000, 0xC4C06000, 0, 0x0000A000, 0 }, -1, { 0, 0, 0 }, 0 },
    { 26, 255, { 0xFF, 0x05, 0x03, 0, 0, 0, 0, 0, 0xC4ADC000, 0x421C0000, 0xC3AE8000, 0, 0x0000B000, 0 }, -1, { 0, 0, 0 }, 0 },
    { 35, 255, { 0xFF, 0x05, 0x03, 0, 0, 0, 0, 0, 0x43CE8000, 0x42140000, 0x4526D000, 0, 0x00009C00, 0 }, -1, { 0, 0, 0 }, 0 },
    { 35, 255, { 0xFF, 0x05, 0x03, 0, 0, 0, 0, 0, 0xC40DC000, 0x40400000, 0xC5740000, 0, 0x00001000, 0 }, -1, { 0, 0, 0 }, 0 },
    { 13, 255, { 0x06, 0x06, 0x03, 0, 0, 0, 0, 0, 0xC41E0000, 0xC0400000, 0xC4C2A000, 0, 0x00001400, 0 }, -1, { 0, 0, 0 }, 0 },
    { 13, 255, { 0x06, 0x06, 0x03, 0, 0, 0, 0, 0, 0xC48AC000, 0xC0400000, 0xC48A4000, 0, 0x0000F400, 0 }, -1, { 0, 0, 0 }, 0 },
    { 26, 255, { 0xFF, 0x06, 0x03, 0, 0, 0, 0, 0, 0x44150000, 0x3F800000, 0x44A2C000, 0, 0x00006800, 0 }, -1, { 0, 0, 0 }, 0 },
    { 26, 255, { 0xFF, 0x06, 0x03, 0, 0, 0, 0, 0, 0x42960000, 0x41100000, 0xC492A000, 0, 0x00005000, 0 }, -1, { 0, 0, 0 }, 0 },
    { 26, 255, { 0xFF, 0x06, 0x03, 0, 0, 0, 0, 0, 0xC50EE000, 0xC1200000, 0x43330000, 0, 0x00005000, 0 }, -1, { 0, 0, 0 }, 0 },
    { 35, 255, { 0xFF, 0x06, 0x03, 0, 0, 0, 0, 0, 0xC55BA000, 0x41B00000, 0x4492C000, 0, 0x00008400, 0 }, -1, { 0, 0, 0 }, 0 },
    { 35, 255, { 0xFF, 0x06, 0x03, 0, 0, 0, 0, 0, 0x44DA2000, 0xBF800000, 0x4419C000, 0, 0x00008400, 0 }, -1, { 0, 0, 0 }, 0 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestEntrySlot quest_area_monsters_12[2] = {
    { 18, 1, { 0xFF, 0x03, 0, 0, 0, 0, 0, 0, 0x452A0000, 0xC47A0000, 0x44D9E000, 0, 0x0000C400, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestEntrySlot quest_area_monsters_10[2] = {
    { 25, 1, { 0xFF, 0x01, 0, 0, 0, 0, 0, 0, 0x45A8C000, 0, 0x4528C000, 0, 0x00008000, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestEntrySlot quest_area_monsters_11[2] = {
    { 1, 1, { 0xFF, 0x01, 0, 0, 0, 0, 0, 0, 0x4504C333, 0xC215999A, 0x4438C000, 0, 0, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestEntrySlot quest_area_monsters_14[2] = {
    { 20, 1, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0x450FE000, 0xC514CB33, 0xC5936733, 0, 0xFFFFC889, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestEntrySlot quest_area_monsters_15[17] = {
    { 9, 1, { 0xFF, 0x05, 0, 0, 0, 0, 0, 0, 0xC40D3852, 0x423D3D71, 0x437CCCCD, 0, 0x0000A000, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 0, { 0, 0, 0 }, 0 },
    { 40, 255, { 0xFF, 0x04, 0x03, 0x08, 0, 0, 0, 0, 0x4533B000, 0xC1400000, 0x45163000, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
    { 40, 255, { 0xFF, 0x04, 0x03, 0x08, 0, 0, 0, 0, 0x43D80000, 0x42080000, 0x455F8000, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
    { 40, 255, { 0xFF, 0x04, 0x03, 0x08, 0, 0, 0, 0, 0xC4856000, 0x40E00000, 0x44CC0000, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
    { 40, 255, { 0xFF, 0x04, 0x03, 0x08, 0, 0, 0, 0, 0x446DC000, 0x42140000, 0xC41D8000, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
    { 40, 255, { 0xFF, 0x04, 0x03, 0x08, 0, 0, 0, 0, 0xC495E000, 0x41F00000, 0xC4DDE000, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
    { 40, 255, { 0xFF, 0x05, 0x03, 0x08, 0, 0, 0, 0, 0xC4778000, 0x42B60000, 0x4457C000, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
    { 40, 255, { 0xFF, 0x05, 0x03, 0x08, 0, 0, 0, 0, 0x456BD000, 0x422C0000, 0xC5140000, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
    { 40, 255, { 0xFF, 0x05, 0x03, 0x08, 0, 0, 0, 0, 0x445C0000, 0x42700000, 0xC4B74000, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
    { 40, 255, { 0xFF, 0x06, 0x03, 0x08, 0, 0, 0, 0, 0xC3180000, 0xC2200000, 0x44D38000, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
    { 40, 255, { 0xFF, 0x06, 0x03, 0x08, 0, 0, 0, 0, 0x44E24000, 0x41100000, 0xC28A0000, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
    { 40, 255, { 0xFF, 0x06, 0x03, 0x08, 0, 0, 0, 0, 0xC484A000, 0x3F800000, 0xC4FAE000, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
    { 40, 255, { 0xFF, 0x07, 0x03, 0x08, 0, 0, 0, 0, 0x44F8A000, 0x41A80000, 0x43520000, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
    { 40, 255, { 0xFF, 0x07, 0x03, 0x08, 0, 0, 0, 0, 0x44158000, 0x41600000, 0x44D78000, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
    { 40, 255, { 0xFF, 0x07, 0x03, 0x08, 0, 0, 0, 0, 0xC4728000, 0x41E00000, 0xC424C000, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestEntrySlot quest_area_monsters_16[55] = {
    { 19, 1, { 0xFF, 0x07, 0, 0, 0, 0, 0, 0, 0x44926000, 0, 0xC47C0000, 0, 0, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 34, 4, { 0xFF, 0x01, 0x01, 0, 0, 0, 0, 0, 0x44826000, 0x42240000, 0xC3930000, 0, 0x0000E400, 0 }, -1, { 0, 0, 0 }, 0 },
    { 34, 2, { 0xFF, 0x01, 0x01, 0x01, 0, 0, 0, 0, 0xC27C0000, 0x41D00000, 0xC2BC0000, 0, 0x00005C00, 0 }, -1, { 0, 0, 0 }, 0 },
    { 34, 1, { 0xFF, 0x01, 0x01, 0x01, 0, 0, 0, 0, 0x429C0000, 0x41F00000, 0xC4838000, 0, 0x0000E000, 0 }, -1, { 0, 0, 0 }, 0 },
    { 27, 255, { 0x02, 0x02, 0x01, 0, 0, 0, 0, 0, 0xC488C000, 0x42A80000, 0x45152000, 0, 0x0000A000, 0 }, -1, { 0, 0, 0 }, 0 },
    { 27, 255, { 0x02, 0x02, 0, 0x01, 0, 0, 0, 0, 0xC4FB0000, 0x42C40000, 0x4483E000, 0, 0x00005000, 0 }, -1, { 0, 0, 0 }, 0 },
    { 27, 255, { 0x02, 0x02, 0x01, 0x02, 0, 0, 0, 0, 0xC4B78000, 0x42440000, 0x43F08000, 0, 0x0000BC00, 0 }, -1, { 0, 0, 0 }, 0 },
    { 33, 4, { 0xFF, 0x02, 0x01, 0x01, 0, 0, 0, 0, 0x45215000, 0xC28E0000, 0x44CEC000, 0, 0x0000C800, 0 }, -1, { 0, 0, 0 }, 0 },
    { 33, 3, { 0xFF, 0x02, 0x01, 0x01, 0, 0, 0, 0, 0x4534D000, 0xC2DA0000, 0x449AE000, 0, 0x0000A400, 0 }, -1, { 0, 0, 0 }, 0 },
    { 33, 2, { 0xFF, 0x02, 0x01, 0x01, 0, 0, 0, 0, 0x453AF000, 0xC1F80000, 0x44D82000, 0, 0x0000E800, 0 }, -1, { 0, 0, 0 }, 0 },
    { 39, 255, { 0xFF, 0x03, 0x01, 0x01, 0, 0, 0, 0, 0x46078400, 0xC4704000, 0x451A0000, 0x0000C000, 0x00007800, 0 }, -1, { 0, 0, 0 }, 0 },
    { 29, 4, { 0xFF, 0x04, 0x01, 0, 0, 0, 0, 0, 0x44E04000, 0xC0400000, 0x44D84000, 0, 0x0000D000, 0 }, -1, { 0, 0, 0 }, 0 },
    { 29, 2, { 0xFF, 0x04, 0x01, 0, 0, 0, 0, 0, 0x44E0C000, 0x41900000, 0x4555E000, 0, 0x00005000, 0 }, -1, { 0, 0, 0 }, 0 },
    { 29, 1, { 0xFF, 0x04, 0x01, 0, 0, 0, 0, 0, 0xC3F50000, 0x42040000, 0x41F00000, 0, 0x00005C00, 0 }, -1, { 0, 0, 0 }, 0 },
    { 40, 255, { 0xFF, 0x04, 0x03, 0x08, 0, 0, 0, 0, 0x4533B000, 0xC1400000, 0x45163000, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
    { 40, 255, { 0xFF, 0x04, 0x03, 0x08, 0, 0, 0, 0, 0x43D80000, 0x42080000, 0x455F8000, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
    { 40, 255, { 0xFF, 0x04, 0x03, 0x08, 0, 0, 0, 0, 0xC4856000, 0x40E00000, 0x44CC0000, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
    { 40, 255, { 0xFF, 0x04, 0x03, 0x08, 0, 0, 0, 0, 0x446DC000, 0x42140000, 0xC41D8000, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
    { 40, 255, { 0xFF, 0x04, 0x03, 0x08, 0, 0, 0, 0, 0xC495E000, 0x41F00000, 0xC4DDE000, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
    { 39, 6, { 0xFF, 0x05, 0x01, 0, 0, 0, 0, 0, 0xC42A8000, 0x43520000, 0xC46C4000, 0, 0x0000D400, 0 }, -1, { 0, 0, 0 }, 0 },
    { 39, 2, { 0xFF, 0x05, 0x01, 0, 0, 0, 0, 0, 0xC413C000, 0x43870000, 0xC36F0000, 0, 0x00005400, 0 }, -1, { 0, 0, 0 }, 0 },
    { 39, 2, { 0xFF, 0x05, 0x01, 0x02, 0, 0, 0, 0, 0xC4438000, 0x43D20000, 0xC30E0000, 0, 0x00006C00, 0 }, -1, { 0, 0, 0 }, 0 },
    { 39, 1, { 0xFF, 0x05, 0x01, 0, 0, 0, 0, 0, 0xC3800000, 0x43960000, 0xC3FD8000, 0, 0x00009C00, 0 }, -1, { 0, 0, 0 }, 0 },
    { 39, 4, { 0xFF, 0x05, 0x01, 0x02, 0, 0, 0, 0, 0xC47D4000, 0x42F00000, 0xC42B4000, 0, 0x00001800, 0 }, -1, { 0, 0, 0 }, 0 },
    { 40, 1, { 0xFF, 0x05, 0x01, 0x04, 0, 0, 0, 0, 0xC5358000, 0x42920000, 0xC3FA8000, 0, 0x00001800, 0 }, -1, { 0, 0, 0 }, 0 },
    { 40, 1, { 0xFF, 0x05, 0x01, 0x04, 0, 0, 0, 0, 0xC5001000, 0x42A20000, 0xC530E000, 0, 0x00001800, 0 }, -1, { 0, 0, 0 }, 0 },
    { 40, 1, { 0xFF, 0x05, 0x01, 0x04, 0, 0, 0, 0, 0x45244000, 0x42900000, 0xC5473000, 0, 0x00001800, 0 }, -1, { 0, 0, 0 }, 0 },
    { 40, 1, { 0xFF, 0x05, 0x01, 0x04, 0, 0, 0, 0, 0x45234000, 0x423C0000, 0xC4640000, 0, 0x00001800, 0 }, -1, { 0, 0, 0 }, 0 },
    { 40, 1, { 0xFF, 0x05, 0x01, 0x04, 0, 0, 0, 0, 0xC4A58000, 0x42800000, 0xC4B8E000, 0, 0x00001800, 0 }, -1, { 0, 0, 0 }, 0 },
    { 40, 255, { 0xFF, 0x05, 0x03, 0x08, 0, 0, 0, 0, 0xC4778000, 0x42B60000, 0x4457C000, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
    { 40, 255, { 0xFF, 0x05, 0x03, 0x08, 0, 0, 0, 0, 0x456BD000, 0x422C0000, 0xC5140000, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
    { 40, 255, { 0xFF, 0x05, 0x03, 0x08, 0, 0, 0, 0, 0x445C0000, 0x42700000, 0xC4B74000, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
    { 29, 2, { 0xFF, 0x06, 0x01, 0, 0, 0, 0, 0, 0xC496E000, 0xC2180000, 0xC3818000, 0, 0x00006000, 0 }, -1, { 0, 0, 0 }, 0 },
    { 29, 1, { 0xFF, 0x06, 0x01, 0, 0, 0, 0, 0, 0xC1500000, 0xC1A00000, 0xC3970000, 0, 0x0000FC00, 0 }, -1, { 0, 0, 0 }, 0 },
    { 40, 1, { 0xFF, 0x06, 0x01, 0x04, 0, 0, 0, 0, 0xC4B56000, 0xC1A00000, 0x44B92000, 0, 0x0000EC00, 0 }, -1, { 0, 0, 0 }, 0 },
    { 40, 1, { 0xFF, 0x06, 0x01, 0x04, 0, 0, 0, 0, 0xC4DC8000, 0xC1D80000, 0xC493C000, 0, 0x00001800, 0 }, -1, { 0, 0, 0 }, 0 },
    { 40, 255, { 0xFF, 0x06, 0x03, 0x08, 0, 0, 0, 0, 0xC3180000, 0xC2200000, 0x44D38000, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
    { 40, 255, { 0xFF, 0x06, 0x03, 0x08, 0, 0, 0, 0, 0x44E24000, 0x41100000, 0xC28A0000, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
    { 40, 255, { 0xFF, 0x06, 0x03, 0x08, 0, 0, 0, 0, 0xC484A000, 0x3F800000, 0xC4FAE000, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
    { 21, 4, { 0x07, 0x07, 0x01, 0, 0, 0, 0, 0, 0xC39C0000, 0x40000000, 0x43C60000, 0, 0x0000D000, 0 }, -1, { 0, 0, 0 }, 0 },
    { 21, 2, { 0x07, 0x07, 0x01, 0, 0, 0, 0, 0, 0x42D00000, 0xC0E00000, 0xC4074000, 0, 0x00008C00, 0 }, -1, { 0, 0, 0 }, 0 },
    { 21, 2, { 0x07, 0x07, 0x01, 0, 0, 0, 0, 0, 0x441BC000, 0xC1100000, 0x44318000, 0, 0x00000800, 0 }, -1, { 0, 0, 0 }, 0 },
    { 21, 1, { 0x07, 0x07, 0x01, 0, 0, 0, 0, 0, 0x44864000, 0x41500000, 0xC3E78000, 0, 0x00004000, 0 }, -1, { 0, 0, 0 }, 0 },
    { 40, 255, { 0xFF, 0x07, 0x03, 0x08, 0, 0, 0, 0, 0x44F8A000, 0x41A80000, 0x43520000, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
    { 40, 255, { 0xFF, 0x07, 0x03, 0x08, 0, 0, 0, 0, 0x44158000, 0x41600000, 0x44D78000, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
    { 40, 255, { 0xFF, 0x07, 0x03, 0x08, 0, 0, 0, 0, 0xC4728000, 0x41E00000, 0xC424C000, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
    { 30, 3, { 0xFF, 0x08, 0x01, 0, 0, 0, 0, 0, 0xC3850000, 0x3F800000, 0x4424C000, 0, 0x00002000, 0 }, -1, { 0, 0, 0 }, 0 },
    { 30, 2, { 0xFF, 0x08, 0x01, 0, 0, 0, 0, 0, 0xC4528000, 0x80000000, 0x44398000, 0, 0x0000E800, 0 }, -1, { 0, 0, 0 }, 0 },
    { 30, 2, { 0xFF, 0x08, 0x01, 0, 0, 0, 0, 0, 0x41A00000, 0x41E00000, 0xC3868000, 0, 0x00005400, 0 }, -1, { 0, 0, 0 }, 0 },
    { 30, 1, { 0xFF, 0x08, 0x01, 0, 0, 0, 0, 0, 0xC4054000, 0x41400000, 0xC3818000, 0, 0x00009800, 0 }, -1, { 0, 0, 0 }, 0 },
    { 21, 3, { 0x09, 0x09, 0x01, 0, 0, 0, 0, 0, 0xC48E0000, 0x40000000, 0xC44B8000, 0, 0x00008400, 0 }, -1, { 0, 0, 0 }, 0 },
    { 21, 2, { 0x09, 0x09, 0x01, 0, 0, 0, 0, 0, 0x44A0E000, 0x40000000, 0xC2BA0000, 0, 0x0000B400, 0 }, -1, { 0, 0, 0 }, 0 },
    { 39, 4, { 0xFF, 0x09, 0x01, 0, 0, 0, 0, 0, 0x45167000, 0x43B80000, 0x43090000, 0, 0x0000DC00, 0 }, -1, { 0, 0, 0 }, 0 },
    { 39, 2, { 0xFF, 0x09, 0x01, 0x02, 0, 0, 0, 0, 0x452BC000, 0x438B0000, 0xC2240000, 0, 0x0000B400, 0 }, -1, { 0, 0, 0 }, 0 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestEntrySlot quest_area_monsters_17[2] = {
    { 24, 1, { 0xFF, 0x01, 0, 0, 0, 0, 0, 0, 0, 0x437A0000, 0, 0, 0, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestEntrySlot quest_area_monsters_18[2] = {
    { 7, 1, { 0xFF, 0x05, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestEntrySlot quest_area_monsters_19[2] = {
    { 7, 1, { 0xFF, 0x06, 0, 0, 0, 0, 0, 0, 0xC1A00000, 0x40E00000, 0xC3FA0000, 0, 0, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestEntrySlot quest_area_monsters_20[2] = {
    { 7, 1, { 0xFF, 0x03, 0, 0, 0, 0, 0, 0, 0xC4AA8CCD, 0x42320000, 0xC4AFA000, 0, 0, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestEntrySlot quest_area_monsters_21[2] = {
    { 7, 1, { 0xFF, 0x03, 0, 0, 0, 0, 0, 0, 0x44860333, 0x40A00000, 0xC4798CCD, 0, 0, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestEntrySlot quest_area_monsters_22[2] = {
    { 7, 1, { 0xFF, 0x06, 0, 0, 0, 0, 0, 0, 0x44898000, 0, 0x44098000, 0, 0, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestEntrySlot* quest_area_monster_tbl[24] = {
    NULL,
    quest_area_monsters_01,
    quest_area_monsters_02,
    quest_area_monsters_03,
    quest_area_monsters_04,
    quest_area_monsters_05,
    quest_area_monsters_06,
    quest_area_monsters_07,
    quest_area_monsters_08,
    quest_area_monsters_09,
    quest_area_monsters_10,
    quest_area_monsters_11,
    quest_area_monsters_12,
    quest_area_monsters_13,
    quest_area_monsters_14,
    quest_area_monsters_15,
    quest_area_monsters_16,
    quest_area_monsters_17,
    quest_area_monsters_18,
    quest_area_monsters_19,
    quest_area_monsters_20,
    quest_area_monsters_21,
    quest_area_monsters_22,
    NULL,
};
QuestEntrySlot quest_variant_slots_01_0[4] = {
    { 1, 1, { 0xFF, 0x0A, 0, 0, 0, 0, 0, 0, 0xC436D333, 0xC2C20000, 0x451324CD, 0, 0, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 1, 1, { 0xFF, 0x02, 0, 0, 0x01, 0, 0, 0, 0xC55A2000, 0x41F0CCCD, 0x44B1E000, 0, 0x00007000, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 1, 1, { 0xFF, 0x05, 0, 0, 0x01, 0, 0, 0, 0x44CEB000, 0x40800000, 0xC52C1CCD, 0, 0, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestEntrySlot quest_variant_slots_01_1[3] = {
    { 1, 1, { 0xFF, 0x04, 0, 0, 0, 0, 0, 0, 0x43730000, 0xC0C00000, 0x4430199A, 0, 0x0000B000, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 1, 1, { 0xFF, 0x08, 0, 0, 0x01, 0, 0, 0, 0xC4786000, 0xC35D3333, 0xC497099A, 0, 0x00002400, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestEntrySlot quest_variant_slots_01_2[3] = {
    { 1, 1, { 0xFF, 0x05, 0, 0, 0, 0, 0, 0, 0x4567A000, 0x42480000, 0x4586D800, 0, 0x0000C000, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 1, 1, { 0xFF, 0x0A, 0, 0, 0x01, 0, 0, 0, 0xC5053CCD, 0xC21C0000, 0xC4CFF666, 0, 0, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestEntrySlot quest_variant_slots_01_3[3] = {
    { 1, 1, { 0xFF, 0x03, 0, 0, 0, 0, 0, 0, 0x45EAEE66, 0xC4B38000, 0x4570F99A, 0, 0x0000C400, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 1, 1, { 0xFF, 0x08, 0, 0, 0x01, 0, 0, 0, 0xC509C000, 0x43160000, 0xC4FAA000, 0, 0x00002800, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestEntrySlot quest_variant_slots_01_4[3] = {
    { 1, 1, { 0xFF, 0x01, 0, 0, 0, 0, 0, 0, 0xC484D000, 0x404CCCCD, 0x44D5ACCD, 0, 0x00001800, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 1, 1, { 0xFF, 0x01, 0, 0, 0, 0, 0, 0, 0x42800000, 0x42040000, 0xC2F20000, 0, 0, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestEntrySlot quest_variant_slots_01_5[3] = {
    { 1, 1, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0xC484D000, 0x404CCCCD, 0x44D5ACCD, 0, 0x00001800, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 1, 1, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0x42800000, 0x42040000, 0xC2F20000, 0, 0, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestMonsterVariantList quest_monster_variants_01[7] = {
    { 0x01, { 0, 0, 0 }, quest_variant_slots_01_0 },
    { 0x02, { 0, 0, 0 }, quest_variant_slots_01_1 },
    { 0x03, { 0, 0, 0 }, quest_variant_slots_01_2 },
    { 0x05, { 0, 0, 0 }, quest_variant_slots_01_3 },
    { 0x08, { 0, 0, 0 }, quest_variant_slots_01_4 },
    { 0x09, { 0, 0, 0 }, quest_variant_slots_01_5 },
    { 0xFF, { 0, 0, 0 }, NULL },
};
QuestEntrySlot quest_variant_slots_02_0[4] = {
    { 2, 1, { 0xFF, 0x0A, 0, 0, 0, 0, 0, 0, 0xC436D333, 0xC2C20000, 0x451324CD, 0, 0, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 2, 1, { 0xFF, 0x02, 0, 0, 0x01, 0, 0, 0, 0xC55A2000, 0xC1F0CCCD, 0x44B1E000, 0, 0x00007000, 0 }, 1, { 0, 0, 0 }, 0x101 },
    { 2, 1, { 0xFF, 0x05, 0, 0, 0x01, 0, 0, 0, 0x44CEB000, 0x40800000, 0xC52C1CCD, 0, 0x00007000, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestEntrySlot quest_variant_slots_02_1[3] = {
    { 2, 1, { 0xFF, 0x06, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 2, 1, { 0xFF, 0x08, 0, 0, 0x01, 0, 0, 0, 0xC4786000, 0xC35D3333, 0xC497099A, 0, 0x00002400, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestEntrySlot quest_variant_slots_02_2[3] = {
    { 2, 1, { 0xFF, 0x05, 0, 0, 0, 0, 0, 0, 0x4567A000, 0x42480000, 0x4586D800, 0, 0x0000C000, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 2, 1, { 0xFF, 0x0A, 0, 0, 0x01, 0, 0, 0, 0xC5053CCD, 0xC21C0000, 0xC4CFF666, 0, 0, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestEntrySlot quest_variant_slots_02_3[3] = {
    { 2, 1, { 0xFF, 0x03, 0, 0, 0, 0, 0, 0, 0x45EAEE66, 0xC4B38000, 0x4570F99A, 0, 0x0000C400, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 2, 1, { 0xFF, 0x08, 0, 0, 0x01, 0, 0, 0, 0xC509C000, 0x43160000, 0xC4FAA000, 0, 0x00002800, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestEntrySlot quest_variant_slots_02_4[3] = {
    { 2, 1, { 0xFF, 0x01, 0, 0, 0, 0, 0, 0, 0x44C10000, 0x42B00000, 0xC4144000, 0, 0x0000CC00, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 2, 1, { 0xFF, 0x01, 0, 0, 0, 0, 0, 0, 0x42800000, 0x42040000, 0xC2F20000, 0, 0, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestEntrySlot quest_variant_slots_02_5[3] = {
    { 2, 1, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0x44C10000, 0x42B00000, 0xC4144000, 0, 0x0000CC00, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 2, 1, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0x42800000, 0x42040000, 0xC2F20000, 0, 0, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestMonsterVariantList quest_monster_variants_02[7] = {
    { 0x01, { 0, 0, 0 }, quest_variant_slots_02_0 },
    { 0x02, { 0, 0, 0 }, quest_variant_slots_02_1 },
    { 0x03, { 0, 0, 0 }, quest_variant_slots_02_2 },
    { 0x05, { 0, 0, 0 }, quest_variant_slots_02_3 },
    { 0x08, { 0, 0, 0 }, quest_variant_slots_02_4 },
    { 0x09, { 0, 0, 0 }, quest_variant_slots_02_5 },
    { 0xFF, { 0, 0, 0 }, NULL },
};
QuestEntrySlot quest_variant_slots_03_0[4] = {
    { 3, 1, { 0xFF, 0x04, 0, 0, 0, 0, 0, 0, 0x43480000, 0x40800000, 0x43BE0000, 0, 0, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 3, 1, { 0xFF, 0x0A, 0, 0, 0x01, 0, 0, 0, 0xC3C80000, 0x41D80000, 0x45610000, 0, 0, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 3, 1, { 0xFF, 0x05, 0, 0, 0x01, 0, 0, 0, 0x44CEB000, 0x40800000, 0xC52C1CCD, 0, 0x00007000, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestEntrySlot quest_variant_slots_03_1[3] = {
    { 3, 1, { 0xFF, 0x04, 0, 0, 0, 0, 0, 0, 0xC3C80000, 0xC28E0000, 0x453B8000, 0, 0, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 3, 1, { 0xFF, 0x06, 0, 0, 0x01, 0, 0, 0, 0xC56D8000, 0, 0xC4A28000, 0, 0, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestEntrySlot quest_variant_slots_03_2[3] = {
    { 3, 1, { 0xFF, 0x01, 0, 0, 0, 0, 0, 0, 0xC502C000, 0x40000000, 0xC5129000, 0, 0x00001000, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 3, 1, { 0xFF, 0x01, 0, 0, 0, 0, 0, 0, 0x42800000, 0x42040000, 0xC2F20000, 0, 0, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestEntrySlot quest_variant_slots_03_3[3] = {
    { 3, 1, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0xC502C000, 0x40000000, 0xC5129000, 0, 0x00001000, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 3, 1, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0x42800000, 0x42040000, 0xC2F20000, 0, 0, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestMonsterVariantList quest_monster_variants_03[5] = {
    { 0x01, { 0, 0, 0 }, quest_variant_slots_03_0 },
    { 0x02, { 0, 0, 0 }, quest_variant_slots_03_1 },
    { 0x08, { 0, 0, 0 }, quest_variant_slots_03_2 },
    { 0x09, { 0, 0, 0 }, quest_variant_slots_03_3 },
    { 0xFF, { 0, 0, 0 }, NULL },
};
QuestEntrySlot quest_variant_slots_04_0[2] = {
    { 4, 1, { 0xFF, 0x04, 0, 0, 0, 0, 0, 0, 0x45449333, 0x411B3333, 0x43A7A666, 0, 0x0000B000, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestEntrySlot quest_variant_slots_04_1[3] = {
    { 4, 1, { 0xFF, 0x01, 0, 0, 0, 0, 0, 0, 0x45110000, 0, 0x440CC000, 0, 0x0000CC00, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 4, 1, { 0xFF, 0x01, 0, 0, 0, 0, 0, 0, 0x42800000, 0x42040000, 0xC2F20000, 0, 0, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestEntrySlot quest_variant_slots_04_2[3] = {
    { 4, 1, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0x45110000, 0, 0x440CC000, 0, 0x0000CC00, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 4, 1, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0x42800000, 0x42040000, 0xC2F20000, 0, 0, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestMonsterVariantList quest_monster_variants_04[4] = {
    { 0x04, { 0, 0, 0 }, quest_variant_slots_04_0 },
    { 0x08, { 0, 0, 0 }, quest_variant_slots_04_1 },
    { 0x09, { 0, 0, 0 }, quest_variant_slots_04_2 },
    { 0xFF, { 0, 0, 0 }, NULL },
};
QuestEntrySlot quest_variant_slots_05_0[4] = {
    { 5, 1, { 0xFF, 0x07, 0, 0, 0, 0, 0, 0, 0x44BB8000, 0x459C4000, 0x44898000, 0, 0, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 5, 1, { 0xFF, 0x07, 0, 0, 0x01, 0, 0, 0, 0x44BB8000, 0x459C4000, 0x44898000, 0, 0, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 5, 1, { 0xFF, 0x07, 0, 0, 0x02, 0, 0, 0, 0x44E10000, 0x459C4000, 0x44ED8000, 0, 0, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestEntrySlot quest_variant_slots_05_1[3] = {
    { 5, 1, { 0xFF, 0x01, 0, 0, 0, 0, 0, 0, 0xC51B1000, 0, 0xC3680000, 0, 0x00001800, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 5, 1, { 0xFF, 0x01, 0, 0, 0, 0, 0, 0, 0x42800000, 0x42040000, 0xC2F20000, 0, 0, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestEntrySlot quest_variant_slots_05_2[3] = {
    { 5, 1, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0xC51B1000, 0, 0xC3680000, 0, 0x00001800, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 5, 1, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0x42800000, 0x42040000, 0xC2F20000, 0, 0, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestMonsterVariantList quest_monster_variants_05[4] = {
    { 0x04, { 0, 0, 0 }, quest_variant_slots_05_0 },
    { 0x08, { 0, 0, 0 }, quest_variant_slots_05_1 },
    { 0x09, { 0, 0, 0 }, quest_variant_slots_05_2 },
    { 0xFF, { 0, 0, 0 }, NULL },
};
QuestEntrySlot quest_variant_slots_06_0[3] = {
    { 6, 1, { 0xFF, 0x06, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 6, 1, { 0xFF, 0x04, 0, 0, 0x01, 0, 0, 0, 0xC44A0000, 0xC28C0000, 0x44D56000, 0, 0, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestEntrySlot quest_variant_slots_06_1[3] = {
    { 6, 1, { 0xFF, 0x01, 0, 0, 0, 0, 0, 0, 0x43290000, 0x420C0000, 0xC5287000, 0, 0, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 6, 1, { 0xFF, 0x01, 0, 0, 0, 0, 0, 0, 0x42800000, 0x42040000, 0xC2F20000, 0, 0, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestEntrySlot quest_variant_slots_06_2[3] = {
    { 6, 1, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0x43290000, 0x420C0000, 0xC5287000, 0, 0, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 6, 1, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0x42800000, 0x42040000, 0xC2F20000, 0, 0, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestMonsterVariantList quest_monster_variants_06[4] = {
    { 0x02, { 0, 0, 0 }, quest_variant_slots_06_0 },
    { 0x08, { 0, 0, 0 }, quest_variant_slots_06_1 },
    { 0x09, { 0, 0, 0 }, quest_variant_slots_06_2 },
    { 0xFF, { 0, 0, 0 }, NULL },
};
QuestEntrySlot quest_variant_slots_07_0[3] = {
    { 7, 1, { 0xFF, 0x05, 0, 0, 0, 0, 0, 0, 0xC43A7333, 0x41800000, 0x4423B333, 0, 0, 0 }, 1, { 0, 0, 0 }, 0x101 },
    { 7, 1, { 0xFF, 0x04, 0, 0, 0, 0, 0, 0, 0x41080000, 0x4211999A, 0x44DF2666, 0, 0, 0 }, 1, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestEntrySlot quest_variant_slots_07_1[3] = {
    { 7, 1, { 0xFF, 0x06, 0, 0, 0, 0, 0, 0, 0xC1A00000, 0x40E00000, 0xC3FA0000, 0, 0, 0 }, 1, { 0, 0, 0 }, 0x101 },
    { 7, 1, { 0xFF, 0x04, 0, 0, 0, 0, 0, 0, 0xC4ED8000, 0, 0x44BB8000, 0, 0, 0 }, 1, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestEntrySlot quest_variant_slots_07_2[3] = {
    { 7, 1, { 0xFF, 0x03, 0, 0, 0, 0, 0, 0, 0x43FA0000, 0xC1100000, 0xC4390000, 0, 0, 0 }, 1, { 0, 0, 0 }, 0x101 },
    { 7, 1, { 0xFF, 0x01, 0, 0, 0, 0, 0, 0, 0x44D48000, 0xC2480000, 0xC4898000, 0, 0, 0 }, 1, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestEntrySlot quest_variant_slots_07_3[3] = {
    { 7, 1, { 0xFF, 0x03, 0, 0, 0, 0, 0, 0, 0x44860333, 0x40A00000, 0xC4798CCD, 0, 0, 0 }, 1, { 0, 0, 0 }, 0x101 },
    { 7, 1, { 0xFF, 0x06, 0, 0, 0, 0, 0, 0, 0x43FA0000, 0x420C0000, 0xC4BB8000, 0, 0, 0 }, 1, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestEntrySlot quest_variant_slots_07_4[3] = {
    { 7, 1, { 0xFF, 0x06, 0, 0, 0, 0, 0, 0, 0x44898000, 0, 0x44098000, 0, 0, 0 }, 1, { 0, 0, 0 }, 0x101 },
    { 7, 1, { 0xFF, 0x04, 0, 0, 0, 0, 0, 0, 0xC3F1C000, 0x4205999A, 0xC2B9999A, 0, 0, 0 }, 1, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestEntrySlot quest_variant_slots_07_5[3] = {
    { 7, 1, { 0xFF, 0x01, 0, 0, 0, 0, 0, 0, 0x41840000, 0x41940000, 0x451C0B33, 0, 0, 0 }, 1, { 0, 0, 0 }, 0x101 },
    { 7, 1, { 0xFF, 0x01, 0, 0, 0, 0, 0, 0, 0x42800000, 0x42040000, 0xC2F20000, 0, 0, 0 }, 1, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestEntrySlot quest_variant_slots_07_6[3] = {
    { 7, 1, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0x41840000, 0x41940000, 0x451C0B33, 0, 0, 0 }, 1, { 0, 0, 0 }, 0x101 },
    { 7, 1, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0x42800000, 0x42040000, 0xC2F20000, 0, 0, 0 }, 1, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestMonsterVariantList quest_monster_variants_07[8] = {
    { 0x01, { 0, 0, 0 }, quest_variant_slots_07_0 },
    { 0x02, { 0, 0, 0 }, quest_variant_slots_07_1 },
    { 0x03, { 0, 0, 0 }, quest_variant_slots_07_2 },
    { 0x04, { 0, 0, 0 }, quest_variant_slots_07_3 },
    { 0x05, { 0, 0, 0 }, quest_variant_slots_07_4 },
    { 0x08, { 0, 0, 0 }, quest_variant_slots_07_5 },
    { 0x09, { 0, 0, 0 }, quest_variant_slots_07_6 },
    { 0xFF, { 0, 0, 0 }, NULL },
};
QuestEntrySlot quest_variant_slots_08_0[3] = {
    { 8, 1, { 0xFF, 0x03, 0, 0, 0, 0, 0, 0, 0xC4484000, 0xC1A00000, 0x44D94000, 0, 0, 0 }, 1, { 0, 0, 0 }, 0x101 },
    { 8, 1, { 0xFF, 0x06, 0, 0, 0x01, 0, 0, 0, 0xC5334E66, 0xC25F3333, 0xC3C3D99A, 0, 0, 0 }, 1, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestEntrySlot quest_variant_slots_08_1[3] = {
    { 8, 1, { 0xFF, 0x01, 0, 0, 0, 0, 0, 0, 0x44A86000, 0x41C00000, 0x44D38000, 0, 0x0000E000, 0 }, 1, { 0, 0, 0 }, 0x101 },
    { 8, 1, { 0xFF, 0x01, 0, 0, 0, 0, 0, 0, 0x42800000, 0x42040000, 0xC2F20000, 0, 0, 0 }, 1, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestEntrySlot quest_variant_slots_08_2[3] = {
    { 8, 1, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0x44A86000, 0x41C00000, 0x44D38000, 0, 0x0000E000, 0 }, 1, { 0, 0, 0 }, 0x101 },
    { 8, 1, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0x42800000, 0x42040000, 0xC2F20000, 0, 0, 0 }, 1, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestMonsterVariantList quest_monster_variants_08[4] = {
    { 0x02, { 0, 0, 0 }, quest_variant_slots_08_0 },
    { 0x08, { 0, 0, 0 }, quest_variant_slots_08_1 },
    { 0x09, { 0, 0, 0 }, quest_variant_slots_08_2 },
    { 0xFF, { 0, 0, 0 }, NULL },
};
QuestEntrySlot quest_variant_slots_09_0[4] = {
    { 9, 1, { 0xFF, 0x04, 0, 0, 0, 0, 0, 0, 0xC48DA99A, 0xC11FAE14, 0x452D9B33, 0, 0x0000A000, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 9, 1, { 0xFF, 0x05, 0, 0, 0x01, 0, 0, 0, 0xC40D3852, 0x423D3D71, 0x437CCCCD, 0, 0x0000A000, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 9, 1, { 0xFF, 0x04, 0, 0, 0x02, 0, 0, 0, 0x44F17C7B, 0x418D851F, 0x45410E3D, 0, 0x0000A000, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestEntrySlot quest_variant_slots_09_1[3] = {
    { 9, 1, { 0xFF, 0x01, 0, 0, 0, 0, 0, 0, 0x4481C000, 0, 0x443EC000, 0, 0x0000EC00, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 9, 1, { 0xFF, 0x01, 0, 0, 0, 0, 0, 0, 0x42800000, 0x42040000, 0xC2F20000, 0, 0, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestEntrySlot quest_variant_slots_09_2[3] = {
    { 9, 1, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0x4481C000, 0, 0x443EC000, 0, 0x0000EC00, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 9, 1, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0x42800000, 0x42040000, 0xC2F20000, 0, 0, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestMonsterVariantList quest_monster_variants_09[4] = {
    { 0x05, { 0, 0, 0 }, quest_variant_slots_09_0 },
    { 0x08, { 0, 0, 0 }, quest_variant_slots_09_1 },
    { 0x09, { 0, 0, 0 }, quest_variant_slots_09_2 },
    { 0xFF, { 0, 0, 0 }, NULL },
};
QuestEntrySlot quest_variant_slots_12_0[4] = {
    { 12, 1, { 0x10, 0x09, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 1, { 0, 0, 0 }, 0x101 },
    { 12, 1, { 0x11, 0x02, 0, 0, 0, 0, 0, 0, 0xC5C0D99A, 0x41333333, 0xC433799A, 0, 0, 0 }, 1, { 0, 0, 0 }, 0x101 },
    { 12, 1, { 0x12, 0x03, 0, 0, 0, 0, 0, 0, 0xC44AF99A, 0x3F666666, 0xC39A0CCD, 0, 0, 0 }, 1, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestEntrySlot quest_variant_slots_12_1[4] = {
    { 12, 1, { 0x10, 0x04, 0, 0, 0, 0, 0, 0, 0xC5194800, 0x4139999A, 0x44299333, 0, 0x0000036E, 0 }, 1, { 0, 0, 0 }, 0x101 },
    { 12, 1, { 0x11, 0x05, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x0000036E, 0 }, 1, { 0, 0, 0 }, 0x101 },
    { 12, 1, { 0x12, 0x05, 0, 0, 0x01, 0, 0, 0, 0, 0, 0, 0, 0x0000036E, 0 }, 1, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestEntrySlot quest_variant_slots_12_2[3] = {
    { 12, 1, { 0x10, 0x01, 0, 0, 0, 0, 0, 0, 0x454A1000, 0, 0xC47D8000, 0, 0x0000E000, 0 }, 1, { 0, 0, 0 }, 0x101 },
    { 12, 1, { 0x11, 0x01, 0, 0, 0, 0, 0, 0, 0x42800000, 0x42040000, 0xC2F20000, 0, 0, 0 }, 1, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestEntrySlot quest_variant_slots_12_3[3] = {
    { 12, 1, { 0x10, 0, 0, 0, 0, 0, 0, 0, 0x454A1000, 0, 0xC47D8000, 0, 0x0000E000, 0 }, 1, { 0, 0, 0 }, 0x101 },
    { 12, 1, { 0x11, 0, 0, 0, 0, 0, 0, 0, 0x42800000, 0x42040000, 0xC2F20000, 0, 0, 0 }, 1, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestMonsterVariantList quest_monster_variants_12[5] = {
    { 0x01, { 0, 0, 0 }, quest_variant_slots_12_0 },
    { 0x02, { 0, 0, 0 }, quest_variant_slots_12_1 },
    { 0x08, { 0, 0, 0 }, quest_variant_slots_12_2 },
    { 0x09, { 0, 0, 0 }, quest_variant_slots_12_3 },
    { 0xFF, { 0, 0, 0 }, NULL },
};
QuestEntrySlot quest_variant_slots_14_0[3] = {
    { 14, 1, { 0x10, 0x05, 0, 0, 0, 0, 0, 0, 0xC30C6666, 0xC1733333, 0xC538F19A, 0, 0, 0 }, 1, { 0, 0, 0 }, 0x101 },
    { 14, 1, { 0x11, 0x02, 0, 0, 0, 0, 0, 0, 0xC537299A, 0x40333333, 0xC4877666, 0, 0, 0 }, 1, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestEntrySlot quest_variant_slots_14_1[3] = {
    { 14, 1, { 0x10, 0x01, 0, 0, 0, 0, 0, 0, 0x450FC000, 0x40400000, 0xC50F6000, 0, 0x0000F000, 0 }, 1, { 0, 0, 0 }, 0x101 },
    { 14, 1, { 0x11, 0x01, 0, 0, 0, 0, 0, 0, 0x42800000, 0x42040000, 0xC2F20000, 0, 0, 0 }, 1, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestEntrySlot quest_variant_slots_14_2[3] = {
    { 14, 1, { 0x10, 0, 0, 0, 0, 0, 0, 0, 0x450FC000, 0x40400000, 0xC50F6000, 0, 0x0000F000, 0 }, 1, { 0, 0, 0 }, 0x101 },
    { 14, 1, { 0x11, 0, 0, 0, 0, 0, 0, 0, 0x42800000, 0x42040000, 0xC2F20000, 0, 0, 0 }, 1, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestMonsterVariantList quest_monster_variants_14[4] = {
    { 0x04, { 0, 0, 0 }, quest_variant_slots_14_0 },
    { 0x08, { 0, 0, 0 }, quest_variant_slots_14_1 },
    { 0x09, { 0, 0, 0 }, quest_variant_slots_14_2 },
    { 0xFF, { 0, 0, 0 }, NULL },
};
QuestEntrySlot quest_variant_slots_15_0[4] = {
    { 15, 1, { 0xFF, 0x07, 0, 0, 0, 0, 0, 0, 0xC5561000, 0xC53B8000, 0x4544E000, 0, 0x00002500, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 15, 1, { 0xFF, 0x0C, 0, 0, 0, 0, 0, 0, 0xC5392000, 0xC4738000, 0xC5F6C000, 0, 0x00001800, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 15, 1, { 0xFF, 0x05, 0, 0, 0, 0, 0, 0, 0x449F8000, 0x41800000, 0x445D8000, 0, 0x0000CC00, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestEntrySlot quest_variant_slots_15_1[3] = {
    { 15, 1, { 0xFF, 0x08, 0, 0, 0, 0, 0, 0, 0xC40F999A, 0xC42B4666, 0xC52FF19A, 0, 0, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 15, 1, { 0xFF, 0x04, 0, 0, 0, 0, 0, 0, 0x4595C666, 0xC43CC000, 0xC4B10000, 0, 0, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestEntrySlot quest_variant_slots_15_2[3] = {
    { 15, 1, { 0xFF, 0x01, 0, 0, 0, 0, 0, 0, 0xC46A8000, 0x3F800000, 0xC4E32000, 0, 0x00001000, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 15, 1, { 0xFF, 0x01, 0, 0, 0, 0, 0, 0, 0x42800000, 0x42040000, 0xC2F20000, 0, 0, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestEntrySlot quest_variant_slots_15_3[5] = {
    { 15, 1, { 0xFF, 0x01, 0, 0, 0, 0, 0, 0, 0x43850000, 0xC482E000, 0xC4DBE000, 0, 0x00000C00, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 15, 1, { 0xFF, 0x01, 0, 0, 0, 0, 0, 0, 0xC438D333, 0xC4BC1000, 0xC4877000, 0, 0, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 15, 1, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0xC46A8000, 0x3F800000, 0xC4E32000, 0, 0x00001000, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 15, 1, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0x42800000, 0x42040000, 0xC2F20000, 0, 0x00001000, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestEntrySlot quest_variant_slots_15_4[3] = {
    { 15, 1, { 0xFF, 0x01, 0, 0, 0, 0, 0, 0, 0x43850000, 0xC482E000, 0xC4DBE000, 0, 0x00000C00, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 15, 1, { 0xFF, 0x01, 0, 0, 0, 0, 0, 0, 0xC438D333, 0xC4BC1000, 0xC4877000, 0, 0, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestMonsterVariantList quest_monster_variants_15[6] = {
    { 0x01, { 0, 0, 0 }, quest_variant_slots_15_0 },
    { 0x03, { 0, 0, 0 }, quest_variant_slots_15_1 },
    { 0x08, { 0, 0, 0 }, quest_variant_slots_15_2 },
    { 0x09, { 0, 0, 0 }, quest_variant_slots_15_3 },
    { 0x0B, { 0, 0, 0 }, quest_variant_slots_15_4 },
    { 0xFF, { 0, 0, 0 }, NULL },
};
QuestEntrySlot quest_variant_slots_16_0[3] = {
    { 16, 1, { 0xFF, 0x06, 0, 0, 0, 0, 0, 0, 0xC343999A, 0xC23C0000, 0xC4F00000, 0, 0x0000D000, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 16, 1, { 0xFF, 0x07, 0, 0, 0x01, 0, 0, 0, 0xC5561000, 0xC53B8000, 0x4544E000, 0, 0, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestEntrySlot quest_variant_slots_16_1[3] = {
    { 16, 1, { 0xFF, 0x03, 0, 0, 0, 0, 0, 0, 0xC52EB000, 0xC1E00000, 0x43E98000, 0, 0x00001600, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 16, 1, { 0xFF, 0x02, 0, 0, 0x01, 0, 0, 0, 0xC4E34CCD, 0xC3FA0000, 0x4426ECCD, 0, 0, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestEntrySlot quest_variant_slots_16_2[3] = {
    { 16, 1, { 0xFF, 0x01, 0, 0, 0, 0, 0, 0, 0xC3F90000, 0, 0x44F7C000, 0, 0x00001800, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 16, 1, { 0xFF, 0x01, 0, 0, 0, 0, 0, 0, 0x42800000, 0x42040000, 0xC2F20000, 0, 0, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestEntrySlot quest_variant_slots_16_3[4] = {
    { 16, 1, { 0xFF, 0x01, 0, 0, 0, 0, 0, 0, 0xC48B2CCD, 0xC4E55000, 0x44B3DCCD, 0, 0x00001600, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 16, 1, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0xC3F90000, 0, 0x44F7C000, 0, 0x00001800, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 16, 1, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0x42800000, 0x42040000, 0xC2F20000, 0, 0, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestEntrySlot quest_variant_slots_16_4[3] = {
    { 16, 1, { 0xFF, 0x01, 0, 0, 0, 0, 0, 0, 0xC48B2CCD, 0xC4E55000, 0x44B3DCCD, 0, 0x00001600, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 16, 1, { 0xFF, 0x01, 0, 0, 0, 0, 0, 0, 0xC438D333, 0xC4BC1000, 0xC4877000, 0, 0, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestMonsterVariantList quest_monster_variants_16[6] = {
    { 0x01, { 0, 0, 0 }, quest_variant_slots_16_0 },
    { 0x03, { 0, 0, 0 }, quest_variant_slots_16_1 },
    { 0x08, { 0, 0, 0 }, quest_variant_slots_16_2 },
    { 0x09, { 0, 0, 0 }, quest_variant_slots_16_3 },
    { 0x0B, { 0, 0, 0 }, quest_variant_slots_16_4 },
    { 0xFF, { 0, 0, 0 }, NULL },
};
QuestEntrySlot quest_variant_slots_18_0[3] = {
    { 18, 1, { 0xFF, 0x03, 0, 0, 0, 0, 0, 0, 0x452A0000, 0xC47A0000, 0x44D9E000, 0, 0x0000C400, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 18, 1, { 0xFF, 0x08, 0, 0, 0x01, 0, 0, 0, 0x441C4000, 0xC4BB8000, 0xC2DA0000, 0, 0, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestEntrySlot quest_variant_slots_18_1[3] = {
    { 18, 1, { 0xFF, 0x01, 0, 0, 0, 0, 0, 0, 0x43334CCD, 0xC534A000, 0xC4AE999A, 0, 0x0000C400, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 18, 1, { 0xFF, 0x01, 0, 0, 0, 0, 0, 0, 0xC4821000, 0xC534A000, 0x447ED333, 0, 0, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestMonsterVariantList quest_monster_variants_18[4] = {
    { 0x03, { 0, 0, 0 }, quest_variant_slots_18_0 },
    { 0x09, { 0, 0, 0 }, quest_variant_slots_18_1 },
    { 0x0B, { 0, 0, 0 }, quest_variant_slots_18_1 },
    { 0xFF, { 0, 0, 0 }, NULL },
};
QuestEntrySlot quest_variant_slots_19_0[3] = {
    { 19, 1, { 0xFF, 0x07, 0, 0, 0, 0, 0, 0, 0x44926000, 0, 0xC47C0000, 0, 0, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 19, 1, { 0xFF, 0x08, 0, 0, 0x01, 0, 0, 0, 0x4552A000, 0, 0x442F8000, 0, 0, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestEntrySlot quest_variant_slots_19_1[3] = {
    { 19, 1, { 0xFF, 0x01, 0, 0, 0, 0, 0, 0, 0xC4960000, 0x40A00000, 0xC42FC000, 0, 0x00000800, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 19, 1, { 0xFF, 0x01, 0, 0, 0x01, 0, 0, 0, 0x42800000, 0x42040000, 0xC2F20000, 0, 0, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestEntrySlot quest_variant_slots_19_2[3] = {
    { 19, 1, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0xC4960000, 0x40A00000, 0xC42FC000, 0, 0x00000800, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 19, 1, { 0xFF, 0, 0, 0, 0x01, 0, 0, 0, 0x42800000, 0x42040000, 0xC2F20000, 0, 0, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestMonsterVariantList quest_monster_variants_19[4] = {
    { 0x05, { 0, 0, 0 }, quest_variant_slots_19_0 },
    { 0x08, { 0, 0, 0 }, quest_variant_slots_19_1 },
    { 0x09, { 0, 0, 0 }, quest_variant_slots_19_2 },
    { 0xFF, { 0, 0, 0 }, NULL },
};
QuestMonsterVariantList quest_monster_variants_20[2] = {
    { 0x07, { 0, 0, 0 }, quest_area_monsters_14 },
    { 0xFF, { 0, 0, 0 }, NULL },
};
QuestEntrySlot quest_variant_slots_24_0[3] = {
    { 24, 1, { 0xFF, 0x01, 0, 0, 0, 0, 0, 0, 0, 0x437A0000, 0, 0, 0, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 24, 1, { 0xFF, 0x01, 0, 0, 0x01, 0, 0, 0, 0, 0x437A0000, 0, 0, 0, 0 }, 0, { 0, 0, 0 }, 0x101 },
    { 0, 0, { 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, -1, { 0, 0, 0 }, 0 },
};
QuestMonsterVariantList quest_monster_variants_24[2] = {
    { 0x0A, { 0, 0, 0 }, quest_variant_slots_24_0 },
    { 0xFF, { 0, 0, 0 }, NULL },
};
QuestMonsterVariantList quest_monster_variants_25[2] = {
    { 0x06, { 0, 0, 0 }, quest_area_monsters_10 },
    { 0xFF, { 0, 0, 0 }, NULL },
};
QuestMonsterVariantList* quest_monster_variant_tbl[41] = {
    NULL,
    quest_monster_variants_01,
    quest_monster_variants_02,
    quest_monster_variants_03,
    quest_monster_variants_04,
    quest_monster_variants_05,
    quest_monster_variants_06,
    quest_monster_variants_07,
    quest_monster_variants_08,
    quest_monster_variants_09,
    NULL,
    NULL,
    quest_monster_variants_12,
    NULL,
    quest_monster_variants_14,
    quest_monster_variants_15,
    quest_monster_variants_16,
    NULL,
    quest_monster_variants_18,
    quest_monster_variants_19,
    quest_monster_variants_20,
    NULL,
    NULL,
    NULL,
    quest_monster_variants_24,
    quest_monster_variants_25,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
};
s16 em_set_quest_area_tbl[5][13] = {
    { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
    { 301, -1, 301, 301, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
    { 301, -1, 302, 304, 301, 301, -1, -1, -1, 301, -1, -1, -1 },
    { 301, 301, 303, 302, 302, 302, -1, -1, -1, 302, -1, 301, -1 },
    { -1, 302, 304, 303, 303, 303, -1, 301, 301, 302, 301, 301, 301 },
};
s16 em_set_season_area_tbl[8][13] = {
    { -1, 101, 101, 101, 101, 101, 101, 101, 101, 101, 101, 101, 101 },
    { -1, 102, 102, 102, 101, 101, 101, 101, 101, 101, 101, 101, 101 },
    { -1, 101, 101, 101, 102, 102, 101, 101, 101, 102, 102, 102, 101 },
    { -1, 101, 101, 101, 103, 103, 101, 101, 101, 103, 103, 103, 101 },
    { -1, 101, 101, 101, 101, 101, 102, 102, 102, 101, 101, 101, 102 },
    { -1, 102, 102, 102, 102, 102, 101, 101, 101, 102, 102, 102, 101 },
    { -1, 102, 102, 102, 101, 101, 102, 102, 102, 101, 101, 101, 102 },
    { -1, 101, 101, 101, 103, 103, 102, 102, 102, 103, 103, 103, 102 },
};
u8 quest_pair_table[0x1C] = {
    0x00, 0x00, 0x01, 0x0C, 0x02, 0x0D, 0x03, 0x0E, 0x04, 0x0F, 0x05, 0x10, 0x06, 0x11, 0x07, 0x12, 0x08, 0x13, 0x09, 0x09, 0x09, 0x09, 0x0A, 0x0A, 0x0B, 0x14, 0x00, 0x00,
};
u8 quest_byte_table[13] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x09, 0x0A, 0x0B,
};
s32 quest_time_class_tbl[3] = { 0, 5, 10 };
u8 quest_supply_drop_03[0xC] = {
    0x0D, 0x08, 0x0A, 0x20, 0x07, 0x30, 0x04, 0x10, 0x00, 0x00, 0x00, 0x00,
};
u8 quest_supply_drop_04[0xC] = {
    0x12, 0x08, 0x0F, 0x20, 0x0C, 0x20, 0x09, 0x20, 0x06, 0x10, 0x00, 0x00,
};
u8 quest_supply_drop_05[0x10] = {
    0x16, 0x04, 0x13, 0x10, 0x10, 0x20, 0x0D, 0x20, 0x0A, 0x20, 0x07, 0x10, 0x04, 0x08, 0x00, 0x00,
};
u8 quest_supply_drop_06[0x14] = {
    0x1B, 0x02, 0x18, 0x08, 0x15, 0x06, 0x12, 0x20, 0x0F, 0x20, 0x0C, 0x20, 0x09, 0x10, 0x06, 0x08, 0x00, 0x00, 0x00, 0x00,
};
u8 quest_supply_drop_07[0x1C] = {
    0x1F, 0x02, 0x1C, 0x04, 0x19, 0x08, 0x16, 0x10, 0x13, 0x10, 0x10, 0x20, 0x0D, 0x20, 0x0A, 0x20, 0x07, 0x10, 0x04, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
u8 quest_supply_drop_08[0x18] = {
    0x24, 0x02, 0x21, 0x02, 0x1E, 0x04, 0x1B, 0x08, 0x18, 0x08, 0x15, 0x10, 0x12, 0x20, 0x0F, 0x20, 0x0C, 0x20, 0x09, 0x10, 0x06, 0x08, 0x00, 0x00,
};
u8 quest_supply_drop_09[0x20] = {
    0x2A, 0x02, 0x27, 0x02, 0x24, 0x04, 0x21, 0x04, 0x1E, 0x08, 0x1B, 0x08, 0x18, 0x10, 0x15, 0x20, 0x12, 0x20, 0x0F, 0x20, 0x0C, 0x10, 0x09, 0x10, 0x06, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
u8 quest_supply_drop_10[0x20] = {
    0x2F, 0x02, 0x2C, 0x02, 0x29, 0x04, 0x26, 0x04, 0x23, 0x08, 0x20, 0x08, 0x1D, 0x10, 0x1A, 0x10, 0x17, 0x20, 0x14, 0x20, 0x11, 0x20, 0x0E, 0x10, 0x0B, 0x10, 0x08, 0x10, 0x05, 0x08, 0x00, 0x00,
};
u8 quest_supply_drop_11[0x24] = {
    0x34, 0x02, 0x31, 0x02, 0x2E, 0x04, 0x2B, 0x04, 0x28, 0x08, 0x25, 0x08, 0x22, 0x10, 0x1F, 0x10, 0x1C, 0x10, 0x19, 0x20, 0x16, 0x20, 0x13, 0x20, 0x10, 0x10, 0x0D, 0x10, 0x0A, 0x10, 0x07, 0x08, 0x04, 0x08, 0x00, 0x00,
};
u8* quest_supply_drop_tbl[13] = {
    quest_supply_drop_empty,
    quest_supply_drop_empty,
    quest_sdata_tbl_a,
    quest_supply_drop_03,
    quest_supply_drop_04,
    quest_supply_drop_05,
    quest_supply_drop_06,
    quest_supply_drop_07,
    quest_supply_drop_08,
    quest_supply_drop_09,
    quest_supply_drop_10,
    quest_supply_drop_11,
    quest_supply_drop_empty,
};
u8 quest_bonus_items_0[0x10] = {
    0x14, 0x00, 0x0F, 0x01, 0x05, 0x04, 0x14, 0x05, 0x0A, 0x07, 0x14, 0x09, 0x0A, 0x0A, 0xFF, 0x00,
};
u8 quest_bonus_items_1[0x10] = {
    0x14, 0x00, 0x0F, 0x03, 0x0A, 0x05, 0x14, 0x06, 0x0A, 0x07, 0x14, 0x09, 0x05, 0x0B, 0xFF, 0x00,
};
u8 quest_bonus_items_2[0x10] = {
    0x14, 0x00, 0x0F, 0x03, 0x14, 0x04, 0x14, 0x06, 0x05, 0x08, 0x14, 0x09, 0xFF, 0x00, 0x00, 0x00,
};
u8 quest_bonus_items_3[0x10] = {
    0x14, 0x00, 0x05, 0x01, 0x14, 0x04, 0x14, 0x05, 0x0F, 0x06, 0x14, 0x07, 0xFF, 0x00, 0x00, 0x00,
};
u8 quest_bonus_items_4[0x10] = {
    0x14, 0x00, 0x0F, 0x03, 0x14, 0x06, 0x14, 0x07, 0x14, 0x09, 0x05, 0x0A, 0xFF, 0x00, 0x00, 0x00,
};
u8* quest_bonus_pick_00[3] = { quest_sdata_tbl_b, quest_sdata_tbl_b, quest_sdata_tbl_b };
u8* quest_bonus_pick_01[3] = { quest_sdata_tbl_b, quest_bonus_items_0, quest_sdata_tbl_b };
u8* quest_bonus_pick_02[3] = { quest_sdata_tbl_b, quest_bonus_items_1, quest_sdata_tbl_b };
u8* quest_bonus_pick_03[3] = { quest_sdata_tbl_b, quest_bonus_items_2, quest_sdata_tbl_b };
u8* quest_bonus_pick_04[3] = { quest_sdata_tbl_b, quest_bonus_items_3, quest_sdata_tbl_b };
u8* quest_bonus_pick_05[3] = { quest_sdata_tbl_b, quest_bonus_items_4, quest_sdata_tbl_b };
u8* quest_bonus_pick_11[3] = { quest_sdata_tbl_b, quest_sdata_tbl_c, quest_sdata_tbl_c };
u8** quest_bonus_pick_tbl[13] = {
    quest_bonus_pick_00,
    quest_bonus_pick_01,
    quest_bonus_pick_02,
    quest_bonus_pick_03,
    quest_bonus_pick_04,
    quest_bonus_pick_05,
    quest_bonus_pick_00,
    quest_bonus_pick_00,
    quest_bonus_pick_00,
    quest_bonus_pick_00,
    quest_bonus_pick_00,
    quest_bonus_pick_11,
    quest_bonus_pick_00,
};
u8 quest_pair_chance_tbl_a[0x10] = {
    32, 32, 32, 22, 22, 22, 22, 22, 32, 22, 22, 22, 22, 22, 22, 22,
};
Q_RewardGroup quest_reward_group_tbl[6] = {
    { 0, 0, 0 },
    { 1, 1, 0 },
    { 2, 1, 1 },
    { 3, 2, 1 },
    { 4, 2, 1 },
    { 5, 3, 0 },
};
u8 quest_pair_chance_tbl_b[0x10] = {
    32, 32, 22, 22, 22, 22, 22, 22, 32, 32, 22, 22, 22, 22, 22, 22,
};
u8 quest_pair_chance_tbl_c[0x10] = {
    32, 32, 22, 22, 22, 22, 22, 22, 32, 32, 22, 22, 22, 22, 22, 22,
};
u16* arena_time_table[12] = {
    quest_grade_time_00,
    quest_grade_time_01,
    quest_grade_time_02,
    quest_grade_time_03,
    quest_grade_time_04,
    quest_grade_time_05,
    quest_grade_time_06,
    quest_grade_time_07,
    quest_grade_time_08,
    quest_grade_time_09,
    quest_grade_time_0A,
    quest_grade_time_0B,
};
u16 multi_arena_clr_time[20] = {
    10800, 4500, 18000, 7200, 32400, 10800, 21600, 9000, 21600, 10800, 27000, 10800, 32400, 16200, 36000, 17100, 36000, 18000, 54000, 27000,
};

/* The dcm archive buffers by stage (`stage_dcm_buffer_tbl`, camera/camera_main.cpp's 50 pointer slots, declared as bytes in the band). */
static inline u8** quest_dcm_buffers(void) {
    return (u8**)stage_dcm_buffer_tbl;
}

/* Copies both of the band's first two picked-pair tables into a result record's own two adjacent
 * 0xC0 buffers, one four-byte pair at a time. */
void quest_item_pair_tbl_copy(Q_ItemPair* dst_a, Q_ItemPair* dst_b) {
    const Q_ItemPair* a;
    const Q_ItemPair* b;
    s32 i = 0;

    a = quest_item_pair_tbl_a;
    b = quest_item_pair_tbl_b;
    for (; i < 0x30; i++) {
        item_pair_copy(dst_a++, a);
        item_pair_copy(dst_b++, b);
        a++;
        b++;
    }
}

/* Replaces the placeholder skill slot (0x16) of the 16-entry chance buffer with the skill slot the
 * player's equipped skills select, then, for the two quest-reward skills, forces slot 2 (and its
 * partner) to the top-rank marker 0x20. */
void quest_pl_skill_slot_set(_PLW* owner, u8* chance, u8 mode) {
    u8 fill = 0x16;
    u8* p;
    s32 n;
    u32 top;

    if (Pl_Skill_ck(owner, 0x91) == 1) {
        fill = 0x1A;
    }
    if (Pl_Skill_ck(owner, 0x92) == 1) {
        fill = 0x1D;
    }
    if (Pl_Skill_ck(owner, 0x93) == 1) {
        fill = 0x10;
    }
    if (Pl_Skill_ck(owner, 0x94) == 1) {
        fill = 8;
    }
    p = chance;
    for (n = 0; n < 16; p++, n++) {
        if (*p == 0x16) {
            *p = fill;
        }
    }
    top = 0;
    if (Pl_cat_skill_ck(owner, 0x10) == 1) {
        top = 1;
    } else if (Pl_cat_skill_ck(owner, 0x11) == 1 && (s32)(ran_suu(0) % 100) < 0x32) {
        top = 1;
    }
    if (top == 1) {
        chance[2] = 0x20;
        if (mode == 0) {
            chance[10] = 0x20;
        } else {
            chance[9] = 0x20;
            chance[13] = 0x20;
        }
    }
}

/* Walks `count` rows of `chance`, and for each row whose chance byte admits it rolls a `total`-wide
 * number into the weighted table and appends the entry the roll lands on.  The first row always
 * rolls 0, so it always takes the table's first non-empty entry. */
s32 quest_lot_pick_first(u8* chance, const Q_LotEntry* table, Q_ItemPair* out, s32 count, u16 total) {
    const Q_LotEntry* entry;
    s32 i;
    s32 picked = 0;

    for (i = 0; i < count; i++, chance++) {
        u16 roll;
        u16 acc;
        u32 hit;

        entry = table;
        if (entry->id == 0) {
            break;
        }
        if (*chance <= ((u16)ran_suu(0) & 0x1F)) {
            break;
        }
        roll = (u16)ran_suu(0) % total;
        if (i == 0) {
            roll = 0;
        }
        hit = 0;
        acc = 0;
        while (entry->id != 0) {
            acc += entry->weight;
            if (roll < acc) {
                hit = 1;
                break;
            }
            entry++;
        }
        if (hit == 1 && entry->id != 0) {
            out->id = entry->id;
            out->num = (u16)entry->value;
            out++;
            picked++;
        }
    }
    return picked;
}

/* The same walk without the first-roll override: every row rolls its own number. */
s32 quest_lot_pick(u8* chance, const Q_LotEntry* table, Q_ItemPair* out, s32 count, u16 total) {
    const Q_LotEntry* entry;
    s32 i;
    s32 picked = 0;

    for (i = 0; i < count; i++) {
        u16 roll;
        u16 acc;
        u32 hit;

        entry = table;
        if (entry->id == 0) {
            break;
        }
        roll = (u16)ran_suu(0) % total;
        hit = 0;
        acc = 0;
        while (entry->id != 0) {
            acc += entry->weight;
            if (roll < acc) {
                hit = 1;
                break;
            }
            entry++;
        }
        if (hit == 1 && entry->id != 0) {
            out->id = entry->id;
            out->num = (u16)entry->value;
            out++;
            picked++;
        }
    }
    return picked;
}

/* The same walk for the band that gates on the row's own chance byte only. */
s32 quest_lot_pick_last(u8* chance, const Q_LotEntry* table, Q_ItemPair* out, s32 count, u16 total) {
    const Q_LotEntry* entry;
    s32 i;
    s32 picked = 0;

    for (i = 0; i < count; i++, chance++) {
        u16 roll;
        u16 acc;
        u32 hit;

        entry = table;
        if (entry->id == 0) {
            break;
        }
        if (*chance <= ((u16)ran_suu(0) & 0x1F)) {
            break;
        }
        roll = (u16)ran_suu(0) % total;
        hit = 0;
        acc = 0;
        while (entry->id != 0) {
            acc += entry->weight;
            if (roll < acc) {
                hit = 1;
                break;
            }
            entry++;
        }
        if (hit == 1 && entry->id != 0) {
            out->id = entry->id;
            out->num = (u16)entry->value;
            out++;
            picked++;
        }
    }
    return picked;
}

/* The four-group roll: once the result row's live bit, the element state probe or the first pick gate
 * lets it through, it fills the first two 8-pair groups of `quest_item_pair_tbl_a` from the item
 * work's first two element tables, then - with the four-byte chance table copied over the buffer's
 * head - the two remaining four-pair groups from its third and fourth tables. */
void quest_pair_roll_all(_PLW* owner, u8 state) {
    u8 chance[16];
    Q_ItemWork* item;
    u16 total;
    Q_LotEntry* entry;

    memcpy(chance, quest_pair_chance_tbl_a, sizeof(chance));
    quest_pl_skill_slot_set(owner, chance, 0);
    item = move_work_item_work_get();
    memset(quest_item_pair_tbl_a, 0, sizeof(quest_item_pair_tbl_a));
    memset(quest_item_pair_tbl_b, 0, sizeof(quest_item_pair_tbl_b));
    memset(quest_item_pair_tbl_c, 0, sizeof(quest_item_pair_tbl_c));
    memset(quest_item_pair_tbl_d, 0, sizeof(quest_item_pair_tbl_d));
    if (state != 4) {
        return;
    }
    if ((item->record_0x3C->flags_0x310 & 0x00800000) != 0 || quest_element_state_ck() == 1 ||
        quest_element_pick_ck((QuestWork*)item, 0, 1) != 0) {
        entry = item->lot_e0a_0x9C;
        total = 0;
        while (entry->id != 0) {
            total += entry->weight;
            entry++;
        }
        if (total != 0) {
            quest_lot_pick_first(chance, item->lot_e0a_0x9C, quest_item_pair_tbl_a, 8, total);
        }
        entry = item->lot_e0b_0xC8;
        total = 0;
        while (entry->id != 0) {
            total += entry->weight;
            entry++;
        }
        if (total != 0) {
            quest_lot_pick_last(chance + 8, item->lot_e0b_0xC8, quest_item_pair_tbl_a + 8, 8, total);
        }
    }
    memcpy(chance, quest_pair_chance_tbl_d, sizeof(quest_pair_chance_tbl_d));
    if (quest_element_pick_ck((QuestWork*)item, 1, 1) != 0) {
        entry = item->lot_e1_0xFC;
        total = 0;
        while (entry->id != 0) {
            total += entry->weight;
            entry++;
        }
        if (total != 0) {
            quest_lot_pick_last(chance, item->lot_e1_0xFC, quest_item_pair_tbl_a + 0x20, 4, total);
        }
    }
    if (quest_element_pick_ck((QuestWork*)item, 2, 1) != 0) {
        entry = item->lot_e2_0x15C;
        total = 0;
        while (entry->id != 0) {
            total += entry->weight;
            entry++;
        }
        if (total != 0) {
            quest_lot_pick_last(chance, item->lot_e2_0x15C, quest_item_pair_tbl_a + 0x24, 4, total);
        }
    }
}

/* The two-group roll that picks both groups with `quest_lot_pick_first` and sets the pl skill slots;
 * the clause `quest_element_pick_ck` gates walks the item work's first two element tables. */
void quest_pair_roll_first(_PLW* owner, u8 state) {
    u8 chance[16];
    Q_ItemWork* item;
    u16 total;
    Q_LotEntry* entry;

    memcpy(chance, quest_pair_chance_tbl_b, sizeof(chance));
    quest_pl_skill_slot_set(owner, chance, 1);
    item = move_work_item_work_get();
    memset(quest_item_pair_tbl_a, 0, sizeof(quest_item_pair_tbl_a));
    memset(quest_item_pair_tbl_b, 0, sizeof(quest_item_pair_tbl_b));
    memset(quest_item_pair_tbl_c, 0, sizeof(quest_item_pair_tbl_c));
    memset(quest_item_pair_tbl_d, 0, sizeof(quest_item_pair_tbl_d));
    if (state != 4) {
        return;
    }
    if (quest_element_pick_ck((QuestWork*)item, 0, 1) == 0) {
        return;
    }
    entry = item->lot_e0a_0x9C;
    total = 0;
    while (entry->id != 0) {
        total += entry->weight;
        entry++;
    }
    if (total != 0) {
        quest_lot_pick_first(chance, item->lot_e0a_0x9C, quest_item_pair_tbl_a, 8, total);
    }
    entry = item->lot_e0b_0xC8;
    total = 0;
    while (entry->id != 0) {
        total += entry->weight;
        entry++;
    }
    if (total != 0) {
        quest_lot_pick_first(chance + 8, item->lot_e0b_0xC8, quest_item_pair_tbl_a + 8, 8, total);
    }
}

/* The same two-group roll with the pl skill slots cleared and the second group picked with
 * `quest_lot_pick_last`. */
void quest_pair_roll_last(_PLW* owner, u8 state) {
    u8 chance[16];
    Q_ItemWork* item;
    u16 total;
    Q_LotEntry* entry;

    memcpy(chance, quest_pair_chance_tbl_b, sizeof(chance));
    quest_pl_skill_slot_set(owner, chance, 0);
    item = move_work_item_work_get();
    memset(quest_item_pair_tbl_a, 0, sizeof(quest_item_pair_tbl_a));
    memset(quest_item_pair_tbl_b, 0, sizeof(quest_item_pair_tbl_b));
    memset(quest_item_pair_tbl_c, 0, sizeof(quest_item_pair_tbl_c));
    memset(quest_item_pair_tbl_d, 0, sizeof(quest_item_pair_tbl_d));
    if (state != 4) {
        return;
    }
    if (quest_element_pick_ck((QuestWork*)item, 0, 1) == 0) {
        return;
    }
    entry = item->lot_e0a_0x9C;
    total = 0;
    while (entry->id != 0) {
        total += entry->weight;
        entry++;
    }
    if (total != 0) {
        quest_lot_pick_first(chance, item->lot_e0a_0x9C, quest_item_pair_tbl_a, 8, total);
    }
    entry = item->lot_e0b_0xC8;
    total = 0;
    while (entry->id != 0) {
        total += entry->weight;
        entry++;
    }
    if (total != 0) {
        quest_lot_pick_last(chance + 8, item->lot_e0b_0xC8, quest_item_pair_tbl_a + 8, 8, total);
    }
}

/* Rolls the part-break rewards: for each of the result row's (two under quest flag 0x100) slot keys, and then
 * for every one of the 41 keys, each listed part with a break count rolls its tier's remaining-lot table
 * (picks and flagged picks from the count's reward group; four rolls per key at most, key 20's four monsters
 * once per saved stat); the rolls are then packed into the first two picked-pair tables. */
void quest_part_reward_roll(QuestWork* work) {
    u8 chance[0xC];
    s32 i;
    Q_ItemPair* out;
    u8* entry;
    s32 base;
    s32 slot;
    s32 progress;
    s32 tier;
    s32 n;
    s32 count;
    Q_RewardGroup* group;
    s32 picks;
    s32 rolls;
    u8* key;
    s32 flagged;
    Q_LotEntry* table;
    Q_LotEntry* lot;
    u16 total;
    s32 j;
    s32 k;
    Q_ItemPair* src;
    Q_ItemPair* pair;

    progress = quest_slot_progress_get(NULL);
    if (progress == 2) {
        return;
    }
    if (work->stat_0x6AA4.valid_0x00 == 1) {
        if (work->stat_0x6AA4.flags_0x08 & 1) {
            work->part_counts_0x6803[20][3] = 0;
        }
        if (work->stat_0x6AA4.flags_0x08 & 2) {
            work->part_counts_0x6803[20][1] = 0;
        }
        if (work->stat_0x6AA4.flags_0x08 & 4) {
            work->part_counts_0x6803[20][4] = 0;
        }
        if (work->stat_0x6AA4.flags_0x08 & 8) {
            work->part_counts_0x6803[20][2] = 0;
        }
    }
    key = &work->record_0x03C->slot_bytes_0x314[0].value;
    if (key != NULL) {
        for (i = 0, out = quest_item_pair_tbl_c; i < 3; out += 0x10, i++, key += 8) {
            if (quest_flag_100_ck(NULL) == 1 && i == 2) {
                break;
            }
            if (*key == 0) {
                continue;
            }
            entry = em_bui_tbl[*key];
            if (entry == NULL) {
                continue;
            }
            rolls = 0;
            for (; entry[0] != 0; entry += 2) {
                if (rolls >= 4) {
                    break;
                }
                tier = 0;
                n = 0;
                count = 0;
                if (work->part_counts_0x6803[*key][entry[0]] != 0) {
                    tier = entry[1];
                    count = work->part_counts_0x6803[*key][entry[0]];
                    if (count > 5) {
                        count = 5;
                    }
                    group = &quest_reward_group_tbl[count];
                    picks = group->picks;
                    flagged = group->flag;
                    j = 0;
                    for (; j < picks; j++) {
                        chance[j] = 32;
                    }
                    for (; j < picks + flagged; j++) {
                        chance[j] = 22;
                    }
                    for (; j < 12; j++) {
                        chance[j] = 0;
                    }
                    work->part_counts_0x6803[*key][entry[0]] = 0;
                }
                if (tier <= 0 || count <= 0) {
                    continue;
                }
                if (*key == 20) {
                    if (entry[0] == 3) {
                        if (work->stat_0x6AA4.flags_0x08 & 1) {
                            continue;
                        }
                        work->stat_0x6AA4.flags_0x08 |= 1;
                    }
                    if (entry[0] == 1) {
                        if (work->stat_0x6AA4.flags_0x08 & 2) {
                            continue;
                        }
                        work->stat_0x6AA4.flags_0x08 |= 2;
                    }
                    if (entry[0] == 4) {
                        if (work->stat_0x6AA4.flags_0x08 & 4) {
                            continue;
                        }
                        work->stat_0x6AA4.flags_0x08 |= 4;
                    }
                    if (entry[0] == 2) {
                        if (work->stat_0x6AA4.flags_0x08 & 8) {
                            continue;
                        }
                        work->stat_0x6AA4.flags_0x08 |= 8;
                    }
                }
                if (progress == 0) {
                    table = em_bui_rem_l[tier - 1];
                } else {
                    table = em_bui_rem_h[tier - 1];
                }
                total = 0;
                for (lot = table; lot->id != 0; lot++) {
                    total += lot->weight;
                }
                if (total == 0) {
                    continue;
                }
                if (progress == 0) {
                    table = em_bui_rem_l[tier - 1];
                } else {
                    table = em_bui_rem_h[tier - 1];
                }
                slot = rolls * 3;
                if (picks > 0) {
                    n = quest_lot_pick(chance, table, &out[slot], picks, total);
                }
                if (flagged > 0) {
                    quest_lot_pick_last(&chance[n], table, &out[n + slot], flagged, total);
                }
                rolls++;
            }
        }
    }
    for (k = 0, out = quest_item_pair_tbl_d; k < 41; k++) {
        entry = em_bui_tbl[k];
        if (entry == NULL) {
            continue;
        }
        rolls = 0;
        for (; entry[0] != 0; entry += 2) {
            if (rolls >= 4) {
                break;
            }
            tier = 0;
            n = 0;
            count = 0;
            if (work->part_counts_0x6803[k][entry[0]] != 0) {
                tier = entry[1];
                count = work->part_counts_0x6803[k][entry[0]];
                if (count > 5) {
                    count = 5;
                }
                group = &quest_reward_group_tbl[count];
                picks = group->picks;
                flagged = group->flag;
                j = 0;
                for (; j < picks; j++) {
                    chance[j] = 32;
                }
                for (; j < picks + flagged; j++) {
                    chance[j] = 22;
                }
                if (j < 12) {
                    chance[0] = 0;
                }
                work->part_counts_0x6803[k][entry[0]] = 0;
            }
            if (tier <= 0 || count <= 0) {
                continue;
            }
            if (progress == 0) {
                table = em_bui_rem_l[tier - 1];
            } else {
                table = em_bui_rem_h[tier - 1];
            }
            total = 0;
            for (lot = table; lot->id != 0; lot++) {
                total += lot->weight;
            }
            if (total == 0) {
                continue;
            }
            if (progress == 0) {
                table = em_bui_rem_l[tier - 1];
            } else {
                table = em_bui_rem_h[tier - 1];
            }
            slot = rolls * 3;
            if (picks > 0) {
                n = quest_lot_pick(chance, table, &out[slot], picks, total);
            }
            if (flagged > 0) {
                quest_lot_pick_last(&chance[n], table, &out[n + slot], flagged, total);
            }
            rolls++;
        }
        if (rolls > 0) {
            out += 0x10;
        }
    }
    for (i = 0, src = quest_item_pair_tbl_c, base = 0x10; i < 2; src += 0x10, base += 0x10, i++) {
        n = 0;
        for (j = 0, pair = src; j < 12; pair++, j++) {
            if (pair->id != 0) {
                item_pair_copy(&quest_item_pair_tbl_a[n + base], pair);
                n++;
            }
        }
    }
    for (i = 0, src = quest_item_pair_tbl_d, base = 0x10; i < 2; src += 0x10, base += 12, i++) {
        n = 0;
        for (j = 0, pair = src; j < 12; pair++, j++) {
            if (pair->id != 0) {
                item_pair_copy(&quest_item_pair_tbl_b[n + base], pair);
                n++;
            }
        }
    }
}

/* Rolls the capture rewards: each of the result row's (two under quest flag 0x100) slot keys with a capture
 * count, then the first three captured keys of all 41, roll that key's capture lot table (two picks, one
 * more with pl skill 0xBD, one more for two or more captures) into the tail of their picked-pair rows, which
 * are then packed after the part-break rewards in the first two picked-pair tables. */
void quest_capture_reward_roll(QuestWork* work, _PLW* owner) {
    u8 chance[0xC];
    s32 picks;
    s32 i;
    u8* key;
    s32 count;
    s32 j;
    s32 progress;
    Q_LotEntry* table;
    Q_LotEntry* lot;
    u16 total;
    s32 rolls;
    s32 k;
    s32 n;
    s32 base;
    Q_ItemPair* src;

    progress = quest_slot_progress_get(NULL);
    if (progress == 2) {
        return;
    }
    for (i = 0; i < 3; i++) {
        if (quest_flag_100_ck(NULL) == 1 && i == 2) {
            break;
        }
        key = &work->record_0x03C->slot_bytes_0x314[i].value;
        if (key == NULL || *key == 0) {
            continue;
        }
        count = work->capture_counts_0x4E2[*key];
        if (count == 0) {
            continue;
        }
        picks = 2;
        if (Pl_Skill_ck(owner, 0xBD) == 1) {
            picks = 3;
        }
        if (count >= 2) {
            picks++;
        }
        j = 0;
        for (; j < picks; j++) {
            chance[j] = 32;
        }
        for (; j < picks; j++) {
            chance[j] = 22;
        }
        for (; j < 12; j++) {
            chance[j] = 0;
        }
        work->capture_counts_0x4E2[*key] = 0;
        if (progress == 0) {
            table = em_hokaku_rem_l[*key];
        } else {
            table = em_hokaku_rem_h[*key];
        }
        total = 0;
        for (lot = table; lot->id != 0; lot++) {
            total += lot->weight;
        }
        if (total == 0) {
            continue;
        }
        if (progress == 0) {
            table = em_hokaku_rem_l[*key];
        } else {
            table = em_hokaku_rem_h[*key];
        }
        if (picks > 0) {
            quest_lot_pick(chance, table, &(quest_item_pair_tbl_c + i * 0x10)[12], picks, total);
        }
    }
    for (rolls = 0, k = 0; k < 41; k++) {
        if (rolls >= 3) {
            break;
        }
        n = work->capture_counts_0x4E2[k];
        if (n == 0) {
            continue;
        }
        picks = 2;
        if (Pl_Skill_ck(owner, 0xBD) == 1) {
            picks = 3;
        }
        if (n >= 2) {
            picks++;
        }
        j = 0;
        for (; j < picks; j++) {
            chance[j] = 32;
        }
        for (; j < picks; j++) {
            chance[j] = 22;
        }
        for (; j < 12; j++) {
            chance[j] = 0;
        }
        work->capture_counts_0x4E2[k] = 0;
        if (progress == 0) {
            table = em_hokaku_rem_l[k];
        } else {
            table = em_hokaku_rem_h[k];
        }
        total = 0;
        for (lot = table; lot->id != 0; lot++) {
            total += lot->weight;
        }
        if (total == 0) {
            continue;
        }
        if (progress == 0) {
            table = em_hokaku_rem_l[k];
        } else {
            table = em_hokaku_rem_h[k];
        }
        if (picks > 0) {
            quest_lot_pick(chance, table, &(quest_item_pair_tbl_d + rolls * 0x10)[12], picks, total);
        }
        rolls++;
    }
    for (i = 0, src = quest_item_pair_tbl_c, base = 28; i < 2; src += 0x10, base += 0x10, i++) {
        n = 0;
        for (j = 0; j < 4; j++) {
            if (src[j + 12].id != 0) {
                item_pair_copy(&quest_item_pair_tbl_a[n + base], &src[j + 12]);
                n++;
            }
        }
    }
    for (i = 0, src = quest_item_pair_tbl_d, base = 40; i < 2; src += 0x10, base += 4, i++) {
        n = 0;
        for (j = 0; j < 4; j++) {
            if (src[j + 12].id != 0) {
                item_pair_copy(&quest_item_pair_tbl_b[n + base], &src[j + 12]);
                n++;
            }
        }
    }
}

/* Rolls the two tiers' monster lot tables (the arena's own table under quest flag 0x100) into the second
 * picked-pair table, 8 picks per roll and two rolls at most, the higher tier first when the move work says
 * so. */
void quest_monster_setup(Q_ItemWork* item, _PLW* owner) {
    u8 chance[0x10];
    u8 order[2];
    s32 rolls;
    s32 tier;
    s32 i;
    Q_LotEntry* table;
    Q_LotEntry* entry;
    u16 total;
    Q_MoveWork* work;

    memcpy(chance, quest_pair_chance_tbl_c, sizeof(chance));
    quest_pl_skill_slot_set(owner, chance, 0);
    work = (Q_MoveWork*)get_move_work_adrs(0);
    if (work == NULL) {
        return;
    }
    rolls = 0;
    order[0] = 0;
    order[1] = 1;
    if (item->tier_count_0x6980[0] != 0 && item->tier_count_0x6980[1] != 0 && work->spawn_0x2274[3].monster_0x6 > work->spawn_0x2274[4].monster_0x6) {
        order[0] = 1;
        order[1] = 0;
    }
    for (tier = 0; tier < 2; tier++) {
        for (i = 0; i < item->tier_count_0x6980[tier]; i++) {
            if (quest_flag_100_ck(NULL) == 1) {
                table = item->lot_e2_0x15C;
            } else {
                table = nora_set_proc.lot_table_get(item->tier_monster_0x6984[tier]);
            }
            if (table != NULL) {
                total = 0;
                for (entry = table; entry->id != 0; entry++) {
                    total += entry->weight;
                }
                if (total != 0) {
                    quest_lot_pick_last(chance, table, &quest_item_pair_tbl_b[order[rolls] * 8], 8, total);
                    rolls++;
                    if (rolls >= 2) {
                        return;
                    }
                }
            }
        }
    }
}

/* The same build for the band's persisted pair tables: zeroes all four, then refills the first from
 * the item work's own weighted lot table - 3, 5 or 8 rows, chosen by the element's sub-flag. */
/* untyped: caller-owned context payload the target never reads */
void quest_element_clear(void* owner, u32 kind, Q_ArenaElement* element) {
    u8 chance[8];
    u16 total;
    Q_LotEntry* entry;
    s32 rows;

    memset(quest_item_pair_tbl_a, 0, sizeof(quest_item_pair_tbl_a));
    memset(quest_item_pair_tbl_b, 0, sizeof(quest_item_pair_tbl_b));
    memset(quest_item_pair_tbl_c, 0, sizeof(quest_item_pair_tbl_c));
    memset(quest_item_pair_tbl_d, 0, sizeof(quest_item_pair_tbl_d));
    if ((u8)kind != 4) {
        return;
    }
    {
        Q_ItemWork* item = move_work_item_work_get();

        memset(chance, 0, sizeof(chance));
        if (element->flag_0x434 == 0) {
            chance[0] = 32;
            chance[1] = 22;
            chance[2] = 12;
            rows = 3;
        } else if (element->flag_0x434 == 1) {
            chance[0] = 32;
            chance[1] = 22;
            chance[2] = 22;
            chance[3] = 12;
            chance[4] = 12;
            rows = 5;
        } else {
            chance[0] = 32;
            chance[1] = 32;
            chance[2] = 22;
            chance[3] = 22;
            chance[4] = 22;
            chance[5] = 12;
            chance[6] = 12;
            chance[7] = 12;
            rows = 8;
        }
        entry = item->lot_0x9C;
        total = 0;
        while (entry->id != 0) {
            total += entry->weight;
            entry++;
        }
        if (total == 0) {
            return;
        }
        quest_lot_pick_first(chance, item->lot_0x9C, quest_item_pair_tbl_a, rows, total);
    }
}

/* Clears the element's payload, then fills it from the item work's own weighted lot table: a table of
 * 3 or 5 rows, chosen by the element's own sub-flag, and only when the caller passed the arena kind. */
/* untyped: caller-owned context payload the target never reads */
void quest_element_build(void* owner, u32 kind, Q_ArenaElement* element) {
    u8 chance[6];
    u16 total;
    Q_LotEntry* entry;
    s32 rows;

    memset(element->payload_0x3F4, 0, sizeof(element->payload_0x3F4));
    if ((u8)kind != 4) {
        return;
    }
    {
        Q_ItemWork* item = move_work_item_work_get();

        memset(chance, 0, sizeof(chance));
        if (element->flag_0x434 == 0) {
            chance[0] = 32;
            chance[1] = 22;
            chance[2] = 12;
            rows = 3;
        } else {
            chance[0] = 32;
            chance[1] = 22;
            chance[2] = 22;
            chance[3] = 12;
            chance[4] = 12;
            rows = 5;
        }
        if ((u8)kind != 4) {
            return;
        }
        entry = item->lot_0x9C;
        total = 0;
        while (entry->id != 0) {
            total += entry->weight;
            entry++;
        }
        if (total == 0) {
            return;
        }
        quest_lot_pick_first(chance, item->lot_0x9C, element->payload_0x3F4, rows, total);
    }
}

/* Credits a finished quest's two count runs (and, for a finished row, its stat) to the save block, then
 * clears every block of the quest result work. */
void quest_result_work_flush(void) {
    Q_ResultWork* result = get_qResult_work();
    Q_MoveWork* work = (Q_MoveWork*)get_move_work_adrs(0);
    Q_ItemWork* item;

    if (work == NULL) {
        return;
    }
    item = move_work_item_work_get();
    if (work->sub_0xFA != 7) {
        userdata_record_a_count_add(result->count_a);
        userdata_record_b_count_add(result->count_b);
        if (item->record_0x3C->state_0x8B == 7) {
            userdata_quest_stat_set(&result->stat_0x3E0);
        }
    }
    memset(result->count_a, 0, sizeof(result->count_a));
    memset(result->count_b, 0, sizeof(result->count_b));
    memset(result->block_0x0A4, 0, sizeof(result->block_0x0A4));
    result->field_0x1E4 = 0;
    result->field_0x1E8 = 0;
    memset(result->block_0x304, 0, sizeof(result->block_0x304));
    result->present_0x3A4 = 0;
    memset(&result->stat_0x3E0, 0, sizeof(result->stat_0x3E0));
}

/* One 8-byte row of a quest list file after its 8-byte header: the record's byte offset in the file and the
 * key `quest_list_values` keeps for it. */
typedef struct QuestListFileRow {
    /* +0x0 */ u32 offset;
    /* +0x4 */ u32 key;
} QuestListFileRow; /* size: 0x8 */

/* The quest list file image: a header whose second word is the row count, then the rows. */
typedef struct QuestListFile {
    /* +0x0 */ u32 unused_0x0;
    /* +0x4 */ s32 count;
    /* +0x8 */ QuestListFileRow rows[1];
} QuestListFile; /* size: 0x10 (lower bound) */

/* Allocates the quest list block, loads the hunt list file (the high-tier one in the demo play mode past
 * id 10000, in play mode 4 or on the server-select screen) and builds the record and key arrays from it. */
void quest_list_load_hunt(void) {
    char path[0x80];
    QuestListBlock* pool;
    ResFileEntry* file;
    QuestListFile* image;
    QuestListFileRow* row;
    QuestRecord** items;
    u16* values;
    s32 i;
    s32 count;

    pool = (QuestListBlock*)work_mem_alloc(sizeof(QuestListBlock));
    quest_list_pool = pool;
    quest_list_values = pool->values;
    quest_list_file = (u8*)&quest_list_values[0x70];
    memset(pool, 0, sizeof(QuestListBlock));
    if (demo_play_ck() == 1) {
        if (system_w.leave_state_0x7d8 >= 10000) {
            file = &quest_file_table[1];
        } else {
            file = &quest_file_table[0];
        }
    } else if (PlayMode_ck() == 4 || isServerSelectState() == 1) {
        file = &quest_file_table[1];
    } else {
        file = &quest_file_table[0];
    }
    cnvt_eur_fname(path, (char*)file->name);
    load_file(path, (u32)quest_list_file, file->size);
    image = (QuestListFile*)quest_list_file;
    items = quest_list_pool->items;
    values = quest_list_values;
    count = image->count;
    row = image->rows;
    quest_list_count = count;
    for (i = 0; i < count; i++) {
        *items = (QuestRecord*)(row->offset + (u32)quest_list_file);
        *values = row->key;
        items++;
        values++;
        row++;
    }
    quest_list_items = quest_list_pool->items;
}

/* Loads the quest list (the hunt lists for `kind` 0, else the arena list), clears the quest work block and
 * fills it: the eight per-category id stacks, the result record, the save block mirror and the timers. */
QuestWork* quest_init(u8 kind) {
    QuestRecord** items;
    QuestRecord* rec;
    QuestEntrySlot* block;
    u16 id;
    s32 i;
    s32 j;
    u8 category;

    if (kind == 0) {
        quest_list_load_hunt();
    } else {
        quest_list_load_arena();
    }
    quest_work_ptr = &quest_work;
    memset(&quest_work, 0, sizeof(QuestWork));
    quest_work_ptr->field_0x6758 = 0x19;
    if (quest_work_ptr->field_0x6758 > 0) {
        for (i = 0; i < 8; i++) {
            quest_work_ptr->slot_values_0x6698[i] = (u16*)work_mem_alloc(quest_work_ptr->field_0x6758 * 2);
        }
    }
    quest_work_ptr->slot_counts_0x66B8[0] = 0;
    quest_work_ptr->slot_counts_0x66B8[1] = 0;
    quest_work_ptr->slot_counts_0x66B8[2] = 0;
    quest_work_ptr->slot_counts_0x66B8[3] = 0;
    quest_work_ptr->slot_counts_0x66B8[4] = 0;
    quest_work_ptr->slot_counts_0x66B8[5] = 0;
    quest_work_ptr->slot_counts_0x66B8[6] = 0;
    quest_work_ptr->slot_counts_0x66B8[7] = 0;
    quest_work_ptr->record_0x03C = (QuestRecord*)work_mem_alloc(0x3000);
    items = quest_list_items;
    i = 0;
    while (i < quest_list_count) {
        rec = *items++;
        category = rec->category_0x08A;
        id = rec->field_0x02C;
        for (j = 0; j < 16; j++) {
            if (id < quest_id_threshold_tbl[j]) {
                break;
            }
        }
        quest_work_ptr->slot_values_0x6698[category][quest_work_ptr->slot_counts_0x66B8[category]] = id;
        quest_work_ptr->slot_counts_0x66B8[category]++;
        i++;
    }
    block = quest_work_ptr->slots_0x34C;
    for (i = 0; i < 6; i++) {
        memset(block, 0, sizeof(QuestEntrySlot));
        block++;
    }
    memcpy(&quest_work_ptr->userdata_0x698, get_userdata(), sizeof(Q_UserData));
    quest_stat_copy(&quest_work_ptr->stat_0x6AA4, &quest_work_ptr->userdata_0x698.quest_stat);
    quest_work_ptr->timer_0x6A48 = quest_timer_zero_d;
    quest_work_ptr->timer_0x6A50 = quest_timer_zero_d;
    quest_work_ptr->timer_0x6A58 = quest_timer_start_d;
    quest_work_ptr->slot_ids_0x309[0] = 0xFF;
    quest_work_ptr->slot_ids_0x309[1] = 0xFF;
    quest_work_ptr->slot_ids_0x309[2] = 0xFF;
    return quest_work_ptr;
}

/* Allocates the quest list block, loads the arena list file and builds the record and key arrays from it. */
void quest_list_load_arena(void) {
    char path[0x80];
    QuestListBlock* pool;
    ResFileEntry* table;
    QuestListFile* image;
    QuestListFileRow* row;
    QuestRecord** items;
    u16* values;
    s32 i;
    s32 count;

    pool = (QuestListBlock*)work_mem_alloc(sizeof(QuestListBlock));
    quest_list_pool = pool;
    quest_list_values = pool->values;
    quest_list_file = (u8*)&quest_list_values[0x70];
    memset(pool, 0, sizeof(QuestListBlock));
    table = &quest_file_table[3];
    cnvt_eur_fname(path, (char*)table->name);
    load_file(path, (u32)quest_list_file, table->size);
    image = (QuestListFile*)quest_list_file;
    items = quest_list_pool->items;
    values = quest_list_values;
    count = image->count;
    row = image->rows;
    quest_list_count = count;
    for (i = 0; i < count; i++) {
        *items = (QuestRecord*)(row->offset + (u32)quest_list_file);
        *values = row->key;
        items++;
        values++;
        row++;
    }
    quest_list_items = quest_list_pool->items;
}

/* The local slot's quest phase byte (+0xE9 of the move work). */
u8 quest_phase_get(void) {
    Q_MoveWork* work = (Q_MoveWork*)get_move_work_adrs(0);

    if (work == NULL) {
        return 0;
    }
    return work->phase_0xE9;
}

/* Loads the carried-item pouch into the quest work: the local player's own gear in a network session, else
 * the two runs the player's record of the third move-work array carries. */
void quest_work_pouch_load(void) {
    _PLW* me;
    Q_ItemPair* src;
    Q_UserData* user = get_userdata();
    s32 i;

    if (quest_work_ptr != NULL) {
        me = (_PLW*)get_move_work_adrs(2);

        if (me != NULL) {
            me += (s8)my_player_no();

            memset(QUEST_WORK->pouch_0x6778, 0, 0x8C);
            if (system_w.net_session_0x90f == 1) {
                src = (Q_ItemPair*)userdata_equip_item_slots_get(user);

                for (i = 0; i < 0x18; i++, src++) {
                    item_pair_copy(&QUEST_WORK->pouch_0x6778[i], src);
                }
                if ((u32)userdata_gunner_ck(user) == 1) {
                    for (i = 0; i < 8; i++, src++) {
                        item_pair_copy(&QUEST_WORK->pouch_0x6778[i + 0x18], src);
                    }
                }
            } else {
                memcpy(QUEST_WORK->pouch_0x6778, me->slot_id, 0x60);
                memcpy(&QUEST_WORK->pouch_0x6778[0x18], me->spare_slot_id, 0x20);
            }
        }
    }
}

/* Releases the monsters of every armed element (stopping early when the quest kind says the hunt is over),
 * then loads the lobby area for the hand-off and leaves the quest play mode. */
void quest_monsters_release(void) {
    Q_MoveWork* work;
    u8 mode;
    u32 active;
    s32 i;
    s32 released;

    if (quest_work_ptr != NULL) {
        work = (Q_MoveWork*)get_move_work_adrs(0);
        if (work != NULL) {
            active = 0;
            mode = PlayMode_ck();
            if (quest_work_ptr != NULL && QUEST_WORK->hunt_active_0x675C != 0) {
                active = 1;
            }
            if (active == 1) {
                released = 0;
                for (i = 0; i < 3; i++) {
                    if (quest_work_ptr->player_state_0x2D4[i] != 0) {
                        em_kind_release(quest_work_ptr->player_state_0x2D4[i]);
                        released++;
                        if (quest_flag_4000000_ck(NULL) == 1) {
                            QUEST_WORK->hunt_end_0x6977 = 1;
                            QUEST_WORK->result_kind_0x308 = 1;
                            break;
                        }
                        if (quest_flag_80000000_ck(NULL) == 1) {
                            QUEST_WORK->hunt_end_0x6977 = 1;
                            QUEST_WORK->result_kind_0x308 = 1;
                            break;
                        }
                        if (quest_flag_100_ck(NULL) == 1 && released == 2) {
                            QUEST_WORK->hunt_end_0x6977 = 1;
                            QUEST_WORK->result_kind_0x308 = 2;
                            break;
                        }
                    }
                }
                if (quest_flag_4000000_ck(NULL) == 0) {
                    if (quest_flag_100_ck(NULL) == 1) {
                        em_kind_release(QUEST_WORK->release_kind_0x6760);
                    } else if (QUEST_WORK->release_kind_0x6760 != 0 && released <= 1) {
                        em_kind_release(QUEST_WORK->release_kind_0x6760);
                    }
                }
                quest_area_spawn_setup(work, QUEST_WORK->area_0x69A8, QUEST_WORK->area_count_0x69A5, 0);
                quest_area_spawn_apply(work, quest_byte_table[work->phase_0xE9], work->sub_0xEA);
                if (mode == 6) {
                    PlayMode_set(3);
                }
            }
        }
    }
}

/* Re-rolls the quest work's clock words, reseeds the quest random source from them and clears the
 * per-quest state. */
#pragma opt_lifetimes off
void quest_work_start_reset(void) {
    Q_ItemWork* work = QUEST_WORK;
    s32 i;

    if (work == NULL) {
        return;
    }
    for (i = 0; i < 15; i++) {
        work->rand_words_0x6C[i] = (u8)ran_suu(0);
    }
    quest_rand_seed_set();
    quest_rand_next();
    for (i = 0; i < 4; i++) {
        work->rand_words_0x6C[i] = (u8)quest_rand_next();
    }
    work->field_0x60 = 0;
    work->field_0x5C = 0;
    work->field_0x16 = 0;
    work->field_0x30C = 0;
    work->rot_0x54 = 0;
    work->flag_0x55 = 0;
    work->field_0x00 = 0;
    work->field_0x01 = 0;
    work->time_limit_0x24 = 0;
    work->field_0x14 = 0;
    work->armed_0x2D4[0] = 0;
    work->slot_key_0x46C[0] = 0xFFFF;
    work->armed_0x2D4[1] = 0;
    work->slot_key_0x46C[1] = 0xFFFF;
    work->armed_0x2D4[2] = 0;
    work->slot_key_0x46C[2] = 0xFFFF;
}
#pragma opt_lifetimes reset

/* Marks every live element as graded and derives the quest grade from which of them the bonus flag
 * covers, then copies each finished element's value into its grade pair. */
void quest_grade_set(void) {
    Q_Element* e = QUEST_WORK->elements_0x94;
    u32 bonus = 0;
    s32 i;
    u8 mask;

    if (quest_flag_2000000_ck(NULL) == 1) {
        bonus = 1;
    }
    mask = 0;
    for (i = 0; i < 3; i++, e++) {
        if ((e->flags & 1) || (e->flags & 2) || (e->flags & 0x1000) || (e->flags & 0x2000) || (e->flags & 4)) {
            e->flags = e->flags | 8;
            if (bonus == 1) {
                mask = (u8)(mask | (u8)(1 << i));
            }
        }
    }
    if (bonus == 1) {
        QUEST_WORK->grade_0x93 = quest_grade_table[mask];
    }
    if (quest_flag_80000000_ck(NULL) == 1) {
        QUEST_WORK->grade_0x93 = 2;
    }
    {
        Q_ItemWork* w = QUEST_WORK;

        for (i = 0; i < 3; i++) {
            if ((w->elements_0x94[i].flags & 2) && w->elements_0x94[i].id == 0) {
                QUEST_WORK->grade_pair_0x68F[i].flag = 0;
                QUEST_WORK->grade_pair_0x68F[i].value = (s8)w->elements_0x94[i].value;
            }
        }
    }
    {
        Q_ItemWork* w = QUEST_WORK;

        for (i = 0; i < 3; i++) {
            if ((w->elements_0x94[i].flags & 0x2000) && w->elements_0x94[i].id == 4) {
                QUEST_WORK->score_0x5D8 = w->elements_0x94[i].value;
            }
        }
    }
}

/* Spawns the current quest work's monsters; the caller's argument is not read. */
void quest_monsters_spawn_now(s32 unused) {
    quest_monsters_spawn(QUEST_WORK);
}

/* The state byte of a result row (the current one when `rec` is NULL), 0 when there is none. */
u8 quest_record_state_get(QuestRecord* rec) {
    if (rec == NULL) {
        rec = quest_record_get();
        if (rec == NULL) {
            return 0;
        }
    }
    return rec->field_0x08B;
}

/* The index of the quest-work element `kind` names: the first of the three whose +0x00 gate bit is set
 * and whose +0x04 id either equals `kind` or maps to the same enemy kind, or -1 when none does. */
s32 quest_element_find(u8 kind) {
    s32 i;
    QuestElement* e = quest_work.elements_0x0094;

    if (e == NULL) {
        return -1;
    }
    for (i = 0; i < 3; i++, e++) {
        if (e->flags & 1) {
            if ((u32)e->id == (u32)kind) {
                return i;
            }
            if (enemy_kind_same_ck((u8)e->id, kind) == 1) {
                return i;
            }
        }
    }
    return -1;
}

/* Spawns the work block's keyed monsters into the move work's spawn records (stopping after the first under
 * quest flag 0x4000000, after two under flag 0x100), then the hand-off's release monster, whose variant for
 * the area's rank `rank` becomes the fourth record. */
void quest_monster_spawn_area(u8 rank, Q_MoveWork* work) {
    EmGroundRec rec;
    QuestEntrySlot* entry;
    s32 i;
    QuestEntrySlot* slot;
    Q_SlotPair* key;
    QuestSpawnRec* spawn;
    struct _ENEMY_WORK* enemy;
    QuestBossSpawn* boss;
    QuestMonsterVariantList* variant;
    s8 index;

    em_ground_rec_clear(&rec);
    slot = quest_work_ptr->slots_0x34C;
    key = quest_work_ptr->key_rows_0x0319;
    quest_work_ptr->spawn_count_0x308 = 0;
    for (i = 0; i < 3; i++, slot++, key++) {
        if (key->id_0x00 == 0) {
            continue;
        }
        quest_work_ptr->spawn_key_0x46C[quest_work_ptr->spawn_count_0x308] = quest_work_ptr->spawn_count_0x308;
        quest_work_ptr->spawn_args_0x478[0][quest_work_ptr->spawn_count_0x308] = key->arg_0x04;
        quest_work_ptr->spawn_args_0x478[1][quest_work_ptr->spawn_count_0x308] = key->arg_0x05;
        quest_work_ptr->spawn_args_0x478[2][quest_work_ptr->spawn_count_0x308] = key->arg_0x06;
        quest_work_ptr->spawn_args_0x478[3][quest_work_ptr->spawn_count_0x308] = key->arg_0x07;
        em_ground_rec_set(&rec, &slot->element_0x08);
        spawn = &work->spawn_0x2274[quest_work_ptr->spawn_count_0x308];
        spawn->order_0x0 = quest_work_ptr->spawn_key_0x46C[quest_work_ptr->spawn_count_0x308];
        spawn->slot_0x8 = slot;
        spawn->count_0x4 = slot->count_0x04;
        spawn->monster_0x6 = key->id_0x00;
        spawn->boss_0xC = NULL;
        if (key->kind_0x02 == 3) {
            boss = em_large_spawn(key->id_0x00, &rec, quest_work_ptr->spawn_count_0x308);
            if (boss == NULL) {
                quest_work_ptr->spawn_key_0x46C[quest_work_ptr->spawn_count_0x308] = 0xFFFF;
                continue;
            }
            enemy = boss->enemy_0x30;
            spawn->boss_0xC = boss;
        } else {
            enemy = em_small_spawn(key->id_0x00, &rec, quest_work_ptr->spawn_count_0x308);
            if (enemy == NULL) {
                quest_work_ptr->spawn_key_0x46C[quest_work_ptr->spawn_count_0x308] = 0xFFFF;
                continue;
            }
        }
        index = quest_element_find(key->id_0x00);
        if (index >= 0) {
            if (key->kind_0x02 == 3) {
                em_quest_element_set_large((struct _ENEMY_WORK*)boss, index);
            } else {
                em_quest_element_set(enemy, index);
            }
        }
        quest_work_ptr->spawn_count_0x308++;
        if (quest_flag_4000000_ck(NULL) == 1) {
            return;
        }
        if (quest_flag_100_ck(NULL) == 1 && quest_work_ptr->spawn_count_0x308 == 2) {
            return;
        }
    }
    if (quest_work_ptr->release_0x6760.id_0x00 != 0 && quest_work_ptr->spawn_count_0x308 <= 1) {
        variant = quest_monster_variant_tbl[quest_work_ptr->release_0x6760.id_0x00];
        if (variant == NULL) {
            return;
        }
        while (variant->rank != 0xFF) {
            if (variant->rank == rank) {
                break;
            }
            variant++;
        }
        if (variant->rank == 0xFF) {
            return;
        }
        entry = &variant->records[quest_work_ptr->release_0x6760.group_0x01];
        em_ground_rec_set(&rec, &entry->element_0x08);
        rec.unused_0x01[1] = 3;
        quest_work_ptr->spawn_key_0x46C[quest_work_ptr->spawn_count_0x308] = quest_work_ptr->spawn_count_0x308;
        quest_work_ptr->spawn_args_0x478[0][quest_work_ptr->spawn_count_0x308] = quest_work_ptr->release_0x6760.arg_0x04;
        quest_work_ptr->spawn_args_0x478[1][quest_work_ptr->spawn_count_0x308] = quest_work_ptr->release_0x6760.arg_0x05;
        quest_work_ptr->spawn_args_0x478[2][quest_work_ptr->spawn_count_0x308] = quest_work_ptr->release_0x6760.arg_0x06;
        quest_work_ptr->spawn_args_0x478[3][quest_work_ptr->spawn_count_0x308] = quest_work_ptr->release_0x6760.arg_0x07;
        if (em_large_spawn(quest_work_ptr->release_0x6760.id_0x00, &rec, quest_work_ptr->spawn_count_0x308) == NULL) {
            quest_work_ptr->spawn_key_0x46C[quest_work_ptr->spawn_count_0x308] = 0xFFFF;
            return;
        }
        work->spawn_0x2274[3].order_0x0 = quest_work_ptr->spawn_key_0x46C[quest_work_ptr->spawn_count_0x308];
        work->spawn_0x2274[3].slot_0x8 = entry;
        work->spawn_0x2274[3].count_0x4 = entry->count_0x04;
        work->spawn_0x2274[3].monster_0x6 = quest_work_ptr->release_0x6760.id_0x00;
        work->spawn_0x2274[3].boss_0xC = NULL;
        quest_work_ptr->spawn_count_0x308++;
    }
}

/* Spawns the quest's monsters: with a quest monster list, the area spawn and the "graded" flag on every
 * finished element; else every monster of the area's own list into the slot its key row (or its own element
 * index) names, two at most, then the hand-off's release monster into the first free key row. */
void quest_monsters_spawn(Q_ItemWork* item) {
    EmGroundRec rec;
    s32* order;
    QuestSpawnRec* spawn;
    Q_MoveWork* work;
    QuestBossSpawn* boss;
    s32 k;
    QuestEntrySlot* list;
    s32 i;
    QuestEntrySlot* quest_list;
    QuestEntrySlot* slot;
    struct _ENEMY_WORK* enemy;

    em_ground_rec_clear(&rec);
    work = (Q_MoveWork*)get_move_work_adrs(0);
    if (work == NULL) {
        return;
    }
    quest_list = NULL;
    if (quest_work_ptr == NULL || (list = quest_work_ptr->monsters_0x675C) == NULL) {
        list = quest_area_monster_tbl[work->area_0x144];
    } else if (quest_work_ptr != NULL) {
        quest_list = list;
    }
    if (list == NULL) {
        return;
    }
    if (quest_list != NULL) {
        quest_monster_spawn_area(work->phase_0xE9, work);
        if (item->elements_0x94[0].flags & 1) {
            item->elements_0x94[0].flags |= 8;
        }
        if (item->elements_0x94[1].flags & 1) {
            item->elements_0x94[1].flags |= 8;
        }
        if (item->elements_0x94[2].flags & 1) {
            item->elements_0x94[2].flags |= 8;
        }
        return;
    }
    for (slot = list; slot->monster_0x00 != 0; slot++) {
        if (slot->element_index_0x28 == -1) {
            continue;
        }
        for (i = 0; i < slot->count_0x04; i++) {
            if ((s8)item->result_kind_0x308 >= 2) {
                continue;
            }
            if (quest_work_ptr != NULL && quest_work_ptr->monsters_0x675C != NULL) {
                for (k = 0; k < 6; k++) {
                    if ((u8)slot->monster_0x00 == quest_work_ptr->key_rows_0x0319[k].id_0x00) {
                        break;
                    }
                }
                if (k >= 6) {
                    break;
                }
            } else {
                k = slot->element_index_0x28;
            }
            item->slot_key_0x46C[k] = (s8)item->result_kind_0x308;
            order = &work->spawn_order_0x225C[k];
            *order = (s8)item->result_kind_0x308;
            em_ground_rec_set(&rec, &slot->element_0x08);
            spawn = &work->spawn_0x2274[k];
            spawn->order_0x0 = *order;
            spawn->slot_0x8 = slot;
            spawn->count_0x4 = slot->count_0x04;
            spawn->monster_0x6 = slot->monster_0x00;
            spawn->boss_0xC = NULL;
            if (slot->element_0x08.byte_0x02 == 3) {
                boss = em_large_spawn(slot->monster_0x00, &rec, (s8)item->result_kind_0x308);
                if (boss == NULL) {
                    continue;
                }
                enemy = boss->enemy_0x30;
                spawn->boss_0xC = boss;
            } else {
                enemy = em_small_spawn(slot->monster_0x00, &rec, (s8)item->result_kind_0x308);
                if (enemy == NULL) {
                    continue;
                }
            }
            item->slot_key_0x46C[(s8)item->result_kind_0x308] = (s8)item->result_kind_0x308;
            if (slot->element_index_0x28 >= 3) {
                continue;
            }
            item->elements_0x94[k].flags = slot->flags_0x2C | 8;
            item->elements_0x94[k].id = slot->monster_0x00;
            item->elements_0x94[k].value = 1;
            if (slot->element_0x08.byte_0x02 == 3) {
                em_quest_element_set_large((struct _ENEMY_WORK*)boss, k);
            } else {
                em_quest_element_set(enemy, k);
            }
            item->armed_0x2D4[k] = slot->monster_0x00;
            item->result_kind_0x308++;
        }
    }
    if (quest_work_ptr == NULL || (list = quest_work_ptr->monsters_0x675C) == NULL) {
        return;
    }
    if (quest_work_ptr->release_0x6760.id_0x00 != 0 && (s8)item->result_kind_0x308 < 2) {
        for (k = 0; k < 6; k++) {
            if (quest_work_ptr->key_rows_0x0319[k].id_0x00 == 0) {
                break;
            }
        }
        if (k < 3) {
            for (slot = list; slot->monster_0x00 != 0; slot++) {
                if (slot->element_index_0x28 == -1 || quest_work_ptr->release_0x6760.id_0x00 != (u8)slot->monster_0x00) {
                    continue;
                }
                for (i = 0; i < slot->count_0x04; i++) {
                    em_ground_rec_set(&rec, &slot->element_0x08);
                    if (slot->element_0x08.byte_0x02 == 3) {
                        boss = em_large_spawn(slot->monster_0x00, &rec, (s8)item->result_kind_0x308);
                        if (boss == NULL) {
                            continue;
                        }
                        enemy = boss->enemy_0x30;
                    } else {
                        enemy = em_small_spawn(slot->monster_0x00, &rec, (s8)item->result_kind_0x308);
                        if (enemy == NULL) {
                            continue;
                        }
                    }
                    item->slot_key_0x46C[(s8)item->result_kind_0x308] = (s8)item->result_kind_0x308;
                    item->elements_0x94[k].flags = slot->flags_0x2C | 8;
                    item->elements_0x94[k].id = slot->monster_0x00;
                    item->elements_0x94[k].value = 1;
                    if (slot->element_0x08.byte_0x02 == 3) {
                        em_quest_element_set_large((struct _ENEMY_WORK*)boss, k);
                    } else {
                        em_quest_element_set(enemy, k);
                    }
                    item->armed_0x2D4[k] = slot->monster_0x00;
                    item->result_kind_0x308++;
                    return;
                }
            }
        }
    }
}

/* Whether the local slot's item work has its arena item count set. */
u32 quest_item_work_flag_ck(void) {
    Q_MoveWork* work = (Q_MoveWork*)get_move_work_adrs(0);
    Q_ItemWork* item;

    if (work == NULL) {
        return 0;
    }
    item = work->item_work;
    if (item == NULL) {
        return 0;
    }
    return (s8)item->count_0x6A2A != 0;
}

/* Fills the work block's monster slots from the current result row's three slot pairs (only two when quest
 * flag 0x100 is set): for each armed pair whose monster has a variant for the quest's rank, the pair, its
 * element block and the flags of the loaded element that names it. */
void quest_pair_apply(void) {
    QuestEntrySlot* slot;
    Q_SlotPair* key;
    Q_SlotPair* src;
    QuestRecordRow* rows;
    u8 rank;
    u8* state;
    s32 i;
    QuestRecord* rec;
    QuestMonsterVariantList* variant;
    s32 index;

    if (quest_work_ptr == NULL) {
        return;
    }
    slot = quest_work_ptr->slots_0x34C;
    key = quest_work_ptr->key_rows_0x0319;
    state = quest_work_ptr->player_state_0x2D4;
    rec = quest_work_ptr->record_0x03C;
    rows = rec->rows_0x394;
    src = (Q_SlotPair*)rec->slot_bytes_0x314;
    rank = quest_pair_table[rec->field_0x08B * 2];
    for (i = 0; i < 3; i++, src++) {
        if (quest_flag_100_ck(NULL) == 1 && i == 2) {
            break;
        }
        if (src->id_0x00 != 0 && src->count_0x03 != 0) {
            variant = quest_monster_variant_tbl[src->id_0x00];
            if (variant != NULL) {
                while (variant->rank != 0xFF) {
                    if (variant->rank == rank) {
                        break;
                    }
                    variant++;
                }
                if (variant->rank != 0xFF) {
                    quest_pair_copy(key, src);
                    *state = key->id_0x00;
                    slot->monster_0x00 = key->id_0x00;
                    slot->count_0x04 = key->count_0x03;
                    quest_element_copy(&slot->element_0x08, &variant->records[key->group_0x01].element_0x08);
                    slot->element_0x08.byte_0x02 = key->kind_0x02;
                    index = quest_element_find(key->id_0x00);
                    if (index == -1) {
                        index = 3;
                    }
                    slot->element_index_0x28 = index;
                    if (index < 3) {
                        slot->flags_0x2C = rows[index].field_0x00 | 8;
                    } else {
                        slot->flags_0x2C = 0;
                    }
                    slot++;
                    key++;
                    state++;
                }
            }
        }
    }
}

/* The two-word runs at +0x08 and +0x14 of a `Q_ElementBlock`, copied as one block each (retail loads both
 * words, then stores them in order). size: 0x8 */
typedef struct QuestElementPair {
    /* +0x0 */ u32 words[2];
} QuestElementPair;

/* Copies the 0x20-byte key block one of those rows carries, field by field. */
void quest_element_copy(Q_ElementBlock* dst, const Q_ElementBlock* src) {
    dst->byte_0x00 = src->byte_0x00;
    dst->byte_0x01 = src->byte_0x01;
    dst->byte_0x02 = src->byte_0x02;
    dst->byte_0x03 = src->byte_0x03;
    dst->byte_0x04 = src->byte_0x04;
    dst->byte_0x05 = src->byte_0x05;
    dst->byte_0x06 = src->byte_0x06;
    dst->byte_0x07 = src->byte_0x07;
    *(QuestElementPair*)&dst->word_0x08 = *(const QuestElementPair*)&src->word_0x08;
    dst->word_0x10 = src->word_0x10;
    *(QuestElementPair*)&dst->word_0x14 = *(const QuestElementPair*)&src->word_0x14;
    dst->word_0x1C = src->word_0x1C;
}

/* Copies the 8-byte slot pair a result row is built from. */
void quest_pair_copy(Q_SlotPair* dst, const Q_SlotPair* src) {
    *dst = *src;
}

/* Clears the area handle list, loads the result row's two arena items and builds one handle per entry of
 * its area block (at most 32). */
void quest_area_list_init(u8 kind) {
    Q_ResultRow* rec;
    u8* area;
    u8 count;
    s32 i;

    for (i = 0; i < 0x20; i++) {
        QUEST_WORK->area_0x69A8[i] = 0;
    }
    rec = QUEST_WORK->record_0x3C;
    if (rec->items_0x384[0].id == 0) {
        QUEST_WORK->arena_items_0x6A2C[0].id = 0;
        QUEST_WORK->arena_items_0x6A2C[0].count = 0;
        QUEST_WORK->arena_items_0x6A2C[0].remaining = 0;
    } else {
        QUEST_WORK->arena_items_0x6A2C[0].id = rec->items_0x384[0].id;
        QUEST_WORK->arena_items_0x6A2C[0].count = rec->items_0x384[0].count;
        QUEST_WORK->arena_items_0x6A2C[0].remaining = rec->items_0x384[0].remaining;
    }
    if (rec->items_0x384[1].id == 0) {
        QUEST_WORK->arena_items_0x6A2C[1].id = 0;
        QUEST_WORK->arena_items_0x6A2C[1].count = 0;
        QUEST_WORK->arena_items_0x6A2C[1].remaining = 0;
    } else {
        QUEST_WORK->arena_items_0x6A2C[1].id = rec->items_0x384[1].id;
        QUEST_WORK->arena_items_0x6A2C[1].count = rec->items_0x384[1].count;
        QUEST_WORK->arena_items_0x6A2C[1].remaining = rec->items_0x384[1].remaining;
    }
    QUEST_WORK->area_state_0x6A28 = 0;
    QUEST_WORK->flag_0x6A29 = 0;
    QUEST_WORK->count_0x6A2A = 0;
    if (rec->area_ofs_0x37C != 0) {
        area = (u8*)(rec->area_ofs_0x37C + (u32)QUEST_WORK->record_0x3C);
    } else {
        area = NULL;
    }
    count = (u8)stage_map_area_count_get(QUEST_WORK->record_0x3C->state_0x8B);
    if ((s32)count > 32) {
        count = 32;
    }
    QUEST_WORK->area_count_0x69A5 = count;
    if (area != NULL) {
        for (i = 0; i < count; i++) {
            QUEST_WORK->area_0x69A8[i] = em_area_entry_make(area, 0, (u8)i);
        }
    }
    QUEST_WORK->hunt_active_0x675C = 1;
}

/* Rebuilds the area handle list for entry kind `kind`, once: only while the list still waits in state 1. */
void quest_area_list_refill(u8 map, u8 kind) {
    Q_ResultRow* rec;
    s32 i;
    u8 count;
    u8* area;

    if ((s8)QUEST_WORK->area_state_0x6A28 == 1) {
        rec = QUEST_WORK->record_0x3C;
        count = (u8)stage_map_area_count_get(rec->state_0x8B);
        if (rec->area_ofs_0x37C != 0) {
            area = (u8*)(rec->area_ofs_0x37C + (u32)QUEST_WORK->record_0x3C);
            for (i = 0; i < count; i++) {
                QUEST_WORK->area_0x69A8[i] = em_area_entry_make(area, kind, (u8)i);
            }
            QUEST_WORK->area_state_0x6A28 = 2;
        }
    }
}

/* Loads the quest the lobby picked into the work block: its result row (from the quest list, or the network
 * staging buffer for ids from 60000), the stage BGM, the enemy level, the slot pairs and the map rank, the
 * intruder setup, the grade and the elements, the faint limit, the demo-play extra file and the area list.
 * Without a quest it only records the move work's quest id. */
void quest_start_load(void) {
    Q_MoveWork* work;
    LbParamWork* param;
    u32 size;
    s32 loaded;
    u8* pair;
    QuestWork* quest;

    work = (Q_MoveWork*)get_move_work_adrs(0);
    if (work == NULL) {
        return;
    }
    param = &lb_param_w;
    loaded = 0;
    if (lb_param_w.field_0x00 != 0) {
        loaded = 1;
        if (param->field_0x00 < 60000) {
            size = quest_work_word_get(param->field_0x00);
            memcpy(quest_work_ptr->record_0x03C, quest_record_find(param->field_0x00), size);
        } else {
            NetCtrlWk::copyStaging((u8*)quest_work_ptr->record_0x03C);
        }
    }
    if (quest_work_ptr != NULL) {
        QUEST_WORK->hunt_active_0x675C = 0;
        memset(&quest_work_ptr->release_0x6760, 0, 0x18);
    }
    work->state_0x22D4 = param->entry_0x08;
    work->bgm_0x22D5 = param->sub_0x29;
    work->bgm_0x22D6 = param->sub_0x2A;
    if (work->state_0x22D4 & 0x80) {
        snd_quest_bgm_set(work->state_0x22D4 & 0x7F, work->bgm_0x22D5, work->bgm_0x22D6);
    }
    if (loaded == 1) {
        if (quest_work_ptr->record_0x03C->flags_0x310 & 8) {
            work->state_0x22D4 = 0x80;
            work->bgm_0x22D5 = 0;
            work->bgm_0x22D6 = 0;
            snd_quest_bgm_set(0, 0, 0);
        }
        if (quest_work_ptr->record_0x03C->em_level_0x390 != 0) {
            work->em_level_0x22DB = quest_work_ptr->record_0x03C->em_level_0x390 - 1;
            em_level_set(quest_work_ptr->record_0x03C->em_level_0x390 - 1);
        } else {
            work->em_level_0x22DB = 0;
            em_level_set(0);
        }
        QUEST_WORK->hunt_active_0x675C = 1;
        work->quest_id_0x120 = param->field_0x00;
        quest_id_set(work->quest_id_0x120);
        quest_pair_apply();
        pair = &quest_pair_table[quest_work_ptr->record_0x03C->field_0x08B * 2];
        switch (quest_work_ptr->record_0x03C->flags_0x310 & 0x30000) {
        case 0x30000:
            work->map_0xED = pair[param->rank_sel_0x09];
            break;
        case 0x20000:
            work->map_0xED = pair[1];
            param->rank_sel_0x09 = 1;
            break;
        default:
            work->map_0xED = pair[0];
            param->rank_sel_0x09 = 0;
            break;
        }
        work->rank_sel_0x22D8 = param->rank_sel_0x09;
        quest_work_ptr->field_0x6975 = 0;
        if (quest_players_state_get() == 1 &&
            nora_set_proc.key_match(&quest_work_ptr->record_0x03C->field_0x32C, (u8*)&quest_work_ptr->release_0x6760) == 1) {
            quest_work_ptr->intruder_done_0x6976 = 1;
            quest_work_ptr->field_0x6975 = 1;
        }
        if (quest_flag_100_ck(NULL) == 1) {
            memcpy(&quest_work_ptr->release_0x6760, &quest_work_ptr->record_0x03C->slot_pairs_0x314[2], sizeof(Q_SlotPair));
            quest_work_ptr->intruder_done_0x6976 = 1;
            quest_work_ptr->field_0x6975 = 1;
        }
        quest = quest_work_ptr;
        if (quest->record_0x03C->field_0x32C != 0 && (u8)((u16)ran_suu(3) % 100) < quest->record_0x03C->field_0x32C) {
            quest->field_0x6975 = 1;
            if (quest_work_ptr->record_0x03C->bonus_rank_0x32D <= 6) {
                quest_work_ptr->bonus_weight_0x6AB4 = quest_rank_weight_tbl[quest_work_ptr->record_0x03C->bonus_rank_0x32D];
            } else {
                quest_work_ptr->bonus_weight_0x6AB4 = 0;
            }
        }
        if (quest_work_ptr->record_0x03C->flags_0x310 & 0x40000) {
            quest_work_ptr->grade_0x93 = 4;
        }
        if (quest_work_ptr->record_0x03C->flags_0x310 & 0x40) {
            quest_work_ptr->grade_0x93 = 2;
        }
        if (quest_work_ptr->record_0x03C->flags_0x310 & 0x800000) {
            quest_work_ptr->grade_0x93 = 5;
        }
        if (quest_flag_4000000_ck(NULL) == 1) {
            quest_work_ptr->grade_0x93 = 4;
        }
        work->area_0xEE = 0;
        work->phase_0xE9 = quest_work_ptr->record_0x03C->field_0x08B;
        stage_map_set(work->map_0xED);
        memcpy(quest_work_ptr->elements_0x0094, quest_work_ptr->record_0x03C->rows_0x394, sizeof(quest_work_ptr->elements_0x0094));
        memcpy(quest_work_ptr->arena_elements_0x01B4, quest_work_ptr->record_0x03C->rows_0x394, sizeof(quest_work_ptr->arena_elements_0x01B4));
        memcpy(&quest_work_ptr->reward_head_0x2E4, &quest_work_ptr->record_0x03C->field_0x348, 0x24);
        quest_work_ptr->counters_0x2D8[0] = 0;
        quest_work_ptr->counters_0x2D8[1] = 0;
        quest_work_ptr->counters_0x2D8[2] = 0;
        if (work->area_0xEE != 0) {
            work->area_0xEE = 0;
            work->sub_0xEA = 0;
        } else {
            work->sub_0xEA = work->area_0xEE;
        }
        quest = quest_work_ptr;
        if (quest->penalty_0x2F4 > 0) {
            quest->field_0x090 = quest->field_0x2E8 / quest->penalty_0x2F4;
            if (quest_work_ptr->field_0x2E8 - quest_work_ptr->field_0x090 * quest_work_ptr->penalty_0x2F4 > 0) {
                quest_work_ptr->field_0x090++;
            }
        } else {
            quest->field_0x090 = 0;
        }
        quest_work_ptr->field_0x091 = 0;
        quest_work_ptr->my_faint_count_0x92 = 0;
        quest_grade_set();
        if (demo_play_ck() == 1) {
            switch (param->demo_quest_0x99) {
            case 19:
                quest_work_ptr->record_0x03C->area_ofs_0x37C = 0x4C0;
                load_file("05/st01em0_a001.esp",
                          quest_work_ptr->record_0x03C->area_ofs_0x37C + (u32)quest_work_ptr->record_0x03C, 0x600);
                break;
            case 20:
                quest_work_ptr->record_0x03C->area_ofs_0x37C = 0x4C0;
                load_file("05/st01em0_a003.esp",
                          quest_work_ptr->record_0x03C->area_ofs_0x37C + (u32)quest_work_ptr->record_0x03C, 0xB00);
                break;
            }
        }
        quest_area_list_init(quest_byte_table[work->phase_0xE9]);
        if (quest_work_ptr->record_0x03C->flags_0x310 & 0x01000000) {
            lb_quest_work_init(quest_work_ptr->record_0x03C->lobby_kind_0x36E);
            snd_quest_bgm_load();
        }
    } else if (quest_work_ptr != NULL) {
        quest_work_ptr->stat_word_0x10 = work->quest_id_0x120;
    } else {
        work->map_0xED = work->quest_id_0x120;
        work->phase_0xE9 = quest_pair_table[work->quest_id_0x120 * 2];
    }
}

/* Enters the quest: allocates the result buffer, resets the quest work and spawns its monsters, applies the
 * saved stat of an arena quest (the part-break counts it already paid), sets the time limit and its class,
 * hands the stage its map and area, and loads the dcm archives the quest's stage and kind need. */
#pragma opt_propagation off
void quest_enter_load(void) {
    char path[0x20];
    Q_MoveWork* work;
    Q_QuestStat* stat;
    s32 minutes;
    s32 time_class;
    u8 stage;
    u8* ids;
    u8 id;
    u8 kind;
    s32 res;
    ResFileEntry* file;

    clear_qResult_work();
    work = (Q_MoveWork*)get_move_work_adrs(0);
    if (work == NULL) {
        return;
    }
    work->result_buffer_0x150 = (u8*)work_mem_alloc(0x4800);
    work->sub_0xFA = 2;
    work->item_flag_0x110 = 0;
    quest_work_ptr->entry_send_0x6A3A[2] = 2;
    quest_result_field_text_get(quest_work_ptr->record_0x03C, 0);
    loading_caption_set();
    quest_work_start_reset();
    quest_monsters_spawn_now(0);
    if (quest_work_ptr->record_0x03C->field_0x08B == 7) {
        stat = &quest_work_ptr->stat_0x6AA4;
        if (stat->valid_0x00 == 1 && lb_param_w.field_0x00 != quest_work_ptr->stat_0x6AA4.word_0x0C) {
            memset(stat, 0, sizeof(Q_QuestStat));
        }
        stat = &quest_work_ptr->stat_0x6AA4;
        if (stat->valid_0x00 != 0) {
            if (quest_work_ptr->stat_0x6AA4.byte_0x01 & 1) {
                quest_work_ptr->part_counts_0x6803[20][3]++;
                quest_arena_value_clear_ck(20, 3);
            }
            if (quest_work_ptr->stat_0x6AA4.byte_0x01 & 2) {
                quest_work_ptr->part_counts_0x6803[20][1]++;
                quest_arena_value_clear_ck(20, 1);
            }
            if (quest_work_ptr->stat_0x6AA4.byte_0x01 & 4) {
                quest_work_ptr->part_counts_0x6803[20][4]++;
                quest_arena_value_clear_ck(20, 4);
            }
            if (quest_work_ptr->stat_0x6AA4.byte_0x01 & 8) {
                quest_work_ptr->part_counts_0x6803[20][2]++;
                quest_arena_value_clear_ck(20, 2);
            }
        } else {
            memset(stat, 0, sizeof(Q_QuestStat));
        }
    }
    if (quest_select_ready_ck() == 1) {
        quest_work_ptr->field_0x01C = quest_grade_ratio_50f * (frames_per_second_60f * Screen_w.frame_scale);
    } else {
        quest_work_ptr->field_0x01C = quest_work_ptr->record_0x03C->field_0x13A * (frames_per_second_60f * Screen_w.frame_scale);
    }
    quest_time_limit_set(quest_work_ptr->field_0x01C);
    work->area_0xEE = work->sub_0xEA;
    ((StageMapView*)stage_w)->mapno = work->map_0xED;
    ((StageMapView*)stage_w)->areano = work->sub_0xEA;
    minutes = quest_work_ptr->field_0x01C / (frames_per_second_60f * Screen_w.frame_scale);
    time_class = 2;
    if (minutes <= quest_time_class_tbl[2]) {
        time_class = 1;
        if (minutes <= quest_time_class_tbl[1]) {
            time_class = 0;
            if (minutes <= quest_time_class_tbl[0]) {
                time_class = -1;
            }
        }
    }
    quest_work_ptr->time_class_0x6A38 = time_class;
    stage = quest_work_ptr->record_0x03C->stage_0x36F;
    if (stage != 0 && stage_dcm_file_table[stage].size != 0) {
        stage_dcm_path_get(path, stage);
        quest_dcm_buffers()[stage] = (u8*)work_mem_alloc(stage_dcm_file_table[stage].size);
        load_file(path, (u32)quest_dcm_buffers()[stage], stage_dcm_file_table[stage].size);
    }
    ids = NULL;
    kind = quest_work_ptr->record_0x03C->field_0x08B;
    if (kind == 6) {
        ids = quest_res_ids_kind_6;
    }
    if (kind == 7) {
        if (quest_work_ptr->stat_0x6AA4.valid_0x00 == 0) {
            ids = quest_res_ids_arena;
        } else {
            ids = &quest_res_ids_arena[1];
        }
    }
    if (kind == 0xB) {
        ids = quest_res_ids_kind_b;
    }
    if (ids != NULL) {
        for (; (id = *ids) != 0; ids++) {
            if (stage_dcm_file_table[id].size != 0 && quest_dcm_buffers()[id] == NULL) {
                stage_dcm_path_get(path, id);
                quest_dcm_buffers()[id] = (u8*)work_mem_alloc(stage_dcm_file_table[id].size);
                load_file(path, (u32)quest_dcm_buffers()[id], stage_dcm_file_table[id].size);
            }
        }
    }
    if (quest_work_ptr->record_0x03C->field_0x02C < 10000) {
        switch (quest_work_ptr->record_0x03C->field_0x08B) {
        case 1:
            res = 1;
            break;
        case 2:
            res = 2;
            break;
        case 3:
            res = 3;
            break;
        case 4:
            res = 4;
            break;
        case 5:
            res = 5;
            break;
        default:
            res = 0;
            break;
        }
        if (res != 0) {
            id = quest_res_id_by_kind[res];
            file = &stage_dcm_file_table[id];
            if (file != NULL && file->size != 0 && quest_dcm_buffers()[id] == NULL) {
                stage_dcm_path_get(path, id);
                quest_dcm_buffers()[id] = (u8*)work_mem_alloc(file->size);
                load_file(path, (u32)quest_dcm_buffers()[id], file->size);
            }
        }
    }
}
#pragma opt_propagation reset

/* Charges one faint of `player` against the quest reward: announces it, and ends the quest when the reward
 * is gone. */
void quest_reward_faint_penalty(u8 player) {
    char text[128];
    s32 amount;

    if (system_w.field_0x7d2 == 1) {
        return;
    }
    if (system_w.field_0x7d1 == 1) {
        return;
    }
    if (QUEST_WORK->record_0x3C == NULL) {
        return;
    }
    if (QUEST_WORK->reward_0x2E8 <= 0) {
        return;
    }
    if (quest_sub_state_end_ck(1) != 0) {
        return;
    }
    amount = QUEST_WORK->penalty_0x2F4;
    if (QUEST_WORK->reward_0x2E8 < amount) {
        amount = QUEST_WORK->reward_0x2E8;
    }
    QUEST_WORK->reward_0x2E8 = QUEST_WORK->reward_0x2E8 - amount;
    if (player == (s8)my_player_no()) {
        QUEST_WORK->my_faint_count_0x92 += 1;
    }
    QUEST_WORK->faint_count_0x91 += 1;
    if (isServerSelectState() == 1) {
        if (player == (s8)my_player_no()) {
            hud_item_msg_push(1, 2, 0);
        } else {
            sprintf(text, quest_entry_fmt_str, ((_PLW*)get_move_work_adrs(2))[player].user_profile_0x5CA);
            strcat(text, quest_str_tbl_4_get(0x25));
            hud_msg_push(0, text);
        }
    } else {
        hud_item_msg_push(1, 2, 0);
    }
    sprintf(text, quest_str_tbl_35_get(9), amount);
    hud_msg_push(0, text);
    if (QUEST_WORK->reward_0x2E8 <= 0) {
        QUEST_WORK->reward_0x2E8 = 0;
        hud_msg_push(0, quest_str_tbl_35_get(10));
        hud_msg_push(0, quest_str_tbl_35_get(0x13));
        return;
    }
    if (player == (s8)my_player_no()) {
        snd_quest_scene_set();
        snd_player_mask_set(player);
        hud_msg_push(0, quest_str_tbl_35_get(0x12));
    }
}

/* Fails the quest when its time or its reward runs out: the result kind 5, then the result screen (or the
 * lobby's entry start on the server-select screen, with the "no reward" flag when the reward is gone). */
static inline void quest_main_step_fail(Q_ItemWork* item, Q_MoveWork* work) {
    quest_work_ptr->entry_send_0x6A3A[2] = 5;
    if (isServerSelectState() == 0) {
        quest_result_enter(item, work, 0);
    } else if (quest_work_ptr->field_0x2E8 <= 0) {
        lb_entry_start_send((struct LbCompanionWork*)item, 0, item->time_limit_0x24, 1);
    } else {
        lb_entry_start_default((struct LbCompanionWork*)item, 0, item->time_limit_0x24);
    }
}

/* Clears the quest: marks the entry-send header, then the start screen (or the lobby's entry start). */
static inline void quest_main_step_clear(Q_ItemWork* item, Q_MoveWork* work) {
    if (item->entry_send_0x6A3A[4] != 4 && (item->entry_send_0x6A3A[0] == 0 || quest_element_404_ck() == 1)) {
        item->entry_send_0x6A3A[0] = 1;
    }
    if (isServerSelectState() == 0) {
        quest_start_enter(item, work);
    } else {
        lb_entry_start_default((struct LbCompanionWork*)item, 0, item->time_limit_0x24);
    }
}

/* Moves the area list on to the next arena item's area. */
static inline void quest_main_step_area_next(Q_ItemWork* item) {
    em_area_spawn_clear();
    item->area_state_0x6A28 = 1;
    item->count_0x6A2A++;
    quest_area_list_refill(get_now_mapno(), item->count_0x6A2A);
}

/* Whether the quest work is busy: either of the system block's two pre-quest flags, or a live work
 * whose record is missing or whose +0x2E8 counter is set. */
u32 quest_work_busy_ck(void) {
    if (system_w.field_0x7d2 == 1) {
        return 1;
    }
    if (system_w.field_0x7d1 == 1) {
        return 1;
    }
    if (quest_work_ptr->record_0x03C != 0) {
        return quest_work_ptr->field_0x2E8 > 0;
    }
    return 1;
}

/* Warps the player to the quest's start position: the local one for a network quest, a fixed spot for the
 * two arena phases, else the stage's own start. */
void quest_start_warp(_PLW* plw) {
    nw4r::math::VEC3 pos;
    s32 angle;
    Q_MoveWork* work;
    u8 saved;
    u8 phase;

    setVec3(&pos, 0.0f, 0.0f, 0.0f);
    work = (Q_MoveWork*)get_move_work_adrs(0);
    if (quest_work_busy_ck() != 0) {
        if (PlayMode_ck() == 2) {
            saved = my_player_no();
            my_player_no_set((s8)plw->chunk_ofs);
            stage_area_start_get(work->sub_0xEA, &pos, &angle);
            pl_act_stage_latch_set(my_player_work_get(), 2);
            pl_warp_start(0xFF, &pos, (u16)angle);
            my_player_no_set((s8)saved);
            return;
        }
        phase = work->phase_0xE9;
        if ((phase == 6 || phase == 0x11) && plw->area_0x16 == 2) {
            pos.x = 9960.0f;
            pos.y = 230.0f;
            pos.z = 300.0f;
            angle = 0xC000;
            pl_act_stage_latch_set(my_player_work_get(), 2);
            pl_warp_start(2, &pos, (u16)angle);
            return;
        }
        stage_start_get(phase, &pos, &angle);
        pl_act_stage_latch_set(my_player_work_get(), 2);
        pl_warp_start(0, &pos, (u16)angle);
    }
}

/* Prepares the item work for a quest entry from its result row: the quest kind, the time-limit rank, the
 * entry stage, the supply pairs (none without a row, an empty list when the row names no item). */
void quest_entry_setup(Q_ItemWork* item) {
    Q_ResultRow* rec = item->record_0x3C;

    item->supply_kind_0x8B = rec->kind_0x374;
    item->supply_step_0x58 = 0;
    item->time_rank_0x59 = item->time_limit_0x24 / 9000;
    item->supply_count_0x8F = 0;
    item->supply_limit_0x8E = 1;
    if ((s8)(u8)item->supply_kind_0x8B != 2 && quest_slot_progress_get(NULL) == 1) {
        item->supply_limit_0x8E = 2;
    }
    item->field_0x5E0 = 40;
    item->bits_0x684[0] = 0;
    item->bits_0x684[1] = 0;
    if (rec == NULL) {
        quest_item_pair_copy_block(item->supply_0x5E2, 1, 0);
    } else if (rec->item_id_0x372 == 0) {
        quest_item_pair_copy_block(item->supply_0x5E2, 0xFFFF, 0);
    } else {
        quest_item_pair_copy_block(item->supply_0x5E2, rec->item_id_0x372, item->supply_kind_0x8B);
    }
}

/* Enters the quest result state (sub-state 5): latches the time limit, clears the AI slots, fills the
 * result stat and posts the kind's message. */
void quest_result_enter(Q_ItemWork* item, Q_MoveWork* work, u8 kind) {
    work->sub_0xFA = 5;
    item->time_base_0x20 = item->time_limit_0x24;
    item->step_0x2C = 2;
    item->field_0x2D = 0;
    quest_time_limit_set((s32)(quest_grade_ratio_10f * Screen_w.frame_scale));
    ai_slots_clear();
    quest_result_stat_fill((QuestWork*)item);
    if (kind == 1) {
        hud_msg_push(0, quest_str_tbl_35_get(8));
    }
    if (kind == 2) {
        hud_msg_push(0, quest_str_tbl_35_get(0x1E));
    }
    if (kind == 3) {
        hud_msg_push(0, quest_str_tbl_35_get(0x21));
    }
    snd_quest_result_bgm_set();
}

/* Enters the quest start state (sub-state 3): latches the time limit and posts the entry message the
 * quest's objective and the entry-send header select. */
void quest_start_enter(Q_ItemWork* item, Q_MoveWork* work) {
    u8 objective;
    Q_ResultRow* rec;

    work->sub_0xFA = 3;
    system_copy_filter_request();
    item->time_base_0x20 = item->time_limit_0x24;
    item->step_0x2C = 2;
    item->field_0x2D = 0;
    if (work->kind_0xFC != 4) {
        if (item->entry_send_0x6A3A[0] == 0) {
            hud_msg_push(0, quest_str_tbl_35_get(0x17));
        } else {
            switch (item->entry_send_0x6A3A[1]) {
            case 0:
                hud_msg_push(0, quest_str_tbl_35_get(0x17));
                break;
            case 1:
                hud_msg_push(0, quest_str_tbl_35_get(0x1F));
                break;
            case 2:
                hud_msg_push(0, quest_str_tbl_35_get(0x20));
                break;
            }
        }
    }
    if (work->kind_0xFC == 4) {
        quest_time_limit_set((s32)(quest_grade_ratio_10f * Screen_w.frame_scale));
        if (system_w.field_0x7cf == 0x15) {
            hud_msg_push(0, quest_str_tbl_35_get(0));
        } else {
            hud_msg_push(0, quest_str_tbl_35_get(1));
        }
    } else {
        rec = (Q_ResultRow*)quest_record_get();
        if ((rec != NULL && (rec->flags_0x310 & 0x20000000) != 0) || (objective = quest_objective_get(NULL), objective == 1) ||
            objective == 4) {
            quest_time_limit_set((s32)(20.0f * Screen_w.frame_scale));
            if (PlayMode_ck() == 2) {
                hud_msg_push(0, quest_str_tbl_35_get(0x27));
            } else if (system_w.field_0x7cf == 0x15) {
                hud_msg_push(0, quest_str_tbl_35_get(2));
            } else {
                hud_msg_push(0, quest_str_tbl_35_get(3));
            }
        } else {
            quest_time_limit_set((s32)(frames_per_second_60f * Screen_w.frame_scale));
            if (system_w.field_0x7cf == 0x15) {
                hud_msg_push(0, quest_str_tbl_35_get(4));
            } else {
                hud_msg_push(0, quest_str_tbl_35_get(5));
            }
        }
    }
    snd_quest_start_bgm_set();
}

/* The quest's per-frame step on the local move work: the entry (step 0), the hunt (step 1: the clock, the
 * goals the grade asks for, the time and reward checks, the supply drops, the area hand-offs, the extra
 * monsters and the element sync), the result wait (step 2) and the lobby's result hand-off (step 4). */
void quest_main_step(void) {
    char text[0x100];
    Q_ItemWork* item;
    Q_MoveWork* work;
    s32 goal;
    s32 left;
    s32 i;
    s32 done;
    s32 mark;
    struct _PLW* players;
    s32 advance;

    work = (Q_MoveWork*)get_move_work_adrs(0);
    if (work == NULL) {
        return;
    }
    item = work->item_work;
    if (item == NULL) {
        return;
    }
    if (work->local_0x112 == 0) {
        snd_quest_frame_begin(0, work->phase_0xE9, work->sub_0xEA);
    }
    system_w.field_0x2a = 0;
    switch ((s8)item->step_0x2C) {
    case 0:
        item->step_0x2C++;
        quest_entry_setup(item);
        if (quest_flag_1000000_ck(NULL) == 1) {
            lb_event_request(0);
        }
        break;
    case 1:
        if (quest_work_ptr->entry_send_0x6A3A[2] != 2) {
            break;
        }
        if ((u8)event_demo_running_ck() != 0 || (u32)lb_quest_work_active_ck() == 1 || pad_hold_ck() == 1) {
            item->timer_0x6A48 = OSGetTime();
            item->timer_0x6A50 = item->timer_0x6A48;
            break;
        }
        if ((system_w.field_0x7d2 == 0 && system_w.field_0x7d1 == 0 && quest_work_ptr->record_0x03C != NULL &&
             quest_work_ptr->field_0x2E8 <= 0 && (quest_work_ptr->field_0x2E8 = 0, 1)) ||
            (quest_special_element_step((QuestWork*)item), quest_grade_goal_step((QuestWork*)item),
             quest_failed_ck((QuestWork*)item) == 1)) {
            quest_main_step_fail(item, work);
            break;
        }
        goal = 0;
        if (isServerSelectState() == 0 && quest_flag_8_ck(NULL) == 1) {
            if (work->field_0xFB == 6) {
                quest_result_enter(item, work, 0);
            } else if (work->field_0xFB == 4) {
                work->field_0xFD = 4;
                goal = 1;
            }
        } else if (quest_flag_80_ck(NULL) == 1 && item->entry_send_0x6A3A[4] == 4) {
            goal = 1;
            item->entry_send_0x6A3A[5] = 4;
        } else if (quest_flag_80000_ck(NULL) == 1) {
            if (quest_element_pick_ck((QuestWork*)item, 4, 1) == 1) {
                item->entry_send_0x6A3A[5] = 4;
                item->entry_send_0x6A3A[2] = 4;
                goal = 1;
            } else {
                goal = 0;
            }
        } else {
            switch (quest_work_ptr->grade_0x93) {
            case 1:
                if (quest_element_pick_ck((QuestWork*)item, 0, 1) == 1 || quest_element_pick_ck((QuestWork*)item, 1, 1) == 1 ||
                    quest_element_pick_ck((QuestWork*)item, 2, 1) == 1) {
                    goal = 1;
                } else {
                    goal = 0;
                }
                break;
            case 2:
                if (quest_element_pick_ck((QuestWork*)item, 0, 1) == 1) {
                    goal = 1;
                } else {
                    goal = 0;
                }
                break;
            case 3:
                if (quest_element_pick_ck((QuestWork*)item, 0, 1) == 1 && quest_element_pick_ck((QuestWork*)item, 1, 1) == 1) {
                    goal = 1;
                } else {
                    goal = 0;
                }
                break;
            case 4:
                goal = 0;
                done = 0;
                for (i = 0; i < 3; i++) {
                    if (quest_element_live_ck((QuestWork*)item, i) == 0) {
                        done++;
                    } else if (quest_element_pick_ck((QuestWork*)item, i, 1) == 1) {
                        done++;
                    }
                }
                if (done == 3) {
                    goal = 1;
                } else {
                    for (i = 0; i < 3; i++) {
                        if (quest_element_failed_ck((QuestWork*)item, i) == 1) {
                            goal = -1;
                            break;
                        }
                    }
                }
                break;
            case 5:
                if (quest_flag_200000_ck(NULL) == 1) {
                    if (quest_element_failed_ck((QuestWork*)item, 0) == 1) {
                        goal = -1;
                    } else if (quest_element_pick_ck((QuestWork*)item, 1, 1) == 1 &&
                               quest_element_pick_ck((QuestWork*)item, 2, 1) == 1) {
                        goal = 1;
                        item->entry_send_0x6A3A[0] = 1;
                        item->entry_send_0x6A3A[1] = 2;
                    } else {
                        goal = 0;
                    }
                } else if (quest_element_pick_ck((QuestWork*)item, 0, 1) == 1) {
                    goal = 1;
                } else if (quest_element_failed_ck((QuestWork*)item, 1) == 1) {
                    goal = -1;
                } else {
                    goal = 0;
                }
                break;
            case 6:
                if (quest_element_pick_ck((QuestWork*)item, 0, 1) == 1 && quest_element_pick_ck((QuestWork*)item, 2, 1) == 1) {
                    goal = 1;
                } else if (quest_element_failed_ck((QuestWork*)item, 0) == 1 || quest_element_failed_ck((QuestWork*)item, 2) == 1) {
                    goal = -1;
                } else {
                    goal = 0;
                }
                break;
            }
            if (goal == -1) {
                quest_main_step_fail(item, work);
                break;
            }
            if (goal == 1) {
                if (isServerSelectState() == 0) {
                    work->field_0xFD = 4;
                }
                item->entry_send_0x6A3A[3] = 4;
            }
        }
        if (goal == 1) {
            quest_main_step_clear(item, work);
            break;
        }
        quest_clock_step(1);
        if (item->time_warned_0x6A39 == 0) {
            mark = item->time_total_0x1C;
            if (mark - 10 >= item->time_limit_0x24) {
                sprintf(text, quest_str_tbl_35_get(7), (s32)((f32)mark / (frames_per_second_60f * Screen_w.frame_scale)));
                hud_msg_push(0, text);
                item->time_warned_0x6A39 = 1;
            }
        }
        left = quest_time_elapsed_get();
        if (left <= 0) {
            if (quest_element_window_check((QuestWork*)item) == 1) {
                item->entry_send_0x6A3A[0] = 1;
                item->entry_send_0x6A3A[2] = 3;
                item->entry_send_0x6A3A[3] = 4;
                system_copy_filter_request();
                quest_main_step_clear(item, work);
            } else {
                quest_work_ptr->entry_send_0x6A3A[2] = 5;
                if (isServerSelectState() == 0) {
                    quest_result_enter(item, work, 1);
                } else {
                    lb_entry_start_default((struct LbCompanionWork*)item, 1, item->time_limit_0x24);
                }
            }
            break;
        }
        if ((quest_flag_8_ck(NULL) != 1 || (work->field_0xFB != 6 && work->field_0xFB != 4)) && item->time_class_0x6A38 > 0) {
            mark = quest_time_class_tbl[item->time_class_0x6A38];
            if ((f32)left <= frames_per_second_60f * Screen_w.frame_scale * (f32)mark) {
                sprintf(text, quest_str_tbl_35_get(6), mark);
                hud_msg_push(0, text);
                item->time_class_0x6A38--;
            }
        }
        quest_supply_drop_step();
        switch ((s8)item->area_state_0x6A28) {
        case 0:
            if (item->count_0x6A2A == 2) {
                break;
            }
            advance = 0;
            switch (item->arena_items_0x6A2C[item->count_0x6A2A].id) {
            case 3:
                if ((quest_row394_get(1) & 2) && quest_element_pick_ck((QuestWork*)item, 1, 1) == 1) {
                    advance = 1;
                }
                break;
            case 4:
                if ((quest_row394_get(2) & 2) && quest_element_pick_ck((QuestWork*)item, 2, 1) == 1) {
                    advance = 1;
                }
                break;
            case 1:
                if (item->arena_items_0x6A2C[item->count_0x6A2A].remaining == 0) {
                    advance = 1;
                }
                break;
            case 2:
                switch ((s8)item->flag_0x6A29) {
                case 0:
                    players = (struct _PLW*)get_move_work_adrs(2);
                    if (players != NULL) {
                        u16 timer = Pl_item_timer_get(&players[(s8)my_player_no()],
                                                      item->arena_items_0x6A2C[item->count_0x6A2A].count);
                        if (timer != 0 && item->arena_items_0x6A2C[item->count_0x6A2A].remaining >= timer) {
                            lb_sub14_send(item->count_0x6A2A);
                            item->flag_0x6A29 = 1;
                            item->flag_0x6A29 = 0;
                            advance = 1;
                        }
                    }
                    break;
                case 1:
                    item->flag_0x6A29 = 0;
                    advance = 1;
                    break;
                }
                break;
            }
            if (advance) {
                quest_main_step_area_next(item);
            }
            break;
        case 2:
            item->area_state_0x6A28++;
            break;
        case 3:
            if (file_loading_ck(NULL, NULL) != 1) {
                quest_area_spawn_setup(work, item->area_0x69A8, item->area_count_0x69A5, item->count_0x6A2A);
                item->area_state_0x6A28 = 0;
                item->flag_0x6A29 = 0;
            }
            break;
        }
        quest_finish_step();
        if (quest_flag_4000000_ck(NULL) == 1) {
            quest_successive_step(work);
        } else if (quest_field32C_ck(NULL) == 1 || quest_flag_100_ck(NULL) == 1) {
            quest_intruder_step();
        }
        item->frame_count_0x28++;
        if ((item->frame_count_0x28 & 0x1F) == 0) {
            for (i = 0; i < 3; i++) {
                if (quest_element_pick_ck((QuestWork*)item, i, 0) == 1 && quest_element_pick_ck((QuestWork*)item, i, 1) == 0) {
                    lb_sub16_send(0x20, i);
                }
            }
        }
        break;
    case 2:
        quest_clock_step(1);
        if ((s8)item->field_0x2D == 0) {
            item->field_0x2D++;
        } else if (quest_time_elapsed_get() <= 0) {
            if ((u8)(work->sub_0xFA - 5) <= 1) {
                work->sub_0xFA = 6;
            }
            if ((u8)(work->sub_0xFA - 3) <= 1) {
                work->sub_0xFA = 4;
            }
            item->step_0x2C++;
        }
        break;
    case 4:
        item->frame_count_0x28++;
        if ((item->frame_count_0x28 & 0x1F) == 0) {
            lb_entry_start_default((struct LbCompanionWork*)item, item->lobby_result_0x6A68, item->time_base_0x20);
        }
        break;
    }
    if (work->local_0x112 == 0) {
        snd_quest_frame_end();
    }
}

/* Whether the quest work's entry-send header has been started. */
u32 quest_entry_active_ck(void) {
    if (quest_work_ptr == NULL) {
        return 0;
    }
    return QUEST_WORK->entry_send_0x6A3A[0] != 0;
}

/* Whether the entry may start: the header must be live and the quest's 0x800000 condition satisfied. */
u32 quest_entry_ready_ck(void) {
    if (quest_work_ptr == NULL) {
        return 0;
    }
    if (QUEST_WORK->entry_send_0x6A3A[0] == 0) {
        return 0;
    }
    if (quest_flag_800000_ck(NULL) == 0) {
        return 0;
    }
    return quest_entry_active_ck();
}

/* Whether the local slot's sub-state byte is the entry pair (6 or 7), or 4. */
u32 quest_move_sub_state_ck(void) {
    Q_MoveWork* work = (Q_MoveWork*)get_move_work_adrs(0);
    u8 state;

    if (work == NULL) {
        return 0;
    }
    state = work->sub_0xFA;
    if (state == 4) {
        return 1;
    }
    return (u8)(state + 250) <= 1;
}

/* Steps the quest finish sequence: waits (the hunt-end wait is five seconds of frames), then respawns the
 * quest's monster once for the session and hands over to the lobby. */
void quest_finish_step(void) {
    Q_ItemWork* work;

    if (quest_flag_80000000_ck(NULL) == 0) {
        return;
    }
    if (get_move_work_adrs(0) == NULL) {
        return;
    }
    work = QUEST_WORK;
    if (work == NULL) {
        return;
    }
    switch (work->phase_0x6978) {
    case 0:
        work->counter_0x697C = work->counter_0x697C + 1;
        if (work->hunt_end_0x6977 != 0) {
            if ((f32)work->counter_0x697C < 5.0f * (frames_per_second_60f * Screen_w.frame_scale)) {
                return;
            }
        }
        if (isServerSelectState() == 1) {
            work->phase_0x6978 = 2;
        } else {
            work->phase_0x6978 = 1;
        }
        break;
    case 1:
        if (isServerSelectState() == 1) {
            if (isReadyCountOne() != 0) {
                if (work->respawn_kind_0x321 != 0 && work->respawn_gate_0x323 == 3) {
                    em_spawn_request(0xFFFF, work->respawn_kind_0x321, 0, 0xFF, 0xFF, 1, 1, 0, 0xFF, NULL, 0);
                    lb_sub18_send();
                }
                work->phase_0x6978 = 3;
            }
        } else {
            work->phase_0x6978 = 2;
        }
        break;
    case 2:
        if (work->respawn_kind_0x321 != 0 && work->respawn_gate_0x323 == 3) {
            em_spawn_request(0xFFFF, work->respawn_kind_0x321, 0, 0xFF, 0xFF, 1, 1, 0, 0xFF, NULL, 0);
        }
        work->phase_0x6978 = 3;
        break;
    }
}

/* Steps the intruder: once the quest work's flag is up it waits (three seconds, or for the first or second
 * monster's action to finish, by the row's +0x32E kind; under quest flag 0x100 it hands the release monster
 * over), asks the enemy side for the intruder, releases the key row's monster, waits for the load and spawns
 * the next intruder; the server-select screen runs the lobby-synchronised form (phases 10-13). */
void quest_intruder_step(void) {
    EmGroundRec rec_a;
    EmGroundRec rec_b;
    Q_MoveWork* work;
    QuestWork* quest;
    u8 kind;
    s32 live;
    u16 last;
    s32 row;
    Q_SlotPair* key;
    QuestMonsterVariantList* variant;
    QuestSpawnRec* spawn;
    u8 rank;

    work = (Q_MoveWork*)get_move_work_adrs(0);
    if (work == NULL) {
        return;
    }
    quest = quest_work_ptr;
    if (quest == NULL) {
        return;
    }
    if (quest->field_0x6975 == 0) {
        return;
    }
    kind = quest_field32E_get(NULL);
    switch (quest->field_0x6978) {
    case 0:
        if (quest_flag_100_ck(NULL) == 1) {
            if (quest->intruder_index_0x6977 != 2) {
                quest->field_0x697C++;
                if (!((f32)quest->field_0x697C < quest_intruder_wait_3f * (frames_per_second_60f * Screen_w.frame_scale))) {
                    memcpy(&quest->key_rows_0x0319[3], &quest->release_0x6760, sizeof(Q_SlotPair));
                    if (isServerSelectState() == 0) {
                        quest->field_0x6978 = 4;
                        return;
                    }
                    lb_sub15_send();
                    quest->field_0x6978 = 11;
                    return;
                }
            }
        } else {
            switch (kind) {
            case 1:
                if (quest->intruder_index_0x6977 != 2) {
                    quest->field_0x697C++;
                    if (!((f32)quest->field_0x697C < quest_intruder_wait_3f * (frames_per_second_60f * Screen_w.frame_scale))) {
                        quest->field_0x697C = 0;
                        quest->field_0x6978++;
                        return;
                    }
                }
                break;
            case 2: {
                struct _ENEMY_WORK* enemy = NULL;
                struct _ENEMY_MINI_WORK* mini = NULL;

                if (quest->intruder_index_0x6977 != 1 && quest->spawn_count_0x308 == 1 &&
                    (u8)em_get_unique_work(0, &enemy, &mini) != 0 && (u32)em_mot_finished_ck(enemy) != 1) {
                    quest->field_0x697C++;
                    if (!((f32)quest->field_0x697C < quest_intruder_wait_3f * (frames_per_second_60f * Screen_w.frame_scale))) {
                        quest->field_0x697C = 0;
                        quest->field_0x6978++;
                        return;
                    }
                }
                break;
            }
            case 3: {
                struct _ENEMY_WORK* enemy = NULL;
                struct _ENEMY_MINI_WORK* mini = NULL;

                if (quest->intruder_index_0x6977 != 1 && quest->spawn_count_0x308 == 2) {
                    live = 0;
                    if (work->spawn_0x2274[0].count_0x4 > 0) {
                        live = 1;
                        last = 0;
                    }
                    if (work->spawn_0x2274[1].count_0x4 > 0) {
                        live++;
                        last = 1;
                    }
                    if (work->spawn_0x2274[2].count_0x4 > 0) {
                        live++;
                        last = 2;
                    }
                    if (live <= 1 && (u8)em_get_unique_work(last, &enemy, &mini) != 0 && (u32)em_mot_finished_ck(enemy) != 1) {
                        quest->field_0x697C++;
                        if (!((f32)quest->field_0x697C < quest_intruder_wait_3f * (frames_per_second_60f * Screen_w.frame_scale))) {
                            quest->field_0x697C = 0;
                            quest->field_0x6978++;
                            return;
                        }
                    }
                }
                break;
            }
            }
        }
        break;
    case 1:
        quest->field_0x697C = 0;
        if (nora_set_proc.element_ck(quest_work_ptr->record_0x03C->intruder_set_0x32F, quest->intruder_index_0x6977, quest) == 0) {
            quest->field_0x6978 = 0;
            return;
        }
        if (quest_key_row_ck(quest->key_rows_0x0319[quest->intruder_index_0x6977 + 3].id_0x00) == 1) {
            quest->field_0x6978 = 0;
            return;
        }
        quest->field_0x6978++;
        return;
    case 2:
        key = &quest->key_rows_0x0319[quest->intruder_index_0x6977 + 3];
        if (em_kind_release_ck(key->id_0x00) != 0) {
            em_kind_release(key->id_0x00);
            quest->intruder_done_0x6976 = 1;
            if (isServerSelectState() == 0) {
                quest->field_0x6978++;
                return;
            }
            quest->field_0x6978 = 10;
            return;
        }
        break;
    case 3:
        if (file_loading_ck(NULL, NULL) != 1) {
            quest->field_0x6978++;
            return;
        }
        break;
    case 4:
        em_ground_rec_clear(&rec_a);
        if (quest_flag_100_ck(NULL) == 1) {
            row = 3;
        } else {
            row = quest->intruder_index_0x6977 + 3;
        }
        key = &quest->key_rows_0x0319[row];
        variant = quest_monster_variant_tbl[key->id_0x00];
        if (variant == NULL) {
            break;
        }
        while ((rank = variant->rank) != 0xFF) {
            if (rank == stage_map_kind_get(get_now_mapno())) {
                break;
            }
            variant++;
        }
        if (variant->rank == 0xFF) {
            break;
        }
        quest_work_ptr->spawn_key_0x46C[row] = row;
        quest_work_ptr->spawn_args_0x478[0][row] = key->arg_0x04;
        quest_work_ptr->spawn_args_0x478[1][row] = key->arg_0x05;
        quest_work_ptr->spawn_args_0x478[2][row] = key->arg_0x06;
        quest_work_ptr->spawn_args_0x478[3][row] = key->arg_0x07;
        spawn = &work->spawn_0x2274[row];
        spawn->order_0x0 = quest_work_ptr->spawn_key_0x46C[row];
        spawn->slot_0x8 = variant->records;
        spawn->count_0x4 = variant->records->count_0x04;
        spawn->monster_0x6 = key->id_0x00;
        spawn->boss_0xC = NULL;
        em_ground_rec_set(&rec_a, &variant->records[key->group_0x01].element_0x08);
        em_intruder_spawn(key->id_0x00, &rec_a, row);
        quest->field_0x6978++;
        return;
    case 5: {
        struct _ENEMY_WORK* enemy = NULL;
        struct _ENEMY_MINI_WORK* mini = NULL;

        if ((u8)em_get_unique_work(quest->intruder_index_0x6977 + 3, &enemy, &mini) != 0) {
            quest->field_0x6978 = 0;
            quest->intruder_index_0x6977++;
            return;
        }
        break;
    }
    case 10:
        if (file_loading_ck(NULL, NULL) != 1) {
            lb_sub15_send();
            quest->field_0x6978++;
            return;
        }
        break;
    case 11:
        if (quest->sync_count_0x69A4 >= countOccupiedServerSlots()) {
            quest->field_0x6978 = 12;
            quest->sync_count_0x69A4 = 0;
            return;
        }
        break;
    case 12:
        em_ground_rec_clear(&rec_b);
        if (quest_flag_100_ck(NULL) == 1) {
            row = 3;
        } else {
            row = quest->intruder_index_0x6977 + 3;
        }
        key = &quest->key_rows_0x0319[row];
        variant = quest_monster_variant_tbl[key->id_0x00];
        if (variant == NULL) {
            break;
        }
        while ((rank = variant->rank) != 0xFF) {
            if (rank == stage_map_kind_get(get_now_mapno())) {
                break;
            }
            variant++;
        }
        if (variant->rank == 0xFF) {
            break;
        }
        quest_work_ptr->spawn_key_0x46C[row] = row;
        quest_work_ptr->spawn_args_0x478[0][row] = key->arg_0x04;
        quest_work_ptr->spawn_args_0x478[1][row] = key->arg_0x05;
        quest_work_ptr->spawn_args_0x478[2][row] = key->arg_0x06;
        quest_work_ptr->spawn_args_0x478[3][row] = key->arg_0x07;
        spawn = &work->spawn_0x2274[row];
        spawn->order_0x0 = quest_work_ptr->spawn_key_0x46C[row];
        spawn->slot_0x8 = variant->records;
        spawn->count_0x4 = variant->records->count_0x04;
        spawn->monster_0x6 = key->id_0x00;
        spawn->boss_0xC = NULL;
        if (isReadyCountOne() == 1) {
            em_ground_rec_set(&rec_b, &variant->records[key->group_0x01].element_0x08);
            em_intruder_spawn(key->id_0x00, &rec_b, row);
            quest->field_0x6978++;
            return;
        }
        /* fall through */
    case 13: {
        struct _ENEMY_WORK* enemy = NULL;
        struct _ENEMY_MINI_WORK* mini = NULL;

        if ((u8)em_get_unique_work(quest->intruder_index_0x6977 + 3, &enemy, &mini) != 0) {
            quest->field_0x6978 = 0;
            quest->intruder_index_0x6977++;
        }
        break;
    }
    }
}

/* Steps the successive hunt (quest flag 0x4000000): once the previous monster is down it releases the key row
 * of the next spawn, waits for the load, refills the row from the result row's slot pair and spawns its
 * monster for the current map's kind as the next spawn record; the server-select screen runs the
 * lobby-synchronised form (phases 10-13). */
void quest_successive_step(Q_MoveWork* work) {
    EmGroundRec rec_a;
    EmGroundRec rec_b;
    QuestWork* quest;
    Q_SlotPair* key;
    QuestSpawnRec* spawn;
    QuestMonsterVariantList* variant;
    u8 rank;

    quest = quest_work_ptr;
    if (quest == NULL) {
        return;
    }
    switch (quest->field_0x6978) {
    case 0:
        if (quest->intruder_index_0x6977 == 0) {
            quest->field_0x6978++;
            return;
        }
        break;
    case 1:
        key = &quest->key_rows_0x0319[quest->spawn_count_0x308];
        if (em_kind_release_ck(key->id_0x00) != 0) {
            em_kind_release(key->id_0x00);
            if (isServerSelectState() == 0) {
                quest->field_0x6978++;
                return;
            }
            quest->field_0x6978 = 10;
            return;
        }
        break;
    case 2:
        if (file_loading_ck(NULL, NULL) != 1) {
            quest->field_0x6978++;
            return;
        }
        break;
    case 3:
        key = &quest->key_rows_0x0319[quest->spawn_count_0x308];
        em_ground_rec_clear(&rec_a);
        variant = quest_monster_variant_tbl[key->id_0x00];
        if (variant == NULL) {
            break;
        }
        while ((rank = variant->rank) != 0xFF) {
            if (rank == stage_map_kind_get(get_now_mapno())) {
                break;
            }
            variant++;
        }
        if (variant->rank == 0xFF) {
            break;
        }
        key->id_0x00 = quest->record_0x03C->slot_pairs_0x314[quest->spawn_count_0x308].id_0x00;
        key->group_0x01 = quest->record_0x03C->slot_pairs_0x314[quest->spawn_count_0x308].group_0x01;
        key->kind_0x02 = quest->record_0x03C->slot_pairs_0x314[quest->spawn_count_0x308].kind_0x02;
        key->arg_0x04 = quest->record_0x03C->slot_pairs_0x314[quest->spawn_count_0x308].arg_0x04;
        key->arg_0x05 = quest->record_0x03C->slot_pairs_0x314[quest->spawn_count_0x308].arg_0x05;
        key->arg_0x06 = quest->record_0x03C->slot_pairs_0x314[quest->spawn_count_0x308].arg_0x06;
        key->arg_0x07 = quest->record_0x03C->slot_pairs_0x314[quest->spawn_count_0x308].arg_0x07;
        quest->spawn_key_0x46C[quest->spawn_count_0x308] = quest->spawn_count_0x308;
        quest->spawn_args_0x478[0][quest->spawn_count_0x308] = key->arg_0x04;
        quest->spawn_args_0x478[1][quest->spawn_count_0x308] = key->arg_0x05;
        quest->spawn_args_0x478[2][quest->spawn_count_0x308] = key->arg_0x06;
        quest->spawn_args_0x478[3][quest->spawn_count_0x308] = key->arg_0x07;
        spawn = &work->spawn_0x2274[quest->spawn_count_0x308];
        spawn->order_0x0 = quest->spawn_key_0x46C[quest->spawn_count_0x308];
        spawn->slot_0x8 = variant->records;
        spawn->count_0x4 = variant->records->count_0x04;
        spawn->monster_0x6 = key->id_0x00;
        spawn->boss_0xC = NULL;
        em_ground_rec_set(&rec_a, &variant->records[key->group_0x01].element_0x08);
        em_intruder_spawn(key->id_0x00, &rec_a, quest->spawn_count_0x308);
        quest->field_0x6978++;
        return;
    case 4: {
        struct _ENEMY_WORK* enemy = NULL;
        struct _ENEMY_MINI_WORK* mini = NULL;

        if ((u8)em_get_unique_work(quest->spawn_count_0x308, &enemy, &mini) != 0) {
            em_quest_element_set(enemy, (u8)quest->spawn_count_0x308);
            quest->field_0x6978 = 0;
            quest->intruder_index_0x6977 = 1;
            quest->spawn_count_0x308++;
            return;
        }
        break;
    }
    case 10:
        if (file_loading_ck(NULL, NULL) != 1) {
            lb_sub1c_send();
            quest->field_0x6978++;
            return;
        }
        break;
    case 11:
        if (quest->sync_count_0x69A4 >= countOccupiedServerSlots()) {
            quest->field_0x6978 = 12;
            quest->sync_count_0x69A4 = 0;
            return;
        }
        break;
    case 12:
        key = &quest->key_rows_0x0319[quest->spawn_count_0x308];
        em_ground_rec_clear(&rec_b);
        variant = quest_monster_variant_tbl[key->id_0x00];
        if (variant == NULL) {
            break;
        }
        while ((rank = variant->rank) != 0xFF) {
            if (rank == stage_map_kind_get(get_now_mapno())) {
                break;
            }
            variant++;
        }
        if (variant->rank == 0xFF) {
            break;
        }
        key->id_0x00 = quest->record_0x03C->slot_pairs_0x314[quest->spawn_count_0x308].id_0x00;
        key->group_0x01 = quest->record_0x03C->slot_pairs_0x314[quest->spawn_count_0x308].group_0x01;
        key->kind_0x02 = quest->record_0x03C->slot_pairs_0x314[quest->spawn_count_0x308].kind_0x02;
        key->arg_0x04 = quest->record_0x03C->slot_pairs_0x314[quest->spawn_count_0x308].arg_0x04;
        key->arg_0x05 = quest->record_0x03C->slot_pairs_0x314[quest->spawn_count_0x308].arg_0x05;
        key->arg_0x06 = quest->record_0x03C->slot_pairs_0x314[quest->spawn_count_0x308].arg_0x06;
        key->arg_0x07 = quest->record_0x03C->slot_pairs_0x314[quest->spawn_count_0x308].arg_0x07;
        quest->spawn_key_0x46C[quest->spawn_count_0x308] = quest->spawn_count_0x308;
        quest->spawn_args_0x478[0][quest->spawn_count_0x308] = key->arg_0x04;
        quest->spawn_args_0x478[1][quest->spawn_count_0x308] = key->arg_0x05;
        quest->spawn_args_0x478[2][quest->spawn_count_0x308] = key->arg_0x06;
        quest->spawn_args_0x478[3][quest->spawn_count_0x308] = key->arg_0x07;
        spawn = &work->spawn_0x2274[quest->spawn_count_0x308];
        spawn->order_0x0 = quest->spawn_key_0x46C[quest->spawn_count_0x308];
        spawn->slot_0x8 = variant->records;
        spawn->count_0x4 = variant->records->count_0x04;
        spawn->monster_0x6 = key->id_0x00;
        spawn->boss_0xC = NULL;
        if (isReadyCountOne() == 1) {
            em_ground_rec_set(&rec_b, &variant->records[key->group_0x01].element_0x08);
            em_intruder_spawn(key->id_0x00, &rec_b, quest->spawn_count_0x308);
            quest->field_0x6978++;
            return;
        }
        /* fall through */
    case 13: {
        struct _ENEMY_WORK* enemy = NULL;
        struct _ENEMY_MINI_WORK* mini = NULL;

        if ((u8)em_get_unique_work(quest->spawn_count_0x308, &enemy, &mini) != 0) {
            em_quest_element_set(enemy, (u8)quest->spawn_count_0x308);
            quest->field_0x6978 = 0;
            quest->intruder_index_0x6977 = 1;
            quest->spawn_count_0x308++;
        }
        break;
    }
    }
}



/* This unit's own `.bss` (`config/RMHE08/splits.txt`, `.bss 0x806C5558..0x806C5858`): the band's four
 * picked-pair tables, 48 four-byte pairs each.  The lot picks write them and the result rows are built
 * from them - `quest_item_pair_tbl_copy` moves `_a`/`_b` into two adjacent 0xC0 buffers of a result
 * record, `quest_element_clear` zeroes all four and refills `_a` from the item work's own lot table. */
Q_ItemPair quest_item_pair_tbl_a[0x30];
Q_ItemPair quest_item_pair_tbl_b[0x30];
Q_ItemPair quest_item_pair_tbl_c[0x30];
Q_ItemPair quest_item_pair_tbl_d[0x30];
/* The quest work block `quest_init` clears and points `quest_work_ptr` at, the text scratch the text getters
 * format into, and the element-check callback table the enemy side fills in. */
QuestWork quest_work;
char quest_text_buffer[0x100];
NoraSetProc nora_set_proc;

/* This unit's own `.sbss` (`splits.txt` `.sbss 0x80794C1C..0x80794C3C`, in address order): the quest list
 * block `quest_list_load_hunt`/`_arena` allocate (GUESS names: `quest_list_pool` is the 0x4400-byte
 * `work_mem_alloc` block, `quest_list_values` is pool + 0x1A0, declared in `quest/quest_entry.h` - and
 * `quest_list_file` is that + 0xE0, the `load_file` destination), then the
 * five table pointers the roll functions read - the per-kind entry lists and the two pairs of remaining-lot
 * tables by count (GUESS: the dump names them `em_bui_tbl`, `em_bui_rem_l/h` and `em_hokaku_rem_l/h`).  Set
 * outside this unit. */
u8 quest_supply_drop_empty[4];
u8* quest_list_file;
QuestListBlock* quest_list_pool;
u16* quest_list_values;
u8** em_bui_tbl;
Q_LotEntry** em_bui_rem_l;
Q_LotEntry** em_bui_rem_h;
Q_LotEntry** em_hokaku_rem_l;
Q_LotEntry** em_hokaku_rem_h;
QuestRecord** quest_list_items;
QuestWork* quest_work_ptr;
s32 quest_list_count;


extern "C" {


/* Prototypes for the definitions below that an earlier function calls (`quest_item_count_sum` is
 * defined at the address its own body lives at, after its two callers). */
s16 quest_item_count_sum(u16 id, s32 who);
QuestRecord* quest_record_get(void);

/* Applies `used` of the arena item `id` to the work block's item table (the entry the table's own
 * count has room for). */
void quest_element_item_apply(u8 id, u16 count, u16 used) {
    QuestWork* work = quest_work_ptr;
    QuestArenaItem* item;
    s8 index;
    s32 rest;

    if (work == NULL) {
        return;
    }
    index = work->count_0x6A2A;
    if (index >= 2) {
        return;
    }
    item = &work->arena_items_0x6A2C[index];
    if (item->id != (u16)id) {
        return;
    }
    if (item->count != count) {
        return;
    }
    if (item->remaining == 0) {
        return;
    }
    rest = item->remaining - used;
    if (rest < 0) {
        rest = 0;
    }
    item->remaining = rest;
}

/* The number of u16 entries the work block's item stack `index` holds (0 when out of range). */
s32 quest_slot_count_get(u8 index) {
    QuestWork* work = quest_work_ptr;

    if (work == NULL || index >= 8) {
        return 0;
    }
    return work->slot_counts_0x66B8[index];
}

/* Copies the work block's item stack `index` into `out`. */
void quest_slot_items_get(u8 index, u16* out) {
    QuestWork* work = quest_work_ptr;
    u16* src;
    s32 i;

    if (work == NULL || index >= 8) {
        return;
    }
    src = work->slot_values_0x6698[index];
    for (i = 0; i < work->slot_counts_0x66B8[index]; i++) {
        *out++ = *src++;
    }
}

/* The record of quest `quest_id`: from the loaded quest list, else (ids from 60000 up) from the network
 * control's staging buffer on the server-select screen or its ten received quest files. */
QuestRecord* quest_record_find(u16 quest_id) {
    NetCtrlWk* net = net_ctrl_wk;
    QuestRecord** items = quest_list_items;
    s32 count = quest_list_count;
    QuestRecord* rec;
    s32 i;

    for (i = 0; i < count; i++) {
        rec = *items++;
        if (rec->field_0x02C == quest_id) {
            return rec;
        }
    }
    if (net != NULL && quest_id >= 60000) {
        if (isServerSelectState() == 1) {
            return (QuestRecord*)net->staging_0xC3EC;
        }
        for (i = 0; i < 10; i++) {
            rec = (QuestRecord*)net->file_buffers_0xC39C[i];
            if (rec->field_0x02C == quest_id) {
                return rec;
            }
        }
    }
    return NULL;
}

/* The u16 the `quest_list_items` entry whose +0x2C key is `key` pairs with (0 when there is none). */
u16 quest_work_word_get(u16 key) {
    s32 count;
    QuestRecord** items;
    s32 i;

    items = quest_list_items;
    i = 0;
    count = quest_list_count;
    for (; i < count; i++) {
        if ((*items++)->field_0x02C == key) {
            break;
        }
    }
    if (i >= count) {
        return 0;
    }
    return quest_list_values[i];
}

/* Moves the player's category-1 items out of `items` (35 slots) into the result's kept list with their values,
 * and drops the category-2 ones. */
void quest_result_keep_items(Q_ItemPair* items, Q_ResultWork* result) {
    s32 i;
    s32 n;

    for (i = 0; i < 0x23; i++) {
        result->kept_value_0x278[i] = 0;
        result->kept_0x1EC[i].id = 0;
        result->kept_0x1EC[i].num = 0;
    }
    n = 0;
    for (i = 0; i < 0x23; i++, items++) {
        if (items->id == 0 || (s16)items->num == 0) {
            continue;
        }
        if (item_category_ck(items->id, 1) != 0) {
            item_pair_copy(&result->kept_0x1EC[n], items);
            result->kept_value_0x278[n] = GetItemData(items->id)->field_0x010;
            n++;
            items->id = 0;
            items->num = 0;
        } else if (item_category_ck(items->id, 2) != 0) {
            items->id = 0;
            items->num = 0;
        }
    }
}

/* Moves the player's category-0x10 items out of `items` (35 slots) into the result's delivered list; returns
 * how many it moved. */
u8 quest_result_deliver_items(Q_ResultWork* result, Q_ItemPair* items) {
    s32 i;
    Q_ItemPair* clear;
    Q_ItemPair* dst;
    s32 j;

    clear = result->delivered_0x304;
    for (i = 0; i < 0x28; i++) {
        clear[i].id = 0;
        clear[i].num = 0;
    }
    result->present_0x3A4 = 0;
    dst = result->delivered_0x304;
    for (j = 0; j < 0x23; j++, items++) {
        if (items->id != 0 && (s16)items->num != 0 && item_category_ck(items->id, 0x10) != 0) {
            dst->id = items->id;
            dst->num = items->num;
            dst++;
            result->present_0x3A4++;
            items->id = 0;
            items->num = 0;
        }
    }
    return result->present_0x3A4;
}

/* Clears both halves of every entry of the run's 35-slot item list whose id is not a usable one. */
void quest_item_list_prune(QuestItemSlot* slots) {
    s32 i;
    QuestItemSlot* slot = slots;

    for (i = 0; i < 35; i++, slot++) {
        if (Pl_item_id_usable_ck(slot->id, 0) == 0) {
            slot->id = 0;
            slot->count = 0;
        }
    }
}

/* Fills the quest result for the slot's move work `work`: the quest id and the carried items (the pouch in a
 * network session, else the player's kept and delivered items), the times, the hunt ranks and the count runs,
 * the arena time medal, the result kind the entry state picks and, for a cleared quest, the reward rolls. */
void quest_result_fill(Q_MoveWork* work) {
    char text[0x80];
    QuestWork* quest = quest_work_ptr;
    Q_ResultWork* result = get_qResult_work();
    struct _PLW* players;
    struct _PLW* me;
    u16* times;
    u16 slot;

    result->progress_0x1E0 = quest->stat_word_0x10;
    result->field_0x1E4 = 0;
    players = (struct _PLW*)get_move_work_adrs(2);
    me = &players[(s8)my_player_no()];
    if (work->sub_0xFA != 8) {
        sprintf(text, quest_str_tbl_35_get(21));
        hud_msg_push(0, text);
    }
    if (system_w.net_session_0x90f == 1) {
        memcpy(result->items_0x154, QUEST_WORK->pouch_0x6778, sizeof(result->items_0x154));
    } else {
        quest_result_keep_items((Q_ItemPair*)me->slot_id, result);
        if (quest_result_deliver_items(result, (Q_ItemPair*)me->slot_id) != 0 && work->sub_0xFA != 8 && work->sub_0xFA != 7) {
            sprintf(text, quest_str_tbl_35_get(28));
            hud_msg_push(0, text);
        }
        quest_item_list_prune((QuestItemSlot*)me->slot_id);
        memcpy(result->items_0x154, me->slot_id, sizeof(result->items_0x154));
        memcpy(&result->items_0x154[0x18], me->spare_slot_id, sizeof(me->spare_slot_id));
    }
    result->elapsed_0x148 = quest_elapsed_time_get();
    result->seconds_0x14C = result->elapsed_0x148 / (frames_per_second_60f * Screen_w.frame_scale);
    result->time_base_0x150 = quest->field_0x020;
    result->rank_0x3A6 = result_hunt_rank_get(&QUEST_WORK->set_c, &QUEST_WORK->set_d);
    result->time_rank_0x3A8 = result_time_rank_get((s32)(quest_elapsed_time_get() / (frames_per_second_60f * Screen_w.frame_scale)));
    result_rank_rows_fill(result->rank_rows_0x3AA);
    result->rank_points_0x3DC = result_rank_points_get(&QUEST_WORK->set_c, &QUEST_WORK->set_d);
    memcpy(result->count_a, &QUEST_WORK->set_c, sizeof(result->count_a));
    memcpy(result->count_b, &QUEST_WORK->set_d, sizeof(result->count_b));
    memcpy(result->block_0x0A4, quest_work_ptr->result_block_0x534, sizeof(result->block_0x0A4));
    result->time_medal_0x434 = 0;
    if (quest_flag_10_ck(NULL) == 1) {
        slot = quest_id_get() - 60000;
        if (slot < 12) {
            times = arena_time_table[slot];
            if (result->elapsed_0x148 <= times[0] * 30) {
                result->time_medal_0x434 = 2;
            } else if (result->elapsed_0x148 <= times[1] * 30) {
                result->time_medal_0x434 = 1;
            }
        }
    }
    if (quest_select_ready_ck() == 1) {
        if ((s32)(u16)quest_id_get() == 1) {
            if (quest_element_flag20_ck(quest_work_ptr, 0) == 1) {
                work->sub_0xFA = 4;
            } else {
                work->sub_0xFA = 6;
            }
        } else {
            work->sub_0xFA = 4;
        }
    } else if (work->sub_0xFA == 4) {
        if (quest_flag_10_ck(NULL) == 1) {
            quest_element_clear(me, work->sub_0xFA, (Q_ArenaElement*)result);
        } else if (quest_flag_2000000_only_ck(NULL) == 1) {
            quest_pair_roll_first(me, work->sub_0xFA);
        } else if (quest_flag_4000000_ck(NULL) == 1) {
            quest_pair_roll_last(me, work->sub_0xFA);
        } else {
            quest_pair_roll_all(me, work->sub_0xFA);
        }
    }
    switch (work->sub_0xFA) {
    case 4:
        result->phase_0x1E2 = 1;
        break;
    case 6:
        result->phase_0x1E2 = 2;
        break;
    default:
        result->phase_0x1E2 = 3;
        memcpy(result->items_0x154, QUEST_WORK->pouch_0x6778, sizeof(result->items_0x154));
        memset(result->rank_rows_0x3AA, 0, sizeof(result->rank_rows_0x3AA));
        result->rank_0x3A6 = 0;
        result->time_rank_0x3A8 = 0;
        result->rank_points_0x3DC = 0;
        break;
    }
    if (result->phase_0x1E2 == 1) {
        if (quest_flag_4000000_ck(NULL) == 0) {
            quest_part_reward_roll(quest_work_ptr);
            result->stat_0x3E0.flags_0x08 = quest_work_ptr->stat_0x6AA4.flags_0x08;
            quest_capture_reward_roll(quest_work_ptr, me);
        }
        quest_monster_setup((Q_ItemWork*)quest_work_ptr, me);
        QuestWork* q = quest_work_ptr;

        if (((q->elements_0x0094[0].flags & 1) || (q->elements_0x0094[0].flags & 0x400)) && q->elements_0x0094[0].id == 25) {
            system_w.field_0x8b2 = 1;
        }
    }
    if (quest->stat_word_0x10 >= 10000) {
        result->kind_0x1E3 = 3;
    } else {
        result->kind_0x1E3 = 1;
    }
    result->credit_0x3F0 = lb_param_w.field_0x04;
}

/* The arena form of the result fill: the quest id, the times and the multiplayer arena's clear-time mark, the
 * arena element build for a cleared run, and the result kind. */
#pragma opt_propagation off
void quest_arena_summary_step(Q_MoveWork* work) {
    QuestWork* quest = quest_work_ptr;
    Q_ResultWork* result = get_qResult_work();
    struct _PLW* players;
    struct _PLW* me;
    s32 row;

    result->progress_0x1E0 = quest->stat_word_0x10;
    result->field_0x1E4 = 0;
    players = (struct _PLW*)get_move_work_adrs(2);
    me = &players[(s8)my_player_no()];
    result->elapsed_0x148 = quest_elapsed_time_get();
    result->seconds_0x14C = result->elapsed_0x148 / (frames_per_second_60f * Screen_w.frame_scale);
    result->time_base_0x150 = quest->field_0x020;
    row = result->progress_0x1E0 - 9000;
    if (multi_arena_clr_time[row * 2 + (system_w.field_0x8af != 0)] > result->elapsed_0x148) {
        result->time_medal_0x434 = 1;
    } else {
        result->time_medal_0x434 = 0;
    }
    if (work->sub_0xFA == 4) {
        quest_element_build(me, work->sub_0xFA, (Q_ArenaElement*)result);
    }
    switch (work->sub_0xFA) {
    case 4:
        result->phase_0x1E2 = 1;
        break;
    case 6:
        result->phase_0x1E2 = 2;
        break;
    default:
        result->phase_0x1E2 = 3;
        break;
    }
}
#pragma opt_propagation reset

/* Standing on the gallery cell (0xFA/0xFF/0xFF) of area 1 with the BGM not held: requests the gallery event and
 * marks the demo running, or holds the BGM when the request is refused. */
void quest_gallery_cell_step(Q_MoveWork* work) {
    struct _PLW* players;
    StageCell cell;

    if (snd_bgm_hold_ck() == 0 && work->sub_0xEA == 1) {
        players = (struct _PLW*)get_move_work_adrs(2);
        if (players != NULL) {
            cell.word = stage_cell_get(players[(s8)my_player_no()].area_cell_0x5AB, 1);
            if (cell.bytes[0] == 0xFA && cell.bytes[1] == 0xFF && cell.bytes[2] == 0xFF) {
                if (lb_event_request(1) == 1) {
                    demo_set_running();
                } else {
                    snd_bgm_hold_set();
                }
            }
        }
    }
}

/* Steps the first quest's intro countdown in the move work: arms a 60-frame timer once element 0 is
 * flagged in phase sub-state 1, holds it while the sub-state differs, and starts lobby event 8 when it
 * runs out. */
void quest_intro_step(Q_MoveWork* work) {
    s32 timer;

    if (quest_select_ready_ck() != 0 && (s32)(u16)quest_id_get() == 1) {
        switch (work->intro_state_0x118) {
        case 0:
            if (quest_element_flag20_ck(quest_work_ptr, 0) == 1 && work->sub_0xEA == 1) {
                work->intro_timer_0x119 = 60;
                work->intro_state_0x118 = 1;
            }
            break;
        case 1:
            if (work->sub_0xEA != 1) {
                work->intro_timer_0x119 = 60;
            } else {
                timer = work->intro_timer_0x119;
                if (timer == 0) {
                    lb_event_request(8);
                    work->intro_state_0x118 = 2;
                } else {
                    work->intro_timer_0x119--;
                }
            }
            break;
        }
    }
}

/* Clears element `index` of the work block's element array. */
void quest_element_reset(u8 index) {
    if (quest_work_ptr != NULL) {
        quest_element_set(quest_work_ptr, index, 0);
    }
}

/* Sets element `index`'s value to 0 once `id`'s own count has caught up with its target. */
void quest_element_value_set(s32 index, u16 id, s32 delta) {
    QuestElement* el;
    s32 i;

    if (quest_work_ptr == NULL) {
        return;
    }
    i = index;
    el = &quest_work_ptr->elements_0x0094[i];
    for (; i < 3; el++, i++) {
        if ((el->flags & 0x8) != 0 && (el->flags & 0x2) != 0) {
            if (el->id == id) {
                s16 have = quest_item_count_sum(id, 0);
                if (el->target_count > have) {
                    continue;
                }
                quest_element_set(quest_work_ptr, (u16)i, 0);
            }
        }
    }
}

/* How many of the item `id`'s count `element`'s value still asks for, returning the element's id
 * (`out` gets the remainder, or 0 when the element is not live). */
u16 quest_element_item_count_get(s32 index, u16* out) {
    QuestElement* el;
    s16 rest;

    if (quest_work_ptr == NULL) {
        return 0;
    }
    el = &quest_work_ptr->elements_0x0094[index];
    *out = 0;
    if ((el->flags & 0x8) != 0 && (el->flags & 0x2) != 0) {
        rest = el->target_count - quest_item_count_sum(el->id, 0);
        if (rest < 0) {
            rest = 0;
        }
        *out = rest;
        return el->id;
    }
    return 0;
}

/* Spends up to `count` of item `id` against the three elements that ask for it, then sends the player's new
 * total; returns the count spent. */
u32 quest_element_item_use(_PLW* owner, u16 id, s32 count) {
    u16 left;
    s32 i;
    s32 used;
    u16 spent;
    u16 item;

    spent = 0;
    used = 0;
    if (quest_work_ptr != NULL) {
        for (i = 0; i < 3; i++) {
            item = quest_element_item_count_get(i, &left);
            if (item != 0 && left != 0 && item == id) {
                if (used == 0) {
                    used++;
                    spent = quest_item_deposit(owner, id, count);
                }
                if (spent != 0) {
                    quest_element_value_set(i, item, -count);
                }
            }
        }
        lb_sub13_send(id, quest_item_count_sum(id, 1), 0);
    }
    return spent;
}

/* The item `id`'s count: `who` 0 sums every player's slot, 1 the local player's, 2 and up the
 * player whose slot follows. */
s16 quest_item_count_sum(u16 id, s32 who) {
    QuestWork* work;
    s16 total = 0;
    s32 player;
    s32 i;
    s32 j;

    if (get_move_work_adrs(0) == NULL) {
        return 0;
    }
    switch (who) {
    case 0:
        work = quest_work_ptr;
        for (i = 0; i < 4; i++) {
            for (j = 0; j < 3; j++) {
                if (work->player_items_0x6A6A[i][j].id == id) {
                    total += work->player_items_0x6A6A[i][j].count;
                }
            }
        }
        break;
    case 1:
        player = (s8)my_player_no();
        i = 0;
        work = quest_work_ptr;
        for (; i < 3; i++) {
            if (work->player_items_0x6A6A[player][i].id == id) {
                return work->player_items_0x6A6A[player][i].count;
            }
        }
        break;
    default:
        i = 0;
        work = quest_work_ptr;
        player = who - 2;
        for (; i < 3; i++) {
            if (work->player_items_0x6A6A[player][i].id == id) {
                return work->player_items_0x6A6A[player][i].count;
            }
        }
        break;
    }
    return total;
}

/* Deposits up to `count` of item `id` from the player `owner` into its quest item list (a new slot when the
 * player holds none yet, else the slot of that id), capped at the most any element still needs; returns the
 * count moved. */
u16 quest_item_deposit(_PLW* owner, u16 id, s16 count) {
    s16 need = 0;
    s32 i;
    QuestItemSlot* held;
    u16 j;
    s16 rest;
    QuestItemSlot* slot;

    for (i = 0; i < 3; i++) {
        rest = quest_element_value_at(id, i);
        if (rest >= 0) {
            rest -= quest_all_player_item_count_sum(id);
            if (need < rest) {
                need = rest;
            }
        }
    }
    if (need <= 0) {
        return 0;
    }
    if (quest_item_count_sum(id, 1) == 0) {
        slot = quest_work.player_items_0x6A6A[owner->chunk_ofs];
        for (j = 0; j < 3; j++, slot++) {
            if (slot->id == 0) {
                slot->id = id;
                if (count >= need) {
                    count = need;
                }
                slot->count = count;
                pl_item_add(owner, id, -count);
                return count;
            }
        }
    } else {
        held = quest_work.player_items_0x6A6A[owner->chunk_ofs];
        for (j = 0; j < 3; j++, held++) {
            if (held->id == id) {
                if (count >= need) {
                    count = need;
                }
                held->count += count;
                pl_item_add(owner, id, -count);
                return count;
            }
        }
    }
    return count;
}

/* Stores `value` into player `player`'s slot holding arena item `id` (taking a free slot when none
 * does), then clears every element whose remaining count has run out. */
void quest_arena_data_step(u8 player, u16 id, s16 value) {
    s16 count = 0;
    s16 i;
    s16 rest;
    s32 j;

    for (i = 0; i < 3; i++) {
        if (quest_work.player_items_0x6A6A[player][i].id == id) {
            count = quest_work.player_items_0x6A6A[player][i].count;
        }
    }
    if (count == 0) {
        for (i = 0; i < 3; i++) {
            if (quest_work.player_items_0x6A6A[player][i].id == 0) {
                quest_work.player_items_0x6A6A[player][i].id = id;
                quest_work.player_items_0x6A6A[player][i].count = value;
                break;
            }
        }
    } else {
        for (i = 0; i < 3; i++) {
            if (quest_work.player_items_0x6A6A[player][i].id == id) {
                quest_work.player_items_0x6A6A[player][i].count = value;
                break;
            }
        }
    }
    for (j = 0; j < 3; j++) {
        rest = (s16)quest_element_value_at(id, (u16)j);
        if (rest < 0) {
            continue;
        }
        rest -= (s16)quest_all_player_item_count_sum(id);
        if (rest <= 0) {
            quest_element_reset((u8)j);
        }
    }
}

/* An element's supply state: 0 while the pad input blocks it or the element has no item, 1 when nothing is
 * left to deliver, 2 otherwise. */
s32 quest_element_supply_state_get(s32 index) {
    u16 item;
    s32 left;

    if (Pl_motion_input_ck(0) == 1) {
        return 0;
    }
    item = em_work_slot_pair_get(index, NULL);
    if (item == 0) {
        return 0;
    }
    left = quest_element_value_index_get(index);
    if (left > 0) {
        left -= quest_item_count_sum(item, 0);
        if (left < 0) {
            left = 0;
        }
    }
    return (left == 0) ? 1 : 2;
}

/* Whether exactly one of the three key bytes of entry 6 is set and all of them are finished (3). */
u32 quest_players_state_get(void) {
    s32 active;
    s32 done;
    s32 i;

    done = 0;
    active = 0;
    for (i = 0; i < 3; i++) {
        if (quest_work_ptr->player_state_0x2D4[i] == 3) {
            done = 1;
        }
        if (quest_work_ptr->player_state_0x2D4[i] != 0) {
            active++;
        }
    }
    if (done == 1 && active <= 1) {
        return 1;
    }
    return 0;
}

/* Warps the local player to entry point 2 of the current map. */
void quest_entry_point_warp(void) {
    nw4r::math::VEC3 pos;
    u32 angle;

    setVec3(&pos, 0.0f, 0.0f, 0.0f);
    copyVec3(&pos, stage_entry_pos_get(get_now_mapno(), 2));
    angle = stage_entry_angle_get(get_now_mapno(), 2);
    pl_act_stage_latch_set(my_player_work_get(), 4);
    pl_warp_start(2, &pos, (u16)angle);
}

/* The current result row, or NULL when the screen has none. */
QuestRecord* quest_record_get(void) {
    QuestRecord* rec = quest_work.record_0x03C;

    if (rec == NULL) {
        return NULL;
    }
    return rec;
}

/* String table 35's `index`-th entry (the item-summary formats). */
char* quest_str_tbl_35_get(u32 index) {
    return (char*)get_str_tbl(35)[index];
}

/* String table 4's `index`-th entry. */
char* quest_str_tbl_4_get(u32 index) {
    return (char*)get_str_tbl(4)[index];
}

/* The pair table's `[row][col]` byte at +0x805F7898. */
u8 quest_pair_table_get(u8 row, u8 col) {
    return *(col + (&quest_pair_table[0] + (row * 2)));
}

/* The byte table's `index`-th entry at +0x805F78B4. */
u8 quest_byte_table_get(u8 index) {
    return *(&quest_byte_table[0] + index);
}

/* The record's name out of string table 5 (an empty string when there is no record). */
char* quest_name_text_get(void) {
    QuestRecord* rec = quest_record_get();
    u8** tbl = get_str_tbl(5);

    quest_text_buffer[0] = 0;
    if (rec != NULL) {
        strcpy(quest_text_buffer, (char*)tbl[rec->field_0x08B]);
    }
    return quest_text_buffer;
}

/* The same name for an explicit record. */
char* quest_name_text_get_of(QuestRecord* rec) {
    u8** tbl = get_str_tbl(5);

    quest_text_buffer[0] = 0;
    if (rec != NULL) {
        strcpy(quest_text_buffer, (char*)tbl[rec->field_0x08B]);
    }
    return quest_text_buffer;
}

/* The record's +0x198 condition row's text out of string table 41. */
char* quest_field198_text_get(void) {
    QuestRecord* rec = quest_record_get();
    u8** tbl = get_str_tbl(41);

    quest_text_buffer[0] = 0;
    if (rec == NULL) {
        return quest_text_buffer;
    }
    strcpy(quest_text_buffer, (char*)tbl[rec->condition_0x198]);
    return quest_text_buffer;
}

/* The same +0x198 condition text for an explicit record. */
char* quest_field198_text_get_of(QuestRecord* rec) {
    u8** tbl = get_str_tbl(41);

    quest_text_buffer[0] = 0;
    strcpy(quest_text_buffer, (char*)tbl[rec->condition_0x198]);
    return quest_text_buffer;
}

/* The record's +0x13A value formatted with string table 6's first format. */
char* quest_field13A_text_get(void) {
    QuestRecord* rec = quest_record_get();
    char** tbl = (char**)get_str_tbl(6);

    quest_text_buffer[0] = 0;
    if (rec != NULL) {
        sprintf(quest_text_buffer, tbl[0], rec->field_0x13A);
    }
    return quest_text_buffer;
}

/* The same +0x13A value for an explicit record. */
char* quest_field13A_text_get_of(QuestRecord* rec) {
    char** tbl = (char**)get_str_tbl(6);

    quest_text_buffer[0] = 0;
    if (rec != NULL) {
        sprintf(quest_text_buffer, tbl[0], rec->field_0x13A);
    }
    return quest_text_buffer;
}

/* The record's +0x34C / +0x350 / +0x354 timers as text: `which` picks which of the three. */
char* quest_time_text_get(u8 which) {
    QuestRecord* rec = quest_record_get();
    char** tbl = (char**)get_str_tbl(6);

    quest_text_buffer[0] = 0;
    if (rec != NULL) {
        switch (which) {
        case 0:
            sprintf(quest_text_buffer, tbl[1], rec->field_0x34C);
            break;
        case 1:
            if (rec->field_0x350 == 0) {
                sprintf(quest_text_buffer, tbl[4]);
            } else {
                sprintf(quest_text_buffer, tbl[1], rec->field_0x350);
            }
            break;
        case 2:
            if (rec->field_0x354 == 0) {
                sprintf(quest_text_buffer, tbl[4]);
            } else {
                sprintf(quest_text_buffer, tbl[1], rec->field_0x354);
            }
            break;
        }
    }
    return quest_text_buffer;
}

/* The same three timers for an explicit record. */
char* quest_time_text_get_of(QuestRecord* rec, u8 which) {
    char** tbl = (char**)get_str_tbl(6);

    quest_text_buffer[0] = 0;
    if (rec != NULL) {
        switch (which) {
        case 0:
            sprintf(quest_text_buffer, tbl[1], rec->field_0x34C);
            break;
        case 1:
            if (rec->field_0x350 == 0) {
                sprintf(quest_text_buffer, tbl[4]);
            } else {
                sprintf(quest_text_buffer, tbl[1], rec->field_0x350);
            }
            break;
        case 2:
            if (rec->field_0x354 == 0) {
                sprintf(quest_text_buffer, tbl[4]);
            } else {
                sprintf(quest_text_buffer, tbl[1], rec->field_0x354);
            }
            break;
        }
    }
    return quest_text_buffer;
}

/* The record's +0x348 value as text. */
char* quest_field348_text_get(void) {
    QuestRecord* rec = quest_record_get();
    char** tbl = (char**)get_str_tbl(6);

    quest_text_buffer[0] = 0;
    if (rec != NULL) {
        sprintf(quest_text_buffer, tbl[1], rec->field_0x348);
    }
    return quest_text_buffer;
}

/* The same +0x348 value for an explicit record. */
char* quest_field348_text_get_of(QuestRecord* rec) {
    char** tbl = (char**)get_str_tbl(6);

    quest_text_buffer[0] = 0;
    if (rec != NULL) {
        sprintf(quest_text_buffer, tbl[1], rec->field_0x348);
    }
    return quest_text_buffer;
}


/* The work block's +0x2E8 value (clamped up to 0) as text. */
char* quest_field2E8_text_get(void) {
    char** tbl = (char**)get_str_tbl(6);
    s32 value;

    quest_text_buffer[0] = 0;
    value = quest_work.field_0x2E8;
    if (value < 0) {
        value = 0;
    }
    sprintf(quest_text_buffer, tbl[1], value);
    return quest_text_buffer;
}

/* The run's clear time (`quest_work` +0x24 frames) as `mm'ss"ff` text. */
char* quest_clear_time_text_get(void) {
    char** tbl = (char**)get_str_tbl(6);
    s32 time = quest_work.field_0x024;
    f32 frames = frames_per_second_60f * Screen_w.frame_scale;
    s32 minutes = time / (s32)frames;
    s32 seconds;

    time -= minutes * (s32)frames;
    seconds = time / (s32)Screen_w.frame_scale;
    quest_text_buffer[0] = 0;
    sprintf(quest_text_buffer, tbl[2], minutes, seconds);
    return quest_text_buffer;
}
/* The elapsed time (`quest_work` +0x1C minus +0x20 frames) as the same text. */
char* quest_elapsed_time_text_get(void) {
    char** tbl = (char**)get_str_tbl(6);
    s32 time = quest_work.field_0x01C - quest_work.field_0x020;
    f32 frames = frames_per_second_60f * Screen_w.frame_scale;
    s32 minutes = time / (s32)frames;
    s32 seconds;

    seconds = time - minutes * (s32)frames;
    seconds /= (s32)Screen_w.frame_scale;
    quest_text_buffer[0] = 0;
    sprintf(quest_text_buffer, tbl[2], minutes, seconds);
    return quest_text_buffer;
}

/* The elapsed time in frames. */
s32 quest_elapsed_time_get(void) {
    return quest_work.field_0x01C - quest_work.field_0x020;
}

/* The run's score pair (+0x90/+0x91) as text. */
char* quest_score_text_get(void) {
    char** tbl = (char**)get_str_tbl(6);

    quest_text_buffer[0] = 0;
    sprintf(quest_text_buffer, tbl[3], quest_work.field_0x091, quest_work.field_0x090);
    return quest_text_buffer;
}

/* The arena item tally as text: the "N of M" string table 35's format 26 carries when the player
 * still holds items and the run is short of them, else the "none" format 27.  `index` 0 sums every
 * arena element, otherwise it reads element `index` alone; the `quest_flag_800000_ck` case reports
 * the whole set at once. */
char* quest_arena_items_text_get(u8 index) {
    s32 have;
    s32 missing;
    s32 count;
    s32 i;
    s32 need;
    s32 value;

    quest_text_buffer[0] = 0;
    have = 0;
    if (index != 0 && quest_flag_800000_ck(NULL) == 1) {
        sprintf(quest_text_buffer, quest_str_tbl_35_get(27));
        return quest_text_buffer;
    }
    if (quest_flag_80000_ck(NULL) == 1 || quest_flag_2000000_ck(NULL) == 1 ||
        quest_flag_80000000_ck(NULL) == 1 || quest_flag_4000000_ck(NULL) == 1) {
        count = (quest_flag_100_ck(NULL) == 1) ? 2 : 3;
        need = 0;
        for (i = 0; i < count; i++) {
            value = quest_arena_count_get(i);
            if (value >= 0) {
                have += value;
            }
            value = quest_arena_need_get(i);
            if (value >= 0) {
                need += value;
            }
        }
        missing = have - need;
    } else {
        have = quest_arena_count_get(index);
        missing = have - quest_arena_need_get(index);
    }
    if (missing < 0 || have <= 0) {
        sprintf(quest_text_buffer, quest_str_tbl_35_get(27));
    } else {
        sprintf(quest_text_buffer, quest_str_tbl_35_get(26), missing, have);
    }
    return quest_text_buffer;
}

/* The arena run's time as the same `mm'ss"ff` text `quest_clear_time_text_get` builds, from the
 * record's clear time instead of the work block's.  The clear time is first mapped through the
 * arena's own tables: the fixed `multi_arena_clr_time` pair when the 0x100000 flag is set and the
 * time is below 10000, else `arena_time_table`'s per-rank table scaled by 30 when 0x10 is set. */
#pragma opt_propagation off
char* quest_arena_time_text_get(QuestRecord* rec, s32 which) {
    char** tbl = (char**)get_str_tbl(6);
    s32 time = which;
    f32 frames;
    s32 minutes;
    s32 seconds;

    if (quest_flag_100000_ck(rec) == 1 && rec->field_0x02C < 10000) {
        s32 arena = rec->field_0x02C - 9000;

        time = multi_arena_clr_time[arena * 2 + (system_w.field_0x8af != 0)];
    } else if (quest_flag_10_ck(rec) == 1) {
        time = arena_time_table[rec->field_0x02C - 60000][(u8)which] * 30;
    }
    frames = frames_per_second_60f * Screen_w.frame_scale;
    minutes = time / (s32)frames;
    seconds = time - minutes * (s32)frames;
    seconds /= (s32)Screen_w.frame_scale;
    quest_text_buffer[0] = 0;
    sprintf(quest_text_buffer, tbl[2], minutes, seconds);
    return quest_text_buffer;
}
#pragma opt_propagation reset

/* The grade (0 = no record) as text: the grade's own format for 1..4, else the language's
 * "no record" string. */
char* quest_grade_text_get(u8 grade) {
    char** tbl = (char**)get_str_tbl(6);
    u32 slot;

    switch (grade) {
    case 0:
        quest_text_buffer[0] = 0;
        sprintf(quest_text_buffer, quest_grade_none_text_table[system_w.field_0x09]);
        return quest_text_buffer;
    case 1:
    case 2:
    case 3:
    case 4:
        slot = grade + 5;
        break;
    }
    quest_text_buffer[0] = 0;
    sprintf(quest_text_buffer, tbl[slot]);
    return quest_text_buffer;
}

/* The current run's grade as text: the same ratio `quest_grade_get` scores, rendered by
 * `quest_grade_text_get` (1 when no element matches, so the "no record" string comes out). */
char* quest_grade_text_cur_get(void) {
    s16 total;
    f32 ratio;
    u8 grade = 1;

    quest_text_buffer[0] = 0;
    if (quest_element_value_get(4, &total) == 0) {
        return quest_text_buffer;
    }
    ratio = percent_scale_100f * ((f32)quest_work.field_0x5D8 / (f32)total);
    if (ratio <= quest_grade_ratio_10f) {
        grade = 4;
    } else if (ratio <= quest_grade_ratio_30f) {
        grade = 3;
    } else if (ratio <= quest_grade_ratio_50f) {
        grade = 2;
    }
    return quest_grade_text_get(grade);
}

/* The run's grade: the score as a percentage of the arena element 4 target, banded at the three
 * `quest_grade_ratio_*f` thresholds (0 when the element is not live). */
s32 quest_grade_get(void) {
    s16 total;
    f32 ratio;

    if (quest_element_value_get(4, &total) == 0) {
        return 0;
    }
    ratio = percent_scale_100f * ((f32)quest_work.field_0x5D8 / (f32)total);
    if (ratio <= quest_grade_ratio_10f) {
        return 4;
    }
    if (ratio <= quest_grade_ratio_30f) {
        return 3;
    }
    if (ratio <= quest_grade_ratio_50f) {
        return 2;
    }
    return 1;
}


/* Whether the same percentage reaches `grade` - the rank the caller's threshold asks for. */
s32 quest_grade_rank_get(u8 grade) {
    s16 total;
    f32 ratio;

    if (quest_element_value_get(4, &total) == 0) {
        return 0;
    }
    ratio = percent_scale_100f * ((f32)quest_work.field_0x5D8 / (f32)total);
    return ratio <= (f32)grade;
}

/* The record's +0x36C slot progress byte, or 0 while the work state is "moving" and for a missing
 * record. */
u8 quest_slot_progress_get(QuestRecord* rec) {
    if (move_work_state_ck() == 1) {
        return 0;
    }
    if (rec == NULL) {
        rec = quest_record_get();
        if (rec == NULL) {
            return 0;
        }
    }
    return rec->field_0x36C;
}

/* The record's name index (string table 5's key), or 0 for a missing record; 1 while the work
 * state is "moving". */
u8 quest_name_index_get(QuestRecord* rec) {
    if (move_work_state_ck() == 1) {
        return 1;
    }
    if (rec == NULL) {
        rec = quest_record_get();
        if (rec == NULL) {
            return 0;
        }
    }
    return rec->field_0x08B;
}

/* The record's +0x372 word, or the current record's. */
u16 quest_field372_get(QuestRecord* rec) {
    if (rec == NULL) {
        rec = quest_record_get();
        if (rec == NULL) {
            return 0;
        }
    }
    return rec->field_0x372;
}

/* The "no time yet" text (string table 6's fifth format). */
char* quest_no_time_text_get(void) {
    char** tbl = (char**)get_str_tbl(6);

    quest_text_buffer[0] = 0;
    sprintf(quest_text_buffer, tbl[5]);
    return quest_text_buffer;
}

/* The record's +0x780 `which`-th monster id rendered through string table 33, the "no time yet"
 * text when the id is 0 or the record is missing, and a lone space for the second slot while both
 * ids are still unset. */
char* quest_monster_text_get(QuestRecord* rec, u8 which) {
    if (rec == NULL) {
        return quest_no_time_text_get();
    }
    if (which == 1 && rec->field_0x30C[0] == 0 && rec->field_0x30C[1] == 0) {
        quest_text_buffer[0] = ' ';
        quest_text_buffer[1] = 0;
        return quest_text_buffer;
    }
    if (rec->field_0x30C[which] == 0) {
        return quest_no_time_text_get();
    }
    strcpy(quest_text_buffer, (char*)str_tbl_33_get(rec->field_0x30C[which]));
    return quest_text_buffer;
}

/* The text a result-screen field kind shows: kinds 0..31 pick one of the record's own text runs or
 * one of this band's text getters, and a null record falls back to the "name unavailable" table. */
char* quest_result_field_text_get(QuestRecord* rec, u8 kind) {
    char buf[256];
    char* brk;

    buf[0] = 0;
    quest_text_buffer[0] = 0;
    if (rec == NULL) {
        if (kind == 0) {
            strcpy(quest_text_buffer, quest_name_unavailable_text[system_w.field_0x09]);
        } else {
            sprintf(quest_text_buffer, " ");
        }
        return quest_text_buffer;
    }
    switch (kind) {
    case 0:
        msg_str_gen(rec->field_0x000, quest_text_buffer);
        break;
    case 22:
        msg_str_gen(rec->field_0x000, buf);
        if ((u32)flfntStrLen(buf) > 22) {
            brk = flKnjMsgNumPtr(buf, 19);
            if (brk != NULL) {
                *brk = 0;
                strcat(buf, "...");
            }
        }
        strcpy(quest_text_buffer, buf);
        break;
    case 1:
        msg_str_gen(rec->field_0x02E, quest_text_buffer);
        break;
    case 16:
        msg_str_gen(rec->field_0x02E, quest_text_buffer);
        brk = flfntStrChr(quest_text_buffer, 10);
        if (brk != NULL) {
            *brk = 0;
        }
        break;
    case 17:
        msg_str_gen(rec->field_0x02E, quest_text_buffer);
        brk = flfntStrChr(quest_text_buffer, 10);
        if (brk != NULL) {
            strcpy(buf, brk + 1);
            strcpy(quest_text_buffer, buf);
        } else {
            quest_text_buffer[0] = 0;
        }
        break;
    case 2:
        if (quest_flag_10000000_ck(rec) == 1) {
            return quest_grade_text_get(0);
        }
        msg_str_gen(rec->field_0x08C, quest_text_buffer);
        break;
    case 3:
        msg_str_gen(rec->field_0x0B5, quest_text_buffer);
        break;
    case 4:
        msg_str_gen(rec->field_0x0DE, quest_text_buffer);
        break;
    case 5:
        msg_str_gen(rec->field_0x13C, quest_text_buffer);
        break;
    case 6:
        return quest_field198_text_get_of(rec);
    case 7:
        msg_str_gen(rec->field_0x19A, quest_text_buffer);
        break;
    case 8:
        msg_str_gen(rec->field_0x1C9, quest_text_buffer);
        break;
    case 9:
        return quest_name_text_get_of(rec);
    case 10:
        return quest_field13A_text_get_of(rec);
    case 11:
        return quest_time_text_get_of(rec, 0);
    case 13:
        return quest_time_text_get_of(rec, 1);
    case 14:
        return quest_time_text_get_of(rec, 2);
    case 15:
        return quest_time_text_get_of(rec, 3);
    case 12:
        return quest_field348_text_get_of(rec);
    case 30:
        return quest_monster_text_get(rec, 0);
    case 31:
        return quest_monster_text_get(rec, 1);
    case 26:
        return quest_arena_time_text_get(rec, 0);
    case 27:
        return quest_arena_time_text_get(rec, 1);
    case 28:
        return quest_arena_time_text_get(rec, 2);
    default:
        break;
    }
    return quest_text_buffer;
}

}  /* extern "C" */
