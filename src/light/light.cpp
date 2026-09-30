/*
 * light/light.cpp - the map light work: its record, its constructors, its per-frame channels and its
 * accessors.
 *
 * `.text` 0x802BEAAC-0x802C474C (103 functions, 23712 B).  Registered from
 * `proposal/802BEAAC_fn_802BEAAC.cpp`.
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
#include "camera/camera.h"
#include "nw4r/math.h"

/* Owner headers (rule 2): every symbol a registered unit defines is declared in that unit's header,
 * never here.  `include/unsplit/unknown.h` carries the module-ambiguous ones. */
#include "ef/fn_800CDB2C.h"
#include "fn_80047398.h"
#include "fn_8004CAD8.h"
#include "g3d/fn_80063888.h"
#include "lobby/lb_npc.h"
#include "mh3_pad.h"
#include "stage/stg_w.h"
#include "unsplit/Runtime.PPCEABI.H.h"
#include "unsplit/unknown.h"

#pragma peephole off

/* ------------------------------------------------------------------------------------------------ */
/* types                                                                                             */
/* ------------------------------------------------------------------------------------------------ */

/* One light channel: a world position, an enable flag, a control byte (its low five bits index the
 * intensity tables 0x805D1E7C/0x805D1E90, bit 0x80 selects the fade direction and 0x40 the half-way
 * intensity test - all read in fn_802BE7E8), a countdown timer and a table index. */
typedef struct LightChannel {
    /* +0x00 */ nw4r::math::VEC3 pos;
    /* +0x0C */ u8 enable;
    /* +0x0D */ u8 ctrl;
    /* +0x0E */ s16 timer;
    /* +0x10 */ u8 table_index;
    /* +0x11 */ u8 pad_0x11[3];
} LightChannel; /* size: 0x14 */

/* The map resource the light work points at.  Only the block fn_802B0688 is handed is traced, so the
 * extent past +0x3C is an approximation. */
typedef struct LightResource {
    /* +0x00 */ u8 unused_0x00[0x3C];
    /* +0x3C */ u8 entry;
} LightResource; /* size: 0x3D traced (the record is larger; approximation) */

/* Four vectors - the base every light record starts with. */
typedef struct LightQuad {
    /* +0x00 */ nw4r::math::VEC3 v[4];
} LightQuad; /* size: 0x30 */

/* Three vectors with the middle extent untraced (fn_802BF004's record). */
typedef struct LightTriple {
    /* +0x00 */ nw4r::math::VEC3 a;
    /* +0x0C */ nw4r::math::VEC3 b;
    /* +0x18 */ u8 unused_0x18[0x24];
    /* +0x3C */ nw4r::math::VEC3 c;
} LightTriple; /* size: 0x48 */

/* Four vectors, a gap and four more (fn_802BF044's record). */
typedef struct LightOctal {
    /* +0x00 */ nw4r::math::VEC3 v[4];
    /* +0x30 */ u8 unused_0x30[0x54];
    /* +0x84 */ nw4r::math::VEC3 w[4];
} LightOctal; /* size: 0xB4 */

/* The sub-record MTX34_ctor constructs; only its extent is traced. */
typedef struct LightMidBlock {
    /* +0x00 */ u8 bytes[0x30];
} LightMidBlock; /* size: 0x30 */

/* Four vectors, that sub-record and a trailing vector (fn_802BF180's record). */
typedef struct LightQuadMid {
    /* +0x00 */ nw4r::math::VEC3 v[4];
    /* +0x30 */ u8 unused_0x30[0x1C];
    /* +0x4C */ MTX34 mid;
    /* +0x7C */ nw4r::math::VEC3 tail;
} LightQuadMid; /* size: 0x88 */

/* Four vectors and a LightTriple (fn_802BEFB4's record). */
typedef struct LightQuadTriple {
    /* +0x00 */ nw4r::math::VEC3 v[4];
    /* +0x30 */ u8 unused_0x30[0x1C];
    /* +0x4C */ LightTriple inner;
} LightQuadTriple; /* size: 0x94 */

/* The scene root's own record (fn_802BF1D8's view): fifteen vector members. */
typedef struct LightRoot {
    /* +0x000 */ nw4r::math::VEC3 v0;
    /* +0x00C */ nw4r::math::VEC3 v1;
    /* +0x018 */ nw4r::math::VEC3 v2;
    /* +0x024 */ nw4r::math::VEC3 v3;
    /* +0x030 */ u8 unused_0x030[0x1C];
    /* +0x04C */ nw4r::math::VEC3 v4;
    /* +0x058 */ nw4r::math::VEC3 v5;
    /* +0x064 */ u8 unused_0x064[0x54];
    /* +0x0B8 */ nw4r::math::VEC3 v6;
    /* +0x0C4 */ nw4r::math::VEC3 v7;
    /* +0x0D0 */ nw4r::math::VEC3 v8;
    /* +0x0DC */ nw4r::math::VEC3 v9;
    /* +0x0E8 */ nw4r::math::VEC3 v10;
    /* +0x0F4 */ nw4r::math::VEC3 v11;
    /* +0x100 */ u8 unused_0x100[0x20];
    /* +0x120 */ nw4r::math::VEC3 v12;
    /* +0x12C */ nw4r::math::VEC3 v13;
    /* +0x138 */ nw4r::math::VEC3 v14;
    /* +0x144 */ u8 unused_0x144[0x04];
} LightRoot; /* size: 0x148 (the parent record's next member starts at +0x148) */

/* The parameter record (fn_802BF0AC's view): four vectors, then a four-element vector run whose two
 * halves the constructor walks separately. */
typedef struct LightParams {
    /* +0x00 */ nw4r::math::VEC3 v0;
    /* +0x0C */ nw4r::math::VEC3 v1;
    /* +0x18 */ nw4r::math::VEC3 v2;
    /* +0x24 */ nw4r::math::VEC3 v3;
    /* +0x30 */ u8 unused_0x030[0x1C];
    /* +0x4C */ nw4r::math::VEC3 runs[4];
    /* +0x7C */ u8 unused_0x07C[0x88];
} LightParams; /* size: 0x104 (the parent record's next member starts at +0x104) */

/* A record whose vector member sits at +0x04 (fn_802C26CC's view). */
typedef struct LightVecAt4 {
    /* +0x00 */ u32 unused_0x00;
    /* +0x04 */ nw4r::math::VEC3 vec;
} LightVecAt4; /* size: 0x10 */

/* fn_802C2664's record: it hands its +0x04 member to fn_802C26CC. */
typedef struct LightWrap {
    /* +0x00 */ u32 unused_0x00;
    /* +0x04 */ LightVecAt4 inner;
} LightWrap; /* size: 0x14 */

/* fn_802C2698's record: the block at +0x08 is handed to mhchar_construct by address. */
typedef struct LightWrap2 {
    /* +0x00 */ u8 unused_0x00[0x08];
    /* +0x08 */ u32 block;
} LightWrap2; /* size: 0x0C traced (the block is larger; approximation) */

/* The light work record.  `resource` is the map resource whose +0x3C block fn_802B0688 queries; every
 * other element is a sub-record a constructor builds, an untraced gap, or a channel. */
typedef struct LightWork {
    /* +0x000 */ LightRoot root;
    /* +0x148 */ LightQuadMid anim;
    /* +0x1D0 */ u8 unused_0x1D0[0x20];
    /* +0x1F0 */ LightQuad lights;
    /* +0x220 */ u8 unused_0x220[0x24];
    /* +0x244 */ LightParams params;
    /* +0x348 */ LightOctal colors;
    /* +0x3FC */ LightQuadTriple entry;
    /* +0x490 */ u8 unused_0x490[0x04];
    /* +0x494 */ LightResource* resource;
    /* +0x498 */ u8 unused_0x498[0x10];
    /* +0x4A8 */ LightChannel channel[3];
    /* +0x4E4 */ u8 unused_0x4E4[0x14];
} LightWork; /* size: 0x4F8 */

/* The scene root whose light work sits at +0x2878.  Only that offset is traced. */
typedef struct SceneRoot {
    /* +0x0000 */ u8 unused_0x0000[0x2878];
    /* +0x2878 */ LightWork light;
} SceneRoot; /* size: 0x2D70 (approximation: only the light block's offset is traced) */

/* The record fn_802C2F08 dispatches on: its state word is at +0x172 and its two arm handlers are
 * fn_802C2DD0 / fn_802C2E6C. */
typedef struct LightArm {
    /* +0x000 */ u8 unused_0x000[0x172];
    /* +0x172 */ u16 state;
} LightArm; /* size: 0x174 traced (the record is larger; approximation) */

/* ------------------------------------------------------------------------------------------------ */
/* externs                                                                                           */
/* ------------------------------------------------------------------------------------------------ */

/* The pooled string and the entry points whose address owns no registered unit, so no header exists
 * for them: the map already names the data (playbook 29), and these are the module-ambiguous band
 * (`include/unsplit/unknown.h` carries `get_now_mapno`, the other declarations here are band gaps the
 * lint counts rather than guesses). */
extern "C" const char lbl_805D1EB8[];
extern "C" u16 lbl_80794B68;
extern "C" int sprintf(char* buffer, const char* format, ...);

/* The rest of this unit's own functions, declared before their definitions. */
extern "C" LightWork* fn_802BECD0(void);
extern "C" LightWork* fn_802BEF00(LightWork* self);
extern "C" LightTriple* fn_802BF004(LightTriple* self);
extern "C" LightOctal* fn_802BF044(LightOctal* self);
extern "C" LightParams* fn_802BF0AC(LightParams* self);
extern "C" void* fn_802C1D68(u8 index);
extern "C" void fn_802C1E14(void* self, void* arg);
extern "C" LightWrap* fn_802C2664(LightWrap* self);
extern "C" LightWrap2* fn_802C2698(LightWrap2* self);
extern "C" LightVecAt4* fn_802C26CC(LightVecAt4* self);
extern "C" LightQuad* fn_802BF138(LightQuad* self);
extern "C" LightQuadMid* fn_802BF180(LightQuadMid* self);
extern "C" LightRoot* fn_802BF1D8(LightRoot* self);
extern "C" LightChannel* fn_802BEF84(LightChannel* self);
extern "C" LightQuadTriple* fn_802BEFB4(LightQuadTriple* self);
extern "C" u16 fn_802BF7E8(void);
extern "C" void fn_802BFA00(u16 id, void* vector, void* arg, f32 a, f32 b);
extern "C" void fn_802C0728(void);
extern "C" void fn_802C2DD0(LightArm* self);
extern "C" void fn_802C2E6C(LightArm* self);

/* The two light-work records the module keeps (`.bss` 0x806BB7E0, 0x9F0 B = 2 x 0x4F8).  Declared,
 * never defined here: the object emits only the references (playbook 29). */
extern LightWork lbl_806BB7E0[2];

/* The level counter fn_802BF7E8 ticks and wraps at 60000. */
#define LIGHT_TICK_WRAP 60000

/* ------------------------------------------------------------------------------------------------ */
/* functions                                                                                         */
/* ------------------------------------------------------------------------------------------------ */

/* Ages the three channel timers by one and hands the channel the given index selects to the
 * per-channel update. */
extern "C" void fn_802BEAAC(LightWork* self, u8 index)
{
    if (--self->channel[0].timer <= 0) {
        self->channel[0].enable = 0;
        self->channel[0].timer = 0;
    }

    if (--self->channel[1].timer <= 0) {
        self->channel[1].enable = 0;
        self->channel[1].timer = 0;
    }

    if (--self->channel[2].timer <= 0) {
        self->channel[2].enable = 0;
        self->channel[2].timer = 0;
    }

    if (index == 3) {
        fn_802BE7E8(self, &self->channel[2], index);
    } else if (index == 1) {
        fn_802BE7E8(self, &self->channel[1], index);
    } else {
        fn_802BE7E8(self, &self->channel[0], index);
    }
}

/* Formats the per-map light file name into the caller's buffer. */
extern "C" int fn_802BECB8(char* buffer, u8 mapno)
{
    return sprintf(buffer, lbl_805D1EB8, mapno);
}

/* Returns the light work of the loaded map: the second record while the special-map flag is up, the
 * first one otherwise. */
extern "C" LightWork* fn_802BECD0(void)
{
    if (screen_split_mode_ck() != 0 && (s8)my_player_no() != 0) {
        return &lbl_806BB7E0[1];
    }
    return &lbl_806BB7E0[0];
}

/* Runs the map's light-record builder with the level counter's bank switched to the given one and
 * puts the previous bank back afterwards. */
extern "C" void* fn_802BEDE8(s8 bank)
{
    s32 previous;
    void* result;

    previous = my_player_no();
    my_player_no_set(bank);
    result = (void*)(u32)camera_work_ck();
    my_player_no_set(previous);
    return result;
}

/* Queries the light resource for the given id with the bank reset and then with it raised, letting
 * each failed query fall through to the id's own record handler. */
extern "C" void fn_802BEE3C(u8 id, void* arg)
{
    s32 previous;

    previous = my_player_no();

    my_player_no_set(0);
    if (!fn_802B0688(&fn_802BECD0()->resource->entry)) {
        fn_802BC564(id, arg);
    }

    my_player_no_set(1);
    if (!fn_802B0688(&fn_802BECD0()->resource->entry)) {
        fn_802BC564(id, arg);
    }

    my_player_no_set(previous);
}

/* Constructs the two light-work records the module keeps. */
extern "C" void fn_802BEEE0(void)
{
    __construct_array(&lbl_806BB7E0[0], (void*)fn_802BEF00, NULL, 0x4F8, 2);
}

/* Constructs a light work record: every sub-record in turn, then the three channels. */
extern "C" LightWork* fn_802BEF00(LightWork* self)
{
    LightChannel* end;
    LightChannel* channel;

    fn_802BF1D8(&self->root);
    fn_802BF180(&self->anim);
    fn_802BF138(&self->lights);
    fn_802BF0AC(&self->params);
    fn_802BF044(&self->colors);
    fn_802BEFB4(&self->entry);

    channel = &self->channel[0];
    end = &self->channel[3];
    do {
        fn_802BEF84(channel);
        channel++;
    } while (channel < end);
    return self;
}


/* Constructs a channel's position vector. */
extern "C" LightChannel* fn_802BEF84(LightChannel* self)
{
    VEC3_ctor(&self->pos);
    return self;
}

/* Constructs a record of four vectors and a LightTriple. */
extern "C" LightQuadTriple* fn_802BEFB4(LightQuadTriple* self)
{
    VEC3_ctor(&self->v[0]);
    VEC3_ctor(&self->v[1]);
    VEC3_ctor(&self->v[2]);
    VEC3_ctor(&self->v[3]);
    fn_802BF004(&self->inner);
    return self;
}

/* Constructs a record of three vectors. */
extern "C" LightTriple* fn_802BF004(LightTriple* self)
{
    VEC3_ctor(&self->a);
    VEC3_ctor(&self->b);
    VEC3_ctor(&self->c);
    return self;
}

/* Constructs a record of eight vectors. */
extern "C" LightOctal* fn_802BF044(LightOctal* self)
{
    VEC3_ctor(&self->v[0]);
    VEC3_ctor(&self->v[1]);
    VEC3_ctor(&self->v[2]);
    VEC3_ctor(&self->v[3]);
    VEC3_ctor(&self->w[0]);
    VEC3_ctor(&self->w[1]);
    VEC3_ctor(&self->w[2]);
    VEC3_ctor(&self->w[3]);
    return self;
}

/* Constructs a record of four vectors. */
extern "C" LightQuad* fn_802BF138(LightQuad* self)
{
    VEC3_ctor(&self->v[0]);
    VEC3_ctor(&self->v[1]);
    VEC3_ctor(&self->v[2]);
    VEC3_ctor(&self->v[3]);
    return self;
}

/* Constructs a record of four vectors and two two-element vector runs. */
extern "C" LightParams* fn_802BF0AC(LightParams* self)
{
    nw4r::math::VEC3* vec;
    nw4r::math::VEC3* end;

    VEC3_ctor(&self->v0);
    VEC3_ctor(&self->v1);
    VEC3_ctor(&self->v2);
    VEC3_ctor(&self->v3);

    vec = &self->runs[0];
    end = &self->runs[2];
    do {
        VEC3_ctor(vec);
        vec++;
    } while (vec < end);

    end = &self->runs[4];
    do {
        VEC3_ctor(vec);
        vec++;
    } while (vec < end);
    return self;
}

/* Constructs a record of four vectors, a mid block and a trailing vector. */
extern "C" LightQuadMid* fn_802BF180(LightQuadMid* self)
{
    VEC3_ctor(&self->v[0]);
    VEC3_ctor(&self->v[1]);
    VEC3_ctor(&self->v[2]);
    VEC3_ctor(&self->v[3]);
    MTX34_ctor(&self->mid);
    VEC3_ctor(&self->tail);
    return self;
}

/* Constructs the scene root record's fifteen vectors. */
extern "C" LightRoot* fn_802BF1D8(LightRoot* self)
{
    VEC3_ctor(&self->v0);
    VEC3_ctor(&self->v1);
    VEC3_ctor(&self->v2);
    VEC3_ctor(&self->v3);
    VEC3_ctor(&self->v4);
    VEC3_ctor(&self->v5);
    VEC3_ctor(&self->v6);
    VEC3_ctor(&self->v7);
    VEC3_ctor(&self->v8);
    VEC3_ctor(&self->v9);
    VEC3_ctor(&self->v10);
    VEC3_ctor(&self->v11);
    VEC3_ctor(&self->v12);
    VEC3_ctor(&self->v13);
    VEC3_ctor(&self->v14);
    return self;
}

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

/* Moves the record to the arm its state selects. */
extern "C" void fn_802C2F08(LightArm* self)
{
    switch (self->state) {
    case 0:
        fn_802C2DD0(self);
        break;
    case 1:
        fn_802C2E6C(self);
        break;
    }
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
