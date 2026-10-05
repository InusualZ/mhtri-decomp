/*
 * light/light.cpp - unit, `.text` 0x802BF278..0x802C2700 (63 functions, 13448 bytes).
 *
 * 15 of 63 functions have a body here.
 *
 * FLAGS.  `cflags_main`.  The record types moved to `light/light_work.h` (shared with
 * `camera/camera_main.cpp`, which holds the head of the module).
 *
 * Sections: the unit's block in config/RMHE08/splits.txt (.bss, .ctors, .data, .sbss, .sdata, .sdata2, .text, extab,
 * extabindex).
 */
/* ---- header inherited from src/light/light.cpp (written against its pre-phase-4 range) ---- */
/*
 * light/light.cpp - the map light work: its record, its constructors, its per-frame channels and its
 * accessors.
 *
 * `.text` 0x802BEAAC-0x802C474C (103 functions, 23712 B).
 *
 * Module `light` and file name `light.cpp` come from evidence class 2 (brief section 2): the range's
 * own symbols the retail symbol table knows are `light_init__Fv` (0x802BF284), `light_move__Fv`
 * (0x802C1D30), `set_amblight__FUc8_GXColor` (0x802C1DA8) and
 * `make_dir_light2__FlPQ34nw4r4math4VEC38_GXColorl` (0x802C1F74) - four real manglings naming one
 * light subsystem, all confirmed by `tools/symbols/dumpmap.py lookup`; `light_init`/`light_move` are
 * the module's own entry points, which is what this file holds.  No `__FILE__` string covers the
 * range (the `menu_item.cpp` string the discovery note mentions sits at 0x805CDFC8 and the dump
 * attributes its emitter to 0x802A22A4, i.e. the registered `menu_item` proposal, not this band).
 *
 * Language C++: four defined manglings (`light_init__Fv`...), `cflags_main` (`Wii/1.3`, `-O3
 * -inline noauto -Cpp_exceptions on`), the same group as the neighbour `stage/stg_w.cpp`.
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for 99 of this range's 103 symbols (checked
 * with `python tools/symbols/symedit.py range 0x802BEAAC 0x802C474C` and
 * `python tools/symbols/dumpmap.py lookup` over the inventory: every unnamed entry is a bare
 * `fn_XXXXXXXX` in config/RMHE08/symbols.txt, and the runtime dump answers either `zz_XXXXXXXX_` or
 * an unrelated engine symbol for it).
 *
 * Seam - unproven.  The range is one maximal unclaimed run (`attribute.py` cut it at its byte cap),
 * and two observations say a real TU boundary sits at, not inside, its end: the `.ctors` word at
 * 0x8056F380 points at fn_802BEEE0 and the one at 0x8056F384 at fn_802C2530, and the `.sdata2`
 * ordering seam `lbl_8079A698 -> lbl_8079A69C` falls between fn_802C4630 and fn_802C474C (the last
 * function of the range), so the extent stays as proposed until the functions match.
 *
 * The light work record.  `LightWork`'s size is 0x4F8 and it is traced, not guessed: lbl_806BB7E0 is
 * a 0x9F0-byte `.bss` object holding exactly two records (fn_802BECD0 returns one of the two, 0x4F8
 * apart, and fn_802BEEE0 constructs both with `__construct_array(0x806BB7E0, fn_802BEF00, 0, 0x4F8,
 * 2)`), and fn_802BEF00's initialization loop runs its `LightChannel` array from +0x4A8 to +0x4E4 in
 * 0x14 steps.  The sub-records the constructors build carry their own traced extents.
 *
 * Flags: the unit is peephole-off - retail keeps the unfused `extsh` + `cmpwi` pair (playbook 39)
 * where `-O3`'s peephole folds them into one `extsh.`, and the two sibling units of the band
 * (`stage/stg_w.cpp`, `stage/fn_802B2978.c`) carry the same pragma for the same reason.
 *
 * Residuals (31 of the range's 103 functions written, 1960 B of 23712; 28 of the 31 byte-identical):
 *
 *  - fn_802BEAAC (86.81 %, 152 B against the target's 144 B).  Retail materialises the zero once at
 *    the top (`li r0, 0x0`) and keeps the decremented timer in r5; ours rematerialises `li r0, 0x0`
 *    inside each of the three arms and takes r0 for the timer, so the object is two instructions
 *    long.  Every other instruction is identical, and `--timer` (the shape landed) beats both the
 *    `s16 t = timer - 1` local (70.69 %) and the field-assignment form (70.69 %).
 *  - fn_802BF7E8 (85.45 %, 40 B against 44 B).  Retail materialises the counter's address into r4
 *    once at the top and stores the wrap with `sth r3, 0x0(r4)`, where ours keeps the `@sda21`
 *    addressing for both stores, which is one instruction short.
 *  - fn_802BF0AC (99.00 %, 140 B each).  The first run walk has its pointer/bound register pair the
 *    other way round (`addi r31, r31, 0xc` / `cmplw r31, r30` against ours on r30/r31); the second
 *    walk, the four leading vectors and the whole rest of the function are identical.
 *
 * Not yet written: the remaining 72 functions of the range, all still `fn_XXXXXXXX` in the map.
 *
 * Inventory and evidence: `python tools/units/ledger.py unit proposal/802BEAAC_fn_802BEAAC.cpp`.
 */

#include "types.h"
#include "light/light_work.h"
#include "camera/camera.h"
#include "nw4r/math.h"

/* Owner headers (rule 2): every symbol a registered unit defines is declared in that unit's header,
 * never here.  `unsplit/unknown.h` carries the module-ambiguous ones. */
#include "ef/fn_800CDB2C.h"
#include "fn_80047398.h"
#include "fn_8004CAD8.h"
#include "g3d/fn_80063888.h"
#include "lobby/lb_npc.h"
#include "mh3_pad.h"
#include "stage/stg_w.h"
#include "unsplit/Runtime.PPCEABI.H.h"
#include "unsplit/unknown.h"

/* ------------------------------------------------------------------------------------------------ */
/* types                                                                                             */
/* ------------------------------------------------------------------------------------------------ */

extern "C" u16 lbl_80794B68;
extern "C" void* fn_802C1D68(u8 index);
extern "C" void fn_802C1E14(void* self, void* arg);
extern "C" LightWrap* fn_802C2664(LightWrap* self);
extern "C" LightWrap2* fn_802C2698(LightWrap2* self);
extern "C" LightVecAt4* fn_802C26CC(LightVecAt4* self);
extern "C" u16 fn_802BF7E8(void);
extern "C" void fn_802BFA00(u16 id, void* vector, void* arg, f32 a, f32 b);
extern "C" void fn_802C0728(void);

#pragma peephole off

/* Resets the global light level counter. */
extern "C" void fn_802BF278(void)
{
    lbl_80794B68 = 0;
}

/* Copy-constructs a light record from the given source. */
extern "C" void* fn_802BF430(u8* self, const u8* src)
{
    color_rgba_copy(self, src);
    return self;
}

/* Returns the light work the scene root carries at +0x2878. */
extern "C" LightWork* fn_802BF460(SceneRoot* root)
{
    return &root->light;
}

/* Whether the current map is one of the two that carry a light work of their own. */
extern "C" int fn_802BF468(void)
{
    if (get_now_mapno() == 21 || get_now_mapno() == 22) {
        return 1;
    }
    return 0;
}

/* Ticks the global light level counter, wrapping it at 60000. */
extern "C" u16 fn_802BF7E8(void)
{
    lbl_80794B68++;
    if (lbl_80794B68 > 60000) {
        lbl_80794B68 = 0;
    }
    return lbl_80794B68;
}

/* Ticks the light level counter. */
extern "C" void fn_802BF814(void)
{
    fn_802BF7E8();
}

/* Adds a light of the given id at the vector. */
extern "C" void fn_802BFAE0(u16 id, void* vector, void* arg, f32 a, f32 b)
{
    fn_802BFA00(id, vector, arg, a, b);
}

/* Releases the object behind the pointer when the flag is up. */
extern "C" void fn_802C1AD8(void* object, const f32* vector)
{
    if (vector != NULL) {
        fn_80064C14(object, vector);
    }
}

/* Clears the disabled bit of a light record's flag word. */
extern "C" void fn_802C1F64(u32* flags)
{
    *flags &= ~0x10;
}

/* Clamps a value into the [low, high] range. */
extern "C" s32 fn_802C2510(s32 value, s32 low, s32 high)
{
    if (value < low) {
        return low;
    }
    if (value > high) {
        return high;
    }
    return value;
}

/* Rebuilds the light the given slot's record holds. */
extern "C" void fn_802C20A4(void* arg)
{
    fn_802C1E14(fn_802C1D68(0), arg);
}

/* Constructs a record by constructing its +0x04 member. */
extern "C" LightWrap* fn_802C2664(LightWrap* self)
{
    fn_802C26CC(&self->inner);
    return self;
}

/* Constructs the block the record carries at +0x08. */
extern "C" LightWrap2* fn_802C2698(LightWrap2* self)
{
    mhchar_construct(&self->block);
    return self;
}

/* Constructs the record's vector member. */
extern "C" LightVecAt4* fn_802C26CC(LightVecAt4* self)
{
    VEC3_ctor(&self->vec);
    return self;
}

/* Re-initialises the light work after the pad's motor callback. */
extern "C" void fn_802C2314(void)
{
    fn_802AEC00();
    fn_802C0728();
}

