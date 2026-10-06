/* enemy/enemy_control.cpp - the enemy control TU: the shared enemy-control routines (the em001 constructor
 *   `fn_801411B8`, the spawn requests, the `emc_work` control block) and the head of the em001 program band.
 * RANGE. .text 0x801411B8-0x80147C94 (154 functions); .ctors 0x8056F31C-0x8056F320 (`fn_80147ABC`, which points at
 *   `emc_work` and tail-calls `fn_80147AC8`), .data 0x805A1A80-0x805A1C58, .bss 0x806A4590-0x806A7734,
 *   .sbss 0x80794A90-0x80794AA0, .sdata2 0x80796DC8-0x80796E08, extab, extabindex.
 * SEAM. The right edge against `enemy/em001_prog.cpp` (0x80147C94) is a byte-budget cut, probably inside one TU:
 *   `em001_prog_tbl` (0x805A1C58) lists `fn_80147C94` and `fn_80147CE0`/`fn_80147E68`/`fn_80147F48`/`fn_801481D8`
 *   together, the extab/extabindex runs continue across the cut, and `tudiscover` answers `tu: merged`.
 * NAMES. The file from its `__FILE__` string: `lbl_805A1BB8` = "enemy_control.cpp", referenced only by
 *   `fn_801411B8`'s assertion (0x801411DC/0x80141258, line 810, "NW4R:Failed assertion bRevision").  `senko_set`,
 *   `kemuri_set`, `em_get_unique_work` and `em_ride_hit_check` are runtime-dump names; `em_spawn_request`,
 *   `emc_marker_replay`, `emc_mini_event_a`/`_b`, `emc_mini_step`, the `em_demo_*` set and `em_area_entry_make` are
 *   GUESSES from their bodies or call sites (the dump answers only `zz_` for each).
 * RESIDUALS. 126 rows unwritten: 0x80141B88-0x80143174, 0x80143190-0x80143BF8, 0x80143C14-0x80144240,
 *   0x80144244-0x801444FC, 0x8014450C-0x80144FB4, 0x80144FE0-0x80147ABC, 0x80147AC8-0x80147C94.
 *  - `fn_8014128C`: ours re-materialises r3 before the `fn_80144FE0` call (148 B against 144 B);
 *  - `fn_801413D0`: ours loads the +0x1 byte ahead of the first check;
 *  - `fn_801417FC`, `fn_80141A4C`: retail sign-extends the halfword with `extsh` after the load, ours loads it with
 *    `lha` (and `fn_801417FC` sets up its arguments in a different order);
 *  - `fn_80143BF8`: ours narrows the `(b - 1) == 0` result with an extra `clrlwi`; `fn_801414D4`: register allocation.
 *   flipcheck: `.bss`/`.ctors`/`.data`/`.sbss`/`.sdata2` claimed, not emitted; `.text`/extab/extabindex short of the
 *   claim; `ckResourceName` is referenced unmangled where the map spells `ckResourceName__FPc`.
 * SHAPES. `#pragma peephole off` over the bodies (retail keeps the unfused `clrlwi`/`rlwinm` + `cmpwi` pairs).
 */

#include "types.h"
#include "nw4r/math.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/enemy_control.h"
#include "enemy/fn_80138074.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */

namespace nw4r {

namespace db {
void Panic(const char* file, int line, const char* fmt, ...);
}
}

/* the `__FILE__` / assert-message / resource-name pool this range's first function reads (the
 * `enemy_control.cpp` name at 0x805A1BB8 is the TU's own source name, the evidence of section 2). */
extern char lbl_805A1BB8[];  /* "enemy_control.cpp" */
extern char lbl_805A1BD0[];  /* "NW4R:Failed assertion bRevision" */
extern char lbl_805A1BF0[];  /* "shadow_tex.brres" */
extern char lbl_805A1C08[];  /* "NW4R:Failed assertion bAllBound" */

extern "C" {
/* the enemy-control work blob (0x806A4590, 0xF50 bytes). */
extern EmcWork emc_work;

/* external callees whose owners' headers do not declare them */
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
u32  isServerSelectState(void);
u8   isReadyCountOne(void);

/* The emc_work control block and the 0x90/0xA0 record arrays. */

/* the resource record `ckResourceName` returns, as far as `fn_801411B8` reads it (the u32 at
 * +0x44 is the value `res_file_ctor` is called with).
 * size: 0x48 */
struct EmResHeader {
    /* +0x00 */ u8 unused_0x00[0x44];
    /* +0x44 */ u32 field_0x44;
};
}

#pragma peephole off

extern "C" {
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
    if (isServerSelectState() == 1) {
        emc_work.field_0x36A = isReadyCountOne();
    } else {
        emc_work.field_0x36A = 1;
    }
    emc_work.field_0x36B = 0;
}

/* Clears all six per-enemy slots. */
void fn_8014131C(void) {
    u32 i;
    i = 0;
    do {
        fn_80141358(i);
        i++;
    } while (i < 6);
}

/* Clears one per-enemy slot (`emc_work + index * 0x18`). */
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

/* Claims the first idle slot for `kind`. */
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

/* Marks a slot released (`state = 2`). */
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

/* Releases one per-enemy slot and drops its two file handles. */
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

/* Releases all six per-enemy slots. */
void fn_80141654(void) {
    u32 i;
    i = 0;
    do {
        fn_801415A8(i);
        i++;
    } while (i < 6);
}

/* Frees one `senko` record (`emc_work + 0x90 + index * 0x14`). */
void fn_80141690(u8 index) {
    emc_work.senko_0x090[index].state = 2;
}

/* Frees all eight `senko` records. */
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

/* Steps the eight `senko` records' two-state timers (state 0 -> 1 -> 2). */
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

/* Runs the empty per-record stub over the ten 0x130 records. */
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

/* Steps the ten `marker_0x130` countdowns. */
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

/* Clears the +0x09 byte of one `marker2_0x1F8` record. */
void fn_80141B2C(u8 index) {
    emc_work.marker2_0x1F8[index].field_0x09 = 0;
}

/* Clears the +0x09 byte of all ten `marker2_0x1F8` records. */
void fn_80141B4C(void) {
    u32 i;
    i = 0;
    do {
        fn_80141B2C(i);
        i++;
    } while (i < 10);
}

/* Copies a `VEC3` (`src` -> `dst`); returns `dst` (the helper `src/Pl/pl_act.cpp` calls). */
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

/* Tail-calls the marker reset. */
void fn_80144240(void* self) {
    fn_80143A40(self);
}
}

/* Takes a free flash record and arms it at `pos` with the value, the byte and the countdown; defined at C++ scope
 * for the map's mangled name. */
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

/* Takes a free smoke record and arms it at `pos` with the value, the byte and a 570-frame countdown. */
void kemuri_set(nw4r::math::VEC3* pos, f32 value, u8 arg2) {
    SenkoRec* rec = fn_8014192C();
    if (rec != 0) {
        copyVec3(&rec->pos_0x00, pos);
        rec->value_0x0C = value;
        rec->value_0x12 = arg2;
        rec->countdown = 570;
    }
}
