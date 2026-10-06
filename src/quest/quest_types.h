/*
 * The quest entry band's shared types (docs/plan.md 6.5 rule 1): the records both `quest/quest_item_slot.cpp`
 * (`.text` 0x803AA4A4..0x803AB3BC) and `quest/quest_entry.cpp` (0x803AB3BC..0x803B465C) read, in one place,
 * and the `Quest*` records the result screens share (moved from the band header `unsplit/menu.h`).
 * Each unit's own functions are declared in its own header.
 */
#ifndef MHTRI_QUEST_QUEST_TYPES_H
#define MHTRI_QUEST_QUEST_TYPES_H

#include "types.h"
#include "quest/quest_result_work.h"   /* Q_QuestStat, Q_ResultWork (shared with menu/ and lobby/) */

/* The player work record `quest_item_work_merge`'s first argument points at (defined in `pl.h`);
 * nothing here reads it, so the forward declaration is all this header needs. */
struct _PLW;

/* The 4-byte `(item id, count)` slot `item_pair_copy` moves - and the record the lot picks write
 * their output with: a u16 id and a u16 count (the pick stores a sign-extended byte payload). */
struct Q_ItemPair {
    /* +0x0 */ u16 id;
    /* +0x2 */ u16 num;
};  /* size: 0x4 */

/* The same 4-byte slot as the item-slot search reads it: the count is a signed byte. */
struct Q_ItemCount {
    /* +0x0 */ u16 id;
    /* +0x2 */ s8 num;
    /* +0x3 */ u8 unused_0x3;
};  /* size: 0x4 */

/* A record/count block of the save data and of the item work: `count` entries followed by the four
 * per-group totals an index of 0x24..0x27 sums. */
struct Q_CountSet {
    /* +0x00 */ u16 count[0x24];
    /* +0x48 */ u16 total[4];
};  /* size: 0x50 */

/* One 4-byte entry of the weighted lot tables the pick helpers walk (`u16` key, one payload byte and
 * one weight byte); an entry whose key is 0 ends the table. */
struct Q_LotEntry {
    /* +0x0 */ u16 id;
    /* +0x2 */ u8 value;
    /* +0x3 */ u8 weight;
};  /* size: 0x4 */

/* One 8-byte slot pair of the result rows: the id a pick is matched against, the 0x30-byte group it
 * indexes, and the two counts the pick helper walks. */
struct Q_SlotPair {
    /* +0x0 */ u8 id_0x00;
    /* +0x1 */ u8 group_0x01;
    /* +0x2 */ u8 kind_0x02;
    /* +0x3 */ u8 count_0x03;
    /* +0x4 */ u8 pad_0x04;
    /* +0x5 */ u8 pad_0x05;
    /* +0x6 */ u8 pad_0x06;
    /* +0x7 */ u8 pad_0x07;
};  /* size: 0x8 */

/* One arena item of a result row: the item `id`, the `count` asked for and the `remaining` count. */
struct Q_ArenaItem {
    /* +0x0 */ u16 id;
    /* +0x2 */ u16 count;
    /* +0x4 */ u16 remaining;
};  /* size: 0x6 */

/* The result row `Q_ItemWork::record_0x3C` points at, as this unit reads it: only the word at +0x310,
 * whose bit 0x00800000 marks a live row.  `quest/quest_entry.cpp`'s own view of the same record
 * (`QuestRecord`, below) names the rest of it; merging the two views is a follow-up. */
struct Q_ResultRow {
    /* +0x000 */ u8 pad_0x000[0x8B];
    /* +0x08B */ u8 state_0x8B;     /* the row's state byte: 7 marks a finished quest */
    /* +0x08C */ u8 pad_0x08C[0x310 - 0x8C];
    /* +0x310 */ u32 flags_0x310;   /* bit 0x00800000: the row is live */
    /* +0x314 */ u8 pad_0x314[0x372 - 0x314];
    /* +0x372 */ u16 item_id_0x372;  /* the quest's item id, 0 when it has none */
    /* +0x374 */ u16 kind_0x374;     /* the row's quest kind (stored into the item work's +0x8B) */
    /* +0x376 */ u8 pad_0x376[0x37C - 0x376];
    /* +0x37C */ u32 area_ofs_0x37C; /* byte offset of the row's area block from the row, 0 when it has none */
    /* +0x380 */ u8 pad_0x380[0x384 - 0x380];
    /* +0x384 */ Q_ArenaItem items_0x384[2];
};  /* size: 0x390 (a prefix of the 0x714-byte row) */

/* One 0x60-byte element of the quest view of the work block (`Q_ItemWork::elements_0x94`): `flags` gates
 * it, `id`/`value` are what the callers match on.  Only those three are named here. */
struct Q_Element {
    /* +0x00 */ u32 flags;
    /* +0x04 */ u16 id;
    /* +0x06 */ u16 value;
    /* +0x08 */ u8 pad_0x08[0x58];
};  /* size: 0x60 */

/* The (flag byte, value byte) pair `quest_grade_set` keeps per element. */
struct Q_GradePair {
    /* +0x0 */ u8 flag;
    /* +0x1 */ s8 value;
};  /* size: 0x2 */

/* The item work (`get_move_work_adrs(0)->0xDC`), the same block `quest_work_ptr` points at; only the
 * offsets this unit names are here (the lot tables' run at 0x9C is one region, addressed but not
 * walked by name).  MEASURED size 0x6AB8 (`accessextent.py quest_work` / `quest_work_ptr`):
 *   * literal - `quest_init` clears the whole block, `memset(quest_work_ptr, 0, 0x6AB8)` at 0x803AD4BC,
 *     and `fn_803B7F70` does the same at 0x803B7FA0 (`accessextent.py quest_work`, the object form);
 *   * cross-check - the block's readers reach 0x6AB4 at their furthest (`fn_803AEED0`'s `stb r0,27316`),
 *     i.e. 0x6AB8 rounded up, over 364 resolved accesses in 38 functions;
 *   * the tool's own DISAGREEMENT line here is the OTHER literal: `memcpy(quest_work_ptr, ..., 0x6000)`
 *     at 0x803AD680 copies the 0x6000-byte save sub-block INTO the record, so 0x6000 is not its extent.
 * The .bss row tiles the record exactly (`quest_text_buffer` begins at 0x806CC310 = 0x806C5858 + 0x6AB8).
 *
 * The 0x94..0x490 union: the quest view (three `Q_Element`s) and the arena view (weighted lot tables) are
 * told apart by the entry kind byte of the slot's move work, `Q_MoveWork::kind_0xFC` - 4 is the arena entry.
 * Evidence: `quest_start_enter` branches on `work->kind_0xFC == 4` for the arena message and time limit, and
 * `quest_element_build`/`quest_element_clear` return unless the caller's kind is 4 before they walk
 * `lot_0x9C`; the hunt paths (`quest_grade_set`, `quest_element_find`) read `elements_0x94` only. */
struct Q_ItemWork {
    /* +0x0000 */ u8 field_0x00;
    /* +0x0001 */ u8 field_0x01;
    /* +0x0002 */ u8 pad_0x0002[0x12];
    /* +0x0014 */ u8 field_0x14;
    /* +0x0015 */ u8 pad_0x0015[0x1];
    /* +0x0016 */ u8 field_0x16;
    /* +0x0017 */ u8 pad_0x0017[0x9];
    /* +0x0020 */ s32 time_base_0x20;  /* the time limit the screen counts down from (copied from +0x24) */
    /* +0x0024 */ s32 time_limit_0x24;  /* the quest time limit in frames */
    /* +0x0028 */ u8 pad_0x0028[0x4];
    /* +0x002C */ u8 step_0x2C;  /* the entry/result states set it to 2; the lobby acts bump it when they take over */
    /* +0x002D */ u8 field_0x2D;
    /* +0x002E */ u8 pad_0x002E[0xE];
    /* +0x003C */ Q_ResultRow* record_0x3C;  /* the current result row, 0 when there is none */
    /* +0x0040 */ Q_ItemCount slots_0x40[5];  /* the five delivered-item slots `quest_item_slot_add` merges into */
    /* +0x0054 */ u8 rot_0x54;  /* the round-robin slot the next new id takes */
    /* +0x0055 */ u8 flag_0x55;  /* set when a slot changed */
    /* +0x0056 */ u8 pad_0x0056[0x4];
    /* +0x005A */ u16 rand_state_0x5A;  /* the quest random source's state halfword */
    /* +0x005C */ s32 field_0x5C;
    /* +0x0060 */ s32 field_0x60;
    /* +0x0064 */ u8 pad_0x0064[0x8];
    /* +0x006C */ u16 rand_words_0x6C[15];  /* the clock snapshot words the seed sums */
    /* +0x008A */ u8 pad_0x008A[0x6];
    /* +0x0090 */ s8 field_0x90;
    /* +0x0091 */ u8 faint_count_0x91;  /* faints so far (all players) */
    /* +0x0092 */ u8 my_faint_count_0x92;  /* faints of the local player */
    /* +0x0093 */ u8 grade_0x93;  /* the quest grade `quest_grade_set` derives */
    /* +0x0094 */ union {
        struct {   /* the quest view of the block */
            /* +0x0094 */ Q_Element elements_0x94[3];  /* the three quest elements */
            /* +0x01B4 */ u8 pad_0x01B4[0x120];
            /* +0x02D4 */ u8 armed_0x2D4[3];  /* per-element "armed" bytes `quest_monsters_release` walks */
            /* +0x02D7 */ u8 pad_0x02D7[0x11];
            /* +0x02E8 */ s32 reward_0x2E8;  /* the quest reward left to lose */
            /* +0x02EC */ u8 pad_0x02EC[0x8];
            /* +0x02F4 */ s32 penalty_0x2F4;  /* the reward one faint costs */
            /* +0x02F8 */ u8 pad_0x02F8[0x10];
            /* +0x0308 */ u8 result_kind_0x308;
            /* +0x0309 */ u8 pad_0x0309[0x3];
            /* +0x030C */ u8 field_0x30C;
            /* +0x030D */ u8 pad_0x030D[0x14];
            /* +0x0321 */ u8 respawn_kind_0x321;  /* the monster id the finish step respawns (0 = none) */
            /* +0x0322 */ u8 pad_0x0322[0x1];
            /* +0x0323 */ u8 respawn_gate_0x323;  /* the finish step respawns only while this is 3 */
            /* +0x0324 */ u8 pad_0x0324[0x148];
            /* +0x046C */ u16 slot_key_0x46C[3];  /* three slot keys, 0xFFFF when empty */
            /* +0x0472 */ u8 pad_0x0472[0x1E];
        };
        struct {   /* the arena view: the same bytes as weighted lot tables */
            /* +0x0094 */ u8 pad_0x0094[0x8];
            /* +0x009C */ union {
                /* +0x009C */ Q_LotEntry lot_0x9C[0xFD];  /* the first weighted lot table */
                struct {
                    /* +0x009C */ Q_LotEntry lot_e0a_0x9C[0xB];   /* element 0's own table (see `lot_e0b`) */
                    /* +0x00C8 */ Q_LotEntry lot_e0b_0xC8[0xD];   /* its second table: `quest_pair_roll_first` walks both */
                    /* +0x00FC */ Q_LotEntry lot_e1_0xFC[0x18];   /* element 1's table */
                    /* +0x015C */ Q_LotEntry lot_e2_0x15C[0xCD];  /* element 2's table */
                };
                /* +0x009C */ u8 pad_0x009C[0x3F4];
            };
        };
    };
    /* +0x0490 */ Q_CountSet set_c;
    /* +0x04E0 */ u8 pad_0x04E0[0x2];
    /* +0x04E2 */ Q_CountSet set_d;
    /* +0x0532 */ u8 pad_0x0532[0xA6];
    /* +0x05D8 */ u16 score_0x5D8;  /* the run's point score */
    /* +0x05DA */ u8 pad_0x05DA[0xB5];
    /* +0x068F */ Q_GradePair grade_pair_0x68F[3];  /* per-element grade pairs (flag byte, value byte) */
    /* +0x0695 */ u8 pad_0x0695[0x60C7];
    /* +0x675C */ s32 hunt_active_0x675C;  /* non-zero while a hunt is running */
    /* +0x6760 */ u8 release_kind_0x6760;  /* the kind released when the hand-off has no armed element */
    /* +0x6761 */ u8 pad_0x6761[0x17];
    /* +0x6778 */ Q_ItemPair pouch_0x6778[35];  /* the carried-item pouch: 0x18 + 8 pairs, then the flag bytes */
    /* +0x6804 */ u8 pad_0x6804[0x173];
    /* +0x6977 */ u8 hunt_end_0x6977;  /* set when the quest hand-off has released its monsters */
    /* +0x6978 */ u8 phase_0x6978;  /* the finish step's phase: 0 wait, 1/2 respawn, 3 done */
    /* +0x6979 */ u8 pad_0x6979[0x3];
    /* +0x697C */ s32 counter_0x697C;  /* frames the finish step has waited */
    /* +0x6980 */ u8 pad_0x6980[0x25];
    /* +0x69A5 */ u8 area_count_0x69A5;  /* how many area records `area_0x69A8` holds */
    /* +0x69A6 */ u8 pad_0x69A6[0x2];
    /* +0x69A8 */ u32 area_0x69A8[32];  /* the area entry handles `em_area_entry_make` returns */
    /* +0x6A28 */ u8 area_state_0x6A28;  /* 1 until `quest_area_list_refill` rebuilds the list (then 2) */
    /* +0x6A29 */ u8 flag_0x6A29;  /* cleared by the area list build; the lobby sets it when the entry reaches its limit */
    /* +0x6A2A */ s8 count_0x6A2A;  /* entries in the arena item table below */
    /* +0x6A2B */ u8 pad_0x6A2B[0x1];
    /* +0x6A2C */ Q_ArenaItem arena_items_0x6A2C[2];  /* the two arena items the record carries */
    /* +0x6A38 */ u8 pad_0x6A38[0x2];
    /* +0x6A3A */ u8 entry_send_0x6A3A[7];  /* the quest entry-send header block */
    /* +0x6A41 */ u8 pad_0x6A41[0x77];
};  /* size: 0x6AB8 */

/* The game's save/user block, seen only as the four count blocks this unit reads.  It is the arena
 * buffer `get_userdata` hands out (`*(void**)(&system_w + 0x95C)`, set by `ef/system_core.cpp`'s
 * `fn_800CF3E4`), and 0x6000 is the whole extent `fn_80047398.cpp`'s `fn_800497B4`/`fn_800498EC`
 * clear with `memset(get_userdata(), 0, 0x6000)`.  Nothing reads at or past it: of the 344 functions
 * that reach the block the furthest is `menu/get_pop_dat_ptr.cpp`'s `fn_803C0F3C`, at `+0x5364` for
 * sixteen 2-byte entries, so the run past the count sets stays unnamed filler. */
struct Q_UserData {
    /* +0x0000 */ u8 pad_0x0000[0x490];
    /* +0x0490 */ Q_CountSet set_c;
    /* +0x04E0 */ u8 pad_0x04E0[0x2];
    /* +0x04E2 */ Q_CountSet set_d;
    /* +0x0532 */ u8 pad_0x0532[0x348E];
    /* +0x39C0 */ Q_CountSet set_a;
    /* +0x3A10 */ u8 pad_0x3A10[0xB0];
    /* +0x3AC0 */ Q_CountSet set_b;
    /* +0x3B10 */ u8 pad_0x3B10[0x6000 - 0x3B10];
};  /* size: 0x6000 */

/* The per-slot "move work" `get_move_work_adrs` indexes; only the fields this unit names are here.
 * MEASURED size 0x22E8 (was 0x22E0 approximate): the record is the stride its own allocator clears -
 * `ef/system_core.cpp`'s `fn_800CF948` sizes slot 0's block as `max * 0x22E8` (`mulli r30,r3,8936` at
 * 0x800CF9D4) and memsets exactly that (0x800CFA68), and no other site in the DOL indexes a move work by
 * a different constant (`grep 8936` = 1 site).  The literal and the reach disagree by 4 bytes, which is
 * the interesting case: the furthest access anything makes is +0x22E3 (`lb_area_change_flag`'s
 * `stb r0,8931(r3)`, the lobby band's own `flag_0x22E3`), so the inferred reach is 0x22E4 and the last 4
 * bytes are cleared but never read.  The literal decides - it is what the allocator reserved and what
 * `max` multiplies - so the tail stays filler. */
struct Q_MoveWork {
    /* +0x0000 */ u8 pad_0x0000[0xDC];
    /* +0x00DC */ Q_ItemWork* item_work;   /* the item work `move_work_item_work_get` returns */
    /* +0x00E0 */ u8 pad_0x00E0[0x9];
    /* +0x00E9 */ u8 phase_0xE9;           /* the quest phase byte `quest_phase_get` returns */
    /* +0x00EA */ u8 sub_0xEA;             /* the phase's sub index `quest_area_spawn_apply` takes */
    /* +0x00EB */ u8 pad_0x00EB[0xFA - 0xEB];
    /* +0x00FA */ u8 sub_0xFA;              /* the sub-state `quest_move_sub_state_ck` tests */
    /* +0x00FB */ u8 pad_0x00FB[1];
    /* +0x00FC */ u8 kind_0xFC;            /* the entry kind (4 = arena entry) */
    /* +0x00FD */ u8 pad_0x00FD[0x10C - 0xFD];
    /* +0x010C */ u16 item_id_0x10C;         /* the item `quest_item_work_merge` records */
    /* +0x010E */ s16 item_value_0x10E;      /* its handed-over value */
    /* +0x0110 */ u8 item_flag_0x110;        /* set while that record is live */
    /* +0x0111 */ u8 pad_0x0111[0x113 - 0x111];
    /* +0x0113 */ u8 state_0x113;           /* 1 while the slot is in its entry state */
    /* +0x0114 */ u8 pad_0x0114[0x22D4 - 0x114];
    /* +0x22D4 */ u8 state_0x22D4;          /* the state byte `quest_move_state_valid_ck`/`_get` read */
    /* +0x22D5 */ u8 pad_0x22D5[0x4];
    /* +0x22D9 */ u8 sub_flag_0x22D9;      /* set to 1 by `quest_move_sub_state_4_set` when its flag argument is 1 */
    /* +0x22DA */ u8 pad_0x22DA[0x2];
    /* +0x22DC */ u8 flag_0x22DC;
    /* +0x22DD */ u8 pad_0x22DD[0x22E8 - 0x22DD];  /* cleared, never read: the lobby's own view names
                                                   * +0x22E3, this one reads up to +0x22DC */
};  /* size: 0x22E8 */

/* One 3-byte entry of the reward-group table `fn_803ABE44` indexes by a group count (clamped to the
 * last entry) and reads `picks`/`flag` from: the band's own roll table at `.data` 0x805F7AC0. */
struct Q_RewardGroup {
    /* +0x0 */ u8 index;   /* the entry's own number, 0..5 */
    /* +0x1 */ u8 picks;   /* how many roll slots the group fills */
    /* +0x2 */ u8 flag;    /* the pair kind the rolls are stored with */
};  /* size: 0x3 */

/* The 8-byte slot pair `quest_pair_copy` moves and the 0x20-byte key block `quest_element_copy`
 * moves; both are the target's own byte-by-byte copies. */
struct Q_PairBlock {
    /* +0x00 */ u8 byte_0x00;
    /* +0x01 */ u8 byte_0x01;
    /* +0x02 */ u8 byte_0x02;
    /* +0x03 */ u8 byte_0x03;
    /* +0x04 */ u8 byte_0x04;
    /* +0x05 */ u8 byte_0x05;
    /* +0x06 */ u8 byte_0x06;
    /* +0x07 */ u8 byte_0x07;
};  /* size: 0x8 */

struct Q_ElementBlock {
    /* +0x00 */ u8 byte_0x00;
    /* +0x01 */ u8 byte_0x01;
    /* +0x02 */ u8 byte_0x02;
    /* +0x03 */ u8 byte_0x03;
    /* +0x04 */ u8 byte_0x04;
    /* +0x05 */ u8 byte_0x05;
    /* +0x06 */ u8 byte_0x06;
    /* +0x07 */ u8 byte_0x07;
    /* +0x08 */ u32 word_0x08;
    /* +0x0C */ u32 word_0x0C;
    /* +0x10 */ u32 word_0x10;
    /* +0x14 */ u32 word_0x14;
    /* +0x18 */ u32 word_0x18;
    /* +0x1C */ u32 word_0x1C;
};  /* size: 0x20 */

/* The arena element record `quest_element_build` fills and the pick's output lands in: the 0x40-byte
 * payload the builder clears, and the sub-flag byte just past it that its caller arms.  Only the two
 * offsets this band touches are named. */
struct Q_ArenaElement {
    /* +0x000 */ u8 pad_0x000[0x3F4];
    /* +0x3F4 */ Q_ItemPair payload_0x3F4[0x10];  /* 0x40 B, the pick's own output region */
    /* +0x434 */ u8 flag_0x434;
};  /* size: 0x435 (approximate: the highest offset this band reads + 1) */

/* ---- the arena/quest-result records (the `Quest*` view) ----
 *
 * The records `src/quest/quest_entry.cpp` and the result-screen units read: `quest_work` is the 0x6AB8-byte
 * block `quest_init` (0x803AD47C) memsets and stores into `quest_work_ptr`.  `QuestWork` and `Q_ItemWork`
 * above (and `lobby/lb_companion_ui.h`'s `LbCompanionWork`) are three views of that one block.
 */
/* One item slot of the result screen's per-player item list. size: 0x4 */
typedef struct QuestItemSlot {
    /* +0x000 */ u16 id;      /* the item id the count belongs to */
    /* +0x002 */ s16 count;
} QuestItemSlot;
/* One 0x6-byte entry of the arena item table at `quest_work.arena_items_0x6A2C`: the item `id`, the
 * `count` the caller matches on, and the `remaining` count `quest_element_item_apply` spends.
 * size: 0x6 */
typedef struct QuestArenaItem {
    /* +0x000 */ u16 id;
    /* +0x002 */ u16 count;
    /* +0x004 */ u16 remaining;
} QuestArenaItem;
/* One 0x8-byte entry of the result record's first per-slot run (+0x314): the slot's `value` byte and the
 * `armed` byte that says it is set.  size: 0x8 */
typedef struct QuestRecordSlotByte {
    /* +0x0 */ u8 value;
    /* +0x1 */ u8 unused_0x1[0x2];
    /* +0x3 */ u8 armed;
    /* +0x4 */ u8 unused_0x4[0x4];
} QuestRecordSlotByte;
/* One 0x8-byte entry of the result record's second per-slot run (+0x330): the slot's state `flags` and
 * its `value` halfword.  size: 0x8 */
typedef struct QuestRecordSlotWord {
    /* +0x0 */ u32 flags;
    /* +0x4 */ u16 value;
    /* +0x6 */ u8 unused_0x6[0x2];
} QuestRecordSlotWord;
/* One 0x60-byte entry of the result record's third run (+0x394); only its leading word is read. size: 0x60 */
typedef struct QuestRecordRow {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ u8 unused_0x04[0x5C];
} QuestRecordRow;
/* The record `quest_work.pad_0x03C` points at: one player's quest/arena result row.  Only the
 * offsets this band reads are named, and the whole tail below +0x372 is not evidenced.
 * size: 0x714 (lower bound) */
typedef struct QuestRecord {
    /* +0x000 */ char field_0x000[0x02C];  /* the leading text run: the record's own name, which
                                             * `quest_result_field_text_get`'s field kinds 0/22 copy */
    /* +0x02C */ u16 field_0x02C;     /* the clear time in frames (compare against 0x2710/0x2328) */
    /* +0x02E */ char field_0x02E[0x05C];  /* the second text run, rendered by field kinds 1/16/17 */
    /* +0x08A */ u8 category_0x08A;   /* the quest category + 1 (`arena_quest_info_build` stores it minus one) */
    /* +0x08B */ u8 field_0x08B;      /* index into string table 5 */
    /* +0x08C */ char field_0x08C[0x029];  /* field kind 2's text run (when the flag test fails) */
    /* +0x0B5 */ char field_0x0B5[0x029];  /* field kind 3's text run */
    /* +0x0DE */ char field_0x0DE[0x05C];  /* field kind 4's text run */
    /* +0x13A */ s16 field_0x13A;     /* formatted through string table 6's format 0 */
    /* +0x13C */ char field_0x13C[0x05C];  /* field kind 5's text run */
    /* +0x198 */ u8 field_0x198;      /* index into string table 41 */
    /* +0x199 */ u8 unused_0x199[0x001];
    /* +0x19A */ char field_0x19A[0x02F];  /* field kind 7's text run */
    /* +0x1C9 */ char field_0x1C9[0x143];  /* field kind 8's text run */
    /* +0x30C */ u8 field_0x30C[2];  /* the two per-slot monster ids (bytes 0/1)
                                      * `quest_monster_text_get` renders through string table 33 */
    /* +0x30E */ u8 unused_0x30E[0x002];
    /* +0x310 */ u32 flags_0x310;    /* the row's flag word the `quest_flag_*_ck` predicates test */
    /* +0x314 */ QuestRecordSlotByte slot_bytes_0x314[3];   /* slot 0's byte is the arena quest-info row's flag byte */
    /* +0x32C */ u8 field_0x32C;
    /* +0x32D */ u8 unused_0x32D;
    /* +0x32E */ u8 field_0x32E;
    /* +0x32F */ u8 unused_0x32F;
    /* +0x330 */ QuestRecordSlotWord slot_words_0x330[3];
    /* +0x348 */ s32 field_0x348;
    /* +0x34C */ s32 field_0x34C;
    /* +0x350 */ s32 field_0x350;
    /* +0x354 */ s32 field_0x354;
    /* +0x358 */ u8 unused_0x358[0x014];
    /* +0x36C */ u16 field_0x36C;     /* `quest_slot_progress_get` returns its low byte */
    /* +0x36E */ u8 unused_0x36E[0x004];
    /* +0x372 */ u16 field_0x372;
    /* +0x374 */ u8 unused_0x374[0x006];
    /* +0x37A */ u16 field_0x37A;
    /* +0x37C */ u8 unused_0x37C[0x018];
    /* +0x394 */ QuestRecordRow rows_0x394[3];
    /* +0x4B4 */ u32 acdata_ofs_0x4B4;  /* byte offset from the record to its 0xA0-byte-per-player acdata equip records (`dl_acdata_to_ar_eqdata`) */
    /* +0x4B8 */ u8 unused_0x4B8[0x25C];
} QuestRecord; /* size: 0x714 (lower bound) */
/* One 0x60-byte entry of the quest work block's element array at `quest_work` +0x94: `flags` gates
 * the entry, `id` is the u16 the callers match on and `value` is the count/target they compare.
 * `key_bytes` is the same +0x04..+0x06 pair as three bytes - `quest_players_state_get` reads the
 * three bytes of entry 6. size: 0x60 */
typedef struct QuestElement {
    /* +0x00 */ u32 flags;
    union {
        struct {
            /* +0x04 */ u16 id;
            union {
                /* +0x06 */ s16 value;
                /* +0x06 */ u16 target_count;   /* the same halfword as the arena passes read it */
            };
        };
        /* +0x04 */ u8 key_bytes[3];
    };
    /* +0x08 */ u8 unused_0x08[0x58];
} QuestElement;
/* One 0x8-byte entry of the work block's key run at +0x319: only its leading key byte is read. size: 0x8 */
typedef struct QuestKeyRow {
    /* +0x0 */ u8 key;
    /* +0x1 */ u8 unused_0x1[0x7];
} QuestKeyRow;
/* The quest/arena work block (`quest_work`, .bss 0x806C5858, 0x6AB8 B - the size `quest_init`
 * memsets).  `quest_work_ptr` (.sbss 0x80794C40) is the same object's address.  Only the offsets
 * the band reads are named, in ascending order. size: 0x6AB8 */
typedef struct QuestWork {
    /* +0x0000 */ u8 unused_0x0000[0x010];
    /* +0x0010 */ u16 stat_word_0x10;           /* `quest_result_stat_fill` stores it into the result stat's last word */
    /* +0x0012 */ u8 unused_0x0012[0x00A];
    /* +0x001C */ s32 field_0x01C;              /* a clear-time-like pair with +0x020 */
    /* +0x0020 */ s32 field_0x020;
    /* +0x0024 */ s32 field_0x024;
    /* +0x0028 */ u8 unused_0x0028[0x014];
    /* +0x003C */ QuestRecord* record_0x03C;    /* the current result row, 0 when there is none */
    /* +0x0040 */ u8 unused_0x0040[0x050];
    /* +0x0090 */ s8 field_0x090;               /* the two bytes `sprintf` formats as the score */
    /* +0x0091 */ s8 field_0x091;
    /* +0x0092 */ u8 unused_0x0092[0x002];
    /* +0x0094 */ QuestElement elements_0x0094[3];
    /* +0x01B4 */ QuestElement arena_elements_0x01B4[3];   /* the same 0x60-stride array's entries 3..5
                                                          * (entry 6's +0x04..+0x06 is +0x2D4 below) */
    /* +0x02D4 */ u8 player_state_0x2D4[3];     /* entry 6's three key bytes */
    /* +0x02D7 */ u8 unused_0x02D7[0x011];
    /* +0x02E8 */ s32 field_0x2E8;              /* clamped to 0 before it is formatted */
    /* +0x02EC */ u8 unused_0x02EC[0x02D];
    /* +0x0319 */ QuestKeyRow key_rows_0x0319[3];
    /* +0x0331 */ u8 unused_0x0331[0x187];
    /* +0x04B8 */ u16 stat_gate_0x4B8;          /* non-zero clears the result stat instead of filling it */
    /* +0x04BA */ u8 unused_0x04BA[0x11E];
    /* +0x05D8 */ u16 field_0x5D8;              /* the run's point score */
    /* +0x05DA */ u8 unused_0x05DA[0x60BE];
    /* +0x6698 */ u16* slot_values_0x6698[8];   /* the eight u16 stacks `quest_slot_items_get` copies */
    /* +0x66B8 */ s32 slot_counts_0x66B8[8];
    /* +0x66D8 */ u8 unused_0x66D8[0x080];
    /* +0x6758 */ s32 field_0x6758;             /* quest_init stores 0x19 here */
    /* +0x675C */ u8 unused_0x675C[0x004];
    /* +0x6760 */ u8 field_0x6760[8];
    /* +0x6768 */ u8 unused_0x6768[0x09B];
    /* +0x6803 */ u8 item_bytes_0x6803[0x149];  /* byte table `quest_arena_need_get` reads at [id * 8 + value] */
    /* +0x694C */ u8 key_bits_0x694C[0x029];    /* per-id bit masks `quest_arena_key_clear` tests against `1 << value` */
    /* +0x6975 */ u8 field_0x6975;             /* the screen's active flag */
    /* +0x6976 */ u8 unused_0x6976[0x002];
    /* +0x6978 */ u8 field_0x6978;             /* the screen phase the dispatchers switch on */
    /* +0x6979 */ u8 unused_0x6979[0x003];
    /* +0x697C */ s32 field_0x697C;
    /* +0x6980 */ u8 unused_0x6980[0x0AA];
    /* +0x6A2A */ s8 count_0x6A2A;             /* entries in the arena item table below */
    /* +0x6A2B */ u8 unused_0x6A2B;
    /* +0x6A2C */ QuestArenaItem arena_items_0x6A2C[2];
    /* +0x6A38 */ u8 unused_0x6A38[0x002];
    /* +0x6A3A */ u8 entry_send_0x6A3A[7];      /* the quest entry-send header block */
    /* +0x6A41 */ u8 unused_0x6A41[0x029];
    /* +0x6A6A */ QuestItemSlot player_items_0x6A6A[4][3];
    /* +0x6A9A */ u8 unused_0x6A9A[0x00A];
    /* +0x6AA4 */ u8 field_0x6AA4[0x014];      /* the run `quest_field6AA4_get` hands out */
} QuestWork;
/* `quest_work`, `quest_work_ptr`, `quest_text_buffer`, the band's data tables and the three `quest_list_*`
 * globals are declared in `quest/quest_entry.h`, their owner's header. */
/* One entry of the `quest_list_items` array (`quest_work_word_get` walks it). */
typedef struct QuestListItem {
    /* +0x02C */ u16 field_0x02C;
    /* +0x02E */ u8 unused_0x02E[0x002];
} QuestListItem; /* size: 0x30 (lower bound) */

#endif /* MHTRI_QUEST_QUEST_TYPES_H */
