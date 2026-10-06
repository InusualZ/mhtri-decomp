/* enemy/em_pop.cpp - the field-side enemy population/roster manager: it walks the 0x224-byte `EmPopRec` roster
 *   records, releases and recycles them, and carries the quest-progress queries around them; its string pool names the
 *   data it consumes (`05/em_set/em_set_m%02da%02d_%03d.esd` 0x805F81F4, `m%03d_%06d_c_pop.dat` 0x805F84E0).
 * RANGE. .text 0x803B465C-0x803B936C (82 functions); .ctors 0x8056F3B4-0x8056F3B8 (the static initialiser),
 *   .data 0x805F7C68-0x805F8220, .bss 0x806CC420-0x806D2A68, .sdata 0x80793688-0x80793690, .sbss 0x80794C48-0x80794C50,
 *   .sdata2 0x8079C560-0x8079C5B8, extab, extabindex.  `enemy/em_model.cpp` continues the band at 0x803B936C.
 * SEAM. Unproven and probably several TUs (the pool repeats, the `.ctors` thunk and the proposed tiling are in
 *   docs/enemy.md).
 * FLAGS. `cflags_menu`, the address neighbour's group (`cflags_main` + `-opt nopeephole`).
 * NAMES. The module and `em_pop` are a GUESS from what the range does and its callers (nine `src/enemy/*` units);
 *   every function name is a GUESS from its own body (the flag masks, offsets and callers its comment names), none
 *   from the dump - among them quest_special_element_step, quest_grade_goal_step, quest_score_deduct,
 *   quest_supply_drop_step, quest_bonus_pick and quest_stat_get.  `quest_flag_2000000_ck` and `quest_flag_800000_ck`
 *   are two-bit predicates (0x2000000 or 0x100; 0x800000 or exactly 0x40 of 0xC0).  The free-hunt tail
 *   (`enemy/em_set_work.h`): em_set_* and quest_ex_condition_show_ck, quest_clock_real_step, the work
 *   `em_set_work`/`em_set_work_ptr`, `em_set_default_tbl`, `em_set_reward_rows` (the 21 0x18-byte rows folded into
 *   one symbol), em_set_frame_step and `em_set_reward_tbl` are GUESS names from their bodies; `__ct__9EmSetWorkFv`,
 *   `__ct__10EmSetEntryFv` and `__sinit_\em_pop_cpp` are the compiler's.
 *   GUESS (from each body and its callers): quest_special_element_step, quest_grade_goal_step, quest_stat_get
 *   GUESS: quest_score_deduct, quest_supply_drop_step, quest_bonus_pick, quest_ex_condition_show_ck
 *   GUESS: quest_ex_condition_ck, em_set_entry_rec_apply, em_set_work_init, em_set_live_count
 *   GUESS: em_set_area_files_load, em_set_entries_spawn, em_set_rotation_step, em_set_start, em_set_field_setup
 *   GUESS: quest_clock_real_step, em_set_move_start, em_set_area_enter, quest_warp_hub, em_set_reward_item_add
 *   GUESS: em_set_kill_record, em_set_work_state_get, em_set_userdata_flag_ck, em_set_result_store
 *   GUESS: em_set_result_restore, em_set_enemy_detach
 * SEAM. The quest head 0x803B465C-0x803B6F2C reads `quest/quest_entry.cpp`'s private `.data` (quest_supply_drop_tbl,
 *   quest_bonus_pick_tbl) and its pooled `.sdata2` (0.0f at 0x8079C530, the int-to-float magic at 0x8079C528): it is
 *   that TU's tail (request quest-q1#27).  Its 0.0f/9950.0f/5.0f literals emit this unit's own pool meanwhile.
 * RESIDUALS. Every row is written.
 *  - em_set_work_init 90.5 %: retail keeps the 16-entry area-set clear as a 2x8 unrolled loop, ours unrolls it
 *    fully (every loop shape and index type tried); em_set_field_setup 96.6 % and quest/quest_entry.cpp's
 *    quest_enter_load: retail keeps `stage * 8` in a saved register across `stage_dcm_path_get`, ours recomputes it;
 *    em_set_kill_record 91.1 %, em_set_result_store 96.2 %, em_set_move_start 96.1 %, em_set_rotation_step 98.5 %:
 *    register numbering; em_set_frame_step 99.8 %: pool label names;
 *  - object order: the two constructors are defined out of line at the end of the file, so they precede
 *    `__sinit_\em_pop_cpp` where retail places them after it; bytes per function are identical, the order
 *    matters only for a flip; the EmSetWork and EmSetEntry constructors have empty bodies by design (the compiler
 *    constructs the members), which the stub check reads as unwritten;
 *  - quest_supply_drop_step 98.9 %: `ran_suu`'s u16 declaration re-masks its result (retail masks the word once and
 *    compares unsigned); quest_bonus_pick 98.7 %: one byte reload in the weight sum; quest_special_element_step
 *    99.2 %, quest_grade_goal_step 99.6 % (`quest_element_pick_ck`'s u8 index, request quest-q1#17);
 *    quest_score_deduct 99.8 %: the pool literal's name;
 *  - `em_work_slot_pair_get`: retail reads the +0x98 halfword with `lhz`, ours with `lha`;
 *  - `quest_all_player_item_count_sum`: the `li` of the element index is scheduled before the pool loads in ours;
 *  - `quest_slot_byte_get`, `quest_element_progress_step`, `quest_arena_need_get`, `stepStagingDownloadForVersion`:
 *    register numbering of the loop temporaries (same size).
 *   data: `.bss`/`.ctors`/`.sdata`/`.sbss` emitted; `.data` 1457 of 1464 B; `.sdata2` 92 B against 88 (the quest
 *   head's literals belong to quest_entry's pool); candidate fold with `quest/quest_entry.cpp` (seam quest-q1#27).
 * SHAPES. The work's and the entry's constructors are user-declared and defined last; the
 *   extra-condition tests count their six board rows with an `s16` index (an `s32` one splits the unrolled loop);
 *   the rotation fill walks a byte offset beside the count; the point rows copy field by field through pointers.
 */

#include "types.h"
#include "nw4r/math.h"
#include "mh3_pad/vec3.h"
#include "ef/fn_800CDB2C.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "enemy/em_pop.h"
#include "enemy/em_model.h"
#include "quest/quest_entry.h"
#include "ef/get_move_work_adrs.h"
#include "font/flfnt.h"
#include "quest/quest_types.h"
#include "enemy/enemy_control.h"
#include "enemy/em020_ai.h"
#include "enemy/fn_8012EC74.h"
#include "enemy/ENEMY_WORK.h"
#include "fn_8004CAD8/get_qResult_work.h"
#include "menu/menu_item.h"
#include "Pl/fn_80273B14.h"
#include "Network/network_pat_control.h"
#include "unsplit/unknown.h"
#include "unsplit/Runtime.PPCEABI.H.h"
#include "lobby/lb_quest_screen.h"
#include "ef/eft052.h"
#include "enemy/fn_801251D0.h"
#include "enemy/em020_area2_action20_ck.h" /* em020_area2_action20_ck (rule 2) */
#include "lobby/lb_entry_start_default.h"  /* lb_entry_start_default (rule 2) */
#include "quest/quest_result_enter.h"      /* quest_result_enter (rule 2) */
#include "lobby/lb_sub17_send.h"           /* lb_sub17_send (rule 2) */
#include "lobby/quest_element_failed_ck.h" /* quest_element_live_ck (rule 2) */
#include "stage/stg_w.h"                   /* get_now_areano (rule 2) */
#include "sound/fn_800D7F54.h"             /* se_req_pos_ps (rule 2) */
#include "mh3_pad/lb_param_w.h"            /* lb_param_w (rule 2) */
#include "unsplit/menu.h"                  /* Screen_w */
#include "enemy/em_set_work.h"             /* the free-hunt work types */
#include "lobby/lb_quest_board_data.h"     /* LbQuestBoardData */
#include "unsplit/OS.h"                    /* OS_BUS_CLOCK */
#include "unsplit/lobby.h"                 /* stage_dcm_buffer_tbl (camera/camera_main.cpp's, declared in the band) */
#include "quest/quest_file_table.h"        /* stage_dcm_file_table (rule 2) */
#include "ef/system_core.h"                /* work_mem_alloc, load_file_req (rule 2) */
#include "NAND/nand.h"                     /* OSGetTime (rule 2) */
#include "camera/stage_dcm_path_get.h"     /* stage_dcm_path_get (rule 2) */
#include "ai/result_hunt_rank_get.h"       /* the result rank helpers (rule 2) */
#include "stage_map_set.h"                 /* stage_map_set (rule 2) */
#include "lobby/lb_quest_work_init.h"      /* lb_quest_work_init (rule 2) */
#include "Pl/pl_act_stage_latch_set.h"     /* pl_act_stage_latch_set, my_player_work_get (rule 2) */
#include "enemy/ENEMY_MINI_WORK.h"         /* _ENEMY_MINI_WORK */
#include "quest/quest_item_slot.h"         /* quest_item_id_low_ck, quest_move_flag_ck (rule 2) */
#include "sound/snd_bank_loader.h"         /* the stage and quest sound loaders (rule 2) */
#include "Pl/fn_80262940.h"                /* init_player_work, player_init_data_load (rule 2) */
#include "light/light.h"                   /* light_init (rule 2) */
#include "stage/shell.h"                   /* shell_work_init (rule 2) */
#include "mh3_pad.h"                       /* setVec3, copyVec3 (rule 2) */
#include "enemy/fn_8013F764.h"             /* em_kind_release (rule 2) */
#include "menu/quest_str_tbl_35_get.h"     /* quest_str_tbl_35_get (rule 2) */
#include "ef/eft_res.h"                    /* eft_control_init, eft_common_load (rule 2) */
#include "enemy/em_common.h"               /* the kill tests (rule 2) */
#include "enemy/enemy_control.h"           /* the entry spawns (rule 2) */
#include "enemy/em_model.h"                /* em_pop_work_step, em_pop_res_load (rule 2) */
#include "enemy/em_prog_work_init.h"         /* em_prog_slots_init, em_prog_work_init (rule 2) */
#include "enemy/em_quest_element_set.h"    /* em_quest_element_set (rule 2) */
#include "draw_shape.h"                    /* draw_shape_stage_load (rule 2) */
#include "fn_80056F24.h"                   /* GlareFilter_on, filter_panel_on (rule 2) */
#include "Pl/pl_act_step.h"                /* pl_area_entry_set (rule 2) */
#include "Pl/quest_spawn_rec_find.h"       /* quest_spawn_rec_find (rule 2) */
#include "camera/camera_work_init.h"       /* camera_work_init, camera_area_reset (rule 2) */
#include "ai/ainpc_init.h"                 /* ainpc_init, ainpc_res_load (rule 2) */
#include "hud/cockpit_quest_init.h"        /* cockpit_quest_init, cockpit_hunt_log_push (rule 2) */
#include "get_FqResult_work.h"             /* get_FqResult_work and the save counters (rule 2) */
#include "sound/fn_800F2A94.h"             /* snd_bgm_hold_ck, snd_hunt_stream_start (rule 2) */
#include "stage/stg_w.h"                   /* the stage area helpers (rule 2) */
#include "lobby/lb_quest_screen.h"         /* quest_area_res_load, quest_time_limit_set (rule 2) */
#include "menu/menu_item.h"                /* menu_item_work_init (rule 2) */
#include "Pl/fn_80288CEC.h"                /* pl_warp_start (rule 2) */
#include "lobby/lb_event_request.h"        /* lb_event_request (rule 2) */
#include "gallery_open.h"                  /* gallery_open (rule 2) */


extern "C" {


/* The text a result-screen field kind shows for the current row: kinds 0..31 pick one of the row's own text
 * runs or one of the quest band's text getters. */
char* quest_result_field_text_cur_get(u8 kind) {
    char buf[256];
    char* brk;
    QuestRecord* rec = quest_record_get();

    quest_text_buffer[0] = 0;
    buf[0] = 0;
    if (rec != NULL) {
        switch (kind) {
        case 0:
        case 22:
            msg_str_gen(rec->field_0x000, quest_text_buffer);
            break;
        case 1:
            msg_str_gen(rec->field_0x02E, quest_text_buffer);
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
        case 7:
            msg_str_gen(rec->field_0x19A, quest_text_buffer);
            break;
        case 8:
            msg_str_gen(rec->field_0x1C9, quest_text_buffer);
            break;
        case 16:
            msg_str_gen(rec->field_0x02E, quest_text_buffer);
            brk = flfntStrChr(quest_text_buffer, 10);
            if (brk != NULL) {
                *brk = 0;
            }
            break;
        case 17:
            if (quest_flag_10000000_ck(rec) == 1) {
                msg_str_gen(rec->field_0x08C, quest_text_buffer);
            } else {
                msg_str_gen(rec->field_0x02E, quest_text_buffer);
                brk = flfntStrChr(quest_text_buffer, 10);
                if (brk != NULL) {
                    strcpy(buf, brk + 1);
                    strcpy(quest_text_buffer, buf);
                } else {
                    quest_text_buffer[0] = 0;
                }
            }
            break;
        case 6:
            return quest_field198_text_get();
        case 9:
            return quest_name_text_get();
        case 10:
            return quest_field13A_text_get();
        case 11:
            return quest_time_text_get(0);
        case 13:
            return quest_time_text_get(1);
        case 14:
            return quest_time_text_get(2);
        case 15:
            return quest_time_text_get(3);
        case 12:
            return quest_field348_text_get();
        case 18:
            return quest_clear_time_text_get();
        case 19:
            return quest_field2E8_text_get();
        case 20:
            return quest_score_text_get();
        case 21:
            return quest_elapsed_time_text_get();
        case 23:
            return quest_arena_items_text_get(0);
        case 24:
            return quest_arena_items_text_get(1);
        case 25:
            return quest_arena_items_text_get(2);
        case 26:
            return quest_arena_time_text_get(rec, 0);
        case 27:
            return quest_arena_time_text_get(rec, 1);
        case 28:
            return quest_arena_time_text_get(rec, 2);
        case 29:
            return quest_grade_text_cur_get();
        case 30:
            return quest_monster_text_get(rec, 0);
        case 31:
            return quest_monster_text_get(rec, 1);
        }
    }
    return quest_text_buffer;
}

/* The row's halfword at +0x37A (0 when there is no row). */
u16 quest_field37A_get(QuestRecord* rec) {
    if (rec == NULL) {
        rec = quest_record_get();
        if (rec == NULL) {
            return 0;
        }
    }
    return rec->field_0x37A;
}

/* The slot's value byte: from the armed byte run when the 0x2000000 test passes, else from the
 * halfword run while the matching state bits are set; 0 when nothing applies. */
u8 quest_slot_byte_get(QuestRecord* rec, u8 slot) {
    QuestRecord* row;
    if (rec == NULL) {
        row = quest_record_get();
        if (row == NULL) {
            return 0;
        }
    } else {
        row = rec;
    }
    if (slot >= 3) {
        return 0;
    }
    if (quest_flag_2000000_ck(row) == 1) {
        if (row->slot_bytes_0x314[slot].armed == 0) {
            return 0;
        }
        return row->slot_bytes_0x314[slot].value;
    }
    QuestRecordSlotWord* word = &row->slot_words_0x330[slot];
    u32 flags = row->flags_0x310;
    if (flags & 5) {
        u32 slot_flags = word->flags;
        if ((slot_flags & 1) || (slot_flags & 0x400) || (slot_flags & 0x4000)) {
            return (u8)word->value;
        }
    }
    if (flags & 0x200000) {
        if (row->slot_bytes_0x314[slot].armed == 0) {
            return 0;
        }
        return row->slot_bytes_0x314[slot].value;
    }
    return 0;
}

/* The slot's halfword value when both the row and the slot carry state bit 2; else 0. */
u16 quest_slot_word_get(QuestRecord* rec, u8 slot) {
    if (rec == NULL) {
        rec = quest_record_get();
        if (rec == NULL) {
            return 0;
        }
    }
    if (slot >= 3) {
        return 0;
    }
    QuestRecordSlotWord* word = &rec->slot_words_0x330[slot];
    if (rec->flags_0x310 & 2) {
        if (word->flags & 2) {
            return word->value;
        }
    }
    return 0;
}

/* Whether the row carries flag 0x100 or has its +0x32C byte set. */
u32 quest_field32C_ck(QuestRecord* rec) {
    QuestRecord* row;
    if (rec == NULL) {
        row = quest_record_get();
        if (row == NULL) {
            return 0;
        }
    } else {
        row = rec;
    }
    if (quest_flag_100_ck(row) == 1) {
        return 1;
    }
    return row->field_0x32C != 0;
}

/* The row's byte at +0x32E (0 when there is no row). */
u8 quest_field32E_get(QuestRecord* rec) {
    if (rec == NULL) {
        rec = quest_record_get();
        if (rec == NULL) {
            return 0;
        }
    }
    return rec->field_0x32E;
}

/* Whether the result row carries flag 0x8. */
u32 quest_flag_8_ck(QuestRecord* rec) {
    if (move_work_state_ck() == 1) {
        return 0;
    }
    if (rec == NULL) {
        rec = quest_record_get();
        if (rec == NULL) {
            return 0;
        }
    }
    if (rec->flags_0x310 & 0x8) {
        return 1;
    }
    return 0;
}

/* Whether the result row carries flag 0x80000. */
u32 quest_flag_80000_ck(QuestRecord* rec) {
    if (move_work_state_ck() == 1) {
        return 0;
    }
    if (rec == NULL) {
        rec = quest_record_get();
        if (rec == NULL) {
            return 0;
        }
    }
    if (rec->flags_0x310 & 0x80000) {
        return 1;
    }
    return 0;
}

/* Whether the result row carries flag 0x2000000 or flag 0x100. */
u32 quest_flag_2000000_ck(QuestRecord* rec) {
    if (move_work_state_ck() == 1) {
        return 0;
    }
    if (rec == NULL) {
        rec = quest_record_get();
        if (rec == NULL) {
            return 0;
        }
    }
    if (rec->flags_0x310 & 0x2000000) {
        return 1;
    }
    return (rec->flags_0x310 & 0x100) != 0;
}

/* Whether the result row carries flag 0x80000000. */
u32 quest_flag_80000000_ck(QuestRecord* rec) {
    if (move_work_state_ck() == 1) {
        return 0;
    }
    if (rec == NULL) {
        rec = quest_record_get();
        if (rec == NULL) {
            return 0;
        }
    }
    if (rec->flags_0x310 & 0x80000000) {
        return 1;
    }
    return 0;
}

/* Whether the result row carries flag 0x4000000. */
u32 quest_flag_4000000_ck(QuestRecord* rec) {
    if (move_work_state_ck() == 1) {
        return 0;
    }
    if (rec == NULL) {
        rec = quest_record_get();
        if (rec == NULL) {
            return 0;
        }
    }
    if (rec->flags_0x310 & 0x4000000) {
        return 1;
    }
    return 0;
}

/* Whether the result row carries flag 0x100. */
u32 quest_flag_100_ck(QuestRecord* rec) {
    if (move_work_state_ck() == 1) {
        return 0;
    }
    if (rec == NULL) {
        rec = quest_record_get();
        if (rec == NULL) {
            return 0;
        }
    }
    if (rec->flags_0x310 & 0x100) {
        return 1;
    }
    return 0;
}

/* Whether the result row carries flag 0x10000000. */
u32 quest_flag_10000000_ck(QuestRecord* rec) {
    if (move_work_state_ck() == 1) {
        return 0;
    }
    if (rec == NULL) {
        rec = quest_record_get();
        if (rec == NULL) {
            return 0;
        }
    }
    if (rec->flags_0x310 & 0x10000000) {
        return 1;
    }
    return 0;
}

/* Whether the result row carries flag 0x100000. */
u32 quest_flag_100000_ck(QuestRecord* rec) {
    if (move_work_state_ck() == 1) {
        return 0;
    }
    if (rec == NULL) {
        rec = quest_record_get();
        if (rec == NULL) {
            return 0;
        }
    }
    if (rec->flags_0x310 & 0x100000) {
        return 1;
    }
    return 0;
}

/* Whether the result row carries flag 0x40000000. */
u32 quest_flag_40000000_ck(QuestRecord* rec) {
    if (move_work_state_ck() == 1) {
        return 0;
    }
    if (rec == NULL) {
        rec = quest_record_get();
        if (rec == NULL) {
            return 0;
        }
    }
    if (rec->flags_0x310 & 0x40000000) {
        return 1;
    }
    return 0;
}

/* Whether the result row carries flag 0x1000000. */
u32 quest_flag_1000000_ck(QuestRecord* rec) {
    if (move_work_state_ck() == 1) {
        return 0;
    }
    if (rec == NULL) {
        rec = quest_record_get();
        if (rec == NULL) {
            return 0;
        }
    }
    if (rec->flags_0x310 & 0x1000000) {
        return 1;
    }
    return 0;
}

/* Whether the result row carries flag 0x10. */
u32 quest_flag_10_ck(QuestRecord* rec) {
    if (move_work_state_ck() == 1) {
        return 0;
    }
    if (rec == NULL) {
        rec = quest_record_get();
        if (rec == NULL) {
            return 0;
        }
    }
    if (rec->flags_0x310 & 0x10) {
        return 1;
    }
    return 0;
}

/* Whether the result row carries flag 0x200000. */
u32 quest_flag_200000_ck(QuestRecord* rec) {
    if (move_work_state_ck() == 1) {
        return 0;
    }
    if (rec == NULL) {
        rec = quest_record_get();
        if (rec == NULL) {
            return 0;
        }
    }
    if (rec->flags_0x310 & 0x200000) {
        return 1;
    }
    return 0;
}

/* Whether the result row carries flag 0x800000, or has exactly flag 0x40 of the 0xC0 pair. */
u32 quest_flag_800000_ck(QuestRecord* rec) {
    if (move_work_state_ck() == 1) {
        return 0;
    }
    if (rec == NULL) {
        rec = quest_record_get();
        if (rec == NULL) {
            return 0;
        }
    }
    if (rec->flags_0x310 & 0x800000) {
        return 1;
    }
    return (rec->flags_0x310 & 0xC0) == 0x40;
}

/* Whether the result row carries flag 0x40000. */
u32 quest_flag_40000_ck(QuestRecord* rec) {
    if (move_work_state_ck() == 1) {
        return 0;
    }
    if (rec == NULL) {
        rec = quest_record_get();
        if (rec == NULL) {
            return 0;
        }
    }
    if (rec->flags_0x310 & 0x40000) {
        return 1;
    }
    return 0;
}

/* Whether the result row carries flag 0x80. */
u32 quest_flag_80_ck(QuestRecord* rec) {
    if (move_work_state_ck() == 1) {
        return 0;
    }
    if (rec == NULL) {
        rec = quest_record_get();
        if (rec == NULL) {
            return 0;
        }
    }
    if (rec->flags_0x310 & 0x80) {
        return 1;
    }
    return 0;
}

/* Whether the result row carries flag 0x2000000 or flag 0x80000000. */
u32 quest_flag_2000000_or_80000000_ck(QuestRecord* rec) {
    if (move_work_state_ck() == 1) {
        return 0;
    }
    if (rec == NULL) {
        rec = quest_record_get();
        if (rec == NULL) {
            return 0;
        }
    }
    if (rec->flags_0x310 & 0x2000000) {
        return 1;
    }
    return (rec->flags_0x310 & 0x80000000) != 0;
}

/* Whether the result row carries flag 0x2000000 (the single-bit twin of the composite test above). */
u32 quest_flag_2000000_only_ck(QuestRecord* rec) {
    if (move_work_state_ck() == 1) {
        return 0;
    }
    if (rec == NULL) {
        rec = quest_record_get();
        if (rec == NULL) {
            return 0;
        }
    }
    if (rec->flags_0x310 & 0x2000000) {
        return 1;
    }
    return 0;
}

/* Whether the result row carries flag 0x800. */
u32 quest_flag_800_ck(QuestRecord* rec) {
    if (move_work_state_ck() == 1) {
        return 0;
    }
    if (rec == NULL) {
        rec = quest_record_get();
        if (rec == NULL) {
            return 0;
        }
    }
    if (rec->flags_0x310 & 0x800) {
        return 1;
    }
    return 0;
}

/* The objective code of a result row (the current one when `rec` is NULL), from its flag word: 1, 9, 2,
 * 3 or 4, and 0 while the slot is in its entry state or no row exists. */
u8 quest_objective_get(QuestRecord* rec) {
    u8 objective = 0;
    if (move_work_state_ck() == 1) {
        return 0;
    }
    if (rec != NULL || (rec = quest_record_get()) != NULL) {
        u32 flags = rec->flags_0x310;
        if (flags & 2) {
            objective = 1;
        } else if (flags & 8) {
            objective = 9;
        } else {
            u32 pair = flags & 5;
            if (pair == 5) {
                objective = 2;
            } else {
                if (pair == 1) {
                    objective = 3;
                }
                if (pair == 4) {
                    objective = 4;
                }
            }
        }
    }
    return objective;
}

/* The same objective code with the 0x200000 state reported as 7 (the result screen's own reading). */
u8 quest_objective_result_get(QuestRecord* rec) {
    u8 objective = 0;
    if (move_work_state_ck() == 1) {
        return 0;
    }
    if (rec != NULL || (rec = quest_record_get()) != NULL) {
        u32 flags = rec->flags_0x310;
        if (flags & 8) {
            objective = 9;
        } else if (flags & 0x200000) {
            objective = 7;
        } else if (flags & 2) {
            objective = 1;
        } else {
            u32 pair = flags & 5;
            if (pair == 5) {
                objective = 2;
            } else {
                if (pair == 1) {
                    objective = 3;
                }
                if (pair == 4) {
                    objective = 4;
                }
            }
        }
    }
    return objective;
}

/* Whether the quest is over: the move work is in state 1, flag 0x4000000 is set, or the current row has its
 * +0x32C byte set. */
u32 quest_field32C_or_4000000_ck(void) {
    QuestRecord* rec;
    if (move_work_state_ck() == 1) {
        return 1;
    }
    rec = quest_record_get();
    if (rec == NULL) {
        return 0;
    }
    if (quest_flag_4000000_ck(0) == 1) {
        return 1;
    }
    return rec->field_0x32C != 0;
}

/* The leading word of entry `index` of the current row's third run (0 when there is no row). */
u32 quest_row394_get(u8 index) {
    QuestRecord* rec = quest_record_get();
    if (rec == NULL) {
        return 0;
    }
    return rec->rows_0x394[index].field_0x00;
}

/* Fills the result work's stat block from the item work when a finished quest's record is in state 7: cleared
 * when the item work carries a stat, else the em020 hit info. */
void quest_result_stat_fill(QuestWork* item) {
    Q_ResultWork* result = get_qResult_work();
    _ENEMY_WORK* work = NULL;
    if (item->record_0x03C->field_0x08B == 7) {
        em_get_unique_work(0, &work, NULL);
        if (work != NULL) {
            if (item->stat_gate_0x4B8 != 0) {
                memset(&result->stat_0x3E0, 0, 16);
            } else if (work->team == 20) {
                em020_hit_info_get(work, (Em020HitInfo*)&result->stat_0x3E0);
                result->stat_0x3E0.word_0x0C = item->stat_word_0x10;
            }
        }
    }
}

/* Steps the special elements (flag 0x2000) by kind: 1 ends the quest when the area-2 em020 reaches action 20,
 * 2 and 3 complete the element when their stage gate opens, 4 fails the quest once the score is gone. */
void quest_special_element_step(QuestWork* work) {
    Q_MoveWork* move;
    struct _ENEMY_WORK* enemy;
    struct _ENEMY_MINI_WORK* mini;
    QuestElement* el = work->elements_0x0094;
    s32 i = 0;

    for (; i < 3; i++, el++) {
        if ((el->flags & 0x2000) != 0x2000) {
            continue;
        }
        switch (el->id) {
        case 1:
            enemy = NULL;
            mini = NULL;
            em_get_unique_work(0, &enemy, &mini);
            if (enemy != NULL && em020_area2_action20_ck(enemy) == 1) {
                el->flags |= 0x40;
                move = (Q_MoveWork*)get_move_work_adrs(0);
                if (move != NULL) {
                    if (isServerSelectState() == 0) {
                        quest_result_enter((Q_ItemWork*)work, move, 3);
                    } else {
                        lb_entry_start_default((struct LbCompanionWork*)work, 3, work->field_0x024);
                    }
                }
            }
            break;
        case 2:
            if (stage_gate_a_ck() == 1) {
                quest_element_set(work, i, 0);
            }
            break;
        case 3:
            if (stage_gate_b_ck() == 1) {
                quest_element_set(work, i, 0);
            }
            break;
        case 4:
            if ((el->flags & 0x40) == 0 && work->field_0x5D8 == 0) {
                el->flags |= 0x40;
                move = (Q_MoveWork*)get_move_work_adrs(0);
                if (move != NULL) {
                    quest_work_ptr->entry_send_0x6A3A[2] = 5;
                    if (isServerSelectState() == 0) {
                        quest_result_enter((Q_ItemWork*)work, move, 2);
                    } else {
                        lb_entry_start_default((struct LbCompanionWork*)work, 2, work->field_0x024);
                    }
                }
            }
            break;
        }
    }
}

/* Under quest flag 0x200000: completes the first unfinished team element (flag 0x400) once its team's damage
 * drops under the mark, and raises the entry-send flag when the elements the grade asks for are picked. */
void quest_grade_goal_step(QuestWork* work) {
    QuestElement* el;
    s32 found;
    s32 i;
    s32 done;
    s32 picked;

    if (quest_flag_200000_ck(NULL) == 0) {
        return;
    }
    quest_time_elapsed_get();
    found = 0;
    el = work->elements_0x0094;
    for (i = 0; i < 3; i++, el++) {
        if ((el->flags & 0x10) != 0x10 && (el->flags & 0x400) == 0x400) {
            found = 1;
            break;
        }
    }
    if (found == 0) {
        return;
    }
    if (em_team_damage_under_ck((u8)el->id, (u8)el->target_count) == 1) {
        quest_element_set(work, i, 0x400);
    }
    done = 0;
    switch (work->grade_0x93) {
    case 1:
        if (quest_element_pick_ck(work, 0, 1) == 1 || quest_element_pick_ck(work, 1, 1) == 1 ||
            quest_element_pick_ck(work, 2, 1) == 1) {
            done = 1;
        } else {
            done = 0;
        }
        break;
    case 2:
        if (quest_element_pick_ck(work, 0, 1) == 1) {
            done = 1;
        } else {
            done = 0;
        }
        break;
    case 3:
        if (quest_element_pick_ck(work, 0, 1) == 1 && quest_element_pick_ck(work, 1, 1) == 1) {
            done = 1;
        } else {
            done = 0;
        }
        break;
    case 4:
        picked = 0;
        for (i = 0; i < 3; i++) {
            if (quest_element_live_ck(work, i) == 0) {
                picked++;
            } else if (quest_element_pick_ck(work, i, 1) == 1) {
                picked++;
            }
        }
        done = picked == 3;
        break;
    }
    if (done == 1) {
        work->entry_send_0x6A3A[0] = 1;
    }
}

/* Scans the three elements for the em020 hunt goal, fills the result stat when one matches, and arms the
 * element whose team window has closed; 1 when one was armed. */
u32 quest_element_window_check(QuestWork* work) {
    QuestElement* el;
    s32 i;
    s32 hit;
    get_qResult_work();
    hit = 0;
    el = work->elements_0x0094;
    for (i = 0; i < 3; i++, el++) {
        if ((el->flags & 0x10) && ((el->flags & 0x400) || (el->flags & 0x4000)) && (u8)el->id == 20) {
            hit = 1;
            quest_result_stat_fill(work);
        }
    }
    if (hit) {
        work->entry_send_0x6A3A[1] = 2;
    } else {
        work->entry_send_0x6A3A[1] = 1;
    }
    el = work->elements_0x0094;
    for (i = 0; i < 3; i++, el++) {
        if (el->flags & 0x10) {
            if ((el->flags & 0x4000) == 0x4000 && em_team_damage_over_ck((u8)el->id, (u8)el->target_count) == 1) {
                quest_element_set(work, i, 0x400);
                work->entry_send_0x6A3A[0] = 1;
                return 1;
            }
            if ((el->flags & 0x404) == 0x404 && em_team_damage_under_ck((u8)el->id, (u8)el->target_count) == 1) {
                quest_element_set(work, i, 0x400);
                work->entry_send_0x6A3A[0] = 1;
                return 1;
            }
        }
    }
    work->entry_send_0x6A3A[1] = 0;
    return 0;
}

/* The state (0x4014, 0x414 or 0x404) of the element whose low id byte is `key`; 0 when none matches. */
u32 quest_element_state_find(u8 key) {
    QuestWork* work = quest_work_ptr;
    u32 flags;
    QuestElement* el;
    s32 i;
    if (move_work_state_ck() == 1) {
        return 0;
    }
    if (work == NULL) {
        return 0;
    }
    el = work->elements_0x0094;
    for (i = 0; i < 3; i++, el++) {
        flags = el->flags;
        if ((flags & 0x4014) == 0x4014 && (u8)el->id == key) {
            return 0x4014;
        }
        if ((flags & 0x414) == 0x414 && (u8)el->id == key) {
            return 0x414;
        }
        if ((flags & 0x404) == 0x404 && (u8)el->id == key) {
            return 0x404;
        }
    }
    return 0;
}

/* Whether any of the three elements carries state 0x404. */
u32 quest_element_404_ck(void) {
    QuestWork* work = quest_work_ptr;
    QuestElement* el;
    s32 i;
    if (move_work_state_ck() == 1) {
        return 0;
    }
    if (work == NULL) {
        return 0;
    }
    el = work->elements_0x0094;
    for (i = 0; i < 3; i++, el++) {
        if ((el->flags & 0x404) == 0x404) {
            return 1;
        }
    }
    return 0;
}

/* Whether any element whose bit 0x10 is set is in its entry state (0x404 or 0x414): the gate the pair
 * rolls in `quest_pair_roll_all` wait on. */
u32 quest_element_state_ck(void) {
    QuestWork* work = quest_work_ptr;
    u32 flags;
    QuestElement* el;
    s32 i;
    if (move_work_state_ck() == 1) {
        return 0;
    }
    if (work == NULL) {
        return 0;
    }
    el = work->elements_0x0094;
    for (i = 0; i < 3; i++, el++) {
        flags = el->flags;
        if (flags & 0x10) {
            if ((flags & 0x404) == 0x404) {
                return 1;
            }
            if ((flags & 0x414) == 0x414) {
                return 1;
            }
        }
    }
    return 0;
}

/* The arena element `id`'s stored value into `out`; 0 when no element matches. */
s32 quest_element_value_get(u16 id, s16* out) {
    QuestElement* el = quest_work.elements_0x0094;
    s32 i;
    for (i = 0; i < 3; i++, el++) {
        if ((el->flags & 0x2000) == 0x2000 && el->id == id) {
            if (out != NULL) {
                *out = el->value;
            }
            return 1;
        }
    }
    return 0;
}

/* Element `index`'s stored value, ignoring its id, else -1. */
s32 quest_element_value_index_get(u16 index) {
    QuestElement* el;
    if (move_work_state_ck() == 1) {
        return -1;
    }
    el = &quest_work.elements_0x0094[index];
    if ((el->flags & 0xA) == 0xA) {
        return el->target_count;
    }
    return -1;
}

/* Element `index`'s stored value when `id` matches its own id, else -1. */
s32 quest_element_value_at(u16 id, u16 index) {
    QuestElement* el;
    if (move_work_state_ck() == 1) {
        return -1;
    }
    el = &quest_work.elements_0x0094[index];
    if ((el->flags & 0xA) == 0xA && el->id == id) {
        return el->target_count;
    }
    return -1;
}

/* Fills `out` with element `index`'s id and what is still missing of its target; returns the id. */
u16 em_work_slot_pair_get(u16 index, s16* out) {
    QuestElement* el;
    if (move_work_state_ck() == 1 && out != NULL) {
        out[0] = 0;
        out[1] = 0;
        return 0;
    }
    el = &quest_work.elements_0x0094[index];
    if ((el->flags & 0xA) == 0xA) {
        if (out != NULL) {
            s16 remaining;
            out[0] = el->id;
            remaining = el->target_count - quest_item_count_sum(el->id, 0);
            if (remaining < 0) {
                remaining = 0;
            }
            out[1] = remaining;
        }
        return el->id;
    }
    if (out != NULL) {
        out[0] = 0;
        out[1] = 0;
    }
    return 0;
}

/* Element `index`'s target minus the local player's held count, clamped up to 0. */
s32 quest_element_remaining_get(u16 index) {
    QuestElement* el;
    if (move_work_state_ck() == 1) {
        return 0;
    }
    el = &quest_work.elements_0x0094[index];
    if ((el->flags & 0xA) == 0xA) {
        s32 remaining = el->target_count - quest_item_count_sum(el->id, 0);
        if (remaining < 0) {
            remaining = 0;
        }
        return remaining;
    }
    return 0;
}

/* The item `id`'s count over every player the system work covers; 0 when no element carries the id. */
s32 quest_all_player_item_count_sum(u16 id) {
    s32 i;
    s32 total = 0;
    s32 player;
    QuestItemSlot* slots;
    for (i = 0; i < 3; i++) {
        QuestElement* el = &quest_work.elements_0x0094[i];
        if ((el->flags & 0xA) == 0xA && el->id == id) {
            break;
        }
    }
    if (i >= 3) {
        return 0;
    }
    slots = quest_work.player_items_0x6A6A[0];
    for (player = 0; player < (s8)system_w.field_0x28; player++, slots += 3) {
        if (slots[0].id == id) {
            total += slots[0].count;
        }
        if (slots[1].id == id) {
            total += slots[1].count;
        }
        if (slots[2].id == id) {
            total += slots[2].count;
        }
    }
    return total;
}

/* Whether arena element `id` carrying `value` still has that item recorded; clears the element when the
 * record is the single-use kind. */
u32 quest_arena_value_clear_ck(u16 id, u16 value) {
    s32 i;
    u32 result = 0;
    QuestElement* el;
    if (move_work_state_ck() == 1) {
        return 0;
    }
    el = quest_work.elements_0x0094;
    for (i = 0; i < 3; i++, el++) {
        if ((el->flags & 0x200) == 0x200 && el->id == id && el->target_count == value) {
            if (quest_work.part_count_run_0x6803[el->id * 8 + value] == 1) {
                quest_element_set(&quest_work, i, 0x200);
            }
            if (quest_work.part_count_run_0x6803[el->id * 8 + value] >= 1) {
                result = 1;
            }
        }
    }
    return result;
}

/* Clears the first element that has state 0x1000 for `key` while its value's bit is set in the key's
 * mask; 1 when one was cleared. */
u32 quest_arena_key_clear(u8 key) {
    QuestElement* el;
    s32 i;
    if (move_work_state_ck() == 1) {
        return 0;
    }
    el = quest_work.elements_0x0094;
    for (i = 0; i < 3; i++, el++) {
        if ((el->flags & 0x1000) == 0x1000 && el->id == key &&
            ((1 << el->target_count) & quest_work.key_bits_0x694C[el->id])) {
            quest_element_set(&quest_work, i, 0x1000);
            return 1;
        }
    }
    return 0;
}

/* Runs the per-frame countdown of the armed arena elements matching `kind`: flags the finished ones, posts
 * the remaining-count message every fifth frame and stores the first finished index in `first`. */
u32 quest_element_progress_step(u16 kind, u8 mode, s8* first) {
    u32 armed;
    s32 i;
    s32 shown;
    u32 result = 0;
    QuestElement* el;
    char text[0x20];
    *first = -1;
    if (move_work_state_ck() == 1) {
        return 0;
    }
    armed = quest_flag_80000_ck(0);
    if (armed == 0) {
        armed = quest_flag_2000000_ck(0);
    }
    el = quest_work.elements_0x0094;
    shown = 0;
    for (i = 0; i < 3; i++, el++) {
        if (el->flags & 0x60) {
            continue;
        }
        if ((el->flags & 9) != 9) {
            continue;
        }
        if (el->id != kind && enemy_kind_same_ck((u8)el->id, (u8)kind) == 0) {
            continue;
        }
        if ((el->flags & 0x80) && mode != 1) {
            el->flags |= 0x40;
            *first = i;
            return 0;
        }
        if ((el->flags & 0x100) && mode == 1) {
            el->flags |= 0x40;
            *first = i;
            return 0;
        }
        if (el->target_count != 0) {
            el->target_count--;
        }
        if (el->target_count == 0) {
            quest_element_set(&quest_work, i, 0x100);
            if (*first < 0) {
                *first = i;
            }
            result = 1;
        }
        if (shown == 0) {
            if (el->flags & 0x8000) {
                s32 remaining = el->target_count;
                if (remaining != 0 && remaining % 5 == 0) {
                    sprintf(text, quest_str_tbl_35_get(37), remaining);
                    hud_msg_push(0, text);
                    shown = 1;
                }
            }
            if (el->flags & 0x10000) {
                s32 remaining = el->target_count;
                if (remaining != 0 && remaining % 5 == 0) {
                    sprintf(text, quest_str_tbl_35_get(38), remaining);
                    hud_msg_push(0, text);
                    shown = 1;
                }
            }
            if (el->flags & 0x20000) {
                u32 k;
                s32 total = 0;
                u32 count = (quest_flag_100_ck(0) == 1) ? 2 : 3;
                for (k = 0; k < count; k++) {
                    s32 need = quest_arena_need_get(k);
                    if (need >= 0) {
                        total += need;
                    }
                }
                if (total != 0) {
                    sprintf(text, quest_str_tbl_35_get(37), total);
                    hud_msg_push(0, text);
                    shown = 1;
                }
            }
        }
        if (armed == 1) {
            break;
        }
    }
    return result;
}

/* How many of arena element `index`'s item the player still needs (-1 when the element is not live). */
s32 quest_arena_need_get(s32 index) {
    QuestElement* el;
    u32 flags;
    if (move_work_state_ck() == 1) {
        return -1;
    }
    el = &quest_work.elements_0x0094[index];
    flags = el->flags;
    if (flags != 0) {
        if (flags & 0x200) {
            u16 count = el->target_count;
            if (count != 0) {
                s32 held = quest_work.part_count_run_0x6803[el->id * 8 + count];
                if (held > 1) {
                    held = 1;
                }
                return 1 - held;
            }
        }
        if ((flags & 0x1000) || (flags & 0x2000)) {
            if (flags & 0x20) {
                return 0;
            }
            return 1;
        }
        if (flags & 2) {
            return quest_element_remaining_get(index);
        }
        return el->target_count;
    }
    return -1;
}

/* How many of arena element `index`'s item the player still holds (-1 when the element is not live). */
s32 quest_arena_count_get(s32 index) {
    QuestElement* el;
    u32 flags;
    if (move_work_state_ck() == 1) {
        return -1;
    }
    el = &quest_work.arena_elements_0x01B4[index];
    flags = el->flags;
    if (flags != 0) {
        if (flags & 0x200) {
            return 1;
        }
        if ((flags & 2) || (flags & 1)) {
            return el->target_count;
        }
        if (flags & 0x10) {
            return -1;
        }
        return 1;
    }
    return -1;
}

/* The work block's quest stat (NULL while the block is absent or the move work is in state 1). */
Q_QuestStat* quest_stat_get(void) {
    QuestWork* work = quest_work_ptr;
    if (move_work_state_ck() == 1) {
        return NULL;
    }
    if (work == NULL) {
        return NULL;
    }
    return &work->stat_0x6AA4;
}

/* Takes `points` off the quest score (clamped at 0) and plays the penalty sound at the area's fixed spot,
 * on enemy `unique_id`'s sound work. */
void quest_score_deduct(u16 points, u16 unique_id) {
    nw4r::math::VEC3 pos;
    nw4r::math::VEC3 spot_a;
    nw4r::math::VEC3 spot_b;
    struct _ENEMY_WORK* enemy;
    struct _ENEMY_MINI_WORK* mini;
    s32 score;

    if (move_work_state_ck() == 1) {
        return;
    }
    score = quest_work.field_0x5D8 - points;
    if (score < 0) {
        score = 0;
    }
    quest_work.field_0x5D8 = score;
    VEC3_ctor(&pos);
    enemy = NULL;
    mini = NULL;
    if ((u8)em_get_unique_work(unique_id, &enemy, &mini) != 0 && enemy != NULL) {
        switch (get_now_areano()) {
        case 1:
            copyVec3(&pos, setVec3(&spot_a, 0.0f, 0.0f, 0.0f));
            break;
        case 2:
            copyVec3(&pos, setVec3(&spot_b, 9950.0f, 0.0f, 0.0f));
            break;
        default:
            return;
        }
        se_req_pos_ps(enemy->se_0xB14, 0x44, 2, &pos);
    }
}

#pragma fp_contract off
/* Steps the quest's supply drops: kind 1 delivers on the time-rank schedule (each step's minute mark, a 1-in-128
 * roll), kind 2 once five minutes before the end; each delivery adds the row's supply pairs (or sends them to the
 * lobby on the server-select screen) and ends the drops at the stage's limit. */
void quest_supply_drop_step(void) {
    QuestWork* work = &quest_work;
    u8* step;
    u8 kind;
    u16 id;

    if (work == NULL) {
        return;
    }
    if (work->supply_kind_0x8B == 0) {
        return;
    }
    step = NULL;
    switch (work->supply_kind_0x8B) {
    case 1:
        if (quest_slot_progress_get(NULL) != 1) {
            return;
        }
        step = quest_supply_drop_tbl[work->time_rank_0x59];
        break;
    case 3:
        return;
    }
    switch (work->supply_kind_0x8B) {
    case 1:
        step += work->supply_step_0x58 * 2;
        if (step[0] == 0) {
            return;
        }
        if (step[0] * 1800 < work->field_0x024) {
            return;
        }
        if (isServerSelectState() == 1 && isReadyCountOne() == 0) {
            return;
        }
        work->supply_step_0x58++;
        if ((ran_suu(0) & 0x7F) >= step[1]) {
            return;
        }
        work->supply_count_0x8F++;
        kind = work->supply_kind_0x8B;
        if (work->supply_count_0x8F == work->supply_limit_0x8E) {
            work->supply_kind_0x8B = 0;
        }
        id = quest_field372_get(NULL);
        if (isServerSelectState() == 0) {
            quest_item_pair_copy_row(work->supply_0x5E2, id, work->supply_count_0x8F, kind);
            hud_msg_push(1, quest_str_tbl_35_get(20));
            return;
        }
        lb_sub17_send(id, work->supply_count_0x8F, work->supply_kind_0x8B, kind);
        break;
    case 2:
        if (work->supply_count_0x8F > 0) {
            return;
        }
        if ((f32)work->field_0x01C - 5.0f * (frames_per_second_60f * Screen_w.frame_scale) < (f32)work->field_0x024) {
            return;
        }
        if (isServerSelectState() == 1 && isReadyCountOne() == 0) {
            return;
        }
        work->supply_count_0x8F++;
        kind = work->supply_kind_0x8B;
        if (work->supply_count_0x8F == work->supply_limit_0x8E) {
            work->supply_kind_0x8B = 0;
        }
        id = quest_field372_get(NULL);
        if (isServerSelectState() == 0) {
            quest_item_pair_copy_row(work->supply_0x5E2, id, work->supply_count_0x8F, kind);
            hud_msg_push(1, quest_str_tbl_35_get(20));
            return;
        }
        lb_sub17_send(id, work->supply_count_0x8F, work->supply_kind_0x8B, kind);
        break;
    }
}

#pragma fp_contract reset
/* Picks the quest's bonus byte (`+0x30D`) by weight from the row's bonus list for its name index (the stage
 * hands the pick over when the row's +0x37A word is 1 and a lobby slot holds 0x1A); returns it. */
u8 quest_bonus_pick(void) {
    QuestWork* work;
    u8 name;
    u16 list_index;
    s32 i;
    u8* entry;
    s32 total;
    u8* scan;
    s32 sum;
    s32 roll;
    u8 bonus;

    if (move_work_state_ck() == 1) {
        return 0;
    }
    work = &quest_work;
    if (work == NULL) {
        return 0;
    }
    name = quest_name_index_get(NULL);
    list_index = quest_field37A_get(NULL);
    if (list_index == 1) {
        for (i = 0; i < 4; i++) {
            if (lb_param_w.slots_0x16[i] == 0x1A) {
                return stage_bonus_pick(name);
            }
        }
    }
    entry = quest_bonus_pick_tbl[name][list_index];
    total = 0;
    scan = entry;
    while (*scan != 0xFF) {
        total += *scan;
        scan += 2;
    }
    roll = ran_suu(0) % total;
    sum = 0;
    while (*entry != 0xFF) {
        sum += *entry++;
        if (roll < sum) {
            break;
        }
        entry++;
    }
    bonus = *entry;
    work->bonus_0x30D = bonus;
    return bonus;
}

/* Whether any of the work block's three key rows holds `key`. */
u32 quest_key_row_ck(u8 key) {
    Q_SlotPair* row;
    s32 i;
    if (&quest_work == NULL) {
        return 0;
    }
    row = quest_work.key_rows_0x0319;
    for (i = 0; i < 3; i++, row++) {
        if (row->id_0x00 == key) {
            return 1;
        }
    }
    return 0;
}

/* With key 0x14 present, whether flag 0x200000 is set (`invert` 0) or clear (`invert` non-zero). */
u32 quest_key20_flag_ck(u8 invert) {
    if (quest_key_row_ck(0x14) == 0) {
        return 0;
    }
    if (invert == 0) {
        return quest_flag_200000_ck(0) != 0;
    }
    return quest_flag_200000_ck(0) != 1;
}

/* One 6-byte row of the extra-condition table: the list's and the board's condition kinds and the range they
 * test.  size: 0x6 */
typedef struct QuestExCondition {
    /* +0x0 */ u8 show_kind_0x0;
    /* +0x1 */ u8 accept_kind_0x1;
    /* +0x2 */ u16 lo_0x2;
    /* +0x4 */ u16 hi_0x4;
} QuestExCondition;

/* The `.sbss` slot holding the extra-condition table (nothing in the game sets it, so both tests fail); the
 * second word is the slot's filler.  size: 0x8 */
typedef struct QuestExConditionSlot {
    /* +0x0 */ QuestExCondition* rows;
    /* +0x4 */ u32 unused_0x04;
} QuestExConditionSlot;

QuestExConditionSlot quest_ex_condition_tbl;

/* The record's extra condition for the quest list: the first kind of its `quest_ex_condition_tbl` row decides -
 * 0/4 pass, 1 tests `param` against the row's range, 2 tests `arg` against the range's low byte, 3 needs the
 * board's six counters at 0. */
u32 quest_ex_condition_show_ck(QuestRecord* rec, u16 param, u8 arg, LbQuestBoardData* data) {
    QuestExCondition* row;
    s16 i;
    if (quest_ex_condition_tbl.rows == NULL) {
        return 0;
    }
    if (rec == NULL) {
        return 0;
    }
    row = &quest_ex_condition_tbl.rows[rec->condition_0x198];
    switch (row->show_kind_0x0) {
    case 0:
    case 4:
        return 1;
    case 1:
        if (row->lo_0x2 <= param && row->hi_0x4 >= param) {
            return 1;
        }
        return 0;
    case 2:
        return (u8)row->lo_0x2 == arg;
    case 3:
        if (data != NULL) {
            for (i = 0; i < 6; i++) {
                if (data->rows_0x13C[i].count_0x6 >= 1) {
                    return 0;
                }
            }
            return 1;
        }
        return 0;
    default:
        return 0;
    }
}

/* The same test with the row's second kind, for the quest board: kind 2 tests `arg` against the high byte. */
u32 quest_ex_condition_ck(QuestRecord* rec, u16 param, u8 arg, LbQuestBoardData* data) {
    QuestExCondition* row;
    s16 i;
    if (quest_ex_condition_tbl.rows == NULL) {
        return 0;
    }
    if (rec == NULL) {
        return 0;
    }
    row = &quest_ex_condition_tbl.rows[rec->condition_0x198];
    switch (row->accept_kind_0x1) {
    case 0:
    case 4:
        return 1;
    case 1:
        if (row->lo_0x2 <= param && row->hi_0x4 >= param) {
            return 1;
        }
        return 0;
    case 2:
        return (u8)row->hi_0x4 == arg;
    case 3:
        if (data != NULL) {
            for (i = 0; i < 6; i++) {
                if (data->rows_0x13C[i].count_0x6 >= 1) {
                    return 0;
                }
            }
            return 1;
        }
        return 0;
    default:
        return 0;
    }
}

/* Finds the staged file whose header carries `version` and steps its download; -1 when none does. */
s32 stepStagingDownloadForVersion(u16 version) {
    NetCtrlWk* wk = net_ctrl_wk;
    s32 file;
    s32 i;
    if (wk != NULL) {
        for (i = 0; i < 10; i++) {
            if (((StagingFileHeader*)wk->file_buffers_0xC39C[i])->version_0x2C == version) {
                file = i;
                break;
            }
        }
    }
    if (i >= 10) {
        return -1;
    }
    return NetCtrlWk::stepStagingDownloadIfUp(file, version);
}

/* Whether `buffer`'s staged file header carries `version`. */
u32 matchesFileVersion(const u8* buffer, u16 version) {
    return ((const StagingFileHeader*)buffer)->version_0x2C == version;
}

/* The default placement (thirteen entries, a zero kind ends it), the per-monster kill-reward rows and the
 * reward row each monster kind uses (NULL for none). */
EmSetDefault em_set_default_tbl[13] = {
    {12, 1, {0x10, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x42CA851F, 0x40966666, 0x448128F6, 0x00000000, 0x000074CD, 0x00000000}, {0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
    {12, 1, {0x10, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x42CA851F, 0x40966666, 0x448128F6, 0x00000000, 0x000074CD, 0x00000000}, {0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
    {3, 1, {0xFF, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000}, {0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
    {3, 1, {0xFF, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000}, {0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
    {16, 1, {0xFF, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000}, {0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
    {16, 1, {0xFF, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000}, {0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
    {1, 1, {0xFF, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000}, {0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
    {1, 1, {0xFF, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000}, {0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
    {15, 1, {0xFF, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC5561000, 0xC53B8000, 0x4544E000, 0x00000000, 0x00002500, 0x00000000}, {0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
    {15, 1, {0xFF, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x44BB8000, 0xC4BB8000, 0xC53B8000, 0x00000000, 0x00000000, 0x00000000}, {0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
    {2, 1, {0xFF, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000}, {0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
    {12, 1, {0x10, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x42CA851F, 0x40966666, 0x448128F6, 0x00000000, 0x000074CD, 0x00000000}, {0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
    {0, 0, {0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000}, {0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
};
EmSetReward em_set_reward_rows[21][2] = {
    {{{10, 30, 35, 25}, {0x0, 0xFE, 0x100, 0xF2}}, {{5, 35, 30, 30}, {0x0, 0xFE, 0x100, 0xF2}}},
    {{{10, 40, 25, 25}, {0x0, 0xFE, 0x100, 0xC1}}, {{5, 40, 25, 30}, {0x0, 0xFE, 0x100, 0xC1}}},
    {{{15, 25, 40, 20}, {0x0, 0xEF, 0xBF, 0xFD}}, {{10, 30, 35, 25}, {0x0, 0xEF, 0xBF, 0xFD}}},
    {{{15, 40, 20, 25}, {0x0, 0xEF, 0xF9, 0xDC}}, {{10, 40, 20, 30}, {0x0, 0xEF, 0xF9, 0xDC}}},
    {{{15, 25, 35, 25}, {0x0, 0x100, 0xF1, 0xFF}}, {{10, 25, 35, 30}, {0x0, 0x100, 0xF1, 0xFF}}},
    {{{10, 40, 25, 25}, {0x0, 0xF9, 0xE4, 0xF1}}, {{5, 35, 30, 30}, {0x0, 0xF9, 0xE4, 0xF1}}},
    {{{25, 60, 15, 0}, {0x0, 0xFC, 0xC0, 0x0}}, {{0, 0, 0, 0}, {0x0, 0x0, 0x0, 0x0}}},
    {{{55, 43, 2, 0}, {0xFC, 0xC0, 0xF9, 0x0}}, {{0, 0, 0, 0}, {0x0, 0x0, 0x0, 0x0}}},
    {{{15, 50, 35, 0}, {0x0, 0xFB, 0xED, 0x0}}, {{0, 0, 0, 0}, {0x0, 0x0, 0x0, 0x0}}},
    {{{20, 55, 25, 0}, {0x0, 0x1D, 0xFB, 0x0}}, {{0, 0, 0, 0}, {0x0, 0x0, 0x0, 0x0}}},
    {{{45, 40, 15, 0}, {0x0, 0xEA, 0xE8, 0x0}}, {{0, 0, 0, 0}, {0x0, 0x0, 0x0, 0x0}}},
    {{{15, 30, 55, 0}, {0x0, 0x1D, 0xC5, 0x0}}, {{0, 95, 5, 0}, {0x0, 0xF7, 0xDC, 0x0}}},
    {{{20, 35, 45, 0}, {0x0, 0xCF, 0xD0, 0x0}}, {{0, 0, 0, 0}, {0x0, 0x0, 0x0, 0x0}}},
    {{{20, 45, 35, 0}, {0x0, 0xCF, 0xD0, 0x0}}, {{0, 0, 0, 0}, {0x0, 0x0, 0x0, 0x0}}},
    {{{25, 45, 30, 0}, {0x0, 0xEA, 0xDB, 0x0}}, {{0, 0, 0, 0}, {0x0, 0x0, 0x0, 0x0}}},
    {{{30, 40, 30, 0}, {0x0, 0xE0, 0xEA, 0x0}}, {{0, 0, 0, 0}, {0x0, 0x0, 0x0, 0x0}}},
    {{{25, 60, 15, 0}, {0x0, 0xE0, 0xF4, 0x0}}, {{0, 0, 100, 0}, {0x0, 0x0, 0xF8, 0x0}}},
    {{{15, 15, 70, 0}, {0x0, 0x0, 0xF4, 0x0}}, {{0, 95, 5, 0}, {0x0, 0xE8, 0xFA, 0x0}}},
    {{{15, 15, 70, 0}, {0x0, 0x0, 0xF4, 0x0}}, {{0, 95, 5, 0}, {0x0, 0xE8, 0xFA, 0x0}}},
    {{{15, 15, 70, 0}, {0x0, 0x0, 0xF4, 0x0}}, {{0, 95, 5, 0}, {0x0, 0xE8, 0xFA, 0x0}}},
    {{{15, 15, 70, 0}, {0x0, 0x0, 0xF4, 0x0}}, {{0, 95, 5, 0}, {0x0, 0xE8, 0xFA, 0x0}}},
};
EmSetReward* em_set_reward_tbl[41] = {
    NULL, em_set_reward_rows[0], em_set_reward_rows[1], em_set_reward_rows[2],
    NULL, NULL, NULL, NULL,
    NULL, NULL, em_set_reward_rows[6], em_set_reward_rows[7],
    em_set_reward_rows[3], NULL, NULL, em_set_reward_rows[4],
    em_set_reward_rows[5], em_set_reward_rows[8], NULL, NULL,
    NULL, NULL, NULL, em_set_reward_rows[9],
    NULL, NULL, em_set_reward_rows[10], em_set_reward_rows[11],
    NULL, NULL, em_set_reward_rows[12], em_set_reward_rows[13],
    em_set_reward_rows[14], em_set_reward_rows[15], em_set_reward_rows[16], NULL,
    em_set_reward_rows[17], em_set_reward_rows[18], em_set_reward_rows[19], em_set_reward_rows[20],
    NULL,
};

/* The free-hunt work (built by the unit's static constructor) and the slot the module reads it through. */
EmSetWork em_set_work;
EmSetWorkSlot em_set_work_ptr = {&em_set_work, 0};

/* The stage dcm archive buffers, by stage. */
static inline u8** em_set_dcm_buffers(void) {
    return (u8**)stage_dcm_buffer_tbl;
}

/* Copies a save record's monster, parameters and stat rows into a placement entry. */
void em_set_entry_rec_apply(EmSetEntry* entry, EmSetSaveRec* rec) {
    entry->monster_0x31 = rec->monster_0x0;
    entry->param_0x32[0] = rec->param_0x1[0];
    entry->param_0x32[1] = rec->param_0x1[1];
    entry->param_0x32[2] = rec->param_0x1[2];
    entry->act_set_0x35 = rec->act_set_0x4;
    entry->act_sub_0x36 = rec->act_sub_0x6;
    entry->stat_a_0x38 = rec->stat_a_0x8;
    entry->stat_b_0x3A = rec->stat_b_0xA;
}

/* Builds the free-hunt work: clears it, copies the save block in, seeds the entries from the default table,
 * allocates the 32 set-file buffers, applies the saved live monsters, and takes the lobby's rotation, point rows
 * and options. */
void em_set_work_init(void) {
    Q_MoveWork* work;
    EmSetDefault* def;
    EmSetEntry* entry;
    EmSetSave* save;
    LbParamWork* param;
    s32 i;
    s32 count;
    s32 ofs;

    work = (Q_MoveWork*)get_move_work_adrs(0);
    if (work != NULL) {
        memset(&em_set_work, 0, sizeof(EmSetWork));
        memcpy(&em_set_work_ptr.work->userdata_0x04E4, get_userdata(), sizeof(Q_UserData));
        entry = em_set_work_ptr.work->entries_0x005C;
        def = em_set_default_tbl;
        i = 0;
        while (def->kind_0x00 != 0) {
            entry->kind_0x00 = def->kind_0x00;
            entry->count_0x02 = def->count_0x04;
            em_ground_rec_set(&entry->ground_0x08.rec, &def->place_0x08);
            i++;
            def++;
            entry++;
            if (i >= 13) {
                break;
            }
        }
        for (i = 0; i < 32; i++) {
            em_set_work_ptr.work->area_buffers_0x6544[i] = (u8*)work_mem_alloc(0x800);
            if (em_set_work_ptr.work->area_buffers_0x6544[i] != NULL) {
                memset(em_set_work_ptr.work->area_buffers_0x6544[i], 0, 0x800);
            }
        }
        em_set_work_ptr.work->active_0x000C[0] = -1;
        em_set_work_ptr.work->active_0x000C[1] = -1;
        em_set_work_ptr.work->active_0x000C[2] = -1;
        save = &em_set_work_ptr.work->userdata_0x04E4.em_set_0x3F98;
        if (save->active_0xD0[0] == 0 && save->active_0xD0[1] == 0) {
            em_set_work_ptr.work->active_0x000C[0] = -1;
            em_set_work_ptr.work->active_0x000C[1] = -1;
            em_set_work_ptr.work->active_0x000C[2] = -1;
        } else {
            for (i = 0; i < 2; i++) {
                if (save->active_0xD0[i] != 0) {
                    em_set_work_ptr.work->active_0x000C[i] = save->active_0xD0[i] - 1;
                    em_set_entry_rec_apply(&em_set_work_ptr.work->entries_0x005C[em_set_work_ptr.work->active_0x000C[i]],
                                           &save->recs_0x00[save->active_0xD0[i]]);
                }
            }
        }
        for (i = 0; i < 16; i++) {
            em_set_work_ptr.work->area_sets_0x001C[i] = -1;
        }
        em_set_work_ptr.work->season_0x000B = em_set_work_ptr.work->userdata_0x04E4.em_set_0x3F98.season_0xD2;
        param = &lb_param_w;
        work->state_0x22D4 = param->entry_0x08;
        work->bgm_0x22D5 = param->sub_0x29;
        work->bgm_0x22D6 = param->sub_0x2A;
        count = 0;
        ofs = 0;
        if (param->rotation_0x1E[0][0] != 0) {
            em_set_work_ptr.work->rotation_bytes_0x65C4[ofs] = param->rotation_0x1E[0][0];
            em_set_work_ptr.work->rotation_bytes_0x65C4[ofs + 1] = param->rotation_0x1E[0][1];
            ofs += 2;
            count++;
        }
        if (param->rotation_0x1E[1][0] != 0) {
            em_set_work_ptr.work->rotation_bytes_0x65C4[ofs] = param->rotation_0x1E[1][0];
            em_set_work_ptr.work->rotation_bytes_0x65C4[ofs + 1] = param->rotation_0x1E[1][1];
            ofs += 2;
            count++;
        }
        if (param->rotation_0x1E[2][0] != 0) {
            em_set_work_ptr.work->rotation_bytes_0x65C4[ofs] = param->rotation_0x1E[2][0];
            em_set_work_ptr.work->rotation_bytes_0x65C4[ofs + 1] = param->rotation_0x1E[2][1];
            ofs += 2;
            count++;
        }
        if (param->rotation_0x1E[3][0] != 0) {
            em_set_work_ptr.work->rotation_bytes_0x65C4[ofs] = param->rotation_0x1E[3][0];
            em_set_work_ptr.work->rotation_bytes_0x65C4[ofs + 1] = param->rotation_0x1E[3][1];
            ofs += 2;
            count++;
        }
        for (i = count; i < 4; i++) {
            em_set_work_ptr.work->rotation_0x65C4[i][0] = 0;
            em_set_work_ptr.work->rotation_0x65C4[i][1] = 0;
        }
        em_set_work_ptr.work->rotation_state_0x65CC = 0;
        em_set_work_ptr.work->rotation_index_0x65CD = 0;
        em_set_work_ptr.work->rotation_ptr_0x65D0 = em_set_work_ptr.work->rotation_0x65C4;
        quest_snd_wk_init();
        work->flag_0x22DC = lb_param_w.hunt_option_0x0A;
        work->rank_sel_0x22D8 = lb_param_w.rank_sel_0x09;
        em_set_work_ptr.work->userdata_flag_0x6644 = em_set_work_ptr.work->userdata_0x04E4.em_set_0x3F98.flag_0xD4;
    }
}

/* Loads the monster of every live entry and counts them. */
void em_set_live_count(void) {
    s32 i;
    em_set_work_ptr.work->live_count_0x000F = 0;
    for (i = 0; i < 3; i++) {
        if (em_set_work_ptr.work->active_0x000C[i] != -1 &&
            em_set_work_ptr.work->entries_0x005C[em_set_work_ptr.work->active_0x000C[i]].monster_0x31 != 0) {
            em_kind_release(em_set_work_ptr.work->entries_0x005C[em_set_work_ptr.work->active_0x000C[i]].monster_0x31);
            em_set_work_ptr.work->live_count_0x000F++;
        }
    }
}

/* Takes the area set rows (the lobby quest's, or the save's season row) and requests every area's set file of
 * map `map`. */
void em_set_area_files_load(Q_MoveWork* work, s32 map) {
    char path[0x100];
    s16* row;
    s32 i;
    u8 count;

    count = stage_map_area_count_get(map);
    if (quest_select_ready_ck() == 1) {
        row = em_set_quest_area_tbl[lb_param_w.field_0x00];
    } else {
        row = em_set_season_area_tbl[em_set_work_ptr.work->season_0x000B];
    }
    em_set_work_ptr.work->area_sets_0x001C[0] = row[0];
    em_set_work_ptr.work->area_sets_0x001C[1] = row[1];
    em_set_work_ptr.work->area_sets_0x001C[2] = row[2];
    em_set_work_ptr.work->area_sets_0x001C[3] = row[3];
    em_set_work_ptr.work->area_sets_0x001C[4] = row[4];
    em_set_work_ptr.work->area_sets_0x001C[5] = row[5];
    em_set_work_ptr.work->area_sets_0x001C[6] = row[6];
    em_set_work_ptr.work->area_sets_0x001C[7] = row[7];
    em_set_work_ptr.work->area_sets_0x001C[8] = row[8];
    em_set_work_ptr.work->area_sets_0x001C[9] = row[9];
    em_set_work_ptr.work->area_sets_0x001C[10] = row[10];
    em_set_work_ptr.work->area_sets_0x001C[11] = row[11];
    em_set_work_ptr.work->area_sets_0x001C[12] = row[12];
    for (i = 0; i < count; i++) {
        if (em_set_work_ptr.work->area_sets_0x001C[i] != -1) {
            sprintf(path, "05/em_set/em_set_m%02da%02d_%03d.esd", map, i, em_set_work_ptr.work->area_sets_0x001C[i]);
            load_file_req(path, (u32)em_set_work_ptr.work->area_buffers_0x6544[i], 0x800, 0, 0, NULL);
        }
    }
}

/* Spawns the live entries' monsters into the move work's spawn records (stops at the first that fails). */
void em_set_entries_spawn(Q_MoveWork* work) {
    QuestSpawnRec* rec;
    struct _ENEMY_WORK* enemy;
    s32 i;
    s32 spawned;
    EmSetEntry* entry;

    spawned = 0;
    i = 0;
    rec = work->spawn_0x2274;
    for (; i < 3; rec++, i++) {
        if (em_set_work_ptr.work->active_0x000C[i] != -1) {
            entry = &em_set_work_ptr.work->entries_0x005C[em_set_work_ptr.work->active_0x000C[i]];
            enemy = em_set_entry_spawn(entry, i);
            if (enemy == NULL) {
                return;
            }
            entry->enemy_0x28 = enemy;
            entry->flags_0x30 = (u8)spawned | 0x80;
            entry->live_0x2C = 1;
            em_quest_element_set(enemy, spawned);
            rec->order_0x0 = i;
            rec->count_0x4 = entry->count_0x02;
            rec->monster_0x6 = entry->monster_0x31;
            spawned++;
        }
    }
}

/* Steps the lobby's monster rotation: waits each slot's delay with no monster live, loads the slot's monster,
 * spawns it once its files are in and records its spawn. */
void em_set_rotation_step(Q_MoveWork* work) {
    struct _ENEMY_WORK* enemy;
    struct _ENEMY_MINI_WORK* mini;
    EmSetEntry* entry;
    s32 order;
    u8 kind;

    switch (em_set_work_ptr.work->rotation_state_0x65CC) {
    case 0:
        if (em_set_work_ptr.work->rotation_index_0x65CD < 4 && em_set_work_ptr.work->live_count_0x000F <= 0) {
            kind = em_set_work_ptr.work->rotation_0x65C4[em_set_work_ptr.work->rotation_index_0x65CD][0];
            if (kind != 0 && kind <= 13) {
                kind--;
                em_set_work_ptr.work->rotation_timer_0x65D4++;
                if (em_set_work_ptr.work->rotation_timer_0x65D4 >=
                    (s32)(60.0f * Screen_w.frame_scale *
                          lb_param_w.rotation_0x1E[em_set_work_ptr.work->rotation_index_0x65CD][1])) {
                    entry = &em_set_work_ptr.work->entries_0x005C[kind];
                    if (entry->kind_0x00 != 0 && em_kind_release_ck(entry->kind_0x00) != 0) {
                        em_set_work_ptr.work->active_0x000C[0] = -1;
                        em_set_work_ptr.work->active_0x000C[1] = -1;
                        em_set_work_ptr.work->active_0x000C[2] = -1;
                        em_set_work_ptr.work->rotation_state_0x65CC = 1;
                        em_set_work_ptr.work->active_0x000C[0] = kind;
                        em_set_entry_rec_apply(entry, &em_set_work_ptr.work->userdata_0x04E4.em_set_0x3F98.recs_0x00[kind + 1]);
                        em_kind_release(entry->monster_0x31);
                        em_set_work_ptr.work->live_count_0x000F++;
                    }
                }
            }
        }
        break;
    case 1:
        if (file_loading_ck(NULL, NULL) != 1) {
            em_set_work_ptr.work->rotation_state_0x65CC = 2;
        }
        break;
    case 2:
        em_set_boss_spawn(&em_set_work_ptr.work->entries_0x005C[em_set_work_ptr.work->active_0x000C[0]],
                          em_set_work_ptr.work->rotation_index_0x65CD + 2);
        em_set_work_ptr.work->rotation_state_0x65CC = 3;
        break;
    case 3:
        enemy = NULL;
        mini = NULL;
        if ((u8)em_get_unique_work(em_set_work_ptr.work->rotation_index_0x65CD + 2, &enemy, &mini) != 0) {
            entry = &em_set_work_ptr.work->entries_0x005C[em_set_work_ptr.work->active_0x000C[0]];
            entry->enemy_0x28 = enemy;
            order = em_set_work_ptr.work->rotation_index_0x65CD + 2;
            work->spawn_0x2274[order].order_0x0 = order;
            work->spawn_0x2274[order].count_0x4 = 1;
            work->spawn_0x2274[order].monster_0x6 = entry->monster_0x31;
            em_set_work_ptr.work->rotation_index_0x65CD++;
            em_set_work_ptr.work->rotation_state_0x65CC = 0;
        }
        break;
    }
}

/* Starts the free hunt: builds the work, clears the quest work, sets the players, enemies and stage up for the
 * hunt's map and requests its files. */
void em_set_start(void) {
    Q_MoveWork* work = (Q_MoveWork*)get_move_work_adrs(0);
    em_set_work_init();
    memset(&quest_work, 0, sizeof(QuestWork));
    init_player_work();
    em_prog_work_init();
    em_prog_slots_init();
    stage_map_set(work->quest_id_0x120);
    eft_common_load();
    if (quest_select_ready_ck() == 1) {
        work->em_level_0x22DB = 0;
        em_level_set(0);
    }
    em_set_live_count();
    em_set_area_files_load(work, 1);
    player_init_data_load();
    ainpc_res_load();
    em_pop_res_load(work->quest_id_0x120);
    draw_shape_stage_load(work->quest_id_0x120);
}

/* Sets the field up for the free hunt: resets the enemy, effect, stage, camera, cockpit and item works, picks the
 * map by the rank byte, restores the saved result, loads the lobby quest's dcm archives, and sets the time limit
 * and the point rows. */
void em_set_field_setup(Q_MoveWork* work) {
    char path[0x20];
    EmSetPointRow* dst;
    EmSetPointRow* src;
    s32 limit;
    s32 extra;
    s32 i;
    s32 stage;

    enemy_work_reset();
    if (work->npc_0xEF != 0) {
        ainpc_init();
    }
    eft_control_init();
    shell_work_init();
    light_init();
    stage_work_init();
    camera_work_init();
    cockpit_quest_init();
    menu_item_work_init();
    if (work->rank_sel_0x22D8 == 0) {
        em_set_work_ptr.work->map_0x0008 = 1;
    } else {
        em_set_work_ptr.work->map_0x0008 = 12;
    }
    em_set_work_ptr.work->area_0x0009 = 1;
    em_set_work_ptr.work->warp_stage_0x000A = 0;
    em_set_result_restore();
    if (quest_select_ready_ck() == 1 && quest_item_id_low_ck(lb_param_w.field_0x00) == 1) {
        lb_quest_work_init(lb_param_w.field_0x00);
        snd_quest_bgm_load();
        work->wait_0x114 = 60;
        stage = lb_param_w.field_0x00;
        if (stage != 0) {
            if (stage_dcm_file_table[stage].size != 0) {
                stage_dcm_path_get(path, stage);
                em_set_dcm_buffers()[stage] = (u8*)work_mem_alloc(stage_dcm_file_table[stage].size);
                load_file(path, (u32)em_set_dcm_buffers()[stage], stage_dcm_file_table[stage].size);
            }
            extra = -1;
            switch (lb_param_w.field_0x00) {
            case 1:
                extra = 14;
                break;
            case 2:
                extra = 19;
                break;
            case 4:
                extra = 20;
                break;
            }
            if (extra != -1 && stage_dcm_file_table[extra].size != 0) {
                stage_dcm_path_get(path, extra);
                em_set_dcm_buffers()[extra] = (u8*)work_mem_alloc(stage_dcm_file_table[extra].size);
                load_file(path, (u32)em_set_dcm_buffers()[extra], stage_dcm_file_table[extra].size);
            }
        }
    }
    work->map_0xED = work->phase_0xE9 = em_set_work_ptr.work->map_0x0008;
    work->area_0xEE = work->sub_0xEA = em_set_work_ptr.work->area_0x0009;
    work->sub_0xFA = 2;
    work->item_flag_0x110 = 0;
    work->result_buffer_0x150 = (u8*)work_mem_alloc(0x4800);
    limit = 50.0f * (60.0f * Screen_w.frame_scale);
    quest_work.field_0x01C = limit;
    quest_work.timer_0x6A58 = 29970.0;
    quest_time_limit_set(limit);
    dst = em_set_work_ptr.work->points_0x65D8;
    src = lb_param_w.points_0x32;
    for (i = 0; i < 17; i++, dst++, src++) {
        dst->kind_0x0 = src->kind_0x0;
        dst->kill_points_0x2 = src->kill_points_0x2;
        dst->capture_points_0x4 = src->capture_points_0x4;
    }
    dst->kind_0x0 = 0;
    dst->kill_points_0x2 = 0;
    dst->capture_points_0x4 = 0;
}

/* Steps the item work's real-time clock: the frame's elapsed time over the clock rate (at least one tick, none
 * in mode 0) comes off the time left, which the frame limit follows while it runs. */
void quest_clock_real_step(s32 mode) {
    Q_ItemWork* work;
    f64 step;

    work = move_work_item_work_get();
    if (work != NULL) {
        work->timer_0x6A50 = work->timer_0x6A48;
        work->timer_0x6A48 = OSGetTime();
        if (work->timer_0x6A50 > 0.0) {
            step = 8000.0 * (work->timer_0x6A48 - work->timer_0x6A50) / (OS_BUS_CLOCK / 4 / 125000) / 1000.0 /
                   work->timer_rate_0x6A58;
            if (step < 1.0) {
                step = 1.0;
            }
            if (mode == 0) {
                step = 0.0;
            }
            work->time_left_0x6A60 -= step;
        } else if (mode == 1) {
            work->time_left_0x6A60 -= 1.0;
        }
        if (work->time_limit_0x24 > 0) {
            work->time_limit_0x24 = work->time_left_0x6A60;
            if (work->time_limit_0x24 < 0) {
                work->time_limit_0x24 = 0;
            }
        }
    }
}

/* Once the player may move: loads the map's resources, enters the area, loads the stage and player sound banks
 * and the stage BGM, and hands the area set buffers to the area spawn.  0 while the player cannot move yet. */
s32 em_set_move_start(Q_MoveWork* work) {
    u8 state;

    if (player_move_start_ck() == 0) {
        return 0;
    }
    player_move_start(-1);
    stage_map_res_load(work->phase_0xE9);
    stage_map_obj_load(work->phase_0xE9);
    stage_area_load(0, work->phase_0xE9, work->sub_0xEA);
    em_set_work_ptr.work->area_count_0x0018 = stage_map_area_count_get(work->phase_0xE9);
    snd_stage_bank_load(work->phase_0xE9, work->sub_0xEA, work->rank_sel_0x22D8);
    snd_player_banks_load();
    state = work->state_0x22D4;
    if (state & 0x80) {
        snd_quest_bgm_set(state & 0x7F, work->bgm_0x22D5, work->bgm_0x22D6);
    }
    quest_area_spawn_setup(work, (u32*)em_set_work_ptr.work->area_buffers_0x6544, em_set_work_ptr.work->area_count_0x0018, 0);
    quest_area_spawn_apply(work, work->phase_0xE9, work->sub_0xEA);
    quest_area_res_load(work->phase_0xE9, work->sub_0xEA);
    return 1;
}

/* Once the files are in: enters the stage area, places the player at its entry, spawns the live entries and
 * the area's enemies and turns the glare filter on.  0 while files are still loading. */
s32 em_set_area_enter(Q_MoveWork* work) {
    u16 angle;

    if (file_loading_ck(NULL, NULL) == 1) {
        return 0;
    }
    stage_area_enter(work->phase_0xE9, work->sub_0xEA);
    copyVec3(&work->entry_pos_0x128, stage_entry_pos_get(work->phase_0xE9, work->sub_0xEA));
    angle = stage_entry_angle_get(work->phase_0xE9, work->sub_0xEA);
    work->entry_angle_0x134 = angle;
    pl_area_entry_set(&work->entry_pos_0x128, angle, work->phase_0xE9, work->sub_0xEA);
    camera_area_reset();
    em_set_entries_spawn(work);
    quest_screen_enemy_start(work);
    em_pop_work_step();
    work->area_slots_0xE0[work->area_slot_index_0xE8] = (s8)stage_area_slot_get(work->phase_0xE9, work->sub_0xEA);
    GlareFilter_on();
    filter_panel_on();
    return 1;
}

/* Warps the local player to the start of stage 1 (the hub): the stage's start position and facing go through
 * `pl_warp_start` with the work's warp stage. */
void quest_warp_hub(void) {
    nw4r::math::VEC3 pos;
    s32 angle;

    setVec3(&pos, 0.0f, 0.0f, 0.0f);
    stage_start_get(1, &pos, &angle);
    pl_act_stage_latch_set(my_player_work_get(), 2);
    pl_warp_start(em_set_work_ptr.work->warp_stage_0x000A, &pos, angle);
}

/* Steps the free hunt each frame: counts the hunt time (up to 25 and 99 minutes), runs the real-time clock, then
 * either requests the lobby event once or runs the gallery cell check, the area sound frame and the rotation. */
void em_set_frame_step(Q_MoveWork* work) {
    struct _PLW* players;
    StageCell cell;

    if ((f32)em_set_work_ptr.work->frames_0x0010 < 25.0f * (60.0f * Screen_w.frame_scale)) {
        em_set_work_ptr.work->frames_0x0010++;
    }
    if ((f32)em_set_work_ptr.work->frames_long_0x0014 < 99.0f * (60.0f * Screen_w.frame_scale)) {
        em_set_work_ptr.work->frames_long_0x0014++;
    }
    quest_clock_real_step(1);
    switch (em_set_work_ptr.work->step_0x0004) {
    case 0:
        if (quest_select_ready_ck() == 1) {
            lb_event_request(0);
        }
        em_set_work_ptr.work->step_0x0004++;
        break;
    case 1:
        if (quest_select_ready_ck() == 1) {
            quest_gallery_cell_step(work);
            if (work->local_0x112 == 0) {
                snd_quest_frame_begin(0, work->phase_0xE9, work->sub_0xEA);
                snd_quest_frame_end();
            }
        } else {
            if (snd_bgm_hold_ck() == 0 && work->sub_0xEA == 1) {
                players = (struct _PLW*)get_move_work_adrs(2);
                if (players != NULL) {
                    cell.word = stage_cell_get(players[(s8)my_player_no()].area_cell_0x5AB, 1);
                    if (cell.bytes[0] == 0xFA && cell.bytes[1] == 0xFF && cell.bytes[2] == 0xFF) {
                        snd_bgm_hold_set();
                        gallery_open(14);
                    }
                }
            }
            if (work->local_0x112 == 0) {
                snd_quest_frame_begin(work->rank_sel_0x22D8, work->phase_0xE9, work->sub_0xEA);
                snd_quest_frame_end();
            }
            em_set_rotation_step(work);
        }
        break;
    }
}

/* The entry sub-state of the local slot's move work: 1 when it is 4, 0 otherwise, -1 without a work. */
s32 quest_move_sub_state_4_get(void) {
    Q_MoveWork* work = (Q_MoveWork*)get_move_work_adrs(0);
    if (work == NULL) {
        return -1;
    }
    return work->sub_0xFA == 4;
}

/* Sets the local slot's sub-state to 4 (and its 0x22D9 flag when `flag` is 1). */
void quest_move_sub_state_4_set(u8 flag) {
    Q_MoveWork* work = (Q_MoveWork*)get_move_work_adrs(0);
    if (work != NULL) {
        work->sub_0xFA = 4;
        if (flag == 1) {
            work->sub_flag_0x22D9 = 1;
        }
    }
}

/* Adds one of `item` to the hunt's reward box: 0 when added, 1 when its stack is already at 99, 2 when the box
 * is full. */
s32 em_set_reward_item_add(u16 item) {
    EmSetRewardItem* slot;
    EmSetRewardItem* scan;
    s32 i;

    slot = em_set_work_ptr.work->rewards_0x0460;
    scan = slot;
    for (i = 0; i < 30; i++, scan++) {
        if (scan->item_0x0 == item) {
            if (scan->count_0x2 < 99) {
                scan->count_0x2++;
                return 0;
            }
            return 1;
        }
    }
    for (i = 0; i < 30; i++, slot++) {
        if (slot->item_0x0 == 0) {
            slot->item_0x0 = item;
            slot->count_0x2++;
            return 0;
        }
    }
    return 2;
}

/* Records a kill of `enemy` (a mini-work record when `mini` is set): drops its spawn count, tallies the kill or
 * capture (capped at 99) in the work and the save block, adds the kill's points, logs it on the cockpit and
 * rolls the monster's reward row once per kill (twice for a large kill, four times for a capture). */
void em_set_kill_record(struct _ENEMY_WORK* enemy, u8 mini) {
    s32 tries;
    struct _ENEMY_WORK* large_enemy;
    s32 size;
    s32 large;
    u16 order;
    u8 monster;
    u32 hit;
    u8 end_kind;
    QuestSpawnRec* rec;
    EmSetReward* reward;
    u16 count;
    u8 log_kind;
    s32 capped;
    EmSetPointRow* row;
    s32 i;
    s32 roll;
    s32 pick;
    s32 sum;

    tries = 0;
    large_enemy = NULL;
    get_userdata();
    size = 0;
    large = 0;
    if (mini == 0) {
        large_enemy = enemy;
        order = enemy->field_0x01A;
        monster = enemy->team;
        hit = em_record_hit_ck(enemy);
        end_kind = em_captured_ck(enemy);
        if (enemy->field_0x1C8 & 1) {
            large = 1;
            size = 100.0f * get_em_chg_scale(enemy);
        }
    } else {
        order = ((struct _ENEMY_MINI_WORK*)enemy)->order_0x16;
        monster = ((struct _ENEMY_MINI_WORK*)enemy)->monster_0x02;
        hit = em_mini_hit_ck(enemy);
        end_kind = em_mini_kill_kind_get(enemy);
    }
    rec = quest_spawn_rec_find(order);
    if (rec != NULL && rec->count_0x4 > 0) {
        rec->count_0x4--;
    }
    reward = em_set_reward_tbl[monster];
    log_kind = 0;
    count = 0;
    if (large != 0) {
        em_set_work_ptr.work->live_count_0x000F--;
        if (em_set_work_ptr.work->live_count_0x000F == 0) {
            em_set_work_ptr.work->rotation_timer_0x65D4 = 0;
        }
        if (end_kind == 0) {
            em_set_work_ptr.work->hunted_0x0368[monster]++;
            tries = 2;
            count = em_set_work_ptr.work->hunted_0x0368[monster];
            log_kind = 1;
            if (count > 99) {
                em_set_work_ptr.work->hunted_0x0368[monster] = 99;
            }
            userdata_hunt_count_add(monster, 1);
        }
        if (end_kind == 1) {
            em_set_work_ptr.work->captured_0x03BA[monster]++;
            if (reward != NULL) {
                reward++;
            }
            tries = 4;
            count = em_set_work_ptr.work->captured_0x03BA[monster];
            log_kind = 2;
            if (count > 99) {
                em_set_work_ptr.work->captured_0x03BA[monster] = 99;
            }
            userdata_capture_count_add(monster, 1);
        }
        userdata_size_record_set(monster, size);
        if (large_enemy != NULL) {
            em_set_enemy_detach(large_enemy);
        }
        snd_hunt_stream_start(0);
    } else {
        if (end_kind == 2) {
            return;
        }
        em_set_work_ptr.work->small_count_0x045E++;
        if (em_set_work_ptr.work->small_count_0x045E > 99) {
            em_set_work_ptr.work->small_count_0x045E = 99;
        }
        if (hit == 1 && reward != NULL) {
            reward++;
        }
        em_set_work_ptr.work->hunted_0x0368[monster]++;
        tries = 1;
        count = em_set_work_ptr.work->hunted_0x0368[monster];
        log_kind = 0;
        if (count > 99) {
            em_set_work_ptr.work->hunted_0x0368[monster] = 99;
        }
        userdata_hunt_count_add(monster, 1);
    }
    if (count > 99) {
        count = 99;
        capped = 1;
    } else {
        capped = 0;
    }
    if (quest_move_flag_ck() == 0) {
        return;
    }
    if (capped == 0) {
        row = em_set_work_ptr.work->points_0x65D8;
        while ((u8)row->kind_0x0 != 0) {
            if ((u8)row->kind_0x0 == monster) {
                if (large != 0) {
                    if (end_kind == 0) {
                        em_set_work_ptr.work->points_0x04D8 += row->kill_points_0x2;
                    }
                    if (end_kind == 1) {
                        em_set_work_ptr.work->points_0x04D8 += row->capture_points_0x4;
                    }
                } else {
                    em_set_work_ptr.work->points_0x04D8 += row->kill_points_0x2;
                }
                break;
            }
            row++;
        }
        if (em_set_work_ptr.work->points_0x04D8 > 999999) {
            em_set_work_ptr.work->points_0x04D8 = 999999;
        }
    }
    cockpit_hunt_log_push(monster, count, log_kind);
    if (reward == NULL) {
        return;
    }
    if (capped == 1) {
        return;
    }
    for (i = 0; i < tries; i++) {
        roll = (u16)ran_suu(0) % 100;
        pick = 0;
        sum = reward->rate_0x0[0];
        if (sum <= roll) {
            pick = 1;
            sum += reward->rate_0x0[1];
            if (sum <= roll) {
                pick = 2;
                sum += reward->rate_0x0[2];
                if (sum <= roll) {
                    pick = 3;
                    sum += reward->rate_0x0[3];
                    if (sum <= roll) {
                        pick = 4;
                    }
                }
            }
        }
        if (pick < 4 && reward->item_0x4[pick] != 0) {
            em_set_reward_item_add(reward->item_0x4[pick]);
        }
    }
}

/* The hunt's points (the cockpit's score). */
s32 em_set_work_state_get(void) {
    return em_set_work_ptr.work->points_0x04D8;
}

/* Whether the save block's free-hunt flag is set. */
s32 em_set_userdata_flag_ck(void) {
    return em_set_work_ptr.work->userdata_flag_0x6644 != 0;
}

/* Settles the carried-item pouch: sells the category-1 pairs (ten each), clears the category-2 pairs (raising
 * `flag`) and drops the ones the player cannot use; returns the credit. */
s32 quest_pouch_items_settle(Q_ItemPair* pouch, u8* flag) {
    s32 i;
    s32 credit = 0;
    *flag = 0;
    for (i = 0; i < 35; i++, pouch++) {
        if (pouch->id != 0 && (s16)pouch->num != 0) {
            if (item_category_ck(pouch->id, 1)) {
                credit += (s16)pouch->num * 10;
                pouch->id = 0;
                pouch->num = 0;
            } else if (item_category_ck(pouch->id, 2)) {
                pouch->id = 0;
                pouch->num = 0;
                *flag = 1;
            } else if (Pl_item_id_usable_ck(pouch->id, 1) == 0) {
                pouch->id = 0;
                pouch->num = 0;
            }
        }
    }
    return credit;
}

/* Stores the free hunt's result: the counts, the reward box and the points, the settled pouch (announcing the
 * sold and dropped items), the live monsters, the hunt time and the result ranks.  `end_kind` is how the hunt
 * ended. */
void em_set_result_store(s8 end_kind) {
    char text[0x80];
    FqResultWork* result;
    struct _PLW* me;
    s32 credit;
    u8 cleared;
    s32 i;

    result = get_FqResult_work();
    me = &((struct _PLW*)get_move_work_adrs(2))[my_player_no()];
    memcpy(result->hunted_0x000, em_set_work_ptr.work->hunted_0x0368, 0x52);
    memcpy(result->captured_0x052, em_set_work_ptr.work->captured_0x03BA, 0x52);
    memcpy(result->counts_c_0x0A4, em_set_work_ptr.work->counts_c_0x040C, 0x52);
    result->small_count_0x0F6 = em_set_work_ptr.work->small_count_0x045E;
    memcpy(result->rewards_0x0F8, em_set_work_ptr.work->rewards_0x0460, 0x78);
    credit = quest_pouch_items_settle((struct Q_ItemPair*)me->slot_id, &cleared);
    if (cleared == 1) {
        sprintf(text, quest_str_tbl_35_get(0x15));
        hud_msg_push(0, text);
    }
    if (credit > 0) {
        userdata_zenny_add(credit);
        sprintf(text, quest_str_tbl_35_get(0x1D), credit);
        hud_msg_push(0, text);
    }
    memcpy(result->pouch_0x1B4, me->slot_id, 0x60);
    memcpy(result->spare_0x214, me->spare_slot_id, 0x20);
    result->points_0x170 = 0;
    for (i = 0; i < 3; i++) {
        if (em_set_work_ptr.work->active_0x000C[i] == -1) {
            result->active_0x174[i] = 0;
        } else {
            result->active_0x174[i] = em_set_work_ptr.work->active_0x000C[i] + 1;
            if (em_set_work_ptr.work->entries_0x005C[em_set_work_ptr.work->active_0x000C[i]].enemy_0x28 != NULL) {
                result->monsters_0x178[i].area_0x0 =
                    em_set_work_ptr.work->entries_0x005C[em_set_work_ptr.work->active_0x000C[i]].enemy_0x28->area_no;
            }
        }
    }
    result->frames_0x1A8 = em_set_work_ptr.work->frames_0x0010;
    result->seconds_0x1AC = (f32)em_set_work_ptr.work->frames_0x0010 / (60.0f * Screen_w.frame_scale);
    result->frames_long_0x1B0 = em_set_work_ptr.work->frames_long_0x0014;
    result->end_kind_0x240 = end_kind;
    system_w.field_0x7ce = end_kind;
    result->hunt_rank_0x242 = result_hunt_rank_get(&em_set_work_ptr.work->hunted_set_0x0368, &em_set_work_ptr.work->captured_set_0x03BA);
    result->time_rank_0x244 = result_time_rank_get(result->seconds_0x1AC);
    result_rank_rows_fill(result->rank_rows_0x246);
    result->rank_points_0x278 = result_rank_points_get(&em_set_work_ptr.work->hunted_set_0x0368, &em_set_work_ptr.work->captured_set_0x03BA);
}

/* Restores the counts, the reward box and the points from the stored free-hunt result. */
void em_set_result_restore(void) {
    FqResultWork* result = get_FqResult_work();
    memcpy(em_set_work_ptr.work->hunted_0x0368, result->hunted_0x000, 0x52);
    memcpy(em_set_work_ptr.work->captured_0x03BA, result->captured_0x052, 0x52);
    memcpy(em_set_work_ptr.work->counts_c_0x040C, result->counts_c_0x0A4, 0x52);
    em_set_work_ptr.work->small_count_0x045E = result->small_count_0x0F6;
    memcpy(em_set_work_ptr.work->rewards_0x0460, result->rewards_0x0F8, 0x78);
    em_set_work_ptr.work->points_0x04D8 = result->points_0x170;
}

/* Releases `enemy`'s hold flag on whichever live entry spawned it. */
void em_set_enemy_detach(struct _ENEMY_WORK* enemy) {
    s32 i;
    for (i = 0; i < 3; i++) {
        if (em_set_work_ptr.work->active_0x000C[i] != -1 &&
            em_set_work_ptr.work->entries_0x005C[em_set_work_ptr.work->active_0x000C[i]].enemy_0x28 == enemy) {
            em_set_work_ptr.work->entries_0x005C[em_set_work_ptr.work->active_0x000C[i]].flags_0x30 &= 0x7F;
        }
    }
}

} /* extern "C" */

/* Builds the work's thirteen entries. */
EmSetWork::EmSetWork() {}

/* Builds an entry's ground record. */
EmSetEntry::EmSetEntry() {}
