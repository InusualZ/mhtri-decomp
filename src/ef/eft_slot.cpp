/*
 * ef/eft_slot.cpp - the `_EFT` family's 10-entry slot pool (`lbl_806BF0A0`, one 0x3C-byte `EftSlot` per entry), the
 *   enemy-record scan that drives it, the `EftDef` definition table and the slot predicates and state setters.
 * RANGE. .text 0x803432B4-0x80348A48 (76 functions); extab 0x80016DB4-0x80016F74, extabindex 0x800362F4-0x80036594,
 *   .ctors 0x8056F3A4-0x8056F3A8, .data 0x805E7B70-0x805E91E8, .bss 0x806BF0A0-0x806BF310, .sdata
 *   0x807930F0-0x807932F0, .sdata2 0x8079B320-0x8079B368.  The tail from 0x80348A48 is `menu/menu_effect_slot.cpp`.
 *   The seam is unproven: no `__FILE__` string covers the band, no data label straddles either edge, and the
 *   band's own `.sdata2` run (0x8079B320-0x8079B368) only brackets the object's end between 0x8034782C and
 *   0x8034CDDC.  The `.data` records 0x805E7FB0/0x805E87F0/0x805E8B48/0x805E9118/0x805E9140 are `{init, step, exit}`
 *   triples of this range's unwritten programs.
 * FLAGS. `cflags_main`; `#pragma peephole off` over every body (retail keeps the unfused `clrlwi` + `cmpwi` of
 *   `eft_slot_live_ck` and the `cntlzw`/`srwi` of `eft_slot_persist_ck`; playbook 39).
 * NAMES. Every definition is a GUESS from its body (the dump has only placeholders here): the pool (`eft_slot_clear`,
 *   `eft_slot_pool_clear`, `eft_slot_find_free`, `eft_slot_find`, `eft_slot_spawn`, `eft_slot_spawn_targets`), the
 *   definition table (`eft_def_get`, `eft_def_flags`, `eft_def_handler`, `eft_def_model_block`), the predicates
 *   (`eft_slot_armed_ck`, `eft_slot_live_ck`, `eft_slot_persist_ck`, `eft_slot_area_ck`), the per-frame work
 *   (`eft_slot_work_bind`, `eft_slot_work_update`, `eft_slot_counters_step`, `eft_slot_kind_set`,
 *   `eft_slot_effect_key`, `eft_slot_state_set`, `eft_slot_state_request`), the match tests (`eft_work_match_ck`, `eft_target_match_ck`,
 *   `eft_work_wide_ck`, `eft_target_wide_ck`, `eft_slot_match_count`), the (mode, value) pair at +0x0F/+0x10
 *   (`eft_slot_mode_set`, `eft_slot_mode_set_imm`, `eft_slot_mode_set_defer`, `eft_slot_mode_set_map`,
 *   `eft_slot_mode_set_work`) and the two instance arms `eft_state_advance`/`eft_instance_release`.
 *   `enemy_data_find`/`enemy_data_grp` are named for the enemy band's callers, which read the record as
 *   `_ENEMY_DATA` (`enemy/ENEMY_DATA.h`); this file keeps its own `EftSlot` view.
 * RESIDUALS. 42 rows unwritten: 0x803432B4-0x80343744 (`fn_803432B4`), 0x80344658-0x80344DA4 (`fn_80344658`, the
 *   shared distance sort), 0x80344E9C-0x80345210 (`fn_80344E9C`, `fn_80345124`), 0x803455F0-0x803456B4
 *   (`fn_803455F0`), 0x803457A8-0x80348A48 (37 rows; the walkers `fn_803457A8`/`fn_80345894` call `fn_80125FF0`
 *   with no second argument, which its two-parameter owner declaration cannot spell).
 *   16 partial rows, including:
 *  - `enemy_data_grp`: the code is the target's; ours names its switch table `@2246` where retail names
 *    `jumptable_805E918C`;
 *  - `eft_def_model_block`: ours materialises `fn_80125FF0`'s second argument (`clrlwi r4`) and reorders the walk;
 *  - `eft_slot_work_update`: one stride reads 0xB20 in retail and 0xB18 in ours, and ours narrows the two ids
 *    (`extsb`, `clrlwi`) at the call where retail does not;
 *  - `eft_slot_clear`: ours indexes the +0x14 store with `stbx` where retail adds first;
 *  - `eft_slot_counters_step`: ours loads the counter `lhz` where retail loads `lha` and masks.
 *   The other 11 partial rows have no recorded cause (`symdiff.py -u ef/eft_slot --all`).
 *   flipcheck: `.ctors`/`.sdata`/`.sdata2` claimed, not emitted; `.text` (0x14F4 of 0x5794), extab (0x98 of 0x1C0),
 *   extabindex (0xE4 of 0x2A0) and `.data` (0x80 of 0x1678) short of the claim and differing.
 * SHAPES. The definition table `lbl_805E9168` (9 entries, index 0 NULL) and `.bss` `lbl_806BF0A0`/`lbl_806BF2F8` are
 *   defined here; `enemy_data_grp`'s switch table is emitted beside the table (one `.data` chunk, playbook 58).
 */

#include "enemy/lbl_806A54E0.h" /* lbl_806A54E0 (rule 2: the owner's header) */
#include "types.h"
#include "nw4r/math.h"
#include "ef.h"
#include "ef/eft_res.h"
#include "unsplit/unknown.h"
#include "unsplit/ef.h"
#include "fn_8004CAD8.h" /* the vector/geometry helpers the range calls */
#include "Network/network_pat_control.h" /* isReadyCountOne */
#include "camera/camera.h" /* get_camera_pos / get_camera_direction / fn_802BE088 */
#include "ef/fn_800CDB2C.h" /* my_player_no */
#include "ef/EftSlot.h"
#include "ef/eft_slot.h" /* the unit's own declarations (eft_net_send via hud/eft_net_send.h) */
#include "enemy/ENEMY_WORK.h" /* the move-work records `eft_slot_spawn_targets`/`eft_slot_work_bind` walk */
#include "enemy/fn_8012BDF4.h" /* em_work_die_ck, fn_8012D1A8 */
#include "enemy/fn_801251D0.h" /* fn_80125FF0, fn_8012A9E8 */
#include "enemy/enemy_control.h" /* fn_80143BF8 */

struct EftEntry;
/* One record of the definition table `lbl_805E9168`, indexed by `EftSlot::key_0x00`.  `kind_0x00` is
 * the byte `eft_def_flags` returns (its two low bits select the spawn path), `field_0x14` points at the
 * three per-kind entry points `eft_def_handler` selects between. size: 0x18 (to the next named member) */
struct EftDef {
    /* +0x00 */ u8 kind_0x00;
    /* +0x01 */ u8 unused_0x01[0x04 - 0x01];
    /* +0x04 */ EftEntry* entries_0x04;   /* the per-motion table `eft_def_model_block` walks */
    /* +0x08 */ u8 unused_0x08[0x10 - 0x08];
    /* +0x10 */ EftEntry* entries_0x10;   /* the second table `fn_803457A8` walks */
    /* +0x14 */ void (**handlers_0x14)(_EFT*);
};

/* One 8-byte entry of a definition's sub-table: `key_0x00` is 255 at the end of the run, and
 * `sub_0x04` is the next level down (the model table, or the 2-byte pair table). size: 0x08 */
struct EftEntry {
    /* +0x00 */ u8 key_0x00;
    /* +0x01 */ u8 unused_0x01[0x04 - 0x01];
    /* +0x04 */ EftEntry* sub_0x04;
};

/* One 0x44-byte record of the 128-entry array `lbl_806A54E0` the live-slot scan walks: the same
 * (team, area, action) key an enemy work record carries, plus the key bytes the slot pool matches
 * against.  size: 0x44 */
struct EftTargetRecord {
    /* +0x00 */ u8 live_0x00;
    /* +0x01 */ u8 unused_0x01;
    /* +0x02 */ u8 team_0x02;
    /* +0x03 */ u8 field_0x03;
    /* +0x04 */ u8 unused_0x04[0x08 - 0x04];
    /* +0x08 */ u8 state_0x08;  /* 1 and 2 are skipped by the live-slot scan */
    /* +0x09 */ u8 key_0x09;    /* the slot key the record matches, 255 = none */
    /* +0x0A */ u8 index_0x0A;  /* the slot's own index argument */
    /* +0x0B */ u8 unused_0x0B[0x44 - 0x0B];
};

/* The 10-slot pool `eft_slot_pool_clear` clears, `eft_slot_find_free` scans for a free entry and `enemy_data_find`
 * looks a key up in. */

extern "C" {
EftSlot lbl_806BF0A0[10]; /* .bss 0x806BF0A0, 0x258 B */
}
/* The definition table the slot's `key_0x00` indexes (36 B, 9 entries; index 0 is the "no definition"
 * slot).  This unit owns it: its only referrers are this range's own functions (`eft_def_get`,
 * `eft_def_flags`, `eft_def_handler` - 6 relocs, 2 per caller, and no object outside the band names it), it
 * is contiguous with the switch table MWCC emits for `enemy_data_grp` below, and the `.data` claim covers
 * 0x805E9168..0x805E91E8 so the object is the target object.  The values and their order are the DOL's
 * words at 0x805E9168..0x805E918C; the eight non-null entries point at the 0x18-byte `EftDef` records
 * the band's `.data` band holds (playbook 58: a unit's sole-referencer data is claimed, not extern'd). */

extern "C" {
EftDef lbl_806BF2F8;   /* .bss  0x806BF2F8, 0x18 B */
}
extern "C" EftDef lbl_805E7FC0;   /* .data 0x805E7FC0 */
extern "C" EftDef lbl_805E7FD8;   /* .data 0x805E7FD8 */
extern "C" EftDef lbl_805E8800;   /* .data 0x805E8800 */
extern "C" EftDef lbl_805E8828;   /* .data 0x805E8828 */
extern "C" EftDef lbl_805E8B58;   /* .data 0x805E8B58 */
extern "C" EftDef lbl_805E9128;   /* .data 0x805E9128 */
extern "C" EftDef lbl_805E9150;   /* .data 0x805E9150 */
extern "C" EftDef* lbl_805E9168[9] = {
    NULL,
    &lbl_806BF2F8,
    &lbl_805E7FC0,
    &lbl_805E8800,
    &lbl_805E8828,
    &lbl_805E8B58,
    &lbl_805E9128,
    &lbl_805E9150,
    &lbl_805E7FD8,
};
/* The 128-entry record array `eft_slot_spawn_targets`/`eft_slot_match_count` walk. */
extern "C" f32 lbl_8079B350;
extern "C" void eft_state_advance(_EFT* self);  /* `_EFT::state_0x05++` - the dispatcher's advance arm */
extern "C" void eft_instance_release(_EFT* self);  /* hands the instance to `eft_res_slot_release`, the shared release path */
extern "C" void eft_slot_clear(u8 index);  /* zeroes one pool entry's key byte and stamps its +0x14 byte 255 */
extern "C" void eft_slot_pool_clear(void);  /* the 10-iteration loop over `eft_slot_clear` */
extern "C" u8 eft_slot_find_free(void);  /* first entry whose `key_0x00` is zero, 255 when the pool is full */
extern "C" EftDef* eft_def_get(EftSlot* slot);  /* `lbl_805E9168[slot->key_0x00]` */
extern "C" u8 eft_def_flags(EftSlot* slot);  /* the definition's first byte; callers test bits 0, 1, 2 and 3 */
extern "C" void (*eft_def_handler(EftSlot* slot, u8 kind))(_EFT*);  /* `def->handlers_0x14[kind]`, NULL when absent */
extern "C" u32 eft_slot_armed_ck(EftSlot* slot);  /* returns the entry's `armed_0x02` byte */
extern "C" u32 eft_slot_live_ck(EftSlot* slot);  /* a definition flag bit set AND `field_0x18` (the live work) non-zero */
extern "C" u32 eft_slot_persist_ck(EftSlot* slot);  /* the definition's bit 1 clear = the instance survives its work */
extern "C" EftSlot* eft_slot_spawn(u8 key1, u8 key0, u8 index);  /* find-or-allocate, then seed the whole 0x3C block */
extern "C" void eft_slot_spawn_targets(void);  /* one spawn per live enemy work record and per live target record */
extern "C" void eft_slot_work_bind(EftSlot* slot);  /* settles `work_0x03`/`armed_0x02` against `my_player_no()` */
extern "C" void eft_slot_work_update(EftSlot* slot);  /* the per-frame work pick; signals states 5, 7 and 8 */
extern "C" u32 eft_work_match_ck(EftSlot* slot, _ENEMY_WORK* work);  /* does the slot already track this work record? */
extern "C" u32 eft_target_match_ck(EftSlot* slot, EftTargetRecord* record);  /* the same test on the 128-entry array */
extern "C" void eft_slot_match_count(EftSlot* slot);  /* counts the matches into the entry's +0x0D byte */
extern "C" u32 eft_work_wide_ck(EftSlot* slot, _ENEMY_WORK* work);  /* wider test: the key, or a second slot sharing +0x17 */
extern "C" u32 eft_target_wide_ck(EftSlot* slot, EftTargetRecord* record);  /* the same wider test, 128-entry array */
extern "C" u32 eft_slot_area_ck(EftSlot* slot);  /* tracked work -> 0; a matching record of the current area -> 1 */
extern "C" void eft_slot_kind_set(EftSlot* slot, u8 key, u8 force);  /* re-points +0x14, the previous value kept in +0x15 */
extern "C" void eft_slot_counters_step(EftSlot* slot);  /* the three per-frame counters, each saturating at its ceiling */
extern "C" void eft_slot_state_set(EftSlot* slot, u8 state, _ENEMY_WORK* work, u8 index);  /* writes +0x08/+0x09, calls handler 2 */
extern "C" void eft_slot_state_request(EftSlot* slot, u8 state, _ENEMY_WORK* work, u8 index);  /* live -> switch now, else record it */
extern "C" u8 eft_slot_effect_key(EftSlot* slot);  /* +0x10 in mode 3, else +0x14 - the key the family's spawner stamps */
extern "C" void eft_slot_mode_set(EftSlot* slot, u8 mode, u8 value, u8 immediate);  /* (mode, value) into +0x0F/+0x10 */
extern "C" void eft_slot_mode_set_imm(void* slot, u8 mode, u8 value);  /* the wrapper that passes immediate = 1 */
extern "C" void eft_slot_mode_set_defer(void* slot, u8 mode, u8 value);  /* the wrapper that passes immediate = 0 */
extern "C" void eft_slot_mode_set_map(void* slot, u8 mode, u8 kind, u8 value, u8 immediate);  /* mode/kind/value onto the pair */
extern "C" void fn_80345A2C(EftSlot* slot);  /* this range's still-unwritten entry `eft_slot_kind_set` calls */
extern "C" void eft_slot_mode_set_work(void* slot, _ENEMY_WORK* work, u8 kind, u8 a, u8 b, u8 immediate);  /* the work record rewrites the triple first */
extern "C" u8* eft_def_model_block(EftSlot* slot, u8 index);  /* the per-motion walk down to the 0x20-byte block */

#pragma peephole off

/* Advances the family's step counter. */
extern "C" void eft_state_advance(_EFT* self) {
    self->state_0x05++;
}

/* Hands the instance to the shared effect release path. */
extern "C" void eft_instance_release(_EFT* self) {
    eft_res_slot_release(self);
}

/* Resets one slot of the 0x3C-byte slot pool: cleared, with its +0x14 byte stamped 255. */
extern "C" void eft_slot_clear(u8 index) {
    u32 offset = (u32)index * 0x3C;

    /* the pool as bytes: neither store names a field (rule 6's byte-offset case) */
    ((u8*)lbl_806BF0A0)[offset] = 0;
    ((u8*)lbl_806BF0A0)[offset + 0x14] = 255;
}

/* Clears the whole 10-slot pool. */
extern "C" void eft_slot_pool_clear(void) {
    u8 i;

    for (i = 0; i < 10; i++) {
        eft_slot_clear(i);
    }
}

/* Returns the first free slot's index, or 255 when the pool is full. */
extern "C" u8 eft_slot_find_free(void) {
    u8 i;

    for (i = 0; i < 10; i++) {
        if (lbl_806BF0A0[i].key_0x00 == 0) {
            return i;
        }
    }
    return 255;
}

/* Looks the 10-entry table up by its group index and the enemy-data id. */
extern "C" void* enemy_data_find(u8 key0, u8 key1) {
    u8 i;

    for (i = 0; i < 10; i++) {
        EftSlot* slot = &lbl_806BF0A0[i];

        if (slot->key_0x00 == key0 && slot->key_0x01 == key1) {
            return slot;
        }
    }
    return NULL;
}

/* The group index the 10-entry table is keyed on, from an enemy's kind byte and its variant. */
extern "C" u8 enemy_data_grp(u8 kind, u8 flag) {
    switch (kind) {
    default:
        return 1;
    case 10:
    case 11:
    case 12:
        return 2;
    case 13:
    case 14:
        return 8;
    case 23:
        return 5;
    case 27:
    case 28:
        return 3;
    case 32:
        return flag == 2 ? 4 : 1;
    case 22:
        return flag == 0 ? 7 : 6;
    }
}

/* Returns the definition record a slot's key byte selects. */
extern "C" EftDef* eft_def_get(EftSlot* slot) {
    return lbl_805E9168[slot->key_0x00];
}

/* Returns the definition's kind byte. */
extern "C" u8 eft_def_flags(EftSlot* slot) {
    return *(u8*)lbl_805E9168[slot->key_0x00];
}

/* Returns the definition's per-kind entry point, or NULL when the definition has none. */
extern "C" void (*eft_def_handler(EftSlot* slot, u8 kind))(_EFT*) {
    EftDef* def = lbl_805E9168[slot->key_0x00];

    if (def->handlers_0x14 == NULL) {
        return NULL;
    }
    switch (kind) {
    case 0:
        return def->handlers_0x14[0];
    case 1:
        return def->handlers_0x14[1];
    case 2:
        return def->handlers_0x14[2];
    }
    return NULL;
}

/* Returns the slot's own kind byte. */
extern "C" u32 eft_slot_armed_ck(EftSlot* slot) {
    return slot->armed_0x02;
}

/* Tests whether the slot's definition asks for the camera-facing variant and the slot is live. */
extern "C" u32 eft_slot_live_ck(EftSlot* slot) {
    if ((eft_def_flags(slot) & 1) != 0 && slot->field_0x18 != 0) {
        return 1;
    }
    return 0;
}

/* Tests whether the slot's definition leaves the instance in the world after its work ends. */
extern "C" u32 eft_slot_persist_ck(EftSlot* slot) {
    return (eft_def_flags(slot) & 2) == 0;
}

/* Spawns (or finds) the slot for one (key, kind) pair and seeds its whole state block. */
extern "C" EftSlot* eft_slot_spawn(u8 key1, u8 key0, u8 index) {
    EftSlot* slot = (EftSlot*)enemy_data_find(key1, key0);
    u8 free_index;

    if (slot != NULL) {
        return slot;
    }
    free_index = eft_slot_find_free();
    if (free_index == 255) {
        return NULL;
    }
    slot = &lbl_806BF0A0[free_index];
    slot->key_0x01 = key0;
    slot->field_0x08 = 0;
    slot->field_0x0A = 0;
    slot->field_0x0B = 0;
    slot->field_0x13 = get_now_mapno();
    slot->field_0x14 = index;
    slot->field_0x11 = 255;
    slot->field_0x12 = 255;
    eft_slot_mode_set_defer(slot, 0, 0);
    slot->field_0x1C = 0;
    slot->field_0x1E = (u16)-1;
    slot->field_0x20 = 0;
    slot->field_0x22 = 0;
    slot->field_0x34 = 0;
    slot->field_0x0D = 0;
    slot->field_0x0E = 0;
    slot->field_0x38 = 0;
    slot->field_0x39 = 255;
    slot->field_0x17 = 255;
    slot->field_0x18 = 0;
    slot->field_0x24 = lbl_8079B350;
    slot->field_0x28 = lbl_8079B350;
    slot->field_0x2C = lbl_8079B350;
    slot->field_0x30 = lbl_8079B350;
    slot->field_0x36 = 255;
    slot->field_0x15 = 255;
    slot->field_0x3A = 0;
    slot->field_0x3B = 255;
    slot->field_0x16 = 0;
    slot->key_0x00 = key1;
    {
        void (*entry)(_EFT*) = eft_def_handler(slot, 0);

        if (entry != NULL) {
            entry((_EFT*)slot);
        }
    }
    slot->field_0x37 = slot->field_0x39 - 1;
    slot->field_0x06 = 0;
    eft_slot_work_bind(slot);
    slot->field_0x04 = 0;
    slot->field_0x05 = 0;
    return slot;
}

/* Spawns the live slots of every enemy work record, then of the 128-entry record array. */
extern "C" void eft_slot_spawn_targets(void) {
    _ENEMY_WORK* work = (_ENEMY_WORK*)get_move_work_adrs(3);
    EftTargetRecord* record = (EftTargetRecord*)lbl_806A54E0;
    u16 count = get_move_work_max(3);
    s32 i;

    for (i = 0; i < (s32)count; i++, work++) {
        if (em_work_die_ck(work) != 1 && work->field_0x46C != 255) {
            eft_slot_spawn(work->field_0x46C, enemy_data_grp(work->team, work->field_0x00A),
                        work->area_no);
        }
    }
    for (i = 0; i < 128; i++, record++) {
        if (record->live_0x00 != 0 && record->state_0x08 != 1 && record->state_0x08 != 2 &&
            record->key_0x09 != 255) {
            eft_slot_spawn(record->key_0x09, enemy_data_grp(record->team_0x02, record->field_0x03),
                        record->index_0x0A);
        }
    }
}

/* Settles the slot's move-work index and the two bytes that follow it. */
extern "C" void eft_slot_work_bind(EftSlot* slot) {
    s8 current = (s8)my_player_no();

    if ((eft_def_flags(slot) & 2) != 0) {
        slot->armed_0x02 = 1;
        slot->work_0x03 = current;
        return;
    }
    slot->work_0x03 = -1;
    {
        u8* work = (u8*)get_move_work_adrs(2);
        u16 count = get_move_work_max(2);
        s8 i;

        for (i = 0; i < (s8)count; i++, work += 0xB20) {
            if (work[0] != 0 && fn_8012D1A8(work[8]) == 0) {
                slot->work_0x03 = i;
                break;
            }
        }
    }
    if (slot->work_0x03 == current) {
        slot->armed_0x02 = 0;
        slot->field_0x0C = slot->work_0x03;
    } else if (slot->work_0x03 == -1) {
        slot->armed_0x02 = 1;
    }
}

/* Picks the move work the slot follows: the free-slot scan, then the forward search for the record
 * whose area the slot was seeded with. */
extern "C" void eft_slot_work_update(EftSlot* slot) {
    s8 current = (s8)my_player_no();
    _ENEMY_WORK* work;
    u16 count;

    if ((eft_def_flags(slot) & 2) != 0) {
        return;
    }
    if (isServerSelectState() != 1) {
        return;
    }
    work = (_ENEMY_WORK*)get_move_work_adrs(2);
    count = get_move_work_max(2);
    if (eft_slot_armed_ck(slot) == 0) {
        if (isReadyCountOne() == 1) {
            u32 found = 0;

            if (work[(s8)slot->work_0x03].active == 0 ||
                fn_8012D1A8(slot->work_0x03) != 0) {
                u16 i;

                for (i = 0; i < count; i++) {
                    if (work[i].active != 0 && fn_8012D1A8((u8)i) == 0) {
                        slot->work_0x03 = (s8)i;
                        found = 1;
                        break;
                    }
                }
            }
            if (found == 1) {
                if ((s8)slot->work_0x03 == current) {
                    slot->armed_0x02 = 1;
                    eft_net_send(slot, 5, 0);
                    return;
                }
                eft_net_send(slot, 7, slot->work_0x03);
                return;
            }
            if (fn_80143BF8() == 1) {
                eft_net_send(slot, 7, slot->work_0x03);
            }
        } else {
            s16 timer = slot->field_0x06;

            if (timer < 900) {
                slot->field_0x06 = timer + 1;
                return;
            }
            eft_net_send(slot, 8, 0);
        }
    } else {
        if (fn_80143BF8() == 1) {
            slot->armed_0x02 = 1;
            eft_net_send(slot, 5, 0);
            return;
        }
        {
            u8 index = slot->work_0x03;

            if (slot->field_0x14 != *((u8*)work + (s8)index * 0xB20 + 0x16)) {
                u8 next = index + 1;
                u16 i;

                for (i = 0; i < (u16)(count - 1); i++, next++) {
                    _ENEMY_WORK* record;

                    if (next >= count) {
                        next = 0;
                    }
                    record = (_ENEMY_WORK*)((u8*)work + next * 0xB20);
                    if (record->active != 0 && fn_8012D1A8(next) == 0 &&
                        slot->field_0x14 == record->field_0x016) {
                        slot->work_0x03 = (s8)next;
                        slot->armed_0x02 = 0;
                        eft_net_send(slot, 5, 0);
                        return;
                    }
                }
            }
        }
    }
}

/* Tests whether the slot is already tracking the enemy work record the scan handed over. */
extern "C" u32 eft_work_match_ck(EftSlot* slot, _ENEMY_WORK* work) {
    if (em_work_die_ck(work) == 0) {
        if (work->action != 0xB && work->action != 0xC &&
            slot->key_0x00 == enemy_data_grp(work->team, work->field_0x00A)) {
            u8 key = work->field_0x46C;

            if (key != 0xFF && key == slot->key_0x01) {
                return 1;
            }
        }
    }
    return 0;
}

/* The same test against one record of the 128-entry array. */
extern "C" u32 eft_target_match_ck(EftSlot* slot, EftTargetRecord* record) {
    if (record->live_0x00 != 0 && record->state_0x08 == 0 &&
        slot->key_0x00 == enemy_data_grp(record->team_0x02, record->field_0x03)) {
        u8 key = record->key_0x09;

        if (key != 0xFF && key == slot->key_0x01) {
            return 1;
        }
    }
    return 0;
}

/* Counts the work records and array entries the slot matches, into the slot's +0x0D byte. */
extern "C" void eft_slot_match_count(EftSlot* slot) {
    _ENEMY_WORK* work = (_ENEMY_WORK*)get_move_work_adrs(3);
    u16 count = get_move_work_max(3);
    EftTargetRecord* record = (EftTargetRecord*)lbl_806A54E0;
    u8 i;

    slot->field_0x0D = 0;
    for (i = 0; i < (u8)count; i++, work++) {
        if (eft_work_match_ck(slot, work) == 1) {
            slot->field_0x0D++;
        }
    }
    for (i = 0; i < 128; i++, record++) {
        if (eft_target_match_ck(slot, record) == 1) {
            slot->field_0x0D++;
        }
    }
}

/* The wider match test: the work record's own key, or another live slot that shares this slot's
 * +0x17 byte (or, for the definitions that ask for it, its +0x14 byte). */
extern "C" u32 eft_work_wide_ck(EftSlot* slot, _ENEMY_WORK* work) {
    if (em_work_die_ck(work) == 0 && work->action != 0xB && work->action != 0xC) {
        u8 key = slot->key_0x00;

        if (key == enemy_data_grp(work->team, work->field_0x00A)) {
            u8 other_key = work->field_0x46C;

            if (other_key == slot->key_0x01 && other_key != 0xFF) {
                return 1;
            }
        }
        {
            EftSlot* other = (EftSlot*)enemy_data_find(key, work->field_0x46C);

            if (other != NULL) {
                u8 a = other->field_0x17;
                u8 b = slot->field_0x17;

                if (b == a && b != 0xFF && a != 0xFF) {
                    return 1;
                }
                if ((eft_def_flags(slot) & 8) != 0 && slot->field_0x14 == other->field_0x14) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

/* The same wider test against one record of the 128-entry array. */
extern "C" u32 eft_target_wide_ck(EftSlot* slot, EftTargetRecord* record) {
    if (record->live_0x00 != 0 && record->state_0x08 == 0) {
        u8 key = slot->key_0x00;

        if (key == enemy_data_grp(record->team_0x02, record->field_0x03)) {
            u8 other_key = record->key_0x09;

            if (other_key == slot->key_0x01 && other_key != 0xFF) {
                return 1;
            }
        }
        {
            EftSlot* other = (EftSlot*)enemy_data_find(key, record->key_0x09);

            if (other != NULL) {
                u8 a = other->field_0x17;
                u8 b = slot->field_0x17;

                if (b == a && b != 0xFF && a != 0xFF) {
                    return 1;
                }
                if ((eft_def_flags(slot) & 8) != 0 && slot->field_0x14 == other->field_0x14) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

/* Whether a record of the current area still matches the slot: any enemy work record it already
 * tracks means "not spawned", any array entry whose area byte is the current area means "spawned". */
extern "C" u32 eft_slot_area_ck(EftSlot* slot) {
    _ENEMY_WORK* work = (_ENEMY_WORK*)get_move_work_adrs(3);
    u16 count = get_move_work_max(3);
    s32 i;

    if (eft_slot_armed_ck(slot) == 0) {
        return 0;
    }
    for (i = 0; i < (s32)count; i++, work++) {
        if (eft_work_wide_ck(slot, work) == 1) {
            return 0;
        }
    }
    {
        EftTargetRecord* record = (EftTargetRecord*)lbl_806A54E0;
        s32 j;

        for (j = 0; j < 128; j++, record++) {
            if (eft_target_wide_ck(slot, record) == 1 && record->index_0x0A == get_now_areano()) {
                return 1;
            }
        }
    }
    return 0;
}

/* Re-points the slot at another effect kind (the +0x14 byte the family's spawner stamps). */
extern "C" void eft_slot_kind_set(EftSlot* slot, u8 key, u8 force) {
    if (force != 0 || eft_slot_armed_ck(slot) != 0) {
        u8 previous = slot->field_0x14;

        if (previous != key) {
            slot->field_0x15 = previous;
            slot->field_0x14 = key;
            slot->field_0x34 = 0;
            fn_80345A2C(slot);
            if (force == 0 && (eft_def_flags(slot) & 4) == 0) {
                eft_net_send(slot, 3, 0);
            }
        }
    }
}

/* Steps the slot's three per-frame counters, each saturating at its own ceiling. */
extern "C" void eft_slot_counters_step(EftSlot* slot) {
    s16 timer;

    if (slot->field_0x34 < 0x7FFF) {
        slot->field_0x34 += 1;
    }
    if (slot->field_0x1C < 0x7FFF) {
        slot->field_0x1C += 1;
    }
    if ((u16)slot->field_0x1E <= 0x7FFE) {
        slot->field_0x1E += 1;
    }
}

/* Switches the slot to another state byte and hands the previous one to the definition's state-2
 * entry point. */
extern "C" void eft_slot_state_set(EftSlot* slot, u8 state, _ENEMY_WORK* work, u8 index) {
    u8 previous = slot->field_0x08;
    void (*entry)(EftSlot*, u8, _ENEMY_WORK*);

    slot->field_0x08 = state;
    slot->field_0x09 = state;
    slot->field_0x0A = 0;
    slot->field_0x0B = 0;
    slot->field_0x22 = 0;
    slot->field_0x20 = 0;
    slot->field_0x05 = index;
    entry = (void (*)(EftSlot*, u8, _ENEMY_WORK*))eft_def_handler(slot, 2);
    if (entry != NULL) {
        entry(slot, previous, work);
    }
    slot->field_0x3A = 0;
    slot->field_0x3B = 255;
}

/* The external state setter: the live slots switch straight away, the others only record it. */
extern "C" void eft_slot_state_request(EftSlot* slot, u8 state, _ENEMY_WORK* work, u8 index) {
    if (eft_slot_armed_ck(slot) == 1 || eft_slot_live_ck(slot) == 1 || (eft_def_flags(slot) & 4) != 0) {
        if ((u32)(state - 4) > 1) {
            eft_slot_mode_set_defer(slot, 0xFF, 0);
        }
        eft_slot_state_set(slot, state, work, index);
        if (eft_slot_live_ck(slot) == 0 || (eft_def_flags(slot) & 4) == 0) {
            eft_net_send(slot, 1, 0);
        }
    } else {
        slot->field_0x09 = state;
    }
}

/* The byte the family's spawner stamps as the slot's effect key: the +0x10 byte in mode 3, the +0x14
 * byte otherwise. */
extern "C" u8 eft_slot_effect_key(EftSlot* slot) {
    if (slot->field_0x0F == 3) {
        return slot->field_0x10;
    }
    return slot->field_0x14;
}

/* Records a new (mode, value) pair on the slot; the previous pair is kept in the two bytes above. */
extern "C" void eft_slot_mode_set(EftSlot* slot, u8 mode, u8 value, u8 immediate) {
    if (immediate != 0 || eft_slot_live_ck(slot) == 1) {
        slot->field_0x11 = slot->field_0x0F;
        slot->field_0x12 = slot->field_0x10;
        slot->field_0x0F = mode;
        slot->field_0x10 = value;
        slot->field_0x1C = 0;
        return;
    }
    if (eft_slot_armed_ck(slot) == 1) {
        u8 previous = slot->field_0x0F;

        if (previous != mode || slot->field_0x10 != value) {
            slot->field_0x11 = previous;
            slot->field_0x12 = slot->field_0x10;
            slot->field_0x0F = mode;
            slot->field_0x10 = value;
            slot->field_0x1C = 0;
            if ((eft_def_flags(slot) & 4) == 0) {
                eft_net_send(slot, 2, 0);
            }
        }
    }
}

/* The two thin wrappers the family's spawner uses to stamp a pair with (1 = immediate). */
extern "C" void eft_slot_mode_set_imm(void* slot, u8 mode, u8 value) {
    eft_slot_mode_set((EftSlot*)slot, mode, value, 1);
}

extern "C" void eft_slot_mode_set_defer(void* slot, u8 mode, u8 value) {
    eft_slot_mode_set((EftSlot*)slot, mode, value, 0);
}

/* Maps a (mode, kind, value) triple onto the pair the slot records, then stamps it. */
extern "C" void eft_slot_mode_set_map(void* slot, u8 mode, u8 kind, u8 value, u8 immediate) {
    u8 pair_mode = 255;
    u8 pair_value = 0;

    switch (mode) {
    case 1:
        if (kind == 2) {
            pair_mode = 0;
            pair_value = value;
        }
        break;
    case 2:
        pair_mode = 1;
        pair_value = value;
        break;
    case 3:
        if (kind == 2) {
            pair_mode = 2;
            pair_value = value;
        }
        break;
    }
    if (immediate == 0) {
        eft_slot_mode_set_imm(slot, pair_mode, pair_value);
        return;
    }
    eft_slot_mode_set_defer(slot, pair_mode, pair_value);
}

/* Stamps a (kind, a, b) triple on the slot, after the enemy work's record rewrites it. */
extern "C" void eft_slot_mode_set_work(void* slot, _ENEMY_WORK* work, u8 kind, u8 a, u8 b, u8 immediate) {
    u8 first = kind;
    u8 second = a;
    u8 third = b;

    if (kind <= 4) {
        fn_8012A9E8(work, &first, &second, &third);
        eft_slot_mode_set_map(slot, first, second, third, immediate);
        return;
    }
    if (immediate == 0) {
        eft_slot_mode_set_imm(slot, 0, 0xFF);
        return;
    }
    eft_slot_mode_set_defer(slot, 0, 0xFF);
}

/* Walks the definition's per-motion table down to the 0x20-byte block the slot's effect key selects. */
extern "C" u8* eft_def_model_block(EftSlot* slot, u8 index) {
    EftDef* def;
    EftEntry* entry;
    EftEntry* model;

    if (index == 0xFF) {
        return NULL;
    }
    def = eft_def_get(slot);
    if (def == NULL) {
        return NULL;
    }
    entry = def->entries_0x04;
    if (entry == NULL) {
        return NULL;
    }
    while (entry->key_0x00 != 0xFF && fn_80125FF0(slot->field_0x13, index) != 1) {
        entry++;
    }
    if (entry->key_0x00 == 0xFF) {
        return NULL;
    }
    model = entry->sub_0x04;
    while (model->key_0x00 != 0xFF && model->key_0x00 != slot->field_0x14) {
        model++;
    }
    if (model->key_0x00 == 0xFF) {
        return NULL;
    }
    return (u8*)model->sub_0x04 + ((u32)index << 5);
}

