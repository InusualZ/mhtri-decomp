/* The lobby event/status band, `.text` 0x802FA9A0..0x8030121C (26 748 B, 120 map symbols).
 *
 * Registered once, at its final home (docs/plan.md 12) from proposal `802FA9A0_fn_802FA9A0.cpp`.
 * Module `lobby` (class 3): the range's own predicates read the lobby work block `lobby_w`
 * (`.bss` 0x806AAB44) at +0x003/+0x15F/+0x161, its `.sbss` run is the lobby pointer block
 * `lobby_world_block`, and its callees are the lobby UI API (`LbStr`) plus the `ef`/`enemy` helpers the
 * lobby screens drive.  No `__FILE__` string is reachable from the range (the literal runs it
 * addresses are id/mask tables) and the runtime dump answers `zz_` for every in-range address, so the
 * file keeps the map's stem - brief section 2, class 4.  `LbCheckKujiraEvent` (0x802FB9DC) is the one
 * real name in the band.
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with `python
 * tools/symbols/symedit.py range 0x802FA9A0 0x8030121C` - the single non-`fn_` row is
 * `LbCheckKujiraEvent__Fv`), so every definition below keeps the map's own name.
 *
 * Seam: the proposal's edges are the discovery `--max-bytes` cap, not a proven TU boundary; the
 * supports are the `.sdata2` pool-run boundary at 0x8079AC20..0x8079AD98 (a run no other unit
 * references, `leak 0`) and the extab/extabindex runs, which are exactly the gaps between the
 * bracketing registered units.  Sections: .text 0x802FA9A0..0x8030121C, extab 0x800156B4..0x80015964
 * (86 x 8 B, one record per framed function), extabindex 0x80034074..0x8003447C (86 x 12 B).  Both
 * unwind runs are claimed; the data runs are not (see the residual list below).
 *
 * Flags: the `lobby` lib's `cflags_lobby` (`-O3`, `-inline noauto`), whose `-Cpp_exceptions on`
 * (flags-audit 2026-09-28) emits the unwind records every lobby target object carries, plus
 * `#pragma peephole off`, which is what makes `fn_802FB3B0` and `fn_802FB948` byte-identical: retail
 * keeps `slwi` after `clrlwi` where the pass folds them into one `rlwinm`, and keeps `srwi`+`clrlwi`
 * separate where it folds them into `rlwinm r,27,24,31`.  Measured over the whole file both ways.
 *
 * A parameter of a narrow type (`u8`/`u16`) arrives already narrowed per the ABI, so every mask the
 * target keeps in a function's body means the original took the value wide and narrowed it in the
 * source - all six of the id-table predicates below are spelled that way (`u32` parameter, narrowing
 * temporary), which is what reproduces the target's `clrlwi`s.
 *
 * Residual (measured; this file is a partial reconstruction):
 *   - 27 of the 120 symbols are written below and all 27 are byte-identical to the target; the unit
 *     reports 4.800359 % fuzzy (matched_code 1284 of 26748 B, 27/120 functions).  The other 93 keep the
 *     map's `fn_XXXXXXXX` names and are absent from this file, so objdiff reports them as 0 %.  The
 *     largest are `fn_803004D0` (2216 B), `fn_802FE550` (1988 B), `fn_802FAB98` (1052 B),
 *     `fn_802FAFC8` (1000 B) and `fn_802FA9A0` (504 B).
 *   - `.data` 0x805D8B00..0x805DAFE8 (37 labels, 5 of them `jumptable_*`), `.rodata`
 *     0x805707C0..0x80570840, `.sdata2` 0x8079AC20..0x8079AD98 (88 pool entries) and `.sdata`
 *     0x80791BC8..0x80792A58 are this range's data runs; the object emits none of them (the tables are
 *     reached through the `extern` declarations the unit header carries), so no data range is claimed -
 *     policy 8.4 forbids a claim that names bytes no object of ours produces, and a partial `.sdata2`
 *     claim is unlinkable anyway (playbook 23).  `datagap.py --unit main/lobby/fn_802FA9A0` reports no
 *     `ours-extra` section at all and lists no flip blocker for this unit.
 *   - `.ctors` 0x8056F394 holds this range's static initializer (it points at `fn_802FF180`, 180 B);
 *     dtk already attributes the word to this unit, but the object emits no `.ctors`, so that static
 *     and its constructor still have to be reconstructed before the word can be claimed.
 *   - The unwind claims are complete for the whole range (86 records) while only 27 functions are
 *     written, so our object's extab/extabindex are short (88/132 B of 688/1032 B).
 */
#include "types.h"
#include "lobby/fn_802FA9A0.h"
#include "fn_8004CAD8.h"
#include "ef/fn_800CDB2C.h"
#include "ef/eft_res.h"
#include "enemy/fn_8012EC74.h"

#pragma peephole off

extern "C" {

/* Walks the id table for ids 0..27 and reports whether every entry it points at is accepted by the
 * `0x8004D27C` predicate; ids above 27 and table holes answer 0, and the 0xFFFF terminator answers 1. */
u32 fn_802FB3B0(u32 raw) {
    u16 id = (u16)raw;
    u16* entry;

    if (id > 27) {
        return 0;
    }
    entry = lbl_805D8BA0[id];
    if (entry == NULL) {
        return 0;
    }
    while (*entry != 0xFFFF) {
        if (fn_8004D27C(*entry) == 0) {
            return 0;
        }
        entry++;
    }
    return 1;
}

/* The same walk for the second id table, which covers ids 28..39; the table is indexed from its own
 * base, so the id is rebased by 28. */
u32 fn_802FB434(u32 raw) {
    u16 id = (u16)raw;
    u16* entry;

    if (id <= 27) {
        return 0;
    }
    entry = lbl_805D8C10[id - 28];
    if (entry == NULL) {
        return 0;
    }
    while (*entry != 0xFFFF) {
        if (fn_8004D27C(*entry) == 0) {
            return 0;
        }
        entry++;
    }
    return 1;
}

/* Dispatches an id to the table that covers it: 0xFFFF (the empty id) and id 0 answer directly, ids
 * 28 and above go to the second table, the rest to the first. */
u32 fn_802FB4BC(u32 raw) {
    u16 id = (u16)raw;

    if (id == 0xFFFF) {
        return 0;
    }
    if (id == 0) {
        return 1;
    }
    if (id >= 28) {
        return fn_802FB434(id);
    }
    return fn_802FB3B0(id);
}

/* Whether `id` and `id + 1` together form the run boundary the caller is looking for: `id` accepted
 * and the next id rejected. */
u32 fn_802FB4F4(u32 id) {
    if (fn_802FB4BC((u16)id) == 1 && fn_802FB4BC((u16)(id + 1)) == 0) {
        return 1;
    }
    return 0;
}

/* Returns the first id of the requested table that `fn_802FB4BC` accepts: ids 27 downwards for kind 0
 * and 39 down to 28 for kind 1; any other kind answers 0. */
u16 fn_802FB54C(u32 raw) {
    u16 id;

    switch ((u8)raw) {
    case 0:
        for (id = 27; id >= 0; id--) {
            if (fn_802FB4BC(id) != 0) {
                return id;
            }
        }
        break;
    case 1:
        for (id = 39; id >= 28; id--) {
            if (fn_802FB4BC(id) != 0) {
                return id;
            }
        }
        break;
    default:
        return 0;
    }
    return 0;
}

/* The mode byte of the block `lobby_world_block` points at, +0x3E01. */
u8 fn_802FB5E4(void) {
    return lobby_world_block->mode_0x3E01;
}

/* The lobby work block's scene byte at +0x003. */
u8 fn_802FB5F0(void) {
    return lobby_w.field_0x003;
}

/* Whether the lobby currently stands in id 5's area. */
u8 fn_802FB8C4(void) {
    return fn_802FB4BC(5);
}

/* A per-kind state byte of the block `lobby_world_block` points at, +0x4654. */
u8 fn_802FB8EC(u32 raw) {
    return lobby_world_block->states_0x4654[(u8)raw];
}

/* Whether id 22 is accepted and the 0x58 event flag is up. */
u32 fn_802FB900(void) {
    if (fn_802FB4BC(22) == 1 && fn_8004D27C(0x58) == 1) {
        return 1;
    }
    return 0;
}

/* Whether the 0xB8 event flag is up. */
u8 fn_802FB948(void) {
    return fn_8004D27C(0xB8) == 1;
}

/* The lobby's "new area reached" gate: id 0's boundary, or id 20's boundary with mode 4 selected. */
u32 fn_802FB97C(void) {
    if (fn_802FB4F4(0) != 0) {
        return 1;
    }
    if (fn_802FB4F4(20) != 0) {
        if ((u8)fn_802FB5E4() == 4) {
            return 1;
        }
    }
    return 0;
}

} /* extern "C" */

/* Whether the whale event is up.  C++ linkage: the map's own name is the mangling
 * `LbCheckKujiraEvent__Fv`, so this one definition sits outside the block above (rule 9). */
u32 LbCheckKujiraEvent(void) {
    return lobby_w.kujira_0x15F == 1;
}

extern "C" {

/* The second event gate byte of the lobby work block, +0x161. */
u32 fn_802FB9F8(void) {
    return lobby_w.field_0x161 == 1;
}

/* Whether id 5 is accepted. */
u8 fn_802FBA60(void) {
    return fn_802FB4BC(5) == 1;
}

/* Forwards to the effect-model release helper. */
void fn_802FBE40(void* self) {
    fn_800F886C(self);
}

/* Clears the five event-state bytes at +0x328 of the whale-event work. */
void fn_802FBE44(LbKujiraEventWork* work) {
    work->field_0x329 = 0;
    work->field_0x328 = 0;
    work->field_0x32B = 0;
    work->field_0x32A = 0;
    work->field_0x32C = 0;
}

/* Forwards to the event's own step once its +0x1E6 done flag is clear. */
void fn_802FDB90(LbKujiraEventWork* work) {
    if (work->done_0x1E6 != 0) {
        return;
    }
    fn_802FDB04(work);
}

/* The same gate for the second event step. */
void fn_802FDF9C(LbKujiraEventWork* work) {
    if (work->done_0x1E6 != 0) {
        return;
    }
    fn_802FDDCC(work);
}

/* Pushes the enemy's motion number back to the caller through the tail call. */
void fn_802FEE4C(struct _ENEMY_WORK* self) {
    em_get_mot_no(self);
}

/* An empty stub in the target (a 4-byte `blr`). */
void fn_802FEE50(void) {
}

/* An empty stub in the target (a 4-byte `blr`). */
void fn_802FF0A4(void) {
}

/* Whether the whale-event work's countdown byte has gone negative. */
u32 fn_802FF234(void) {
    return lobby_world_block->work_0x4832.count_0x02 < 0;
}

/* Re-opens the whale-event work: clears the bit field, mirrors it into the flag byte, draws a new
 * timer and seeds the countdown. */
void fn_802FF248(void) {
    LbKujiraWork* work = &lobby_world_block->work_0x4832;

    work->bits_0x01 = 0;
    fn_802FF29C(0);
    work->flag_0x00 = work->bits_0x01;
    fn_802FF478(work);
    work->count_0x02 = -4;
}

/* Sets the `kind`-th bit of the whale-event work's bit field. */
void fn_802FF29C(u32 raw) {
    lobby_world_block->work_0x4832.bits_0x01 |= (u8)(1 << (u8)raw);
}

/* The whale-event work's timer draw. */
void fn_802FF478(LbKujiraWork* work) {
    work->timer_0x04 = ran_suu(1);
}

/* The byte the `idx`-th entry of a work record selects out of the `.sdata` table at 0x80792A38. */
u8 fn_802FF90C(LbEventSlot* self, u32 idx) {
    return lbl_80792A38[self->kind_0x74[(u16)idx]];
}

} /* extern "C" */
