/*
 * `draw_shape.cpp` - the 2D shape/texture draw layer of the menu/UI code.
 *
 * Evidence class 1 (a `__FILE__` string): every `nw4r::db::Panic` assert reachable from this range passes
 * the bare source name `"draw_shape.cpp"` (`.data:0x8058178C`) - `itemicon_tex_load` line 1982,
 * `gpframe_tex_load` line 2060, `menu_tex_load` line 2136, `village_tex_load` line 2592 and the
 * `fn_80055AC4` resource loader line 2668 - and no other code in the image references that string
 * (`grep -rl 8058178C build/RMHE08/asm/` returns only this range's objects and the `.data` pool).  The
 * line numbers increase with the functions' addresses, so the whole range is one source file.  Module:
 * the address band sits between the game-root files (`main.cpp`, `sys_mem.cpp`, `fn_80040598.cpp`,
 * `nw_resource.cpp`) and `g3d/`, and the siblings of the same band were registered top-level
 * (`mh3_pad.cpp`); the file is therefore `src/draw_shape.cpp` in the `main` lib, `cflags_main`.
 *
 * Seam: the proposal covers only part of the TU.  `drawshape_exec`/`drawshape_init`/`drawshape_set_*`
 * (0x8005306C..0x800547A8) and the rest of the file live in the previous proposal
 * (`proposal/8004CAD8_fn_8004CAD8.cpp`); 0x80054C64 is a proposal cap, not a TU boundary.  The
 * registration below claims exactly the proposal's range and says so here.
 *
 * rule 7 deferred: the symbol map has only fn_XXXXXXXX for this range (checked
 * `grep -E '^fn_8005(4C|4F|50|53|55|56)' config/RMHE08/symbols.txt`, which is the whole inventory minus
 * the five named entries itemicon_tex_load/gpframe_tex_load/menu_tex_load/village_tex_load/
 * set_arena_idx_switch).
 *
 * Results (objdiff `report generate`, the official metric; target object = the split
 * `build/RMHE08/obj/draw_shape.o`, source compiled with the unit's real `cflags_main` command line):
 * 32 of the 45 functions have bodies, **28 of those are >= 80 %** (16 at 100 %).
 * 100 %: fn_80054C64, fn_80054FAC, fn_80054FDC, fn_8005504C, fn_80055E30, set_arena_idx_switch,
 *        fn_80055E48, fn_80055E50, fn_80055E58, fn_80055E64, fn_800565D0, fn_800565D4, fn_800565FC,
 *        fn_80056A20, fn_80056A3C, fn_80056F04.
 * 90-100 %: fn_80054FE8 95.80, fn_8005696C 95.21, fn_80056E1C 96.55, fn_80055D8C 93.00,
 *        fn_80056A84 92.22, fn_80055EC4 89.95.
 * 80-90 %: fn_80055D20 87.22, fn_80055C5C 86.73, fn_80055DC8 86.73, fn_800569CC 84.52,
 *        fn_80054C74 82.45, fn_80055CC4 81.96.
 *
 * Residuals, below the bar:
 *  - fn_800565E4 62.50 and fn_80056954 62.50: retail sign/zero-extends each argument (`extsh`/`clrlwi`)
 *    before the FIFO store; every source spelling tried (s16/u16 param, int + cast, a `s16`/`u16` local)
 *    makes MWCC fold the extension away, because the store truncates anyway.  Two instructions each.
 *  - fn_80056A54 70.42 and fn_80056E00 71.43: register colouring only - retail materialises the base in
 *    r7/r3 before the zero constant, ours the other way round.
 *
 * Not reconstructed (13): the four named resource loaders itemicon_tex_load/gpframe_tex_load/
 * menu_tex_load/village_tex_load (they need the nw4r `ResFile` owner type, rule 9), the load callbacks
 * fn_800553B4/fn_80055534/fn_80055674/fn_80055770/fn_80055838/fn_80055AC4 (they need `load_file_req` and
 * the `nw_resource` loader entry points, whose owner headers are not in this branch) and the two big GX
 * draws fn_80055F58 (0x678) / fn_80056608 (0x34C) plus fn_80056AF0 (0x310).
 */

#include "types.h"
#include "gx.h"
#include "nw4r/math.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the symbols deleted above (rule 2) */

/* ---------------------------------------------------------------------------------------------------
 * The two engine globals this range drives.
 * ------------------------------------------------------------------------------------------------- */

/* `lbl_8066A920` (0x8066A920, .bss): the per-draw 2D shape work block.  `fn_80055E58` returns it and
 * `fn_80055E64` initialises the two matrices and the three sub-handles; `fn_80054C74` fills the GX tex /
 * tlut objects from the resource tables at +0x258 / +0x2DC, indexed by the shape id.  The unread bytes
 * are padding for this view. size: 0x364 */
typedef struct DrawShapeWork {
    /* +0x00 */ u8 pad_0x00[0x74];
    /* +0x74 */ u8 matrix_0x74[0x30];  /* MTX34_ctor's 3x4 float matrix */
    /* +0xA4 */ u8 matrix_0xA4[0x30];
    /* +0xD4 */ u32 field_0xD4;        /* GXInitTexObj wrapS / tlut name */
    /* +0xD8 */ u32 field_0xD8;        /* GXInitTexObj wrapT / tlut name */
    /* +0xDC */ u8 pad_0xDC[0x04];
    /* +0xE0 */ u32 field_0xE0;        /* fn_80052BC0 handle */
    /* +0xE4 */ u32 field_0xE4;        /* fn_800534B0 handle */
    /* +0xE8 */ u8 pad_0xE8[0x10];
    /* +0xF8 */ u32 texObj_0xF8[0x40]; /* the GXTexObj storage */
    /* +0x1F8 */ u32 tlutObj_0x1F8[0x18];
    /* +0x258 */ u32 texHandle_0x258[0x21];  /* resource handle table, indexed by shape id */
    /* +0x2DC */ u32 plttHandle_0x2DC[0x21];
    /* +0x360 */ u32 field_0x360;            /* fn_80052BC0 handle */
} DrawShapeWork; /* size: 0x364 */

extern DrawShapeWork lbl_8066A920;

/* `lbl_8066ACF8` (0x8066ACF8, .bss): the draw-shape state block - blend mode/alpha (0x3C..0x3F), the
 * two scale floats, the tint colour at +0x50, the resource handle and position, and the 2D sprite
 * state `fn_80056AF0` walks.  Only the bytes this range touches are named. size: 0x138 */
typedef struct DrawShapeState {
    /* +0x00 */ u8 pad_0x00[0x3C];
    /* +0x3C */ u8 field_0x3C;      /* active flag (fn_80056A20/3C) */
    /* +0x3D */ u8 field_0x3D;
    /* +0x3E */ u8 field_0x3E;      /* alpha */
    /* +0x3F */ u8 field_0x3F;      /* blend mode */
    /* +0x40 */ u8 pad_0x40[0x04];
    /* +0x44 */ f32 field_0x44;     /* scale x */
    /* +0x48 */ f32 field_0x48;     /* scale y */
    /* +0x4C */ u8 pad_0x4C[0x04];
    /* +0x50 */ GXColor color_0x50; /* tint */
    /* +0x54 */ u32 field_0x54;
    /* +0x58 */ u32 field_0x58;
    /* +0x5C */ u8 field_0x5C;      /* draw enabled */
    /* +0x5D */ u8 field_0x5D;
    /* +0x5E */ u8 pad_0x5E[0x02];
    /* +0x60 */ u8 field_0x60;
    /* +0x61 */ u8 pad_0x61[0x01];
    /* +0x62 */ u8 field_0x62;
    /* +0x63 */ u8 field_0x63;
    /* +0x64 */ nw4r::math::VEC3 vec_0x64;  /* position */
    /* +0x70 */ u32 field_0x70;
    /* +0x74 */ u8 field_0x74;      /* fn_80056F04 clears it */
    /* +0x75 */ u8 pad_0x75[0xBF];
    /* +0x134 */ u32 field_0x134;   /* copied into the 2D sprite's +0x38 */
} DrawShapeState; /* size: 0x138 */

extern DrawShapeState lbl_8066ACF8;

/* `lbl_807948A0`/`A1` (.sbss): the current arena index pair - `fn_80055E30` sets both, the
 * `set_arena_idx_switch` setter and the two getters below drive the second byte. */
extern u8 lbl_807948A0;
extern u8 lbl_807948A1;

/* The 2D sprite block `fn_80055EC4` constructs and `fn_80056E1C` re-initialises: a captured EFB buffer
 * (+0x08/+0x0C), a GX texture object (+0x10), the screen-space rectangle and the copy scale. size: 0x3C */
typedef struct DrawShape2D {
    /* +0x00 */ u8 field_0x00;
    /* +0x01 */ u8 field_0x01;
    /* +0x02 */ u8 field_0x02;      /* alpha */
    /* +0x03 */ u8 field_0x03;      /* colour */
    /* +0x04 */ u8 field_0x04;      /* colour */
    /* +0x05 */ u8 pad_0x05[0x03];
    /* +0x08 */ u32 buf_0x08;       /* EFB capture buffer address */
    /* +0x0C */ u32 size_0x0C;      /* its byte size */
    /* +0x10 */ u32 texObj_0x10;    /* the GXTexObj / capture buffer */
    /* +0x14 */ u8 field_0x14;
    /* +0x15 */ u8 field_0x15;
    /* +0x16 */ u8 field_0x16;
    /* +0x17 */ u8 field_0x17;
    /* +0x18 */ u8 pad_0x18[0x04];
    /* +0x1C */ u16 width_0x1C;
    /* +0x1E */ u16 height_0x1E;
    /* +0x20 */ u8 pad_0x20[0x08];
    /* +0x28 */ u32 field_0x28;
    /* +0x2C */ u32 field_0x2C;
    /* +0x30 */ f32 field_0x30;
    /* +0x34 */ f32 field_0x34;
    /* +0x38 */ u32 field_0x38;
} DrawShape2D; /* size: 0x3C */

/* `Screen_w` (0x8065903C, .bss, unsplit): the screen geometry `fn_80056E1C` halves.  A private view;
 * the type in `main.cpp` is not in a header, so this names only the three fields read here. size: 0xC */
typedef struct ScreenGeom {
    /* +0x0 */ s16 width;
    /* +0x2 */ s16 height;
    /* +0x4 */ u8 pad_0x04[0x04];
    /* +0x8 */ f32 aspect;
} ScreenGeom; /* size: 0xC */

extern ScreenGeom Screen_w;

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

/* The float constants in `.sdata2` and the assert strings in `.data`/`.sdata`. */
extern f32 lbl_80795C18; /* 0.0f */
extern f32 lbl_80795C1C; /* 0.5f */
extern f32 lbl_80795C24; /* 1.0f */
extern f32 lbl_80795C40;
extern f32 lbl_80795C44;
extern u8  lbl_80581910[];
extern char lbl_8058191C[];
extern char lbl_80581944[];

extern "C" {
/* The `drawshape_*` 2D helpers this range calls (0x8004F..0x80053.., same band). */
void  fn_800539B4(u16 idx);
void  fn_800529A0(u32 a, u32 b);
void  fn_80052844(void* tex, void* pltt, u16 idx, u32 arg);
s32   fn_800528DC(void);
void  fn_80052B84(void* a, void* b);
void  fn_80052BC0(void* out, u32 arg);
u16   fn_80052D98(void);
void* fn_80052E30(void);
void* fn_80052E54(void);
s32   fn_80052EF0(void);
void  fn_800534B0(void* out, u32 arg);
void  fn_80053A90(void* a, void* b);
void  fn_8009A490(void* a, void* b, void* c, void* d, void* e, void* f, void* g, void* h);
void  fn_8009A5C4(void* a, void* b, void* c, void* d, void* e, void* f, void* g, void* h);
void  fn_8004C4F0(void* dst, const void* src);
void  fn_80057810(u32 a, u32 b, u32 c);

/* SDK. */
void  DCInvalidateRange(void* p, u32 size);
void  GXInitTlutObj(void* obj, void* data, u32 fmt, u16 num);
void  GXLoadTlut(void* obj, u32 idx);
void  GXInitTexObj(void* obj, void* image, u16 w, u16 h, u32 fmt, u32 wrapS, u32 wrapT, u8 mipmap);
void  GXInitTexObjCI(void* obj, void* image, u16 w, u16 h, u32 fmt, u32 tlut, u32 wrapS, u32 wrapT,
                     u8 mipmap);
void  GXSetTexCoordGen2(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f);
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
extern "C" u32* fn_80054FAC(u32* dst, u32* src) {
    fn_80054FDC(dst, src);
    return dst;
}

/* Store `value` through `dst` after asserting it is 32-byte aligned. */
extern "C" u32* fn_80054FE8(u32* dst, u32 value) {
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

extern "C" void fn_80055E30(u8 idx) {
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
    fn_80052BC0(&self->field_0xE0, 0);
    fn_800534B0(&self->field_0xE4, 0);
    fn_80052BC0(&self->field_0x360, 0);
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
    fn_80052BC0(&texHandle, w->texHandle_0x258[idx]);
    fn_800534B0(&plttHandle, w->plttHandle_0x2DC[idx]);
    *outTex = (u32)w->texObj_0xF8;
    *outTlut = (u32)w->tlutObj_0x1F8;
    w->field_0xD8 = 0;
    w->field_0xD4 = 0;

    if (fn_800528DC() == 1) {
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

/* ---------------------------------------------------------------------------------------------------
 * The 2D capture sprite.
 * ------------------------------------------------------------------------------------------------- */

extern "C" s32 fn_80055EC4(DrawShape2D* self) {
    void* buf = (void*)0x90C60000;

    memset(self, 0, 0x38);
    self->texObj_0x10 = (u32)buf;
    memset(buf, 0, 0x23000);
    DCInvalidateRange(buf, 0x23000);
    self->field_0x01 = 0;
    self->field_0x02 = 0x80;
    self->field_0x03 = 0x20;
    self->field_0x17 = 0;
    self->field_0x16 = 0;
    self->field_0x15 = 0;
    self->field_0x14 = 0;
    self->field_0x30 = lbl_80795C18;
    self->field_0x34 = lbl_80795C18;
    return 0;
}

extern "C" void fn_80056E1C(DrawShape2D* self) {
    void* buf = (void*)0x90C83000;

    memset(self, 0, 0x3C);
    self->width_0x1C = (u16)(Screen_w.width / 2);
    self->height_0x1E = (u16)(Screen_w.height / 2);
    self->size_0x0C = 0x46000;
    self->buf_0x08 = (u32)buf;
    memset(buf, 0, 0x46000);
    DCInvalidateRange((void*)self->buf_0x08, self->size_0x0C);
    self->field_0x02 = 0xFF;
    self->field_0x03 = 0xFF;
    self->field_0x04 = 0xFF;
    fn_80057810(0, 0, 1);
    self->field_0x28 = 0x40;
    self->field_0x2C = 0xC0;
    self->field_0x30 = lbl_80795C40;
    self->field_0x34 = lbl_80795C44;
    self->field_0x38 = lbl_8066ACF8.field_0x134;
}

/* ---------------------------------------------------------------------------------------------------
 * The GX FIFO writers and the texture-coordinate tail call.
 * ------------------------------------------------------------------------------------------------- */

extern "C" void fn_800565D0(void) {
}

extern "C" void fn_800565D4(f32 x, f32 y) {
    GXWGFifo.f32 = x;
    GXWGFifo.f32 = y;
}

extern "C" void fn_800565E4(int x, int y) {
    s16 sx = (s16)x;
    s16 sy = (s16)y;
    GXWGFifo.s16 = sx;
    GXWGFifo.s16 = sy;
}

extern "C" void fn_80056954(int x, int y) {
    u16 ux = (u16)x;
    u16 uy = (u16)y;
    GXWGFifo.u16 = ux;
    GXWGFifo.u16 = uy;
}

extern "C" void fn_800565FC(u32 a, u32 b, u32 c, u32 d) {
    GXSetTexCoordGen2(a, b, c, d, 0, 0x7D);
}

/* ---------------------------------------------------------------------------------------------------
 * The draw-shape state setters (lbl_8066ACF8).
 * ------------------------------------------------------------------------------------------------- */

extern "C" void fn_800569CC(void) {
    DrawShapeState* s = &lbl_8066ACF8;

    s->field_0x3C = 0;
    s->field_0x3D = 0;
    s->field_0x3E = 0x80;
    s->field_0x3F = 0x20;
    s->field_0x44 = lbl_80795C18;
    s->field_0x48 = lbl_80795C18;
    s->color_0x50.a = 0;
    s->color_0x50.b = 0;
    s->color_0x50.g = 0;
    s->color_0x50.r = 0;
    s->field_0x54 = 0;
    s->field_0x58 = 0;
    s->field_0x5C = 0;
    s->field_0x5D = 0;
}

extern "C" void fn_8005696C(u8 a, u8 b, u8 c, f32 scaleX, f32 scaleY, const void* color, u32 handle) {
    DrawShapeState* s = &lbl_8066ACF8;

    s->field_0x3E = a;
    s->field_0x3D = b;
    s->field_0x3F = c;
    s->field_0x44 = scaleX;
    s->field_0x48 = scaleY;
    fn_8004C4F0(&s->color_0x50, color);
    s->field_0x54 = handle;
}

extern "C" void fn_80056A20(void) {
    DrawShapeState* s = &lbl_8066ACF8;

    s->field_0x3C = 1;
    s->field_0x70 = 0;
}

extern "C" void fn_80056A3C(void) {
    DrawShapeState* s = &lbl_8066ACF8;

    s->field_0x3C = 0;
    s->field_0x70 = 0;
}

extern "C" void fn_80056A54(u32 a, u32 b, u32 c) {
    DrawShapeState* s = &lbl_8066ACF8;

    s->field_0x5C = 1;
    s->field_0x5D = 0;
    s->field_0x58 = a;
    s->field_0x54 = c;
    s->field_0x60 = (u8)b;
    s->field_0x62 = 0;
}

extern "C" void fn_80056A84(const nw4r::math::VEC3* pos, u32 handle, u8 flag) {
    DrawShapeState* s = &lbl_8066ACF8;

    s->field_0x5C = 1;
    s->field_0x5D = 0;
    s->field_0x58 = 0;
    s->field_0x54 = handle;
    s->field_0x60 = 0;
    s->field_0x62 = 1;
    copyVec3(&s->vec_0x64, pos);
    s->field_0x63 = flag;
}

extern "C" void fn_80056E00(void) {
    lbl_8066ACF8.field_0x58 = 0;
    lbl_8066ACF8.field_0x5C = 0;
    lbl_8066ACF8.field_0x5D = 0;
    fn_80056A3C();
}

extern "C" void fn_80056F04(void) {
    lbl_8066ACF8.field_0x74 = 0;
    fn_80057810(0, 0, 1);
}
