/* enemy/enemy_control.cpp - the enemy control translation unit, `.text` 0x801411B8..0x80147CE0
 * (155 functions).  This file is the lower half of the maximal unclaimed run
 * 0x801411B8..0x80149D6C; the upper half 0x80147CE0..0x80149D6C is already registered as
 * `enemy/fn_80147CE0.cpp`.  The two are ONE translation unit (evidence below) and the merge is
 * requested in this worker's outbox (`config_requests`), to be applied by the orchestrator at the
 * re-split; no source of the landed upper unit was touched.
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/dumpmap.py lookup`: every address resolves to a `zz_XXXXXXXX_` dump name and
 * a bare `.text` entry in config/RMHE08/symbols.txt, so no real function name exists to use).  A few
 * symbols arrive already mangled (`senko_set__FPQ34nw4r4math4VEC3fUcs`, `em_get_unique_work__FUs...`)
 * and those are called through their real signatures per rule 9, not spelled as identifiers.
 *
 * WHAT IT IS.  The enemy "control" TU: the shared enemy-control routines and the em001 enemy's
 * program handler band.  Its first function `fn_801411B8` is the em001 enemy's constructor - it
 * asserts the NW4R revision and the bound flag, sets up the `_ENEMY_WORK` record and installs the
 * em001 handlers; the band continues with the em001 program functions that the table
 * `em001_prog_tbl` (0x805A1C58) lists, ending at `fn_80147C94`; `fn_80147CE0` (the upper unit's
 * entry) is the next handler in that same table.
 *
 * MODULE AND NAME (brief section 2, evidence order).  Option 1, a `__FILE__` string, decides it.
 *   `lbl_805A1BB8` at 0x805A1BB8 is `.string "enemy_control.cpp"`, and the ONLY code in the whole
 *   image that references it is this range's `fn_801411B8` (0x801411DC/0x80141258, paired with
 *   0x805A1BD0 = "NW4R:Failed assertion bRevision" and line 0x32A):
 *
 *       bl fn_80093B0C ; cmpwi r3,0 ; bne .L ; <Panic("enemy_control.cpp", 810, "NW4R:...")>
 *
 *   `grep -rl lbl_805A1BB8 build/RMHE08/asm/` returns exactly `auto_fn_801411B8_text.s`.  A
 *   source-file name is the original source file, so the module is `enemy` (both bracketing
 *   registered units are `enemy`) and the file is `enemy_control.cpp`.  The upper half references no
 *   `__FILE__` string at all (its data references are the two vtables and two numeric tables), i.e.
 *   the run has one source name and it lives here.
 *
 * ONE TU, NOT TWO (the seam at 0x80147CE0 is a byte-budget cut, not a boundary).
 *   0x801411B8 is `attribute.py`'s `--max-bytes` (27436) cut of the maximal unclaimed run; the run
 *   is 0x801411B8..0x80149D6C (35764 B), so discovery cut it at the last function boundary under the
 *   cap and `enemy/fn_80147CE0.cpp` (0x80147CE0..0x80149D6C) is the piece above.  The evidence that
 *   the two pieces are one TU:
 *     * the `__FILE__` string above is the run's only source name and lives in this range;
 *     * `em001_prog_tbl` (0x805A1C58) lists this range's `fn_80147C94` AND the upper unit's
 *       `fn_80147CE0`/`fn_80147E68`/`fn_80147F48`/`fn_801481D8` - one enemy program's handler row set
 *       straddling the cut;
 *     * this range's `extab` run ends exactly at 0x8000D9DC and its `extabindex` run at 0x800285B4,
 *       which are exactly where `enemy/fn_80147CE0.cpp`'s begin - the two section runs are
 *       continuous across the cut, so the cut is not a section boundary;
 *     * the `.sdata2` pool is referenced from both sides (this range 0x80796DC8..0x80796DFC, the upper
 *       unit 0x80796E18..0x80796EE0);
 *     * discovery's own `tudiscover` verdict for the range was `tu: merged` (the candidate `.sdata2`
 *       seam lbl_80796E10 -> lbl_80796E28 was not taken).
 *   The vtables the upper unit's constructors install (`lbl_805A1368`, `lbl_805A4478`) point at
 *   `fn_8013xxxx` (`enemy/fn_80138074.c`) and `fn_8015xxxx` virtuals, NOT into this range - the
 *   cross-seam link is `em001_prog_tbl`, and that is what is recorded here.
 *
 * LANGUAGE.  C++.  Four callees arrive mangled (`senko_set__FPQ34nw4r4math4VEC3fUcs`,
 * `kemuri_set__FPQ34nw4r4math4VEC3fUc`, `em_get_unique_work__FUsPP11_ENEMY_WORKPP16_ENEMY_MINI_WORK`,
 * `em_ride_hit_check__FPQ34nw4r4math4VEC3UcPP11_ENEMY_WORKPUlPf`) and rule 9 forbids spelling a
 * mangling at the call site, so each is declared at C++ scope with the signature its mangling
 * encodes.  Every plain definition keeps the map's `fn_XXXXXXXX` stem (the `extern "C"` block).
 *
 * Sections.  Besides `.text` the unit owns the `extab` run 0x8000D664..0x8000D9DC and the
 * `extabindex` run 0x80028080..0x800285B4, taken from the target's own per-function entry
 * boundaries (107 of the 155 functions carry an entry).  The split also assigned this unit the one
 * `.ctors` word 0x8056F31C..0x8056F320 - the static initializer `fn_80147ABC` (it points at
 * `emc_work` and tail-calls `fn_80147AC8`).  No data (the `.sdata2`/`.data`/`.rodata` runs the
 * range references are shared pools, docs/plan.md 8.4).
 *
 * Status (measured in this worktree with `python tools/units/recompile.py enemy/enemy_control.cpp
 * --measure <symbol>`, against MAIN's retired per-function split objects, which hold the same
 * original bytes).  This is a PARTIAL landing: 28 functions are reconstructed and EVERY one is at
 * or above the 80 % bar; the remaining 127 functions of the range (the em001 program handler band
 * from 0x80141B88 up) are NOT yet written and are the residual this worker hands on - the unit is
 * registered and measurable, so they are incremental.  `#pragma peephole off` is load-bearing for
 * the whole unit (retail keeps the unfused clrlwi/rlwinm + cmpwi pairs `-O3` fuses - the same
 * finding as `enemy/fn_80147CE0.cpp`).
 *   * 100.00: fn_801411B8, fn_8014131C, fn_80141358, fn_80141470, fn_801414C8, fn_801415A8,
 *     fn_80141654, fn_80141690, fn_801416B0, fn_801416EC, fn_801418EC, fn_801418F0,
 *     fn_8014192C, fn_80141B2C, fn_80141B4C, fn_80143174, fn_80144240, fn_801444FC, fn_80144FB4,
 *     fn_80147ABC, senko_set, kemuri_set.
 *   * fn_8014128C 97.22, fn_801414D4 97.07, fn_80141A4C 94.25, fn_801413D0 92.75,
 *     fn_801417FC 91.25, fn_80143BF8 85.00.
 *   * Residuals, by measurement: fn_8014128C 97.22 (target 0x90 B / ours 0x94 B - the
 *     +0x360/0x364/0x368 control-word stores and `fn_80144FE0`'s argument colouring), fn_801413D0
 *     (first-check register colouring), fn_801417FC (target 0xF0 B / ours 0xE8 B), fn_80143BF8
 *     (target 0x1C B / ours 0x20 B - the `(b - 1) == 0` lowering).  All are above the bar and their
 *     bodies are complete.
 *
 * 
 * Inventory, addresses and sizes: `python tools/units/ledger.py unit enemy/enemy_control.cpp`.
 */

#include "types.h"
#include "nw4r/math.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/enemy_control.h"
#include "enemy/fn_80138074.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */

/* retail keeps the unfused clrlwi/rlwinm + cmpwi pairs this band's -O3 peephole folds, so the whole
 * unit is built with the peephole off (the same finding as `enemy/fn_80147CE0.cpp`,
 * `enemy/fn_8013F764.cpp` and `enemy/fn_8012BDF4.cpp`). */
#pragma peephole off

/* ----------------------------------------------------------------------------------------------- *
 * the callees and the data pools
 * ----------------------------------------------------------------------------------------------- */

/* `nw4r::db::Panic(const char*, int, const char*, ...)` - declared at its owner's real spelling so the
 * C++ front-end reproduces the map's `Panic__Q24nw4r2dbFPCciPCce` (rule 9). */
#ifdef __cplusplus
namespace nw4r { namespace db { void Panic(const char* file, int line, const char* fmt, ...); } }
#endif

/* the `__FILE__` / assert-message / resource-name pool this range's first function reads (the
 * `enemy_control.cpp` name at 0x805A1BB8 is the TU's own source name, the evidence of section 2). */
extern char lbl_805A1BB8[];  /* "enemy_control.cpp" */
extern char lbl_805A1BD0[];  /* "NW4R:Failed assertion bRevision" */
extern char lbl_805A1BF0[];  /* "shadow_tex.brres" */
extern char lbl_805A1C08[];  /* "NW4R:Failed assertion bAllBound" */

extern "C" {

/* the enemy-control work blob (0x806A4590, 0xF50 bytes). */
extern EmcWork emc_work;

/* external callees (the ones whose owner is unregistered; rule 2 moves them to
 * `include/unsplit/enemy.h` when this batch lands) */
s32  fn_80093B0C(void);
void fn_80093990(void* self);
u32  fn_800E264C(void* self);
void* ckResourceName(char* name);
s32* res_file_ctor(s32* dst, s32 value);
u32  fn_8009380C(void* self, s32* value);
void fn_800F0F38(u8 kind);
void fn_800D58B0(s32 handle);

/* this unit's own functions, forward-declared so they may be called before their definitions */
EmcSlot* fn_801413D0(u32 index);
void fn_80141358(u8 index);
void fn_8014131C(void);
void fn_801415A8(u8 index);
void fn_80141690(u8 index);
void fn_801416B0(void);
SenkoRec* fn_801416EC(void);
void fn_801418F0(void);
void fn_80141B4C(void);
void fn_80144FE0(EmcWork* work);
void fn_80145390(void);
void fn_80145620(void);
void fn_80145C84(void);
void fn_80146E54(u8 kind);
void fn_801472D8(u8 kind);
void* fn_80147AC8(EmcWork* work);
void fn_80143A40(void* self);
u32  fn_8042CB9C(void);
u8   fn_8042CC20(void);

/* ----------------------------------------------------------------------------------------------- *
 * the emc_work control block and the 0x90/0xA0 record arrays
 * ----------------------------------------------------------------------------------------------- */

/* the resource record `ckResourceName` returns, as far as `fn_801411B8` reads it (the u32 at
 * +0x44 is the value `res_file_ctor` is called with).
 * size: 0x48 */
struct EmResHeader {
    /* +0x00 */ u8 unused_0x00[0x44];
    /* +0x44 */ u32 field_0x44;
};

/* the enemy's em001 per-activation constructor: asserts the NW4R revision, sets up the record and
 * installs the em001 handler set.  `lbl_805A1BB8` is the TU's `__FILE__` string. */
void fn_801411B8(u8* self) {
    u32 handle;
    if (fn_80093B0C() == 0) {
        nw4r::db::Panic((const char*)lbl_805A1BB8, 810, (const char*)lbl_805A1BD0);
    }
    fn_80093990(self);
    handle = fn_800E264C(self);
    if (handle != 1) {
        struct EmResHeader* res = (struct EmResHeader*)ckResourceName(lbl_805A1BF0);
        if (res != 0) {
            s32 tmp;
            s32 value;
            res_file_ctor(&tmp, res->field_0x44);
            value = tmp;
            handle = fn_8009380C(self, &value);
        }
    }
    if (handle == 0) {
        nw4r::db::Panic((const char*)lbl_805A1BB8, 829, (const char*)lbl_805A1C08);
    }
}

/* the em001 activation entry: resets every sub-system and the `emc_work` control words. */
void fn_8014128C(void) {
    fn_8014131C();
    fn_801416B0();
    fn_801418F0();
    fn_80141B4C();
    emc_work.field_0x360 = 0;
    emc_work.field_0x364 = 0;
    emc_work.field_0x368 = -1;
    fn_80144FE0(&emc_work);
    fn_80145620();
    fn_80145390();
    fn_80145C84();
    if (fn_8042CB9C() == 1) {
        emc_work.field_0x36A = fn_8042CC20();
    } else {
        emc_work.field_0x36A = 1;
    }
    emc_work.field_0x36B = 0;
}

/* clear all six per-enemy slots. */
void fn_8014131C(void) {
    u32 i;
    i = 0;
    do {
        fn_80141358(i);
        i++;
    } while (i < 6);
}

/* clear one per-enemy slot (`emc_work + index * 0x18`). */
void fn_80141358(u8 index) {
    EmcSlot* slot = &emc_work.slot_0x000[index];
    s32 tmp;
    res_file_ctor(&tmp, 0);
    slot->id = index;
    slot->state = 0;
    slot->kind = 0;
    slot->flags = 0;
    slot->handle_0x08 = -1;
    slot->handle_0x0C = -1;
    slot->handle_0x10 = -1;
    slot->handle_0x14 = -1;
}

/* the first of the six slots whose state byte equals `index`. */
EmcSlot* fn_801413D0(u32 index) {
    EmcSlot* slot = &emc_work.slot_0x000[0];
    if (slot->state == (u8)index) {
        return slot;
    }
    if ((++slot)->state == (u8)index) {
        return slot;
    }
    if ((++slot)->state == (u8)index) {
        return slot;
    }
    if ((++slot)->state == (u8)index) {
        return slot;
    }
    if ((++slot)->state == (u8)index) {
        return slot;
    }
    if ((++slot)->state == (u8)index) {
        return slot;
    }
    return 0;
}

/* claim the first idle slot for `kind`. */
u8 fn_80141470(u8 kind) {
    EmcSlot* slot = fn_801413D0(0);
    if (slot != 0) {
        slot->state = 1;
        slot->kind = kind;
        slot->flags = 0;
        return slot->id;
    }
    return 0xFF;
}

/* mark a slot released (`state = 2`). */
void fn_801414C8(EmcSlot* slot) {
    slot->state = 2;
}

/* the index of the first slot whose kind matches, among the claimed ones. */
u8 fn_801414D4(u32 kind0) {
    EmcSlot* slots = &emc_work.slot_0x000[0];
    u8 kind = (u8)kind0;
    if (slots[0].state != 0 && slots[0].kind == kind) {
        return 0;
    }
    if (slots[1].state != 0 && slots[1].kind == kind) {
        return 1;
    }
    if (slots[2].state != 0 && slots[2].kind == kind) {
        return 2;
    }
    if (slots[3].state != 0 && slots[3].kind == kind) {
        return 3;
    }
    if (slots[4].state != 0 && slots[4].kind == kind) {
        return 4;
    }
    if (slots[5].state != 0 && slots[5].kind == kind) {
        return 5;
    }
    return 0xFF;
}

/* release one per-enemy slot and drop its two file handles. */
void fn_801415A8(u8 index) {
    EmcSlot* slot = &emc_work.slot_0x000[index];
    if (slot->state != 0) {
        u8 kind = slot->kind;
        fn_800F0F38(kind);
        fn_801472D8(kind);
        slot->id = index;
        slot->state = 0;
        slot->kind = 0;
        slot->field_0x04 = 0;
        fn_800D58B0(slot->handle_0x08);
        fn_800D58B0(slot->handle_0x10);
        slot->handle_0x08 = -1;
        slot->handle_0x0C = -1;
        slot->handle_0x10 = -1;
        slot->handle_0x14 = -1;
        fn_80146E54(kind);
    }
}

/* release all six per-enemy slots. */
void fn_80141654(void) {
    u32 i;
    i = 0;
    do {
        fn_801415A8(i);
        i++;
    } while (i < 6);
}

/* free one `senko` record (`emc_work + 0x90 + index * 0x14`). */
void fn_80141690(u8 index) {
    emc_work.senko_0x090[index].state = 2;
}

/* free all eight `senko` records. */
void fn_801416B0(void) {
    u32 i;
    i = 0;
    do {
        fn_80141690(i);
        i++;
    } while (i < 8);
}

/* the first free `senko` record (`state == 2`). */
SenkoRec* fn_801416EC(void) {
    SenkoRec* rec = &emc_work.senko_0x090[0];
    if (rec->state == 2) {
        return rec;
    }
    if ((++rec)->state == 2) {
        return rec;
    }
    if ((++rec)->state == 2) {
        return rec;
    }
    if ((++rec)->state == 2) {
        return rec;
    }
    if ((++rec)->state == 2) {
        return rec;
    }
    if ((++rec)->state == 2) {
        return rec;
    }
    if ((++rec)->state == 2) {
        return rec;
    }
    if ((++rec)->state == 2) {
        return rec;
    }
    return 0;
}

/* step the eight `senko` records' two-state timers (state 0 -> 1 -> 2). */
void fn_801417FC(void) {
    SenkoRec* rec = &emc_work.senko_0x090[0];
    s32 i;
    for (i = 0; i < 4; i++) {
        switch (rec[0].state) {
        case 0:
            rec[0].countdown--;
            if (rec[0].countdown <= 0) {
                rec[0].state = 1;
                rec[0].countdown = 3;
            }
            break;
        case 1:
            rec[0].countdown--;
            if (rec[0].countdown <= 0) {
                rec[0].state = 2;
                rec[0].countdown = 0;
            }
            break;
        }
        switch (rec[1].state) {
        case 0:
            rec[1].countdown--;
            if (rec[1].countdown <= 0) {
                rec[1].state = 1;
                rec[1].countdown = 3;
            }
            break;
        case 1:
            rec[1].countdown--;
            if (rec[1].countdown <= 0) {
                rec[1].state = 2;
                rec[1].countdown = 0;
            }
            break;
        }
        rec += 2;
    }
}

/* per-record stub (its body is empty in the target). */
void fn_801418EC(u8 index) {
    (void)index;
}

/* run the empty per-record stub over the ten 0x130 records. */
void fn_801418F0(void) {
    u32 i;
    i = 0;
    do {
        fn_801418EC(i);
        i++;
    } while (i < 10);
}

/* the first of the ten `marker_0x130` records whose countdown has run out. */
SenkoRec* fn_8014192C(void) {
    SenkoRec* rec = &emc_work.marker_0x130[0];
    if (rec->countdown <= 0) {
        return rec;
    }
    if ((++rec)->countdown <= 0) {
        return rec;
    }
    if ((++rec)->countdown <= 0) {
        return rec;
    }
    if ((++rec)->countdown <= 0) {
        return rec;
    }
    if ((++rec)->countdown <= 0) {
        return rec;
    }
    if ((++rec)->countdown <= 0) {
        return rec;
    }
    if ((++rec)->countdown <= 0) {
        return rec;
    }
    if ((++rec)->countdown <= 0) {
        return rec;
    }
    if ((++rec)->countdown <= 0) {
        return rec;
    }
    if ((++rec)->countdown <= 0) {
        return rec;
    }
    return 0;
}

/* step the ten `marker_0x130` countdowns. */
void fn_80141A4C(void) {
    SenkoRec* rec = &emc_work.marker_0x130[0];
    s32 i;
    for (i = 0; i < 10; i++) {
        if (rec[i].countdown > 0) {
            rec[i].countdown--;
            if (rec[i].countdown <= 0) {
                rec[i].countdown = 0;
            }
        }
    }
}

/* clear the +0x09 byte of one `marker2_0x1F8` record. */
void fn_80141B2C(u8 index) {
    emc_work.marker2_0x1F8[index].field_0x09 = 0;
}

/* clear the +0x09 byte of all ten `marker2_0x1F8` records. */
void fn_80141B4C(void) {
    u32 i;
    i = 0;
    do {
        fn_80141B2C(i);
        i++;
    } while (i < 10);
}

/* copy a `VEC3` (`src` -> `dst`); returns `dst` (the helper `src/Pl/pl_act.cpp` calls). */
void* fn_80143174(void* dst, void* src, s32 arg2) {
    nw4r::math::VEC3* d = (nw4r::math::VEC3*)dst;
    nw4r::math::VEC3* s = (nw4r::math::VEC3*)src;
    (void)arg2;
    d->x = s->x;
    d->y = s->y;
    d->z = s->z;
    return dst;
}

/* whether the em001 control byte at +0x36B is 1. */
u8 fn_80143BF8(void) {
    return emc_work.field_0x36B == 1;
}

/* the em001 control block's static initializer (the unit's one `.ctors` entry at 0x8056F31C). */
void fn_80147ABC(void) {
    fn_80147AC8(&emc_work);
}

/* the em001 control block's transition word (`s16` at +0x368; `fn_8014128C` sets it to -1). */
s16 fn_801444FC(void) {
    return emc_work.field_0x368;
}

/* the u32 entry `index` of the table at +0xF48, or 0 when the pointer is null. */
u32 fn_80144FB4(u8 index) {
    u32* arr = emc_work.field_0xF48;
    if (arr != 0) {
        return arr[index];
    }
    return 0;
}

/* tail-call the marker reset. */
void fn_80144240(void* self) {
    fn_80143A40(self);
}

}  /* extern "C" */

/* `senko_set(VEC3*, f32, u8, s16)` - the map's `senko_set__FPQ34nw4r4math4VEC3fUcs`; defined at
 * C++ scope so the front-end reproduces the mangling (rule 9). */
void senko_set(nw4r::math::VEC3* pos, f32 value, u8 arg2, s16 arg3) {
    SenkoRec* rec = fn_801416EC();
    if (rec != 0) {
        copyVec3(&rec->pos_0x00, pos);
        rec->value_0x0C = value;
        rec->value_0x12 = arg2;
        rec->countdown = arg3;
        rec->state = 0;
    }
}

/* `kemuri_set(VEC3*, f32, u8)` - the map's `kemuri_set__FPQ34nw4r4math4VEC3fUc`. */
void kemuri_set(nw4r::math::VEC3* pos, f32 value, u8 arg2) {
    SenkoRec* rec = fn_8014192C();
    if (rec != 0) {
        copyVec3(&rec->pos_0x00, pos);
        rec->value_0x0C = value;
        rec->value_0x12 = arg2;
        rec->countdown = 570;
    }
}

