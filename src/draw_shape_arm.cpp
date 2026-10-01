/*
 * `draw_shape_arm.cpp` - the 2D capture sprite and the draw-shape state setters: `.text` 0x80055EC4..0x80056F24 (the sprite
 * constructor/re-initialiser, the GX FIFO writers, the `lbl_8066ACF8` state block setters and `draw_shape_arm`).
 *
 * Seam: phase 4 cut the old `draw_shape.cpp` at 0x80055EC4 (its `.text` ran 0x80054C64..0x80056F24) and put the part below it in
 * the unit that continues `fn_8004CAD8`'s `drawshape_*` half.  The `.data` pool with the `"draw_shape.cpp"` string stays with
 * `draw_shape.cpp`; this unit's own data is `.sbss` 0x8, `.sdata` 0x10 and `.sdata2` 0x30 (the float pool the bodies read).
 * Name: GUESS - the range holds `draw_shape_arm` (the map's real symbol) and the sprite state it arms, and the `draw_shape_*` stem is
 * its band's (evidence class 3).
 *
 * Residuals (measured when the range was one unit, `draw_shape.cpp`): `fn_800565E4`/`fn_80056954` 62.50 % (retail sign/zero-extends
 * each argument before the FIFO store; MWCC folds the extension away), `draw_shape_arm` 70.42 % and `fn_80056E00` 71.43 % (register
 * colouring only).  Not reconstructed: the two big GX draws `fn_80055F58` (0x678) / `fn_80056608` (0x34C) and `fn_80056AF0` (0x310).
 */

#include "types.h"
#include "gx.h"
#include "nw4r/math.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "mh3_pad.h" /* copyVec3 (rule 2) */
#include "draw_shape_arm.h" /* the `.sdata2` float pool this unit owns */
#include "fn_80056F24/lbl_8066ACF8.h" /* DrawShapeState / lbl_8066ACF8, owned by fn_80056F24.cpp (rule 2) */
#include "EXI/GXSetTexCoordGen2.h" /* the SDK texcoord generator (rule 2) */
#include "fn_80047398.h" /* color_rgba_copy, owned by userdata_item.cpp's range (rule 2) */

extern "C" {
void  fn_80057810(u32 a, u32 b, u32 c);

/* SDK. */
void  DCInvalidateRange(void* p, u32 size);
}


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
    self->field_0x30 = draw_shape_f32_zero;
    self->field_0x34 = draw_shape_f32_zero;
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
    self->field_0x30 = draw_shape_sprite_scale_x;
    self->field_0x34 = draw_shape_sprite_scale_y;
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
    s->field_0x44 = draw_shape_f32_zero;
    s->field_0x48 = draw_shape_f32_zero;
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
    color_rgba_copy((u8*)&s->color_0x50, (const u8*)color);
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

extern "C" void draw_shape_arm(u32 a, u32 b, u32 c) {
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
