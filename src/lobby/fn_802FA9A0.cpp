/* lobby/fn_802FA9A0.cpp - the lobby event/status band.
 * RANGE. .text 0x802FA9A0-0x8030121C (120 functions); .ctors 0x8056F394-0x8056F398 (the static initializer
 *   `fn_802FF180`), .rodata 0x805707C0-0x80570840, .data 0x805D8B00-0x805DAFE8, .bss 0x806BE0D8-0x806BE108, .sdata
 *   0x80792948-0x80792A58, .sdata2 0x8079AC20-0x8079AD98 (a pool run no other unit references), extab, extabindex.
 * FLAGS. `cflags_lobby` and file-scope `#pragma peephole off`: retail keeps `slwi` after `clrlwi` and `srwi` + `clrlwi`
 *   apart where the pass folds them into one `rlwinm` (`fn_802FB3B0`, `fn_802FB948`).
 * NAMES. The map and the dump give only placeholders (`LbCheckKujiraEvent__Fv` at 0x802FB9DC is the one real name), so
 *   the file keeps the map's stem.  Module `lobby`: the predicates read `lobby_w` +0x003/+0x15F/+0x161 and
 *   `lobby_world_block`, and the callees are the lobby UI API plus the `ef`/`enemy` helpers the screens drive.
 * RESIDUALS. 93 rows unwritten: 0x802FA9A0-0x802FB3B0, 0x802FB600-0x802FB8C4, 0x802FBA14-0x802FBA60,
 *   0x802FBA94-0x802FBE40, 0x802FBE60-0x802FDB90, 0x802FDBA4-0x802FDF9C, 0x802FDFB0-0x802FEE4C, 0x802FEE54-0x802FF0A4,
 *   0x802FF0A8-0x802FF234, 0x802FF2C0-0x802FF478, 0x802FF4AC-0x802FF90C, 0x802FF928-0x8030121C.
 *   flipcheck: `.ctors`/`.rodata`/`.data`/`.bss`/`.sdata`/`.sdata2` claimed, not emitted (the tables are reached
 *   through `extern` declarations in `lobby/fn_802FA9A0.h`); `.text`/extab/extabindex short of the claim.
 * SHAPES. The six id-table predicates take a `u32` and narrow it in a temporary: a narrow parameter arrives narrowed
 *   per the ABI and drops the `clrlwi`s retail keeps.
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

/* Whether the whale event is up; outside the block above because the map spells it mangled (C++ linkage, rule 9). */
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
    eft_res_slot_release(self);
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
