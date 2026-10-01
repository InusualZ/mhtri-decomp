/*
 * `draw_shape.cpp` - the 2D shape/texture draw layer of the menu/UI code: `.text` 0x8005270C..0x80055EC4 (the `drawshape_*` half that
 * `fn_8004CAD8.cpp` held up to 0x800547A8, then the FIFO writers, resource-handle helpers, atlas-cell geometry and the work block).
 * The 2D capture sprite and the state setters from 0x80055EC4 are `draw_shape_arm.cpp`.
 *
 * Evidence class 1 (a `__FILE__` string): every `nw4r::db::Panic` assert reachable from the range passes the bare source name
 * `"draw_shape.cpp"` (`.data:0x8058178C`) - `itemicon_tex_load` line 1982, `gpframe_tex_load` line 2060, `menu_tex_load` line 2136,
 * `village_tex_load` line 2592 and the `fn_80055AC4` resource loader line 2668 - and no other code references that string; the line
 * numbers rise with the functions' addresses, so the range is one source file.  Module: the game-root band (`main` lib,
 * `cflags_main`), next to `mh3_pad.cpp`.
 *
 * Phase 4: the unit is the fold of the old `draw_shape.cpp` head (0x80054C64..0x80055EC4) and the tail of `fn_8004CAD8.cpp`
 * (0x8005270C..0x80054C64, no body written yet); `fn_8004CAD8.cpp` keeps `.text` 0x8004CAD8..0x8005270C.
 *
 * Naming note: the map has only `fn_XXXXXXXX` for the unnamed functions here (the named entries are `itemicon_tex_load`,
 * `gpframe_tex_load`, `menu_tex_load`, `village_tex_load`, `set_arena_idx_switch`).
 *
 * Results (objdiff, source compiled with the unit's real `cflags_main` command line, measured as the old `draw_shape.cpp` range):
 * 100 %: fn_80054C64, res_file_assign, fn_80054FDC, fn_8005504C, set_arena_idx, set_arena_idx_switch, fn_80055E48, fn_80055E50,
 *        fn_80055E58, fn_80055E64.
 * 90-100 %: res_file_ctor 95.80, fn_80055D8C 93.00.  80-90 %: fn_80055D20 87.22, fn_80055C5C 86.73, fn_80055DC8 86.73,
 *        fn_80054C74 82.45, fn_80055CC4 81.96.
 *
 * Not reconstructed: the four named resource loaders (they need the nw4r `ResFile` owner type, rule 9) and the load callbacks
 * fn_800553B4/fn_80055534/fn_80055674/fn_80055770/fn_80055838/fn_80055AC4 (they need `load_file_req` and the `nw_resource` loader
 * entry points).
 */

#include "types.h"
#include "gx.h"
#include "nw4r/math.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the symbols deleted above (rule 2) */
#include "draw_shape_arm.h" /* the `.sdata2` float pool, owned by draw_shape_arm.cpp (rule 2) */

/* ---------------------------------------------------------------------------------------------------
 * The two engine globals this range drives.
 * ------------------------------------------------------------------------------------------------- */

/* `lbl_8066A920` (0x8066A920, .bss): the per-draw 2D shape work block.  `fn_80055E58` returns it and
 * `fn_80055E64` initialises the two matrices and the three sub-handles; `fn_80054C74` fills the GX tex /
 * tlut objects from the resource tables at +0x258 / +0x2DC, indexed by the shape id.  The unread bytes
 * are padding for this view. size: 0x364 */
typedef struct DrawShapeWork {
    /* +0x00 */ u8 pad_0x00[0x74];
    /* +0x74 */ MTX34 matrix_0x74;  /* MTX34_ctor's 3x4 float matrix */
    /* +0xA4 */ MTX34 matrix_0xA4;
    /* +0xD4 */ u32 field_0xD4;        /* GXInitTexObj wrapS / tlut name */
    /* +0xD8 */ u32 field_0xD8;        /* GXInitTexObj wrapT / tlut name */
    /* +0xDC */ u8 pad_0xDC[0x04];
    /* +0xE0 */ u32 field_0xE0;        /* res_tex_ctor handle */
    /* +0xE4 */ u32 field_0xE4;        /* res_pltt_ctor handle */
    /* +0xE8 */ u8 pad_0xE8[0x10];
    /* +0xF8 */ u32 texObj_0xF8[0x40]; /* the GXTexObj storage */
    /* +0x1F8 */ u32 tlutObj_0x1F8[0x18];
    /* +0x258 */ u32 texHandle_0x258[0x21];  /* resource handle table, indexed by shape id */
    /* +0x2DC */ u32 plttHandle_0x2DC[0x21];
    /* +0x360 */ u32 field_0x360;            /* res_tex_ctor handle */
} DrawShapeWork; /* size: 0x364 */

extern DrawShapeWork lbl_8066A920;


/* `lbl_807948A0`/`A1` (.sbss): the current arena index pair - `set_arena_idx` sets both, the
 * `set_arena_idx_switch` setter and the two getters below drive the second byte. */
extern u8 lbl_807948A0;
extern u8 lbl_807948A1;

/* ---------------------------------------------------------------------------------------------------
 * Declarations.  The `fn_*` callees are unsplit (their band brackets no sound module), the `GX*`/`DC*`
 * symbols are the SDK's, and the globals are unsplit too, so these are the local views the call sites
 * encode; `memset` comes from its owner's header (rule 2).
 * ------------------------------------------------------------------------------------------------- */

/* nw4r's debug panic (the map's mangling is `Panic__Q24nw4r2dbFPCciPCce`). */
namespace nw4r {
namespace db {
void Panic(const char* file, int line, const char* fmt, ...);
}  // namespace db
}  // namespace db

/* The assert strings in `.data`/`.sdata`. */
extern u8  lbl_80581910[];
extern char lbl_8058191C[];
extern char lbl_80581944[];

extern "C" {
/* The `drawshape_*` 2D helpers this range calls (0x8004F..0x80053.., same band). */
void  fn_800539B4(u16 idx);
void  draw_shape_tex_slots_clear(u32 a, u32 b);
void  draw_shape_tex_slot_set(void* tex, void* pltt, u16 idx, u32 arg);
s32   res_tex_has_pltt(void);
void  res_tex_assign(void* a, void* b);
void  res_tex_ctor(void* out, u32 arg);
u16   fn_80052D98(void);
void* fn_80052E30(void);
void* fn_80052E54(void);
s32   fn_80052EF0(void);
void  res_pltt_ctor(void* out, u32 arg);
void  res_pltt_assign(void* a, void* b);
void  fn_8009A490(void* a, void* b, void* c, void* d, void* e, void* f, void* g, void* h);
void  fn_8009A5C4(void* a, void* b, void* c, void* d, void* e, void* f, void* g, void* h);

/* SDK. */
void  GXInitTlutObj(void* obj, void* data, u32 fmt, u16 num);
void  GXLoadTlut(void* obj, u32 idx);
void  GXInitTexObj(void* obj, void* image, u16 w, u16 h, u32 fmt, u32 wrapS, u32 wrapT, u8 mipmap);
void  GXInitTexObjCI(void* obj, void* image, u16 w, u16 h, u32 fmt, u32 tlut, u32 wrapS, u32 wrapT,
                     u8 mipmap);
}

/* ---------------------------------------------------------------------------------------------------
 * The FIFO writers and the tiny accessors.
 * ------------------------------------------------------------------------------------------------- */

/* Two float FIFO writes (the `drawshape_set_scale` pair). */
extern "C" void fn_80054C64(f32 x, f32 y) {
    GXWGFifo.f32 = x;
    GXWGFifo.f32 = y;
}

extern "C" void fn_80054FDC(u32* dst, u32* src) {
    *dst = *src;
}

extern "C" void fn_8005504C(u32* dst, u32 value) {
    *dst = value;
}

/* Copy through the one-word handle and return it (the resource-handle initialiser). */
extern "C" u32* res_file_assign(u32* dst, u32* src) {
    fn_80054FDC(dst, src);
    return dst;
}

/* Store `value` through `dst` after asserting it is 32-byte aligned. */
extern "C" u32* res_file_ctor(u32* dst, u32 value) {
    fn_8005504C(dst, value);
    if ((value & 0x1F) != 0) {
        nw4r::db::Panic(lbl_80581944, 0x3C, lbl_8058191C);
    }
    return dst;
}

extern "C" u8 fn_80055E48(void) {
    return lbl_807948A0;
}

extern "C" u8 fn_80055E50(void) {
    return lbl_807948A1;
}

extern "C" void set_arena_idx(u8 idx) {
    lbl_807948A0 = idx;
    lbl_807948A1 = 0;
}

/* C++ linkage: the map name is the mangling `set_arena_idx_switch__FUc`. */
void set_arena_idx_switch(u8 idx) {
    lbl_807948A1 = idx;
}

/* ---------------------------------------------------------------------------------------------------
 * The two-digit / atlas-cell geometry helpers: they turn a value into a pair of (left, right) and
 * (top, bottom) 16-bit coordinates in the atlas.
 * ------------------------------------------------------------------------------------------------- */

extern "C" void fn_80055C5C(s16* a, s16* b, u32 raw) {
    u8 value = (u8)raw;
    s16 ones = (s16)((value % 10) * 0x26);
    a[0] = ones;
    a[1] = (s16)((value / 10) * 0x26);
    b[0] = (s16)(ones + 0x26);
    b[1] = (s16)(a[1] + 0x26);
}

extern "C" void fn_80055CC4(s16* a, s16* b, u32 raw) {
    s32 value = (u8)raw;
    s16 lo = (s16)((value % 8) * 0x1C);
    a[0] = lo;
    a[1] = (s16)(a[1] + (value >> 3) * 0x1C);
    b[0] = (s16)(lo + 0x1C);
    b[1] = (s16)(a[1] + 0x1C);
}

extern "C" void fn_80055D20(s16* a, s16* b, u32 raw) {
    u8 value = (u8)raw;
    s16 lo = (s16)((value % 12) * 0x14);
    a[0] = lo;
    a[1] = (s16)((value / 12 + 1) * 0x14);
    b[0] = (s16)(lo + 0x14);
    b[1] = (s16)(a[1] + 0x14);
}

extern "C" void fn_80055D8C(s16* a, s16* b, u32 raw) {
    u8 value = (u8)raw;
    s16 lo = (s16)(lbl_80581910[value] * 0x14);
    a[0] = lo;
    a[1] = 0;
    b[0] = (s16)(lo + 0x14);
    b[1] = 0x14;
}

extern "C" void fn_80055DC8(s16* a, s16* b, u32 raw) {
    u8 value = (u8)raw;
    s16 lo = (s16)((value % 12) << 6);
    a[0] = lo;
    a[1] = (s16)((value / 12) << 6);
    b[0] = (s16)(lo + 0x40);
    b[1] = (s16)(a[1] + 0x40);
}

/* ---------------------------------------------------------------------------------------------------
 * The per-draw work block.
 * ------------------------------------------------------------------------------------------------- */

extern "C" DrawShapeWork* fn_80055E64(DrawShapeWork* self) {
    MTX34_ctor(&self->matrix_0x74);
    MTX34_ctor(&self->matrix_0xA4);
    res_tex_ctor(&self->field_0xE0, 0);
    res_pltt_ctor(&self->field_0xE4, 0);
    res_tex_ctor(&self->field_0x360, 0);
    return self;
}

extern "C" DrawShapeWork* fn_80055E58(void) {
    return fn_80055E64(&lbl_8066A920);
}

/* Build one GX tex / tlut object pair for shape `idx`: pick the resource handles out of the tables,
 * initialise the tlut object when the texture is colour-indexed, then fill the GX tex object. */
extern "C" void fn_80054C74(u16 idx, u32* outTex, u32* outTlut) {
    DrawShapeWork* w = &lbl_8066A920;
    u32 texHandle;
    u32 plttHandle;
    u32 image;
    u16 width;
    u16 height;
    u32 format;
    u32 field_0x24;
    u32 field_0x20;
    u8 mipmap;

    fn_800539B4(idx);
    res_tex_ctor(&texHandle, w->texHandle_0x258[idx]);
    res_pltt_ctor(&plttHandle, w->plttHandle_0x2DC[idx]);
    *outTex = (u32)w->texObj_0xF8;
    *outTlut = (u32)w->tlutObj_0x1F8;
    w->field_0xD8 = 0;
    w->field_0xD4 = 0;

    if (res_tex_has_pltt() == 1) {
        if (fn_80052EF0() == 0) {
            *outTlut = 0;
        } else {
            u16 num = fn_80052D98();
            void* tlutFmt = fn_80052E30();
            void* tlutData = fn_80052E54();
            GXInitTlutObj((void*)*outTlut, tlutData, (u32)tlutFmt, num);
            GXLoadTlut((void*)*outTlut, 0);
            fn_8009A5C4(&texHandle, &image, &width, &height, &format, &field_0x24, &field_0x20,
                        &mipmap);
            GXInitTexObjCI((void*)*outTex, (void*)image, width, height, format, w->field_0xD4,
                           w->field_0xD8, mipmap, 0);
        }
    } else {
        *outTlut = 0;
        fn_8009A490(&texHandle, &image, &width, &height, &format, &field_0x24, &field_0x20, &mipmap);
        GXInitTexObj((void*)*outTex, (void*)image, width, height, format, w->field_0xD4,
                     w->field_0xD8, mipmap);
    }
}

