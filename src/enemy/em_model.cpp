/* enemy/em_model.cpp - the roster spawn and model band of the enemy population manager: the 0x224-byte `EmPopRec`
 *   record accessors, the roster slot effect/position setters, the 32-slot model-handle table and the per-kind roster
 *   helpers.
 * RANGE. .text 0x803B936C-0x803BE30C (57 functions); .data 0x805F8220-0x805F84E0, .bss 0x806D2A68-0x806D2AF8,
 *   .sdata 0x80793690-0x807936A8, .sbss 0x80794C50-0x80794C60, .sdata2 0x8079C5B8-0x8079C630, extab, extabindex.  It
 *   defines `em_pop_w` (0x80794C58) and `em_handle_tbl` (0x806D2A78); `enemy/em_pop.cpp` is the band's head.
 * SEAM. Unproven; probably several TUs with `enemy/em_pop.cpp` (docs/enemy.md, Seams).
 * FLAGS. `cflags_menu`, like `enemy/em_pop.cpp`.
 * NAMES. `em_model` is a GUESS from the model-handle table and the 0x803BA6F0-0x803BE30C model helpers; the
 *   function names are `enemy/em_pop.cpp`'s scheme (its header).
 * RESIDUALS. 40 rows unwritten: 0x803B936C-0x803B9450, 0x803B9508-0x803B993C, 0x803B99C0-0x803B9A40,
 *   0x803B9A84-0x803B9E50, 0x803B9E90-0x803BA0A0, 0x803BA1B4-0x803BA69C, 0x803BA6F0-0x803BAE80, 0x803BAF44-0x803BCF70,
 *   0x803BD058-0x803BD99C, 0x803BD9F8-0x803BDECC, 0x803BE128-0x803BE30C.
 *  - `em_roster_record_result_get`: retail tests the kinds {3,4} with a range compare and computes the case-6 result
 *    branch-free (`subfic`/`nor`/`srawi`/`andi.`), ours uses a compare chain and branches;
 *  - `em_roster_record_get`: retail returns through `bne` + `mr r3,r4` + `blr`, ours through `beqlr` (the spellings
 *    tried for this row and the one above are in docs/enemy.md);
 *  - `em_weight_table_pick`: retail reloads the weight halfword (`lhz`); `em_roster_record_copy`: register
 *    allocation only.
 *   flipcheck: `.data`/`.sdata`/`.sdata2` claimed, not emitted; `.bss`/`.sbss`/`.text`/extab/extabindex short of
 *   the claim.
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
#include "g3d/g3d_anmchr.h"
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

extern "C" {

/* The 32-slot model-handle table's entry (the table itself is defined at the foot of the file):
 * `em_roster_record_unlink` walks the table, the allocator fills it (`__nw__FUl(0xC)` per slot). */
struct EmHandleEntry {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ u8 in_use_0x04;
    /* +0x05 */ u8 pad_0x05[3];
    /* +0x08 */ EmPopRec* owner_0x08;
}; /* size: 0xC */

extern EmHandleEntry* em_handle_tbl[0x20];

/* The record's first 0x1A0 bytes - the run `em_roster_record_copy` moves. */
struct EmPopRecHead {
    /* +0x000 */ u16 head_0x000;
    /* +0x002 */ u16 head_0x002;
    /* +0x004 */ u32 body_0x004[0x48];   /* the target copies this run in 0x24 8-byte steps */
    /* +0x124 */ u32 tail_0x124[0x20];   /* and this one in 0x10 */
}; /* size: 0x1A4 */

/* Maps the record's action code to the population result the callers store (0 when unset). */
u32 em_roster_record_result_get(const EmPopRec* rec) {
    switch (rec->action_0x003) {
    case 0x20:
        switch (rec->field_0x00A) {
        case 3:
        case 4:
            return 4;
        case 1:
            return 1;
        case 2:
            return 2;
        case 5:
            return 3;
        case 6:
            return 5;
        }
        return 0;
    case 0x22:
        return (rec->field_0x00A & 2) != 0;
    case 0x1b:
        return (u8)(rec->field_0x00A == 3);
    }
    return 0;
}

/* Places the record's position and its aim position on the same vector. */
void em_roster_record_pos_set(s32 index, nw4r::math::VEC3* pos, u8 area) {
    EmPopRec* rec = em_roster_record_get(index);
    if (rec != NULL) {
        copyVec3(&rec->pos_0x014, pos);
        copyVec3(&rec->aim_0x1D0, pos);
    }
}

/* Hands one live record back to the pool. */
void em_roster_record_release(u32 index) {
    EmPopRec* rec = em_roster_record_get(index);
    if (rec != NULL) {
        em_roster_record_clear(rec);
    }
}

/* The record's population id, or -1 when the slot is empty or the id is unset. */
s32 em_roster_record_slot_id_get(u32 index) {
    EmPopRec* rec = em_roster_record_get(index);
    s32 id = 0;
    if (rec != NULL) {
        id = rec->slot_id_0x1FA;
    }
    if (id > 0) {
        return id;
    }
    return -1;
}

/* Whether the record is live and still in its first two states. */
u32 em_roster_record_alive_ck(u32 index) {
    EmPopRec* rec = em_roster_record_get(index);
    if (rec != NULL) {
        if (rec->field_0x008 <= 1) {
            return 1;
        }
    }
    return 0;
}

/* The first free roster slot, stamped with its own index; NULL when the pool is full. */
EmPopRec* em_roster_free_record_get(void) {
    EmPopWork* work = em_pop_w.work;
    EmPopRec* rec = work->recs;
    s32 index = 0;
    s32 num = work->rec_num;
    for (; index < num; index++, rec++) {
        if (rec->state_0x000 == 0) {
            rec->index_0x002 = index;
            return rec;
        }
    }
    return NULL;
}

/* Drops every handle that still names `rec`, releases its model and zeroes the record. */
EmPopRec* em_roster_record_clear(EmPopRec* rec) {
    em_roster_record_unlink(rec);
    if (rec->g3d_0x144 != NULL) {
        push_g3d_wk(rec->g3d_0x144);
    }
    return (EmPopRec*)memset(rec, 0, 0x224);
}

/* The first free sub-record slot; NULL when the pool is full. */
EmPopSubRec* em_roster_sub_free_get(void) {
    EmPopWork* work = em_pop_w.work;
    EmPopSubRec* rec = work->subs;
    s32 num = work->sub_num;
    while (num > 0) {
        if (rec->field_0x000 == 0) {
            return rec;
        }
        rec++;
        num--;
    }
    return NULL;
}

/* The same recycle path for one of the manager's sub-records (stride 0x20C). */
u32 em_roster_sub_record_clear(EmPopSubRec* rec) {
    em_roster_record_unlink((EmPopRec*)rec);
    if (rec->g3d_0x144 != NULL) {
        push_g3d_wk(rec->g3d_0x144);
    }
    return (u32)memset(rec, 0, 0x20C);
}

/* Whether the local slot's sub index is `sub`. */
u32 quest_move_area_sub_ck(u8 sub) {
    Q_MoveWork* work = (Q_MoveWork*)get_move_work_adrs(0);
    if (work != NULL && work->sub_0xEA == sub) {
        return 1;
    }
    return 0;
}

/* Picks one entry of a 0xFFFF-terminated weight table at random, by weight; returns its value (0xFFFF when the
 * table is empty). */
u16 em_weight_table_pick(const EmWeightEntry* table) {
    const EmWeightEntry* entry;
    s32 total = 0;
    u16 roll;
    s32 acc;
    entry = table;
    while (entry->weight != 0xFFFF) {
        total += entry->weight;
        entry++;
    }
    if ((u16)total == 0) {
        return 0xFFFF;
    }
    roll = ran_suu(0) % (u16)total;
    acc = 0;
    while (table->weight != 0xFFFF) {
        acc += table->weight;
        if (roll < (u16)acc) {
            return table->value;
        }
        table++;
    }
    return 0xFFFF;
}

/* Drops every handle-table slot that still names `rec`. */
void em_roster_record_unlink(EmPopRec* rec) {
    u32 i;
    for (i = 0; i < 0x20; i++) {
        if (em_handle_tbl[i] != NULL && em_handle_tbl[i]->in_use_0x04 != 0 && em_handle_tbl[i]->owner_0x08 == rec) {
            em_handle_tbl[i]->in_use_0x04 = 0;
            em_handle_tbl[i]->owner_0x08 = NULL;
        }
    }
}

/* Copies the record's head block (the part the takeover path moves). */
void em_roster_record_copy(EmPopRec* dst, const EmPopRec* src) {
    *(EmPopRecHead*)dst = *(const EmPopRecHead*)src;
}

/* Returns the live roster record at `index`, or NULL when the slot is out of range or not live. */
EmPopRec* em_roster_record_get(u32 index) {
    EmPopWork* work = em_pop_w.work;
    if ((s32)index >= (s32)work->rec_num) {
        return NULL;
    }
    EmPopRec* rec = &work->recs[index];
    if (rec->state_0x000 == 1) {
        return rec;
    }
    return NULL;
}

/* The index of the `kind` records (live, in an accepted state) into `out` until `max` are found; returns how
 * many matched. */
u8 em_roster_kind_collect(u8 kind, u8* out, u8 max) {
    EmPopWork* work = em_pop_w.work;
    EmPopRec* rec = work->recs;
    s32 i;
    s32 count = 0;
    for (i = 0; i < (s32)work->rec_num; rec++, i++) {
        if (rec->state_0x000 == 0) {
            continue;
        }
        if (rec->kind_0x004 != kind) {
            continue;
        }
        if (rec->field_0x006 == 0 && rec->field_0x008 != 1) {
            continue;
        }
        if (rec->field_0x006 == 1 && (rec->field_0x008 != 1 || rec->field_0x009 != 0)) {
            continue;
        }
        if (rec->field_0x005 > 1 && (u8)(rec->field_0x005 + 252) > 1 && (u8)(rec->field_0x005 + 242) > 1) {
            continue;
        }
        if (out != NULL) {
            *out = (u8)i;
            out++;
            count++;
            if (count >= max) {
                return (u8)count;
            }
        } else {
            count++;
        }
    }
    return (u8)count;
}

/* The aim position of record `index` when it is live, of `kind` and in an accepted state; else NULL. */
nw4r::math::VEC3* em_roster_kind_aim_pos_get(u8 index, u8 kind) {
    EmPopRec* rec = &em_pop_w.work->recs[index];
    if (rec->state_0x000 == 0) {
        return NULL;
    }
    if (rec->kind_0x004 != kind) {
        return NULL;
    }
    if (rec->field_0x006 == 0 && rec->field_0x008 != 1) {
        return NULL;
    }
    if (rec->field_0x006 == 1 && (rec->field_0x008 != 1 || rec->field_0x009 != 0)) {
        return NULL;
    }
    return &rec->aim_0x1D0;
}

/* The +0x005 byte of record `index` under the same acceptance test; -1 when it fails. */
s32 em_roster_kind_field5_get(u8 index, u8 kind) {
    EmPopRec* rec = &em_pop_w.work->recs[index];
    if (rec->state_0x000 == 0) {
        return -1;
    }
    if (rec->kind_0x004 != kind) {
        return -1;
    }
    if (rec->field_0x006 == 0 && rec->field_0x008 != 1) {
        return -1;
    }
    if (rec->field_0x006 == 1 && (rec->field_0x008 != 1 || rec->field_0x009 != 0)) {
        return -1;
    }
    return (s8)rec->field_0x005;
}

/* ---------------------------------------------------------------------------------------------- */
/* This unit's own data, defined at the foot.  Both symbols are referenced only from this range (checked
 * over every `bl`/`lbl_` reference in the split asm). */

/* .sbss 0x80794C58 - the manager work pointer, written whole by the work allocator. */
EmPopWorkSlot em_pop_w;

/* .bss 0x806D2A78 - the 32-slot model-handle table. */
EmHandleEntry* em_handle_tbl[0x20];

} /* extern "C" */
