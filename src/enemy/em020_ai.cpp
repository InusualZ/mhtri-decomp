/*
 * enemy/em020_ai.cpp - the tail of the em020 monster-AI file, `.text` 0x80375424..0x80378F9C
 * (78 functions / 0x3B78 B) with extab 0x80017A74..0x80017C44 (58 records) and extabindex
 * 0x80037614..0x800378CC (58 x 12 B).
 *
 * WHAT IT IS.  Monster-AI code of the em020 program.  Every body takes the shared `_ENEMY_WORK`
 * record (`include/enemy/ENEMY_WORK.h`) and drives it through the enemy core API -
 * `em_frame_check` (124 calls), `em_parts_damage_level_get`, `em_magma_check`, `get_em_chg_scale`,
 * `get_joint_wpos_em`, `em_mot_set`/`em_mot_set_ck`/`em_mot_end_ck` - and through the game's work
 * blocks `system_w` (38 calls), `lobby_w` (45), `get_move_work_adrs`, `my_player_no`,
 * `work_mem_alloc`/`work_mem_free`.
 *
 * MODULE AND NAME (brief section 2, evidence order).  1. No `__FILE__` string is reachable from the
 * range: every `lis`/`addi` pair and every `lbl_` reference in its 78 split objects resolves to the
 * `.data` program tables, the `.bss`/`.sbss` work blocks, the shared `.sdata2` float pool or a call -
 * never to a source-file-name literal (checked by reading every relocation target of the range's
 * objects out of the DOL: the only string in its own `.data` run is a Japanese network message at
 * 0x805EE428).  2. `dumpmap.py lookup` answers `zz_<addr>_` for every address in the range except the
 * two `em0XX_prog_tbl` rows below.  3. The module is `enemy` from the `.data` program table
 * `em020_prog_tbl` (0x805EE098, `scope:global`, size 0x70) whose entry-point list is this band's own
 * functions - `fn_8036E2BC`/`fn_8036E320`/`fn_8036E6B8`/`fn_8036E570`/`fn_8036E574`/`fn_8036E...`,
 * `fn_80375084` (+0x20), `fn_80375290` (+0x24), `fn_803753A0` (+0x34), `fn_80375424` (+0x3C),
 * `fn_80375494` (+0x58) - and from the code (`_ENEMY_WORK` field for field, `em_*` callees only).
 * The file is therefore named for the program the table names (`em020`), on the module's `em*`
 * scheme (`em024_ai.cpp`, `em035_prog.cpp`).
 *
 * SEAM (unproven - the range is an `attribute.py` `--max-bytes` run, not a TU boundary).
 * `tudiscover.py at 0x80375424` reports the 7-function match set 0x80375424..0x803757E0 with a strong
 * `.sdata2` seam at 0x80375290 (outside the brief's range, and explained by the mergeable-constant
 * pool: 0x8079BC68..0x8079BC88 is referenced from both sides, so it is a shared constant run, not an
 * object boundary), and a weak right boundary.  The evidence that does pin this file's right edge is
 * the `.data` block boundary plus the call closure: `fn_80378464`'s two jump tables
 * (0x805EE4B8..0x805EE514) are the last `.data` of the em020 block, `em019_prog_tbl` starts the next
 * block at 0x805EE518, and `fn_80378F7C` (the last function here) is called only from 0x8036C284 /
 * 0x8036C6E8 - both em020-side - while `fn_80378F9C` (the first function of `enemy/em019_ai.cpp`) is
 * called only from 0x8037939C upward.  This file's left edge (0x80375424) is FALSE: `em020_prog_tbl`
 * references functions at 0x8036E2BC..0x80375290, so the original em020 file starts well before the
 * brief's range; the head is left to its own lane and filed as a `config_requests` `range` entry.
 *
 * Naming note: the names this file *references* in other units are still the map's generated
 * `fn_XXXXXXXX` stems (the enemy core band 0x8012xxxx/0x8013xxxx and the game-root 0x8042xxxx band,
 * checked with `tools/symbols/symedit.py range`); every symbol this file DEFINES is named from its
 * own body and renamed in the map with `symedit.py rename`.
 *
 * Sections this unit claims: `.text` 0x80375424..0x80378F9C, extab 0x80017A74..0x80017C44,
 * extabindex 0x80037614..0x800378CC, and the `.data` run its own jump tables occupy
 * (0x805EE490..0x805EE518).
 *
 * Residuals: the range is registered as `NonMatching`; the bodies still unwritten keep the map's
 * `fn_XXXXXXXX` names, and the ones written but not yet byte-identical are listed in the outbox with
 * their first divergence.  Re-measure with `ninja build/RMHE08/report.json` +
 * `python tools/objdiff/symdiff.py -u enemy/em020_ai.cpp <symbol>`.
 */


#include "types.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/em020_ai.h"
#include "enemy/fn_8011D448.h"
#include "enemy/fn_801251D0.h"
#include "enemy/fn_80138074.h"
#include "stage/fn_802B2AA0.h"
#include "fn_80423E74.h"
#include "sys_mem.h"
#include "stage/stg_w.h"

/* `stage/fn_802B2AA0.h` used to open a file-wide `#pragma peephole off` that this unit picked up by
 * including it.  Measured: with the leak gone this unit drops (unit fuzzy 5.168 -> 4.893;
 * `em020_aim_target_ck` 100 -> 40, `em020_hit_info_get` 100 -> 96.38, `em020_work_free` 100 -> 95.43,
 * `em020_quest_pages_clear` 100 -> 90.83, `em020_row_ptr` 100 -> 80), so the unit owns the pass and
 * states it here.  A codegen pragma in the shared header is a matching hazard (stylelint rule 10). */
#pragma peephole off


/* The shared `.bss` lobby state block `lbl_806BF530` (0x806BF530, 0x2EB8 B): this unit reads its
 * `+0x03` quest-active byte, and the lobby's `lb_npc.cpp` reads the same byte.  Its own home is the
 * unclaimed `.bss` blob, so the declaration stays here until a unit owns the block (rule 2's unsplit
 * case, like `lbl_806BE340`). */
extern u8 lbl_806BF530[];
/* The shared `.bss` quest-page block (0x806BE340, ten 0x130-byte records) whose +0x03 byte
 * `em020_quest_page_ptr` returns the address of. */
extern u8 lbl_806BE340[];
/* The `.sbss` one-byte flag `em020_unknown_flag_set` writes. */
extern u8 lbl_80794BF4;
/* The pooled `.sdata2` constants this unit loads through `r2`.  Declared, never defined (playbook
 * 29/58): a definition would make MWCC emit a second copy and grow `.sdata2` instead of pairing. */
extern f32 lbl_8079BC6C; /* 0.65f */

#ifdef __cplusplus
extern "C" {
#endif

/* --- this unit's own entry points (renamed in the map in the same batch) --- */

/* The em020 area hit's damage-level gate: every part's damage level is folded into `out->levels_0x01`
 * and the enemy's current facing angle and damage numerator are copied out.
 * 0x80375540 */
void em020_hit_info_get(struct _ENEMY_WORK* self, struct Em020HitInfo* out)
{
    if (self->area_no == 3) {
        out->hit_0x00 = 1;
        out->levels_0x01 = 0;
        if ((self->flags_0x836 & 2) != 0) {
            out->levels_0x01 |= 1;
        }
        if ((self->flags_0x836 & 0x8000) != 0) {
            out->levels_0x01 |= 2;
        }
        if (em_parts_damage_level_get(self, 3) >= 2) {
            out->levels_0x01 |= 4;
        }
        if (em_parts_damage_level_get(self, 5) >= 1) {
            out->levels_0x01 |= 8;
        }
        out->angle_0x02 = self->parts_0x838[0].value_0x04;
        out->damage_0x04 = self->field_0x7A0;
    } else {
        out->hit_0x00 = 0;
    }
}

/* The em020 "res user data" apply step: hands the area's third resource record to the shared
 * `fn_8013A654` installer.
 * 0x803754EC */
void em020_res_user_data_apply(struct _ENEMY_WORK* self)
{
    fn_8013A654(self, 3);
}

/* The em020 aim-target predicate: whether the `+0x836` bit 15 "aim target found" flag is set.
 * 0x803754F4 */
u32 em020_aim_target_ck(struct _ENEMY_WORK* self)
{
    return (self->flags_0x836 & 0x8000) != 0;
}

/* The em020 low-HP predicate: the damage numerator's share of the denominator is at or below 0.65
 * and the work sits in area 3.  This one body keeps the peephole pass (its `xoris` is part of the
 * unsigned int-to-float conversion the target keeps).
 * 0x80375424 */
u32 em020_hp_ratio_ck(struct _ENEMY_WORK* self)
{
    f32 ratio;

    if (self->area_no != 3) {
        return 0;
    }
    ratio = (f32)self->field_0x7A0 / (f32)self->field_0x7A4;
    if (ratio <= lbl_8079BC6C) {
        return 1;
    }
    return 0;
}

/* The em020 map-7 area-3 action request: when the work stands on map 7 in area 3, asks the shared
 * action setter for action 13 sub-state 3.
 * 0x80375494 */
void em020_map_area_action_set(struct _ENEMY_WORK* self)
{
    if (stage_map_kind_get(self->field_0x1E0) == 7 && self->area_no == 3) {
        fn_80128AEC(self, 13, 3);
    }
}

/* The em020 area-2 action-20 sub-state-30 predicate.
 * 0x8037550C */
u32 em020_area2_action20_ck(struct _ENEMY_WORK* self)
{
    if (self->team == 20 && self->area_no == 2 && self->stack_0x961[0] == 30) {
        return 1;
    }
    return 0;
}

/* Releases the work's sub-record through the shared teardown, then frees the block when the caller's
 * size argument is positive.
 * 0x803757E0 */
void* em020_work_free(struct _ENEMY_WORK* self, s16 size)
{
    if (self != NULL) {
        fn_8013918C(self, 0);
        if (size > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* The empty stub the program table reserves for em020.
 * 0x80376964 */
void em020_noop(void)
{
}

/* The em020 quest-page pointer: the address of the shared quest-page block's fourth record.
 * 0x80376968 */
u8* em020_quest_page_ptr(void)
{
    return lbl_806BE340 + 3;
}

/* The em020 "false" program-table stub.
 * 0x803788A0 */
u32 em020_false_ck(void)
{
    return 0;
}

/* Writes the em020 one-byte `.sbss` flag.
 * 0x803759BC */
void em020_unknown_flag_set(u8 value)
{
    lbl_80794BF4 = value;
}

/* The em020 quest-active predicate: whether the shared lobby block's `+0x03` byte is 1.
 * 0x8037583C */
u32 em020_quest_active_ck(void)
{
    return lbl_806BF530[3] == 1;
}

/* Clears the shared lobby block's `+0x03` quest-active byte.
 * 0x80375858 */
void em020_quest_active_clear(void)
{
    lbl_806BF530[3] = 0;
}

/* Clears the nine quest-page records after the first in the shared quest-page block.
 * 0x80376978 */
void em020_quest_pages_clear(void)
{
    s32 i;

    for (i = 1; i < 10; i++) {
        memset(lbl_806BE340 + i * 304, 0, 304);
    }
}

/* The address of the em020 twelve-entry 31-byte row array at the shared lobby block's `+0x13E6`.
 * 0x80378F7C */
u8* em020_row_ptr(u8 index)
{
    u8* rows = lbl_806BF530 + 0x13E6;
    return rows + index * 31;
}

#ifdef __cplusplus
}
#endif
