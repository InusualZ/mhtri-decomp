/*
 * enemy/em004_act.cpp - enemy 004's action band: the per-area lookups, the aim-height fade that drives the
 *   MHchar TEV colours, the spawn-record fill and the static initializer.
 * RANGE. .text 0x801C29F8-0x801CA8DC (107 functions); extab 0x8000FB84-0x8000FE4C, extabindex
 *   0x8002B830-0x8002BC5C, .ctors 0x8056F354-0x8056F358 (`fn_801CA870`), .rodata 0x80570320-0x80570410, .data
 *   0x805B3B80-0x805B4F8C (`em004_prog_tbl` first), .bss 0x806A7B18-0x806A7B30, .sdata 0x80791AD8-0x80791B20,
 *   .sdata2 0x80798FE8-0x80799220.  Left edge: `fn_801C28FC`, `enemy/em006_prog.cpp`'s static initializer, ends
 *   there; right edge: `fn_801CA870` is this TU's static initializer and the 0.0 entry repeats at `lbl_80799220`
 *   from `fn_801CA8DC` on.
 * FLAGS. `cflags_main`; file-wide `#pragma peephole off` (retail keeps the unfused `clrlwi`/`rlwinm` + `cmpwi`
 *   pairs).
 * NAMES. `em004` is the runtime dump's `em004_prog_tbl`, which opens the TU's `.data`; the `_act` suffix is a
 *   GUESS.  The map has only `fn_` stems for the functions.
 *   The `.bss` record names (`vec_pair_801CA004_0`) are GUESSes.
 * RESIDUALS. 101 rows unwritten: 0x801C29F8-0x801C2A58, 0x801C2A78-0x801CA004, 0x801CA258-0x801CA4CC,
 *   0x801CA5D8-0x801CA7F0, 0x801CA870-0x801CA8DC.
 *   2 partial rows:
 *  - `fn_801CA004`: retail calls `setTevKColor__6MHcharFUl14_GXTevKColorIDP8_GXColor` (a `_GXColor*`) where ours
 *    calls `...ID8_GXColor` (by value, 3 relocations), hence ours' extra stack copy and `clrlwi` per call; retail
 *    also loads the step constant before `timer_0x328` and compares `area_no` signed (`cmpwi`, ours `cmplwi`);
 *  - `fn_801CA170`: retail runs the inner search as a compare loop (`cmplw`/`blt`), ours as a guarded `bdnz` count
 *    loop, and retail re-materialises `lbl_805B3CD8` where ours hoists it.
 *   flipcheck: `.ctors`/`.data`/`.rodata`/`.sdata`/`.sdata2` claimed, not emitted; `.text` (0x424 of 0x7EE4),
 *   extab (0x20 of 0x2C8) and extabindex (0x30 of 0x42C) short of the claim and differing;
 *   our `setTevKColor__6MHcharFUl14_GXTevKColorID8_GXColor` has no map row (retail's spelling is `...IDP8_GXColor`).
 */

#include "types.h"
#include "nw4r/math.h"
#include "gx.h"
#include "sound/mhchar.h"
#include "sys_mem.h"
#include "fn_8004CAD8.h"
#include "mh3_pad.h"
#include "g3d/g3d_calcworld.h"
#include "draw_shape.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/em005_act.h"
#include "enemy/em004_act.h"
#include "enemy/fn_801251D0.h"
#include "enemy/fn_8012BDF4.h"
#include "enemy/fn_8012EC74.h"
#include "enemy/fn_80138074.h"
#include "enemy/fn_80147CE0.h"
#include "unsplit/enemy.h"
#include "unsplit/unknown.h"
#include "stage/stg_w.h"
#include "sound/fn_800D7F54.h"
#include "sound/se.h"
#include "unsplit/ef.h"
#include "ef/eft009.h"

/* Retail keeps the unfused `clrlwi`/`rlwinm` + `cmpwi` pairs the peephole folds. */
#pragma peephole off

/* The unit's `.data` lookup table, declared: the source does not emit it yet. */
extern u8 lbl_805B3CD8[];

/* the 8-byte `{key, count, subs}` lookup records `fn_801CA170` walks (the pool at 0x805B3CD8). */
struct EmLookupSub {
    /* +0x0 */ u8 key;
    /* +0x1 */ u8 value;
    /* +0x2 */ u8 pad_0x02[2];
    /* +0x4 */ u32 data;
} /* size: 0x08 */;
struct EmLookupEntry {
    /* +0x0 */ u8 key;
    /* +0x1 */ u8 count;
    /* +0x2 */ u8 pad_0x02[2];
    /* +0x4 */ EmLookupSub* subs;
} /* size: 0x08 */;

extern "C" {

/* ----------------------------------------------------------------------------------------------- *
 * the action band's step functions
 * ----------------------------------------------------------------------------------------------- */

/* 0x801CA5D4 - a 4-byte `blr` (the map's own empty body). */
void fn_801CA5D4(_ENEMY_WORK* self) {}

/* 0x801CA170 - finds the `self->area_no` entry of the 0x805B3CD8 table, then its record keyed `a`: returns
 * the record's second byte and stores its value through `out`. */
u32 fn_801CA170(_ENEMY_WORK* self, u8 a, u32* out) {
    if (stage_map_kind_get(self->field_0x1E0) != 4)
        return 0;
    const EmLookupEntry* table = (const EmLookupEntry*)lbl_805B3CD8;
    for (u8 i = 0; table[i].key != 0xFF; i++) {
        if (self->area_no == table[i].key) {
            const EmLookupSub* sub = table[i].subs;
            u8 count = table[i].count;
            u8 j = 0;
            while (j < count) {
                if (sub->key == a) {
                    *out = sub->data;
                    return sub->value;
                }
                j++;
                sub++;
            }
            return 0;
        }
    }
    return 0;
}

/* 0x801CA4CC - picks the motion by the map id, feeds the aim slot and advances the aim height. */
void fn_801CA4CC(_ENEMY_WORK* self, u8* out_mode, u8* out_flag) {
    em_fall_height_get(self);
    em_fall_start(self);
    *out_mode = 0x0C;
    *out_flag = 0;
    switch (stage_map_kind_get(self->field_0x1E0)) {
    case 4:
        if ((s32)(u8)self->area_no == 4)
            fn_80126278(self, (u16)((self->area_no & 0xF) << 8), &self->pos);
        break;
    case 8:
        if ((s32)(u8)self->area_no == 1)
            fn_80126278(self, (u16)((self->area_no & 0xF) << 8), &self->pos);
        break;
    case 9:
        if ((s32)(u8)self->area_no == 0)
            fn_80126278(self, (u16)((self->area_no & 0xF) << 8), &self->pos);
        break;
    }
    self->pos.y = self->pos.y + lbl_8079900C;
}

/* 0x801CA7F0 - fills a spawn record: kind 0x1C, the fixed vector and the three scalar fields. */
void fn_801CA7F0(EmSpawnRec* rec, u8 a, u16 b, u16 c) {
    VEC3 v;
    setVec3(&v, lbl_80798FF8, lbl_80798FF8, lbl_80799014);
    rec->id = 0x1C;
    copyVec3(&rec->pos, &v);
    rec->field_0x10 = a;
    rec->field_0x12 = b;
    rec->field_0x14 = c;
}

/* 0x801CA004 - the per-frame aim-height fade: track the pool float toward the map's direction and
 * drive the three MHchar TEV colours from it. */
void fn_801CA004(_ENEMY_WORK* self) {
    if (em_alt_mode_ck(self) == 1) {
        self->timer_0x328.field_0x328 = self->timer_0x328.field_0x328 + lbl_80799214;
        if (self->timer_0x328.field_0x328 > lbl_80799218)
            self->timer_0x328.field_0x328 = lbl_80799218;
    } else {
        self->timer_0x328.field_0x328 = self->timer_0x328.field_0x328 - lbl_80799214;
        if (self->timer_0x328.field_0x328 < lbl_80798FF8)
            self->timer_0x328.field_0x328 = lbl_80798FF8;
    }
    _GXColor color;
    ((MHchar*)self->char_0x024)->getTevKColor(3, GX_KCOLOR1, &color);
    color.a = (u8)(s32)self->timer_0x328.field_0x328;
    ((MHchar*)self->char_0x024)->setTevKColor(3, GX_KCOLOR1, color);
    ((MHchar*)self->char_0x024)->getTevKColor(1, GX_KCOLOR1, &color);
    color.a = (u8)(s32)self->timer_0x328.field_0x328;
    ((MHchar*)self->char_0x024)->setTevKColor(1, GX_KCOLOR1, color);
    u32 armed = 0;
    if (stage_map_kind_get(self->field_0x1E0) == 4 && (self->area_no == 4 || self->area_no == 6))
        armed = 1;
    ((MHchar*)self->char_0x024)->getTevKColor(2, GX_KCOLOR3, &color);
    if (armed == 1)
        color.a = 0xFF;
    else
        color.a = 0;
    ((MHchar*)self->char_0x024)->setTevKColor(2, GX_KCOLOR3, color);
}

/* 0x801C2A58 - flips the halfword rotation and clears the third angle. */
extern "C" void fn_801C2A58(_ENEMY_WORK* self)
{
    self->field_0x1C0 = (s32)(u16)(self->field_0x1C0 + 0x8000);
    self->field_0x1C4 = 0;
}

} /* extern "C" */

/* The unit's `.bss`: the two-vector record `fn_801CA870` seeds.  The name is a GUESS (a pair of model-space
 * points). */
VEC3 vec_pair_801CA004_0[2];  /* +0x806A7B18 */
