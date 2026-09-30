/*
 * `fn_80056F24.cpp` - the game-root screen fade / filter / glare band.
 *
 * `.text` 0x80056F24..0x80059550 (0x262C B, 59 symbols), `extab` 0x80007218..0x800072D0,
 * `extabindex` 0x8001F494..0x8001F5A8, `.ctors` 0x8056F2CC..0x8056F2D0 (the `fn_800594DC` static-init
 * word).  This is the maximal unclaimed run the discovery capped at 0x80056F24 (`attribute.py`
 * `proposal/80056F24_fn_80056F24.cpp`); the neighbouring runs were registered alongside it this session.
 *
 * Naming - which evidence class decided it.
 *   * Class 1 (`__FILE__` string) FAILS for this range.  Every data reference the range makes is a float
 *     pool word in `.sdata2` (0x80795C48..0x80795C98), the fade table `lbl_8058B178` in `.data`, and the
 *     two work blocks `lbl_8066AC88` / `lbl_8066ACF8` in `.bss`; there is no assert/log string at all, so
 *     no source-file name is carried by the range.  The nearest one, `s_draw_shape.cpp` at 0x8058178C, is
 *     referenced only from the *previous* proposal (0x80054C64..0x80056F24) - not from here.
 *   * Class 2 (runtime-dump name) FAILS: `dumpmap.py lookup 0x80056F24` answers `FUN_80056f24` and every
 *     other address in the range answers `zz_<addr>_` - placeholders, not evidence.
 *   * Class 3/4 decides it: the file keeps the map's `fn_80056F24` stem, registered in the game-root band
 *     (`main` lib, `cflags_main`) exactly where its link neighbours sit - `fn_80040598.cpp`,
 *     `mh3_pad.cpp`, `fn_80047398.cpp`, the `8004C9A0`/`8004CAD8` runs and `draw_shape.cpp`, the unit
 *     whose range ends where this one begins.  The range is *probably* the tail of the `draw_shape.cpp`
 *     TU (it drives the same `lbl_8066ACF8` state block and its `extab`/`.ctors` continue `draw_shape`'s
 *     without a gap), but a source-file name is only evidence where the string is referenced, and the
 *     string sits in the other proposal - so this registration stays disjoint and keeps the map stem.  A
 *     later pass that finds the name can re-home it; the seam is a proposal cap, not a proven TU edge.
 *
 * The blocks this range drives (private views - `draw_shape.cpp` names the 0x8066ACF8 one
 * `DrawShapeState` and `main.cpp` names `Screen_w`'s one `ScreenWork`; the type names differ here so the
 * two registrations do not define one type twice, rule 1, until their halves merge):
 *
 *   * `lbl_8066AC88` (.bss, 0x70 B) - the four fade slots in SoA form.  One array per field, index
 *     `i * 4`, so `color[4]` at +0x00, `state[4]` at +0x10, `rate[4]` at +0x30, `tbl[4]` at +0x40,
 *     `target[4]` at +0x50 and `inc[4]` at +0x60.
 *   * `lbl_8066ACF8` (.bss, 0x14C B) - the draw/filter state block the fade and glare setters write and
 *     `fn_80058BD0`/`fn_8005920C` drive.  Only the bytes this range touches are named.
 *   * `lbl_8058B178` (.data, 0xA8 B) - the fade step table, 0x18 B per entry.
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked
 * `python tools/symbols/dumpmap.py lookup <addr>` for every address in the range - `FUN_`/`zz_`
 * placeholders; and `grep -E '^fn_8005(6F|7|8|9)' config/RMHE08/symbols.txt` is the whole inventory
 * minus the six named entries GlareFilter_on/fade_set/fade_reset/get_fade_stat/filter_reset/
 * setFilterPrio).  The mangled names the range does carry (`GlareFilter_on__Fv`, `fade_set__Fll`,
 * `fade_reset__Fl`, `get_fade_stat__Fl`, `filter_reset__Fv`, `setFilterPrio__FUc`) are spelled by their
 * real declarations, never as callable identifiers (rule 9).
 *
 * Status: partial (phase B first pass).  The straightforward setters, slots and FIFO writers are
 * reconstructed; the large GX setup bodies (`fn_80056F24`, `fn_800572F0`, `fn_800579A4`, `fn_8005880C`,
 * `fn_80058BD0`, `fn_80057DE0`, `fn_80057EF4`, `fn_80058140`, `fn_8005920C`, the `disp_beta` trio) are
 * not written yet.  The per-symbol scores are in the unit notes / outbox.
 */

#include "types.h"
#include "gx.h"
#include "unsplit/unknown.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */

/* The peephole pass is off for this whole TU: the retail FIFO writers keep the narrowing the pass would
 * fold away (`clrlwi r0,r3,24; stb r0,-0x8000(r3)` in `fn_800572A4`/`fn_80058B64`, `extsh r0,r3,16` in
 * `fn_800572CC`/`fn_80058B8C`/`fn_80057DC8`), and they are the only functions in the range whose shape
 * depends on it.  The same per-unit device `ef/eft002.cpp` and `ef/eft004.cpp` use (playbook 39). */
#pragma peephole off

/* ---------------------------------------------------------------------------------------------------
 * The work blocks.
 * ------------------------------------------------------------------------------------------------- */

/* The fade table entry `lbl_8058B178` (0x18 B): the colour at +0x04, the from/to slots at +0x08/+0x0C
 * and the step count at +0x10.  `fade_set` reads all four; only the unread head is padding. */
typedef struct FadeStep {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ u32 color_0x04;
    /* +0x08 */ s32 from_0x08;
    /* +0x0C */ s32 to_0x0C;
    /* +0x10 */ s32 steps_0x10;
    /* +0x14 */ u32 pad_0x14;
} FadeStep; /* size: 0x18 */

extern "C" FadeStep lbl_8058B178[];

/* `lbl_8066AC88` (.bss, 0x70 B): the four fade slots, one array per field (`fn_80058030` zeroes all
 * 0x70, `fn_80058140` walks the four `state` entries). size: 0x70 */
typedef struct FadeWork {
    /* +0x00 */ u32 color[4];   /* ARGB, the top byte is the alpha the walk eases */
    /* +0x10 */ s32 state[4];   /* 0 idle, 1 fade-in, 2 fade-out, 3 finished */
    /* +0x20 */ u8 pad_0x20[0x10];
    /* +0x30 */ s32 rate[4];    /* the per-tick alpha step */
    /* +0x40 */ s32 tbl[4];     /* the `lbl_8058B178` index `fade_set` was called with */
    /* +0x50 */ s32 target[4];  /* the alpha the slot eases to */
    /* +0x60 */ s32 inc[4];     /* the table entry's step count */
} FadeWork; /* size: 0x70 */

extern "C" FadeWork lbl_8066AC88;

/* The 2D sprite panel at +0xB0 of the draw state (`fn_800578B4`/`fn_8005792C` memset and size it; the
 * +0x2C..+0x40 runs are the eight edge offsets `fn_80057DE0`/`fn_80057EF4` fill). size: 0x44 */
typedef struct FilterPanel {
    /* +0x00 */ u8 field_0x00;
    /* +0x01 */ u8 pad_0x01[0x1B];
    /* +0x1C */ f32 field_0x1C;   /* scale x */
    /* +0x20 */ f32 field_0x20;   /* scale y */
    /* +0x24 */ s16 field_0x24;   /* scaled height x */
    /* +0x26 */ s16 field_0x26;   /* scaled height y */
    /* +0x28 */ u8 pad_0x28[0x1C];
} FilterPanel; /* size: 0x44 */

/* The sprite slot at +0x3C of the draw state (`fn_800591B8` delegates to its owner and ticks the
 * count).  Its owner type lives in the neighbouring `draw_shape.cpp` registration; this is the partial
 * view this range touches. size: 0x38 */
typedef struct DrawSpriteSlot {
    /* +0x00 */ u8 pad_0x00[0x18];
    /* +0x18 */ s32 count;
    /* +0x1C */ u8 pad_0x1C[0x1C];
} DrawSpriteSlot; /* size: 0x38 */

/* The screen-fade/filter state block `lbl_8066ACF8` (.bss, 0x14C B; `fn_800584F8` memsets all of it).
 * Only the bytes this range touches are named. size: 0x14C */
typedef struct GameDrawState {
    /* +0x000 */ u8 pad_0x00[0x01];
    /* +0x001 */ u8 field_0x01;   /* render-path selector */
    /* +0x002 */ u8 field_0x02;   /* draw-enable bitmask */
    /* +0x003 */ u8 pad_0x03[0x01];
    /* +0x004 */ u8 field_0x04;
    /* +0x005 */ u8 field_0x05;   /* alpha-combine mode */
    /* +0x006 */ u8 field_0x06;   /* blend-src factor `setFilterPrio` flips */
    /* +0x007 */ u8 field_0x07;   /* blend-dst factor */
    /* +0x008 */ u8 pad_0x08[0x08];
    /* +0x010 */ void* field_0x10;   /* the begin/draw callback `fn_800584F8` installs */
    /* +0x014 */ void* field_0x14;
    /* +0x018 */ void* field_0x1C;   /* the end callback */
    /* +0x01C */ u8 pad_0x1C[0x0C];
    /* +0x028 */ void* field_0x28;   /* the +0x3C sprite block */
    /* +0x02C */ void* field_0x2C;   /* the +0x74 block */
    /* +0x030 */ void* field_0x30;   /* the +0xB0 panel */
    /* +0x034 */ u8 pad_0x034[0x08];
    /* +0x03C */ DrawSpriteSlot sprite_0x3C; /* the sprite slot `fn_800591B8` ticks */
    /* +0x074 */ u8 field_0x074;  /* glare filter on */
    /* +0x075 */ u8 pad_0x075[0x01];
    /* +0x076 */ u8 field_0x076;  /* glare colour r */
    /* +0x077 */ u8 field_0x077;  /* glare colour g */
    /* +0x078 */ u8 field_0x078;  /* glare colour b */
    /* +0x079 */ u8 field_0x079;  /* glare fade mode */
    /* +0x07A */ u8 field_0x07A;  /* glare fade counter */
    /* +0x07B */ u8 pad_0x07B[0x09];
    /* +0x084 */ u32 field_0x084; /* glare blend mode */
    /* +0x088 */ u8 pad_0x088[0x04];
    /* +0x08C */ u32 field_0x08C; /* glare filter mode */
    /* +0x090 */ u8 pad_0x090[0x20];
    /* +0x0B0 */ FilterPanel panel_0x0B0;
    /* +0x0F4 */ u8 pad_0x0F4[0x28];
    /* +0x11C */ u8 field_0x11C;  /* filter priority `setFilterPrio` stores */
    /* +0x11D */ u8 pad_0x11D[0x03];
    /* +0x120 */ u8 vec_0x120[0x08]; /* `fn_800584E8` copies this out */
    /* +0x128 */ u8 vec_0x128[0x08]; /* the screen-size pair `fn_800584D8` copies out */
    /* +0x130 */ u32 field_0x130; /* the capture buffer size */
    /* +0x134 */ u32 field_0x134; /* the capture buffer address */
    /* +0x138 */ u32 field_0x138; /* the copy destination */
    /* +0x13C */ u8 field_0x13C;  /* screen-fade phase: 0 idle, 1..3 easing */
    /* +0x13D */ u8 field_0x13D;  /* fade colour r, set to 0xFF by `fn_80059374` */
    /* +0x13E */ u8 field_0x13E;  /* fade colour g */
    /* +0x13F */ u8 field_0x13F;  /* fade colour b */
    /* +0x140 */ u8 field_0x140;  /* the eased alpha byte */
    /* +0x141 */ u8 pad_0x141[0x03];
    /* +0x144 */ f32 field_0x144; /* the fade value `fn_80059374`/`fn_80059420` ease */
    /* +0x148 */ f32 field_0x148;
} GameDrawState; /* size: 0x14C */

extern "C" GameDrawState lbl_8066ACF8;

/* `Screen_w` (0x8065903C, .bss, unsplit): only the aspect ratio this range halves.  `main.cpp` defines
 * it with a wider `ScreenWork` view, so this stays a local read (like `draw_shape.cpp`'s `ScreenGeom`).
 * size: 0xC */
typedef struct FilterScreenGeom {
    /* +0x0 */ u16 width;
    /* +0x2 */ u16 height;
    /* +0x4 */ u8 pad_0x04[0x04];
    /* +0x8 */ f32 aspect;
} FilterScreenGeom; /* size: 0xC */

extern "C" FilterScreenGeom Screen_w;

/* The `.sdata2` pool words this range reads (`lbl_80795C48` 0.0f, `lbl_80795C58` 0.5f, `lbl_80795C70`
 * 0.6f, `lbl_80795C74` 0.0f, `lbl_80795C7C` 255.0f, `lbl_80795C80` 0.7111111f). */
extern "C" f32 lbl_80795C48;
extern "C" f32 lbl_80795C58;
extern "C" f32 lbl_80795C70;
extern "C" f32 lbl_80795C74;   /* 0.0f */
extern "C" f32 lbl_80795C78;   /* 1.0f */
extern "C" f32 lbl_80795C7C;   /* 255.0f */
extern "C" f32 lbl_80795C80;   /* 0.7111111f */
extern "C" f32 lbl_80795C84;   /* 25.0f */
extern "C" f32 lbl_80795C88;   /* 90.0f */
extern "C" f32 lbl_80795C8C;   /* 50.0f */
extern "C" f32 lbl_80795C90;   /* 18.0f */
extern "C" f32 lbl_80795C94;   /* 5.0f */
extern "C" f32 lbl_80795C98;   /* 0.65f */

/* ---------------------------------------------------------------------------------------------------
 * Declarations.  The `fn_*` callees are unsplit band neighbours, the `GX*` symbols are the SDK's, and
 * `SinFIdx` is called through its owner namespace (rule 9).
 * ------------------------------------------------------------------------------------------------- */

namespace nw4r { namespace math { f32 SinFIdx(f32); } }

extern "C" {
void GXSetTexCoordGen2(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f);

void copyVec2(void* dst, const void* src);
void fn_8004030C(struct _MH_VEC2* v);
void fn_800569CC(void);
void fn_80056F04(void);

s32   fn_80056AF0(void* p);
void  fn_80056A3C(s32 count);
}

/* The screen-size pair is a free C++ function (`get_ScreenSize(_MH_VEC2*)`); its map spelling is a
 * mangling, so it is declared by signature (rule 9).  The band neighbours below are plain `fn_`
 * thunks: the target object references them by their plain map name, so they carry C linkage. */
void get_ScreenSize(struct _MH_VEC2* v);
extern "C" {
void fn_80055EC4(void* block);
void fn_80056E1C(void* block);
void fn_80055F58(void);
void fn_80056F24(void);
void fn_800579A4(void);
}

/* ---------------------------------------------------------------------------------------------------
 * The FIFO writers (one store-width each; the same family the other `auto` units carry).
 * ------------------------------------------------------------------------------------------------- */

extern "C" void fn_80057290(void) {
}

extern "C" void fn_80057294(f32 x, f32 y) {
    GXWGFifo.f32 = x;
    GXWGFifo.f32 = y;
}

extern "C" void fn_800572A4(u32 r, u32 g, u32 b, u32 a) {
    GXWGFifo.u8 = r;
    GXWGFifo.u8 = g;
    GXWGFifo.u8 = b;
    GXWGFifo.u8 = a;
}

extern "C" void fn_800572CC(s32 x, s32 y) {
    GXWGFifo.s16 = x;
    GXWGFifo.s16 = y;
}

/* The six-argument TEV coordgen wrapper (the caller supplies args 1-4, the two selectors are fixed). */
extern "C" void fn_800572E4(u32 a, u32 b, u32 c, u32 d) {
    GXSetTexCoordGen2(a, b, c, d, 0, 0x7D);
}

extern "C" void fn_80057DB8(void) {
}

extern "C" void fn_80057DBC(s32 v) {
    GXWGFifo.s32 = v;
}

extern "C" void fn_80057DC8(s32 x, s32 y) {
    GXWGFifo.s16 = x;
    GXWGFifo.s16 = y;
}

extern "C" void fn_80058B50(void) {
}

extern "C" void fn_80058B54(f32 x, f32 y) {
    GXWGFifo.f32 = x;
    GXWGFifo.f32 = y;
}

extern "C" void fn_80058B64(u32 r, u32 g, u32 b, u32 a) {
    GXWGFifo.u8 = r;
    GXWGFifo.u8 = g;
    GXWGFifo.u8 = b;
    GXWGFifo.u8 = a;
}

extern "C" void fn_80058B8C(s32 x, s32 y) {
    GXWGFifo.s16 = x;
    GXWGFifo.s16 = y;
}

extern "C" void fn_80058BA4(u32 a, u32 b, u32 c, u32 d) {
    GXSetTexCoordGen2(a, b, c, d, 0, 0x7D);
}

/* ---------------------------------------------------------------------------------------------------
 * Glare-filter setters.
 * ------------------------------------------------------------------------------------------------- */

/* `fn_80057810`: the glare blend/filter selectors. arg0 picks the source fade mode, arg1 the filter
 * mode and arg2 the compare mode. */
extern "C" void fn_80057810(u8 arg0, u8 arg1, u8 arg2) {
    GameDrawState* s = &lbl_8066ACF8;
    if (arg0 == 0) {
        s->field_0x07A = 1;
        s->field_0x079 = 1;
    } else {
        s->field_0x079 = 1;
        s->field_0x07A = 3;
    }
    if (arg1 == 0) {
        s->field_0x084 = 3;
    } else {
        s->field_0x084 = 2;
    }
    if (arg2 == 0) {
        s->field_0x08C = 1;
        return;
    }
    s->field_0x08C = 2;
}

/* `fn_80057888`: the glare colour. */
extern "C" void fn_80057888(u8 r, u8 g, u8 b) {
    lbl_8066ACF8.field_0x076 = r;
    lbl_8066ACF8.field_0x077 = g;
    lbl_8066ACF8.field_0x078 = b;
}

void GlareFilter_on(void) {
    lbl_8066ACF8.field_0x074 = 1;
}

/* The two panel initialisers: clear the 0x44 B panel and seed the scale/height pair from the screen
 * aspect.  `fn_800578B4` takes the panel, `fn_8005792C` uses the one inside the state block. */
extern "C" void fn_800578B4(FilterPanel* p) {
    memset(p, 0, 0x44);
    p->field_0x1C = lbl_80795C58;
    p->field_0x24 = (s16)(Screen_w.aspect * lbl_80795C58);
    p->field_0x20 = lbl_80795C58;
    p->field_0x26 = (s16)(Screen_w.aspect * lbl_80795C58);
}

extern "C" void fn_8005792C(void) {
    FilterPanel* p = &lbl_8066ACF8.panel_0x0B0;
    memset(p, 0, 0x44);
    p->field_0x1C = lbl_80795C58;
    s16 h = (s16)(Screen_w.aspect * lbl_80795C58);
    p->field_0x24 = h;
    p->field_0x20 = lbl_80795C58;
    p->field_0x26 = h;
}

/* ---------------------------------------------------------------------------------------------------
 * The fade slots.
 * ------------------------------------------------------------------------------------------------- */

/* `fn_80058008`/`fn_8005801C`: the panel enable flag. */
extern "C" void fn_80058008(void) {
    lbl_8066ACF8.panel_0x0B0.field_0x00 = 1;
}

extern "C" void fn_8005801C(void) {
    lbl_8066ACF8.panel_0x0B0.field_0x00 = 0;
}

/* Zero all four fade slots. */
extern "C" void fn_80058030(void) {
    memset(&lbl_8066AC88, 0, 0x70);
}

/* `fade_set(slot, tableIndex)`: arm slot `slot` from the `lbl_8058B178` step entry `tableIndex`.  The
 * coloured alpha the walk eases is the table's +0x04; the tick count is |steps| and the per-tick step is
 * the alpha span divided by it, rounded up. */
void fade_set(s32 slot, s32 tableIndex) {
    FadeWork* fw = &lbl_8066AC88;
    FadeStep* step = &lbl_8058B178[tableIndex];
    fw->tbl[slot] = tableIndex;
    fw->color[slot] = step->color_0x04;
    s32 span = step->to_0x0C - step->from_0x08;
    if (span < 0) {
        span -= span * 2;
        fw->state[slot] = 2;
    } else {
        fw->state[slot] = 1;
    }
    fw->inc[slot] = step->steps_0x10;
    s32 ticks = step->steps_0x10;
    s32 abs_ticks = ticks < 0 ? -ticks : ticks;
    fw->rate[slot] = span / abs_ticks;
    if (span > abs_ticks * fw->rate[slot]) {
        fw->rate[slot] += 1;
    }
    fw->target[slot] = step->to_0x0C;
}

/* `fade_reset(slot)`: clear the six fields of one slot. */
void fade_reset(s32 slot) {
    lbl_8066AC88.color[slot] = 0;
    lbl_8066AC88.rate[slot] = 0;
    lbl_8066AC88.state[slot] = 0;
    lbl_8066AC88.tbl[slot] = 0;
    lbl_8066AC88.target[slot] = 0;
    lbl_8066AC88.inc[slot] = 0;
}

/* `fn_8005811C`: idle test for a slot. */
extern "C" u32 fn_8005811C(u8 slot) {
    return lbl_8066AC88.state[slot] == 0;
}

/* `fn_8005825C`: the slot's packed ARGB colour. */
extern "C" u32 fn_8005825C(s32 slot) {
    return lbl_8066AC88.color[slot];
}

s32 get_fade_stat(s32 slot) {
    return lbl_8066AC88.state[slot];
}

/* ---------------------------------------------------------------------------------------------------
 * The filter/glare dispatch helpers.
 * ------------------------------------------------------------------------------------------------- */

/* `setFilterPrio`: store the priority (`fn_80059374` reads it back through +0x11C) and flip the
 * blend-factor pair around it. */
void setFilterPrio(u8 prio) {
    lbl_8066ACF8.field_0x11C = prio;
    if (prio == 0) {
        lbl_8066ACF8.field_0x06 = 2;
        lbl_8066ACF8.field_0x07 = 3;
    } else {
        lbl_8066ACF8.field_0x07 = 2;
        lbl_8066ACF8.field_0x06 = 3;
    }
}

/* `filter_reset`: re-run the glare teardown and the panel reset. */
void filter_reset(void) {
    fn_800569CC();
    fn_80056F04();
    fn_8005792C();
}

/* The two `Screen_w` vector copies into the state block. */
extern "C" void fn_800584D8(void* dst) {
    copyVec2(dst, lbl_8066ACF8.vec_0x128);
}

extern "C" void fn_800584E8(void* dst) {
    copyVec2(dst, lbl_8066ACF8.vec_0x120);
}

/* `system_copy_filter_request`/`system_copy_filter_arm`/`system_copy_filter_clear`: the copy-filter handshake with the system block. */
extern "C" void system_copy_filter_request(void) {
    if (system_w.field_0x868 != 1) {
        system_w.field_0x868 = 1;
    }
}

extern "C" void system_copy_filter_arm(void) {
    if (system_w.field_0x868 == 2) {
        system_w.pad_0x869[0] = 1;
    }
}

extern "C" void system_copy_filter_clear(void) {
    system_w.field_0x868 = 0;
    system_w.pad_0x869[0] = 0;
}

/* ---------------------------------------------------------------------------------------------------
 * The eased screen-fade alpha.
 * ------------------------------------------------------------------------------------------------- */

/* `fn_800591AC`: the sine ease, `SinFIdx(0.7111111f * x)` (the owner namespace, rule 9). */
extern "C" f32 fn_800591AC(f32 x) {
    return nw4r::math::SinFIdx(lbl_80795C80 * x);
}

/* `fn_8005913C`: advance the eased alpha for an active fade, or finish it when the value is spent. */
extern "C" void fn_8005913C(GameDrawState* s) {
    if (s->field_0x13C >= 2) {
        if (s->field_0x144 <= lbl_80795C74) {
            s->field_0x13C = 0;
        } else {
            s->field_0x140 = (u8)(lbl_80795C7C * fn_800591AC(s->field_0x144));
        }
    }
}

/* ---------------------------------------------------------------------------------------------------
 * The static-init chain (the `.ctors` word at 0x8056F2CC points at `fn_800594DC`).
 * ------------------------------------------------------------------------------------------------- */

/* The sub-object chain `fn_800594DC` walks: the global state's +0x3C block, whose +0x28 node is the
 * `VEC3_ctor` target.  Only the node's base is used. size: 0x3C */
typedef struct CtorBlock {
    /* +0x00 */ u8 pad_0x00[0x28];
    /* +0x28 */ nw4r::math::VEC3 node_0x28;
    /* +0x34 */ u8 pad_0x34[0x08];
} CtorBlock; /* size: 0x3C */

/* `fn_800584F8`: initialise the whole draw/filter state block `lbl_8066ACF8` - the capture-buffer pair,
 * the three sub-blocks and their callbacks. */
extern "C" void fn_800584F8(void) {
    GameDrawState* s = &lbl_8066ACF8;
    memset(s, 0, 0x14C);
    fn_8004030C((struct _MH_VEC2*)s->vec_0x120);
    get_ScreenSize((struct _MH_VEC2*)s->vec_0x128);
    s->field_0x130 = 0x118000;
    s->field_0x134 = 0x90B48000;
    s->field_0x04 = 0;
    s->field_0x138 = 0x90E30000;
    s->field_0x01 = 4;
    s->field_0x02 = 1 | 2 | 4 | 8;
    s->field_0x10 = (void*)fn_80055F58;
    s->field_0x14 = (void*)fn_80056F24;
    s->field_0x1C = (void*)fn_800579A4;
    s->field_0x28 = (void*)&s->sprite_0x3C;
    s->field_0x2C = (void*)&s->field_0x074;
    s->field_0x30 = (void*)&s->panel_0x0B0;
    fn_80055EC4(&s->sprite_0x3C);
    fn_80056E1C(&s->field_0x074);
    fn_800578B4(&s->panel_0x0B0);
    s->field_0x05 = 1;
    s->field_0x06 = 2;
    s->field_0x07 = 3;
    s->field_0x148 = lbl_80795C70;
}

/* `fn_800591B8`: tick the sprite slot at +0x3C while its owner says it is not finished. */
extern "C" void fn_800591B8(GameDrawState* s) {
    if (fn_80056AF0(&s->sprite_0x3C) != 1) {
        s32 remaining = s->sprite_0x3C.count;
        if (remaining <= 0) {
            fn_80056A3C(remaining);
        } else {
            s->sprite_0x3C.count = remaining - 1;
        }
    }
}

/* `fn_80059374`: start (arg 0..2) or restart the screen fade.  A fade that is already easing (phase 1
 * or 2), or a finished phase-3 fade still above its floor, is left alone. */
extern "C" void fn_80059374(s32 arg0) {
    GameDrawState* s = &lbl_8066ACF8;
    if (arg0 == 0) {
        if ((u8)(s->field_0x13C + 0xFF) <= 1) {
            return;
        }
        if (s->field_0x13C == 3 && s->field_0x144 >= lbl_80795C84) {
            return;
        }
    }
    switch (arg0) {
    case 0:
        s->field_0x13C = 1;
        s->field_0x144 = lbl_80795C88;
        break;
    case 1:
        s->field_0x13C = 2;
        s->field_0x144 = lbl_80795C88;
        break;
    case 2:
        s->field_0x13C = 3;
        s->field_0x144 = lbl_80795C8C;
        break;
    }
    s->field_0x140 = 0xFF;
    s->field_0x13F = 0xFF;
    s->field_0x13E = 0xFF;
    s->field_0x13D = 0xFF;
}

/* `fn_80059420`: ease the fade value down by the phase's step until it reaches the floor. */
extern "C" void fn_80059420(void) {
    GameDrawState* s = &lbl_8066ACF8;
    switch (s->field_0x13C) {
    case 1:
        if (s->field_0x144 <= lbl_80795C74) {
            return;
        }
        s->field_0x144 -= lbl_80795C90;
        return;
    case 2:
        if (s->field_0x144 <= lbl_80795C74) {
            return;
        }
        s->field_0x144 -= lbl_80795C94;
        return;
    case 3:
        if (s->field_0x144 <= lbl_80795C74) {
            return;
        }
        s->field_0x144 -= lbl_80795C98;
        return;
    default:
        if (s->field_0x144 <= lbl_80795C74) {
            return;
        }
        s->field_0x144 -= lbl_80795C78;
        return;
    }
}

extern "C" void fn_800594C8(void) {
    lbl_8066ACF8.field_0x13C = 0;
}

/* The constructor chain `fn_800594DC` runs over the global state: the +0x3C sub-object's own +0x28
 * node is handed to `VEC3_ctor`.  Typed so no raw offset reaches a field (rule 6). */
extern "C" void* fn_8005951C(CtorBlock* self) {
    VEC3_ctor(&self->node_0x28);
    return self;
}

extern "C" void* fn_800594E8(GameDrawState* s) {
    fn_8005951C((CtorBlock*)&s->sprite_0x3C);
    return s;
}

extern "C" void fn_800594DC(void) {
    fn_800594E8(&lbl_8066ACF8);
}
