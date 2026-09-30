/* enemy/fn_801CA004.cpp - the enemy action/state band `.text` 0x801CA004..0x801CCBC4 (47
 * functions, 0x2BC0 bytes).  Registered once, at its final home (docs/plan.md 12), from
 * `proposal/801CA004_fn_801CA004.cpp`.
 *
 * Module and name (brief section 2, evidence order).  No `__FILE__` string is reachable from the
 * range (checked with `grep -rl lbl_805A1BB8 build/RMHE08/asm/`: the image's only `enemy` source
 * name, `enemy_control.cpp`, is referenced by `fn_801411B8` alone, 10 KB below), and
 * `python tools/symbols/dumpmap.py lookup 0x801CA004` answers `zz_01ca004_` (a placeholder is not
 * evidence).  Both bracketing registered units are `enemy/*` (`enemy/fn_801B7020.cpp` below,
 * `enemy/fn_801D428C.cpp` above), every function takes the shared `_ENEMY_WORK` record, and the
 * band's naming scheme is the map's own `fn_XXXXXXXX` stem - so the file keeps the map stem (brief
 * option 4).  Discovery's seam note ("one source file (enemy_control.cpp): a candidate seam inside
 * it was not taken") is the `owner_merge` artifact the brief warns about: `enemy_control.cpp` is
 * merely the latest accepted source name before the run, not an owner of it (the same note sits on
 * the neighbouring `proposal/801D428C`).
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/dumpmap.py lookup`: every address resolves to a `zz_XXXXXXXX_` dump name
 * and a bare `.text` entry in config/RMHE08/symbols.txt, so no real function name exists to use).
 *
 * Language.  C++: the range's callees include C++ manglings (`getTevKColor__6MHcharFUl14_...`,
 * `setTevKColor__6MHcharFUl14_...`, `__nw__FUl`, `calcDistanceSqXZ__FPQ34nw4r4math4VEC3P...`,
 * `calcVecAng2__FPQ34nw4r4math4VEC3P...`), declared at C++ scope with the signature their mangling
 * encodes (rule 9); every plain `fn_XXXXXXXX` definition stays `extern "C"`.
 *
 * Status (measured with `python tools/units/recompile.py enemy/fn_801CA004 --measure <symbol>`).
 * This is a PARTIAL landing: 31 of the range's 47 functions are reconstructed, 29 of them at or
 * above the 80 % bar and 25 byte-identical.
 *   * 100.00: fn_801CA4CC, fn_801CA5D4, fn_801CA7F0, fn_801CAA8C, fn_801CAF6C, fn_801CAF70,
 *     fn_801CAFBC, fn_801CB008, fn_801CB050, fn_801CB098, fn_801CB114, fn_801CB190, fn_801CB20C,
 *     fn_801CB28C, fn_801CB308, fn_801CB500, fn_801CB57C, fn_801CB618, fn_801CB694, fn_801CB7B0,
 *     fn_801CB82C, fn_801CB8C0, fn_801CB93C, fn_801CBBD8, fn_801CBC64.
 *   * fn_801CC5DC 96.15, fn_801CBB0C 92.16, fn_801CBA4C 91.67, fn_801CA004 87.77.
 *   * Residuals, by measurement: fn_801CC5DC (the two `fmuls` operand order and the argument
 *     evaluation order of the two `em_turn_in_window` calls), fn_801CBB0C / fn_801CBA4C (the float/int
 *     argument evaluation order of `em_approach_start` - target loads f1 before r4), fn_801CA004 (target
 *     364 B / ours 396 B - the `em_alt_mode_ck` gate and the colour blocks), fn_801CA170 77.33 (the
 *     inner lookup loop lowers to a `mtctr`/`bdnz` countdown, the target walks a pointer with a
 *     counter) and fn_801CA8DC 75.02 (target 324 B / ours 336 B - the tail average loop).
 * The unwritten residual (16 functions): the TevKColor/motion band 0x801CA258..0x801CAE70 and the
 * two jump-table dispatchers `fn_801CAC04`/`fn_801CB9DC` and the 0x801CBD30 monster - the last three
 * would emit their own `.data` jump tables, which the unit's split block does not yet claim, so they
 * are left for a worker that can also claim the tables at 0x805B5328/0x805B53E4.
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
#include "enemy/fn_801251D0.h"
#include "enemy/fn_8012BDF4.h"
#include "enemy/fn_8012EC74.h"
#include "enemy/fn_80138074.h"
#include "enemy/fn_80147CE0.h"
#include "unsplit/enemy.h"
#include "unsplit/unknown.h"
#include "stage/stg_w.h"

/* retail keeps the unfused clrlwi/rlwinm + cmpwi pairs this band's -O3 peephole folds, so the whole
 * unit is built with the peephole off (the same finding as `enemy/fn_801B7020.cpp`,
 * `enemy/fn_80147CE0.cpp` and `enemy/fn_801D428C.cpp`). */
#pragma peephole off

/* the `.sdata2` float pool this range reads (shared pool, not this unit's data - docs/plan.md 8.4). */
extern f32 lbl_80799214;
extern f32 lbl_80799218;
extern f32 lbl_8079921C;
extern f32 lbl_80799220;
extern f32 lbl_80799224;
extern f32 lbl_80799228;
extern f32 lbl_8079922C;
extern f32 lbl_80799230;
extern f32 lbl_80799234;
extern f32 lbl_80799238;
extern f32 lbl_8079923C;
extern f32 lbl_80799240;
extern f32 lbl_80799244;
extern f32 lbl_80799248;
extern f32 lbl_8079924C;
extern f32 lbl_80799250;
extern f32 lbl_80799254;
extern f32 lbl_80799258;
extern f32 lbl_8079925C;
extern f32 lbl_80799260;
extern f32 lbl_807992A0;
extern f32 lbl_807992A4;
extern f32 lbl_807992A8;
extern f32 lbl_80798FF8;
extern f32 lbl_8079900C;
extern f32 lbl_80799014;

/* the `.rodata`/`.data` tables this range references (shared pools, not this unit's data). */
extern u8 lbl_80570410[];
extern u8 lbl_80570450[];
extern u8 jumptable_805B5328[];
extern u8 jumptable_805B53E4[];
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

/* 0x801CAF6C - a 4-byte `blr`. */
void fn_801CAF6C(_ENEMY_WORK* self) {}

/* 0x801CAA8C - clear the action block: the float, the word and the armed byte. */
void fn_801CAA8C(_ENEMY_WORK* self) {
    self->timer_0x328.field_0x328 = lbl_80799220;
    self->timer_0x328.field_0x32C = 0;
    self->action_0x328.armed_0x328.field_0x330 = 0xFF;
}

/* 0x801CA170 - the area's sub-record lookup: the `self->area_no` entry of the 0x805B3CD8 pool,
 * then the record whose key is `a`; the value goes out through `out` and the record's second byte
 * is the answer. */
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

/* 0x801CA4CC - pick the motion by the map id, feed the aim slot and advance the aim height. */
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

/* 0x801CA8DC - average the motion slots' aim angles (the wrap-aware mean `fn_801CAA20` steps the
 * rotation by); 0xFFFF means none. */
u16 fn_801CA8DC(_ENEMY_WORK* self, u8 a) {
    if (self->field_0x218 == 0)
        return 0xFFFF;
    u32 angles[10];
    u8 count = 0;
    u8 i = 0;
    EmMotionSlot* slot = self->slots_0x244;
    VEC3* vec = &self->slots_0x244[0].vec;
    u32* p = angles;
    for (; i < 10; i++, slot++, vec++) {
        if (slot->flags == 0)
            break;
        if (a == 1 && (u32)(slot->value - 0x6000) > 0x4000)
            continue;
        if ((slot->flags & 0x800) == 0)
            continue;
        u32 x;
        u32 y;
        calcVecAngXY(vec, &x, &y);
        *p++ = y;
        count++;
    }
    if (count == 0)
        return 0xFFFF;
    if (count == 1)
        return (u16)angles[0];
    s32 avg = angles[0];
    u32* q = &angles[1];
    for (s8 i = 1; (u8)i < count; i++, q++) {
        s32 delta = (s32)*q - avg;
        if (delta > 0x8000)
            delta -= 0x10000;
        else if (delta < -0x8000)
            delta += 0x10000;
        avg += delta / (i + 1);
    }
    return (u16)avg;
}

/* 0x801CA7F0 - fill a spawn record: kind 0x1C, the fixed vector and the three scalar fields. */
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

/* 0x801CAF70 - enter the 0x0B motion. */
void fn_801CAF70(_ENEMY_WORK* self) {
    em_fall_height_get(self);
    em_fall_start(self);
    fn_80128AAC(self, 3, 0x0B);
    fn_80133BB4(self);
}

/* 0x801CAFBC - enter the 0x14 motion. */
void fn_801CAFBC(_ENEMY_WORK* self) {
    em_fall_height_get(self);
    em_fall_start(self);
    fn_80128AAC(self, 3, 0x14);
    fn_80133BB4(self);
}

/* 0x801CB008 - arm motion 7/5. */
void fn_801CB008(_ENEMY_WORK* self) {
    em_move_mode_set(self, 0);
    fn_80128AAC(self, 7, 5);
    fn_80133BB4(self);
}

/* 0x801CB050 - arm motion 7/6. */
void fn_801CB050(_ENEMY_WORK* self) {
    em_move_mode_set(self, 0);
    fn_80128AAC(self, 7, 6);
    fn_80133BB4(self);
}

/* 0x801CB098 - state 0 arms motion 1/0x0A, state 1 closes on the motion end. */
void fn_801CB098(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 1, 10, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
}

/* 0x801CB114 - state 0 arms motion 2/4, state 1 closes on the motion end. */
void fn_801CB114(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 2, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
}

/* 0x801CB190 - the same body as 0x801CB114 (the band's second 2/4 step). */
void fn_801CB190(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 2, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
}

/* 0x801CB20C - state 0 re-seats the motion and arms 0x1A/6, state 1 runs it out. */
void fn_801CB20C(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set_ck(self, 0x1A, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1)
            em_action_finish_fall(self);
        break;
    }
}

/* 0x801CB28C - state 0 arms motion 0x1B/6, state 1 runs it out. */
void fn_801CB28C(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 3);
        em_mot_set_ck(self, 0x1B, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1)
            em_action_finish_walk(self);
        break;
    }
}

/* 0x801CB308 - the sub-state dispatcher over the six step functions above. */
void fn_801CB308(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_801CB098(self);
        break;
    case 1:
        fn_801CB114(self);
        break;
    case 2:
        fn_801CB190(self);
        break;
    case 3:
        fn_801CB20C(self);
        break;
    case 6:
        fn_801CB28C(self);
        break;
    }
}

/* 0x801CB500 - state 0 arms motion 0x12/0x0A, state 1 runs it out. */
void fn_801CB500(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x12, 0x0A, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
}

/* 0x801CB57C - state 0 arms 0x13/6 and the 0x12C-frame timer, state 1 counts it down. */
void fn_801CB57C(_ENEMY_WORK* self) {
    em_busy_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x13, 6, 0);
        self->timer_0x020 = 0x12C;
        break;
    case 1:
        if (--self->timer_0x020 <= 0)
            em_state_set(self, 1, 3);
        break;
    }
}

/* 0x801CB618 - state 0 arms 0x14/8, state 1 runs it out. */
void fn_801CB618(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x14, 8, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
}

/* 0x801CB694 - the four-step 0x17/0x18/0x19 action. */
void fn_801CB694(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x17, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 0x18, 0, 0);
            self->timer_0x020 = 0x708;
            fn_80132224(self);
        }
        break;
    case 2:
        fn_8013221C(self, lbl_8079924C, 1, 0x14);
        if (--self->timer_0x020 <= 0) {
            self->state++;
            em_mot_set(self, 0x19, 4, 0);
            fn_80132264(self);
        }
        break;
    case 3:
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
}

/* 0x801CB7B0 - state 0 arms 0x7C/2, state 1 runs it out. */
void fn_801CB7B0(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x7C, 2, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
}

/* 0x801CB82C - state 0 arms 4/4; a nonzero `a` first re-seats the motion. */
void fn_801CB82C(_ENEMY_WORK* self, u8 a) {
    if (a == 1)
        em_busy_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 4, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
}

/* 0x801CB8C0 - state 0 arms 3/4, state 1 runs it out. */
void fn_801CB8C0(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 3, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
}

/* 0x801CB93C - state 0 arms 2/4, state 1 waits the `a`-selected frame window. */
void fn_801CB93C(_ENEMY_WORK* self, u8 a) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 2, 4, 0);
        break;
    case 1:
        if (em_frame_check(self, 1, ((s32)(u8)a == 1) ? lbl_80799250 : lbl_80799254, lbl_80799220) == 1)
            em_action_finish(self);
        break;
    }
}

/* 0x801CBA4C - state 0 arms 6/4 and starts the `a`-selected fade, state 1 waits it out. */
void fn_801CBA4C(_ENEMY_WORK* self, u8 a) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 6, 4, 0);
        if ((s32)(u8)a != 1)
            em_approach_start(self, lbl_80799258, 0);
        else
            em_approach_start(self, lbl_80799220, 0);
        break;
    case 1:
        if (em_approach_step(self, 0, 0x40) == 1)
            em_action_finish(self);
        break;
    }
}

/* 0x801CBB0C - state 0 arms 7/6, sets the motion rate and starts the `a`-selected fade. */
void fn_801CBB0C(_ENEMY_WORK* self, u8 a) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 7, 6, 0);
        em_mot_speed_set(self, lbl_8079925C);
        if ((s32)(u8)a != 1)
            em_approach_start(self, lbl_80799258, 0);
        else
            em_approach_start(self, lbl_80799220, 0);
        break;
    case 1:
        if (em_approach_step(self, 0, 0x40) == 1)
            em_action_finish(self);
        break;
    }
}

/* 0x801CBBD8 - state 0 starts the `lbl_80570410` table effect, state 1 waits it out. */
void fn_801CBBD8(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_turn_seq_start(self, lbl_80570410, 0, 0, 0);
        break;
    case 1:
        if (em_turn_seq_step(self, lbl_80570410) == 1)
            em_action_finish(self);
        break;
    }
}

/* 0x801CBC64 - state 0 starts the `lbl_80570450` table effect (and the 0x482 fade), state 1 waits. */
void fn_801CBC64(_ENEMY_WORK* self, u8 a) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_turn_seq_start(self, lbl_80570450, 0, 1, 0);
        if (self->field_0x482 == 1)
            em_mot_speed_set(self, lbl_80799260);
        break;
    case 1:
        if (a == 1 && em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        } else if (em_turn_seq_step(self, lbl_80570450) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801CC5DC - the strafe/approach action: state 0 seats the offset vector, state 1 steers it. */
void fn_801CC5DC(_ENEMY_WORK* self, u8 a) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_move_vec2_clr(self);
        switch (a) {
        case 0: {
            em_mot_set(self, 0xD2, 4, 0);
            em_mot_speed_set(self, lbl_8079925C);
            f32 scale = get_em_chg_scale(self);
            f32 rate = get_em_base_scale(self);
            self->offset_0x30C.vec_0x310.x = lbl_807992A0 * rate * scale;
            break;
        }
        case 1: {
            em_mot_set(self, 0xD3, 4, 0);
            em_mot_speed_set(self, lbl_8079925C);
            f32 scale = get_em_chg_scale(self);
            f32 rate = get_em_base_scale(self);
            self->offset_0x30C.vec_0x310.x = lbl_807992A4 * rate * scale;
            break;
        }
        }
        rotVecY(&self->offset_0x30C.vec_0x310, self->field_0x1C0);
        em_hit_window_set_default(self, 0, 0x18);
        break;
    case 1:
        switch (a) {
        case 0:
            em_turn_in_window(self, lbl_80799220, lbl_807992A8, 0x4000);
            break;
        case 1:
            em_turn_in_window(self, lbl_80799220, lbl_807992A8, -0x4000);
            break;
        }
        if (em_frame_check(self, 3, lbl_80799220, lbl_807992A8) == 1)
            CancelFade(self);
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
}

} /* extern "C" */

/* This unit's own `.bss` (`splits.txt` `.bss 0x806A7B18..0x806A7B30`), in address order: the 1 two-vector record(s)
 * its static constructor `fn_801CA870` builds (`.data` tables point at them).  Names are GUESSes: each record is a
 * pair of model-space points. */
VEC3 vec_pair_801CA004_0[2];  /* +0x806A7B18 */
