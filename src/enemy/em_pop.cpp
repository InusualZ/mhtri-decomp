/*
 * em_pop.cpp - the enemy population/roster manager (`enemy` module).
 *
 * `.text` 0x803B465C..0x803BE30C (139 symbols / 40112 B), extab 0x80018DC4..0x80019164,
 * extabindex 0x8003930C..0x8003987C, plus this unit's own `.bss` (the model-handle table) and
 * `.sbss` (the work pointer) words.
 *
 * What it is.  The range is the field-side manager of the per-map enemy population: it walks the
 * monster roster (the 0x224-byte `EmPopRec` records), releases and recycles them, and its own string
 * pool names the data it consumes - `05/em_set/em_set_m%02da%02d_%03d.esd` (0x805F81F4),
 * `m%03d_%06d_c_pop.dat` (0x805F84E0) and `B-L-p02-ankou` (0x805F84B0).  Nine registered
 * `src/enemy/*` units call into the range (five of them call 0x803B9BA0; `enemy/fn_80138074.c`
 * drives the three roster helpers this file now defines), and `enemy/fn_8035E034.cpp` already
 * documents the roster record this file's accessor hands back.
 *
 * Naming evidence (brief section 2, in order).  1. No `__FILE__` string covers the range: every
 * `lis`+`addi` pair in 0x803B465C..0x803BE30C resolves to a numeric table, never a bare source-file
 * name (checked against the DOL's `.data`/`.rodata`/`.sdata` bytes).  2. `dumpmap.py lookup` answers
 * `zz_` for every symbol in the range - no real runtime-dump name.  3. The bracketing registered
 * units name different modules (`menu/multi_result.cpp` below, `Network/fn_803D3CE8.cpp` above), so
 * no neighbour scheme reaches the range.  4. **GUESS**, recorded here as the brief requires: module
 * `enemy` and file name `em_pop` are derived from what the range does (it builds, indexes and
 * recycles the per-map enemy population the `em_set`/`_pop.dat` files describe), plus the callers -
 * nine `src/enemy/*` units.  Every function name in this file is likewise derived from its own body.
 *
 * Status / residuals (this pass: 12 of 139 bodies, unit 2.53 % fuzzy; 6 rows at 100 %, 11 at or
 * above 80 %, mean 94.4 % over the 12 written; object .text 1092 B of the target's 40112 B).
 *  - Seam UNPROVEN (brief section 8.3).  The right edge 0x803BE30C is the discovery `--max-bytes`
 *    cap, not a TU boundary; the left edge 0x803B465C carries `tudiscover`'s strongest signal
 *    (`.data` referrer runs `jumptable_805F7BE8` -> `jumptable_805F7C68`, share 0.077).  The
 *    remaining 127 symbols keep their original bytes in the target object and measure 0 %.
 *  - Row residuals: `em_roster_record_result_get` 71.11 % (the target folds the {3,4} arms with one
 *    `subi`+`cmplwi` and materialises the `6 -> 5` default branchlessly; the plain switch here emits
 *    two compares and a real branch - 188 B vs 184 B); `em_roster_record_get` 86.56 % (the target
 *    keeps the record in r4 and moves it to r3 only for the return, ours folds it into r3 and takes
 *    `beqlr` - 56 B vs 64 B); `em_roster_record_unlink` 89.40 % (the target's 32-iteration table walk
 *    is unrolled 4x, ours is not - 216 B vs 232 B); `em_roster_free_record_get` 87.78 % (a
 *    register-number difference, same size); `em_roster_record_copy` 98.26 %.
 *  - `.data`/`.sdata2` are NOT claimed: the range owns a `.data` run
 *    (`lbl_805F7744`..`lbl_805F84E0`, sole referencer - the em_set path, the `_c_pop.dat` path and
 *    `B-L-p02-ankou` live in it) and the `.sdata2` pool 0x8079C560..0x8079C62C, but a partial
 *    `.sdata2` claim is not linkable (playbook 23) and this pass does not emit the tables.  Claim
 *    them with the bodies that emit them (the em_set loader is 0x803B7A9C, the pop loader 0x803B9F8C).
 *  - The target object carries a 4-byte `.ctors` word (a static initializer) that no body here emits;
 *    dtk added the claim during the split.  Writing the range's static-initializer function closes it.
 *  - `src/enemy/fn_8035E034.cpp` carries its own prefix view of the roster record (`EmRosterRec`,
 *    size 0x1F4); folding it into `include/enemy/em_pop.h` (rule 1) is the follow-up.
 *  - `include/unsplit/unknown.h` and `include/unsplit/menu.h` had their declarations of this range's
 *    symbols re-homed here (rule 2) with the consumers swept; every swept unit re-measured identical
 *    to `main`.
 *
 * Flags probed: this unit is built with the address neighbour's `cflags_menu`
 * (`cflags_main` + `-opt nopeephole`).  The range's 139 functions carry `-Cpp_exceptions on` extab
 * (116 records in the target's extab run 0x80018DC4..0x80019164), which `cflags_main` supplies;
 * `-opt nopeephole` is inherited from `menu/multi_result.cpp`, not yet measured per function here.
 */

#include "types.h"
#include "nw4r/math.h"
#include "mh3_pad/vec3.h"
#include "ef/fn_800CDB2C.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "enemy/em_pop.h"

extern "C" {

/* ---------------------------------------------------------------------------------------------- */
/* This unit's own data.  Both symbols are referenced only from this range (checked over every
 * `bl`/`lbl_` reference in the split asm), so both are this unit's to define. */

/* .sbss 0x80794C58 - the manager work pointer, written whole by the work allocator. */
EmPopWorkSlot em_pop_w;

/* .bss 0x806D2A78 - the 32-slot model-handle table: `em_roster_record_unlink` walks it, the
 * allocator fills it (`__nw__FUl(0xC)` per slot). */
struct EmHandleEntry {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ u8 in_use_0x04;
    /* +0x05 */ u8 pad_0x05[3];
    /* +0x08 */ EmPopRec* owner_0x08;
}; /* size: 0xC */

EmHandleEntry* em_handle_tbl[0x20];

/* The record's first 0x1A0 bytes - the run `em_roster_record_copy` moves. */
struct EmPopRecHead {
    /* +0x000 */ u16 head_0x000;
    /* +0x002 */ u16 head_0x002;
    /* +0x004 */ u32 body_0x004[0x48];   /* the target copies this run in 0x24 8-byte steps */
    /* +0x124 */ u32 tail_0x124[0x20];   /* and this one in 0x10 */
}; /* size: 0x1A4 */

/* ---------------------------------------------------------------------------------------------- */
/* The roster accessor: every table search in the range goes through it. */

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

/* Drops every handle that still names `rec`, releases its model and zeroes the record. */
EmPopRec* em_roster_record_clear(EmPopRec* rec) {
    em_roster_record_unlink(rec);
    if (rec->g3d_0x144 != NULL) {
        push_g3d_wk(rec->g3d_0x144);
    }
    return (EmPopRec*)memset(rec, 0, 0x224);
}

/* The same recycle path for one of the manager's sub-records (stride 0x20C). */
u32 em_roster_sub_record_clear(EmPopSubRec* rec) {
    em_roster_record_unlink((EmPopRec*)rec);
    if (rec->g3d_0x144 != NULL) {
        push_g3d_wk(rec->g3d_0x144);
    }
    return (u32)memset(rec, 0, 0x20C);
}

/* The first free roster slot, stamped with its own index; NULL when the pool is full. */
EmPopRec* em_roster_free_record_get(void) {
    EmPopWork* work = em_pop_w.work;
    EmPopRec* rec = work->recs;
    s32 index;
    s32 num = work->rec_num;
    for (index = 0; index < num; index++, rec++) {
        if (rec->state_0x000 == 0) {
            rec->index_0x002 = index;
            return rec;
        }
    }
    return NULL;
}

/* The first free sub-record slot; NULL when the pool is full. */
EmPopSubRec* em_roster_sub_free_get(void) {
    EmPopWork* work = em_pop_w.work;
    EmPopSubRec* rec = work->subs;
    u32 num = work->sub_num;
    while (num > 0) {
        if (rec->field_0x000 == 0) {
            return rec;
        }
        rec++;
        num--;
    }
    return NULL;
}

/* Copies the record's head block (the part the takeover path moves). */
void em_roster_record_copy(EmPopRec* dst, const EmPopRec* src) {
    *(EmPopRecHead*)dst = *(const EmPopRecHead*)src;
}

/* Drops every handle-table slot that still names `rec`. */
void em_roster_record_unlink(EmPopRec* rec) {
    u32 i;
    for (i = 0; i < 0x20; i++) {
        EmHandleEntry* entry = em_handle_tbl[i];
        if (entry != NULL && entry->in_use_0x04 != 0 && entry->owner_0x08 == rec) {
            entry->in_use_0x04 = 0;
            entry->owner_0x08 = NULL;
        }
    }
}

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

} /* extern "C" */
