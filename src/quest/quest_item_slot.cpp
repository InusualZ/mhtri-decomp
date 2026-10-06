/* quest/quest_item_slot.cpp - the head of the quest entry band: record counters, the item-pair and item-slot
 * helpers, the small quest state probes and the static initialiser of the quest zone tables.
 *
 * `.text` 0x803AA4A4..0x803AB3BC, `extab` 0x80018A6C..0x80018B0C, `extabindex` 0x80038E08..0x80038EF8,
 * `.ctors` 0x8056F3B0..0x8056F3B4, `.data` 0x805F2940..0x805F2A38, `.sdata` 0x807935D0..0x807935D8 and
 * `.sdata2` 0x8079C4C0..0x8079C50C.  The cut at 0x803AB3BC is the `.data` emission-order seam: the static
 * initialiser `quest_zone_tbl_init` (0x803AB1F0, the `.ctors` word's target) is the unit's last function and
 * every `.data` object it fills is address-taken nowhere else.  The rest of the band is `quest/quest_entry.cpp`.
 *
 * NAME - GUESS.  No `__FILE__` string; `quest_item_slot` is taken from the unit's largest family (the five-slot
 * item list).  Every symbol without a dump name is a GUESS derived from its body and callers (rule 7).
 *
 * The zone tables: five `.data` objects of one `u8` head and two or three (position, reach, flag) triples whose
 * positions the initialiser writes (GUESS: arena/zone spots; the heads and the reach/flag bytes are the `.data`
 * image's own).  The initialiser is an ordinary function here, so this object carries no `.ctors` word (its
 * `.ctors` claim is the target's); `assignVec3` is the `out = in` vector copy at 0x80051490, called
 * through `ef.h`'s `Vec*` declaration (the `enemy` units declare the same address with `VEC3*`: an open type split).
 *
 * RESIDUAL.
 *   * `quest_item_pair_copy_cell`/`_block`/`_row` 89-94 %: callee-saved colouring (target `rec`=r29, `dst`/
 *     `flagged`=r30, `kind`=r31; ours a cyclic shift), plus `quest_item_slot_find`'s mask/load scheduling.
 *   * `quest_item_slot_add` 92.50 %: target's `extsh` temp, unsigned `cmplwi r3,5` and preserved loop counter.
 *     Kept: the `Q_ItemCount* slot` declaration inside the branch, `Q_ItemCount::num` as `s8`, the moving pointer.
 *   * `.ctors`: the object carries no `.ctors` word (the initialiser is an ordinary function), so the claim's
 *     four bytes are the target's; `.data` and `.sdata2` match byte for byte.
 *
 * SHAPES: the zone objects are defined after `quest_zone_tbl_init` and only declared before it - defined first,
 * MWCC addresses them through one section anchor (`lis`/`addi` once, `r30` base) where the target loads each
 * object's own address; `quest_item_slots_use` is one `switch` with `default:` first and an inner `switch
 * (result)` over 0-2 / 3; `quest_id_head_ck`/`_tail_ck` compare `(u16)(id + 0xFFFF)` against 2 / 3.
 */

#include "quest/quest_item_slot.h"
#include "quest/quest_entry.h"
#include "ef/fn_800CDB2C.h"    /* `move_work_state_ck` - owned by ef/system_core.cpp (rule 2) */
#include "unsplit/unknown.h"    /* `system_w` - the system block no registered unit claims */
#include "mh3_pad/lb_param_w.h"  /* `lb_param_w` - owned by src/mh3_pad.cpp (rule 2) */
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
#include "nw4r/math.h"
#include "ef.h"                /* assignVec3 (0x80051490, no registered owner), `Vec` */

/* The roll `quest_item_slots_use` indexes by the low three bits of a random draw: 0 takes the slots, 1 a
 * fixed item, 2 nothing (`.sdata` 0x807935D0..0x807935D8). */
s8 quest_item_use_roll_tbl[8] = { 0, 0, 0, 0, 0, 0, 1, 2 };

/* The recorded count of `kind` in the first record set, plus the live item work's copy of it.  The index
 * spelling 0x24..0x27 sums the set's four group totals instead of reading a count. */
s32 quest_record_a_count_get(u32 kind) {
    Q_UserData* user = get_userdata();
    s32 recorded;

    if ((u8)(kind + 0xDC) <= 3) {
        s32 sum = user->set_a.total[0] + user->set_a.total[1];
        sum += user->set_a.total[2];
        sum += user->set_a.total[3];
        recorded = sum;
    } else {
        recorded = user->set_a.count[(u8)kind];
    }
    if (move_work_state_ck() == 1) {
        return recorded;
    }
    Q_ItemWork* work = move_work_item_work_get();
    if (work == NULL) {
        return recorded;
    }
    s32 live;
    if ((u8)(kind + 0xDC) <= 3) {
        s32 sum = work->set_c.total[0] + work->set_c.total[1];
        sum += work->set_c.total[2];
        sum += work->set_c.total[3];
        live = sum;
    } else {
        live = work->set_c.count[(u8)kind];
    }
    return (u16)(recorded + live);
}

/* The same accessor for a caller that passes the index wider than a byte. */
s32 quest_record_a_count_get_wide(s32 kind) {
    return quest_record_a_count_get((u8)kind);
}

/* The recorded count of `kind` in the second record set, plus the live item work's copy of it. */
s32 quest_record_b_count_get(u32 kind) {
    Q_UserData* user = get_userdata();
    s32 recorded;

    if ((u8)(kind + 0xDC) <= 3) {
        s32 sum = user->set_b.total[0] + user->set_b.total[1];
        sum += user->set_b.total[2];
        sum += user->set_b.total[3];
        recorded = sum;
    } else {
        recorded = user->set_b.count[(u8)kind];
    }
    if (move_work_state_ck() == 1) {
        return recorded;
    }
    Q_ItemWork* work = move_work_item_work_get();
    if (work == NULL) {
        return recorded;
    }
    s32 live;
    if ((u8)(kind + 0xDC) <= 3) {
        s32 sum = work->set_d.total[0] + work->set_d.total[1];
        sum += work->set_d.total[2];
        sum += work->set_d.total[3];
        live = sum;
    } else {
        live = work->set_d.count[(u8)kind];
    }
    return (u16)(recorded + live);
}

/* The same accessor for a caller that passes the index wider than a byte. */
s32 quest_record_b_count_get_wide(s32 kind) {
    return quest_record_b_count_get((u8)kind);
}

/* Zeroes the first slot, then copies the item record's leading slice into it: 0x10 slots while the
 * caller's `kind` is 2 or the slot's own progress flag is set, 0x18 otherwise. */
void quest_item_pair_copy_block(Q_ItemPair* dst, u16 id, u8 kind) {
    dst[0].id = 0;
    dst[0].num = 0;
    if (id != 0xFFFF) {
        Q_ItemPair* rec;
        s32 flagged = 0;

        if (id >= 0x64) {
            rec = q_item_pair_tbl_high[id - 0x64];
        } else {
            rec = q_item_pair_tbl_low[id];
            if (quest_slot_progress_get(0) == 1) {
                flagged = 1;
            }
        }
        if (kind == 2) {
            flagged = 1;
        }
        if (rec != 0) {
            s32 n = 0x10;
            if (flagged == 0) {
                n = 0x18;
            }
            for (s32 i = 0; i < n; i++) {
                item_pair_copy(dst, rec);
                dst++;
                rec++;
            }
        }
    }
}

/* Copies one 8-slot row of an item record into the caller's buffer, when the row's `kind` admits it. */
void quest_item_pair_copy_row(Q_ItemPair* dst, u16 id, s8 row, u8 kind) {
    if (row < 1) {
        return;
    }
    if (id == 0xFFFF) {
        return;
    }
    if (kind == 1) {
        if (quest_slot_progress_get(0) != 1) {
            return;
        }
    } else if (kind != 2) {
        return;
    }
    if (kind != 2 && id >= 0x64) {
        return;
    }
    Q_ItemPair* rec;
    if (id >= 0x64) {
        rec = q_item_pair_tbl_high[id - 0x64];
    } else {
        rec = q_item_pair_tbl_low[id];
    }
    if (rec == 0) {
        return;
    }
    rec += (row - 1) * 8 + 0x10;
    dst += (row - 1) * 8 + 0x10;
    for (s32 i = 0; i < 8; i++) {
        item_pair_copy(dst, rec);
        dst++;
        rec++;
    }
}

/* Copies one 4-slot cell of an item record into the caller's buffer at the cell's own offset. */
void quest_item_pair_copy_cell(Q_ItemPair* dst, u16 id, s8 col) {
    if (id == 0xFFFF) {
        return;
    }
    Q_ItemPair* rec;
    if (id >= 0x64) {
        rec = q_item_pair_tbl_high[id - 0x64];
    } else {
        rec = q_item_pair_tbl_low[id];
    }
    if (rec == 0) {
        return;
    }
    dst += col * 4 + 0x20;
    rec += col * 4 + 0x20;
    for (s32 i = 0; i < 4; i++) {
        item_pair_copy(dst, rec);
        dst++;
        rec++;
    }
}

/* The count of the five-slot list's entry for `id`, or 0 when the list has none. */
s16 quest_item_slot_find(Q_ItemCount* slots, u16 id) {
    if (slots[0].id == id) {
        return (s8)slots[0].num;
    }
    if (slots[1].id == id) {
        return (s8)slots[1].num;
    }
    if (slots[2].id == id) {
        return (s8)slots[2].num;
    }
    if (slots[3].id == id) {
        return (s8)slots[3].num;
    }
    if (slots[4].id == id) {
        return (s8)slots[4].num;
    }
    return 0;
}

/* Adds `count` of `id` to the five-slot list.  An id already in the list is merged into its slot and
 * clamped to the item record's own cap; a new id takes the first free slot, or the round-robin slot
 * the caller's rotation byte points at. */
s32 quest_item_slot_add(Q_ItemCount* slots, u8* rot, u16 id, s16 count) {
    ItemDataRecord* rec = GetItemData(id);
    s32 result;
    s32 i;

    if (rec == NULL) {
        return 1;
    }
    result = quest_item_slot_find(slots, id);
    if (result == 0) {
        result = 5;
        Q_ItemCount* slot = slots;

        for (i = 0; i < 5; i++, slot++) {
            if (slot->id == 0 && count > 0) {
                slot->id = id;
                slot->num = (s8)count;
                result = 0;
                break;
            }
        }
        if (result == 5) {
            slot = &slots[(s8)*rot];
            slot->id = id;
            slot->num = (s8)count;
            *rot = *rot + 1;
            if ((s8)*rot >= 5) {
                *rot = 0;
            }
        }
    } else {
        Q_ItemCount* slot = slots;

        for (i = 0; i < 5; i++, slot++) {
            if (slot->id == id) {
                s8 cap = (s8)rec->max_num_0x003;

                if (count > 0 && (s8)slot->num >= cap) {
                    slot->num = cap;
                    result = 3;
                    break;
                }
                slot->num = slot->num + (s8)count;
                if ((s8)slot->num <= 0) {
                    slot->id = 0;
                    slot->num = 0;
                    result = 4;
                    break;
                }
                if ((s8)slot->num > cap) {
                    slot->num = cap;
                    result = 2;
                    break;
                }
                result = 1;
                break;
            }
        }
    }
    return result;
}

/* Records the item a caller hands over in the local slot's move work, then merges it into the item
 * work's own five-slot list with the negated value (the callers pass -1, so the merge adds one).
 * `owner` is the player work record `src/enemy/fn_801B0010.cpp` passes and the target never reads. */
void quest_item_work_merge(_PLW* owner, u16 id, s16 value) {
    Q_MoveWork* work = (Q_MoveWork*)get_move_work_adrs(0);
    Q_ItemWork* item;

    if (work == NULL) {
        return;
    }
    work->item_id_0x10C = id;
    work->item_value_0x10E = value;
    work->item_flag_0x110 = 1;
    item = work->item_work;
    if (item == NULL) {
        return;
    }
    quest_item_slot_add(item->slots_0x40, &item->rot_0x54, id, (s16)-value);
    item->flag_0x55 = 1;
}

/* Rolls what the carried item slots do at the end of a quest: on the common roll every filled slot is
 * handed to the player (announcing each one taken), on roll 1 a fixed item is, on roll 2 nothing is; then
 * the slots are cleared. */
void quest_item_slots_use(_PLW* plw) {
    Q_ItemWork* item;
    s32 i;
    s32 used;
    Q_ItemCount* slot;
    Q_MoveWork* work;
    s16 result;

    used = 0;
    work = (Q_MoveWork*)get_move_work_adrs(0);
    if (work == NULL) {
        return;
    }
    item = work->item_work;
    if (item == NULL) {
        return;
    }
    if ((s8)item->flag_0x55 != 0) {
        s8 roll = quest_item_use_roll_tbl[(u16)ran_suu(0) & 7];

        switch (roll) {
        default:
            slot = item->slots_0x40;
            for (i = 0; i < 5; i++, slot++) {
                if (slot->id != 0) {
                    result = pl_item_add(plw, slot->id, (s8)slot->num);
                    switch (result) {
                    case 0:
                    case 1:
                    case 2:
                        if (Pl_master_ck(plw) == 1) {
                            hud_item_msg_push(2, 8, slot->id);
                        }
                        used = 1;
                        break;
                    case 3:
                        if (Pl_master_ck(plw) == 1) {
                            hud_item_msg_push(2, 0x11, slot->id);
                        }
                        used = 1;
                        break;
                    }
                }
            }
            if (used == 0 && Pl_master_ck(plw) == 1) {
                hud_item_msg_push(1, 9, 0);
                snd_item_fail_play();
            }
            break;
        case 1:
            result = pl_item_add(plw, 0xC3, 1);
            switch (result) {
            case 0:
            case 1:
            case 2:
                if (Pl_master_ck(plw) == 1) {
                    hud_item_msg_push(2, 8, 0xC3);
                }
                break;
            case 3:
                if (Pl_master_ck(plw) == 1) {
                    hud_item_msg_push(2, 0x11, 0xC3);
                }
                break;
            default:
                if (Pl_master_ck(plw) == 1) {
                    hud_item_msg_push(1, 9, 0);
                    snd_item_fail_play();
                }
                break;
            }
            break;
        case 2:
            if (Pl_master_ck(plw) == 1) {
                hud_item_msg_push(1, 9, 0);
                snd_item_fail_play();
            }
            break;
        }
        item->flag_0x55 = 0;
        item->slots_0x40[0].id = 0;
        item->slots_0x40[0].num = 0;
        item->slots_0x40[1].id = 0;
        item->slots_0x40[1].num = 0;
        item->slots_0x40[2].id = 0;
        item->slots_0x40[2].num = 0;
        item->slots_0x40[3].id = 0;
        item->slots_0x40[3].num = 0;
        item->slots_0x40[4].id = 0;
        item->slots_0x40[4].num = 0;
        item->rot_0x54 = 0;
        item->flag_0x55 = 0;
        return;
    }
    if (Pl_master_ck(plw) == 1) {
        hud_item_msg_push(1, 9, 0);
        snd_item_fail_play();
    }
}

/* Whether the local slot's move work has its +0x22DC flag byte set. */
u32 quest_move_flag_ck(void) {
    Q_MoveWork* work = (Q_MoveWork*)get_move_work_adrs(0);

    if (work == NULL) {
        return 0;
    }
    return work->flag_0x22DC != 0;
}

/* Whether the local slot has a quest selected: the player's move work must be live and the option
 * block must carry both the selected flag and a nonzero quest id. */
u32 quest_select_ready_ck(void) {
    if (move_work_state_ck() != 1) {
        return 0;
    }
    if (lb_param_w.flag_0x0b == 1 && lb_param_w.field_0x00 != 0) {
        return 1;
    }
    return 0;
}

/* Whether an item id belongs to the low pair table (every id below 0x64); id 0 is the empty slot. */
u32 quest_item_id_low_ck(u16 id) {
    if (id == 0) {
        return 0;
    }
    return id < 100;
}

/* The current quest id when it is a low-table key (below 0x64), and 0 otherwise. */
u32 quest_id_low_get(void) {
    u32 id = quest_id_get();

    return quest_item_id_low_ck((u16)id) == 1 ? (u8)id : 0;
}

/* Whether the current quest id is one of the three at the head of the low list, and the local slot has
 * a quest selected at all (`(u16)(id + 0xFFFF) <= 2`, i.e. id is 1, 2 or 3). */
u8 quest_id_head_ck(void) {
    if (quest_select_ready_ck() == 0) {
        return 0;
    }
    return (u16)(quest_id_get() + 0xFFFF) <= 2;
}

/* The complementary probe for a slot whose move work is not yet in its quest state: the current quest
 * id is past the head of the low list (`(u16)(id + 0xFFFF) > 3`, i.e. id is 5 or above). */
u8 quest_id_tail_ck(void) {
    if (move_work_state_ck() == 1) {
        return 0;
    }
    return (u16)(quest_id_get() + 0xFFFF) > 3;
}

/* Whether the local slot's move work carries a state: bit 7 of the +0x22D4 byte (`rlwinm` MB=ME=24
 * selects 0x00000080).  The band's entry state machine and its neighbour read the same bit before
 * handing the low seven bits on, so it is the "there is a state code here" flag, not a mere nonzero test. */
u32 quest_move_state_valid_ck(void) {
    Q_MoveWork* work = (Q_MoveWork*)get_move_work_adrs(0);

    if (work == NULL) {
        return 0;
    }
    return (work->state_0x22D4 & 0x80) != 0;
}

/* The state code itself: the low seven bits of that same byte, or 0xFF when the slot has no move work
 * or no state. */
u32 quest_move_state_get(void) {
    Q_MoveWork* work = (Q_MoveWork*)get_move_work_adrs(0);
    u8 state;

    if (work == NULL) {
        return 0xFF;
    }
    state = work->state_0x22D4;
    if (state == 0) {
        return 0xFF;
    }
    return state & 0x7F;
}

/* Sends the player to the hub while the slot is in its entry state, else to the quest's start position. */
void quest_warp_by_state(_PLW* plw) {
    if (move_work_state_ck() == 1) {
        quest_warp_hub();
        return;
    }
    quest_start_warp(plw);
}

/* Announces a player's entry: to the quest scene sound while the slot is in its entry state, to the lobby
 * when the server is selecting, else as a faint-penalty charge. */
void quest_player_enter_notify(u8 player) {
    if (move_work_state_ck() == 1) {
        snd_quest_scene_set();
    } else if (isServerSelectState() == 1) {
        lb_entry_notify_send(0, player);
    } else {
        quest_reward_faint_penalty(player);
    }
}

/* The quest phase the whole game switches on while no entry is running - 1 unless the slot is in its
 * entry state, in which case the pause gate decides. */
u32 quest_play_state_ck(void) {
    if (move_work_state_ck() == 0) {
        return quest_work_busy_ck();
    }
    return 1;
}

/* Hands the local slot's item work to the pick gate for slot `idx`, or 0 when the slot has no move
 * work (or no item work) to hand over. */
u32 quest_item_work_notify(s32 idx) {
    Q_MoveWork* work = (Q_MoveWork*)get_move_work_adrs(0);

    if (work == NULL) {
        return 0;
    }
    if (work->item_work == NULL) {
        return 0;
    }
    return quest_element_pick_ck((QuestWork*)work->item_work, (u8)idx, 1);
}

/* The quest zone tables (GUESS names and field names): one object per zone group - a head byte and the
 * (position, reach, flag) triples `quest_zone_tbl_init` fills the positions of.  `Q_ZoneSet3` has three
 * triples, `Q_ZoneSet2` two (its last triple has no flag byte: the object ends at the reach). */
struct Q_ZoneSet3 {
    /* +0x00 */ u8 head_0x00;
    /* +0x04 */ Vec pos_0x04;
    /* +0x10 */ f32 reach_0x10;
    /* +0x14 */ u8 flag_0x14;
    /* +0x18 */ Vec pos_0x18;
    /* +0x24 */ f32 reach_0x24;
    /* +0x28 */ u8 flag_0x28;
    /* +0x2C */ Vec pos_0x2C;
    /* +0x38 */ f32 reach_0x38;
    /* +0x3C */ u8 flag_0x3C;
};  /* size: 0x40 */

struct Q_ZoneSet2 {
    /* +0x00 */ u8 head_0x00;
    /* +0x04 */ Vec pos_0x04;
    /* +0x10 */ f32 reach_0x10;
    /* +0x14 */ u8 flag_0x14;
    /* +0x18 */ Vec pos_0x18;
    /* +0x24 */ f32 reach_0x24;
};  /* size: 0x28 */

extern Q_ZoneSet3 quest_zone_a;
extern Q_ZoneSet2 quest_zone_b;
extern Q_ZoneSet3 quest_zone_c;
extern Q_ZoneSet2 quest_zone_d;
extern Q_ZoneSet2 quest_zone_e;

/* Fills the positions of the five zone tables (the `.ctors` entry of this band). */
void quest_zone_tbl_init(void) {
    nw4r::math::VEC3 t0;
    nw4r::math::VEC3 t1;
    nw4r::math::VEC3 t2;
    nw4r::math::VEC3 t3;
    nw4r::math::VEC3 t4;
    nw4r::math::VEC3 t5;
    nw4r::math::VEC3 t6;
    nw4r::math::VEC3 t7;
    nw4r::math::VEC3 t8;
    nw4r::math::VEC3 t9;
    nw4r::math::VEC3 t10;
    nw4r::math::VEC3 t11;

    assignVec3(&quest_zone_a.pos_0x04, (Vec*)setVec3(&t0, -2020.0f, -25.0f, -1724.0f));
    assignVec3(&quest_zone_a.pos_0x18, (Vec*)setVec3(&t1, 5858.5f, -12.0f, -324.0f));
    assignVec3(&quest_zone_a.pos_0x2C, (Vec*)setVec3(&t2, 0.0f, 0.0f, 0.0f));
    assignVec3(&quest_zone_b.pos_0x04, (Vec*)setVec3(&t3, 5517.0f, -25.0f, -384.0f));
    assignVec3(&quest_zone_b.pos_0x18, (Vec*)setVec3(&t4, 0.0f, 0.0f, 0.0f));
    assignVec3(&quest_zone_c.pos_0x04, (Vec*)setVec3(&t5, -4353.0f, -50.0f, 805.0f));
    assignVec3(&quest_zone_c.pos_0x18, (Vec*)setVec3(&t6, 1350.0f, -50.0f, -840.0f));
    assignVec3(&quest_zone_c.pos_0x2C, (Vec*)setVec3(&t7, 0.0f, 0.0f, 0.0f));
    assignVec3(&quest_zone_d.pos_0x04, (Vec*)setVec3(&t8, 3800.0f, -1200.0f, 850.0f));
    assignVec3(&quest_zone_d.pos_0x18, (Vec*)setVec3(&t9, 0.0f, 0.0f, 0.0f));
    assignVec3(&quest_zone_e.pos_0x04, (Vec*)setVec3(&t10, -244.399002f, -25.0f, 2122.48389f));
    assignVec3(&quest_zone_e.pos_0x18, (Vec*)setVec3(&t11, 0.0f, 0.0f, 0.0f));
}

/* `.data` 0x805F2940..0x805F2A38 (`splits.txt`): the five objects tile the claim (0x40 + 0x28 + 0x40 + 0x28
 * + 0x28).  The vectors are zero here; `quest_zone_tbl_init` fills them. */
Q_ZoneSet3 quest_zone_a = { 0, { 0.0f, 0.0f, 0.0f }, 650.0f, 12, { 0.0f, 0.0f, 0.0f }, 500.0f, 0xFF,
                            { 0.0f, 0.0f, 0.0f }, 0.0f, 0 };
Q_ZoneSet2 quest_zone_b = { 9, { 0.0f, 0.0f, 0.0f }, 400.0f, 0xFF, { 0.0f, 0.0f, 0.0f }, 0.0f };
Q_ZoneSet3 quest_zone_c = { 0, { 0.0f, 0.0f, 0.0f }, 550.0f, 3, { 0.0f, 0.0f, 0.0f }, 500.0f, 0xFF,
                            { 0.0f, 0.0f, 0.0f }, 0.0f, 0 };
Q_ZoneSet2 quest_zone_d = { 0, { 0.0f, 0.0f, 0.0f }, 370.0f, 0xFF, { 0.0f, 0.0f, 0.0f }, 0.0f };
Q_ZoneSet2 quest_zone_e = { 6, { 0.0f, 0.0f, 0.0f }, 268.0f, 0xFF, { 0.0f, 0.0f, 0.0f }, 0.0f };
