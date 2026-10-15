/*
 * g3d/g3d_resanmcamera.cpp - nw4r g3d camera-animation channel evaluator (fn_8008A28C) and its accessors.
 * RANGE. .text 0x8008A220-0x8008A664 (4 functions); extab, extabindex, .data 0x8058FF20-0x80590010.  fn_8008A28C
 *   passes "g3d_resanmcamera.cpp"; fn_8008A220's two strings close the fragment.
 * FLAGS. `cflags_g3d`; file-scope `#pragma pool_data off` (each string its own `lis`/`addi`), `#pragma peephole
 *   off` and `#pragma fp_contract off`.
 * NAMES. Map stems.  res_user_data_init is a GUESS (the dead-stripped caller of the `_ac.h` constructor).
 * RESIDUALS. none in code or data; the extra dead-stripped function makes flipcheck read .text/extab as
 *   different, while the linked DOL is byte-identical.
 * SHAPES. fn_8008A220/fn_8008A284 are `g3d_resuser_ac.h` inline accessors: `inline`, referenced by the
 *   dead-stripped res_user_data_init at the head of the file, so MWCC emits their weak copies first and their
 *   strings after the out-of-line ones (the inline tail, playbook 80).  fn_8008A644 is defined after
 *   fn_8008A28C, in retail's order.
 */

#include "types.h"
#include "nw4r/g3d/res_common.h"
#include "g3d/g3d_anmscn.h" /* fn_800689B0 (rule 2) */
#include "g3d/g3d_resanm.h"  /* fn_800898B0 (rule 2) */

#pragma pool_data off

#pragma peephole off
#pragma fp_contract off

/* nw4r::db::Panic(const char*, int, const char*, ...) - the owner's real C++ declaration, never the
 * mangled spelling (rule 9). */
namespace nw4r {
namespace db {

void Panic(const char *file, int line, const char *message, ...);

} /* namespace db */
} /* namespace nw4r */

/*
 * One animation channel slot: a constant, or a self-relative offset to the key data (nw4r's
 * `ResAnmData` shape).  Which one it is, is decided by a flag bit in `ResAnmCamData::flags`.
 */
typedef union {
    f32 value;  /* +0x00 */
    u32 offset; /* +0x00 */
} ResAnmCamValue; /* size: 0x4 */

/* The resolved `ResAnmCamData` that `fn_800689B0` returns: the projection type, the flag word and the
 * 15 channel slots. */
typedef struct {
    u8 unused_0x00[0x14];      /* +0x00 */
    s32 projType;              /* +0x14 */
    u32 flags;                 /* +0x18 */
    u8 unused_0x1C[0x4];       /* +0x1C */
    ResAnmCamValue field_0x20; /* +0x20 */
    ResAnmCamValue field_0x24; /* +0x24 */
    ResAnmCamValue field_0x28; /* +0x28 */
    ResAnmCamValue field_0x2C; /* +0x2C */
    ResAnmCamValue field_0x30; /* +0x30 */
    ResAnmCamValue field_0x34; /* +0x34 */
    ResAnmCamValue field_0x38; /* +0x38 */
    ResAnmCamValue field_0x3C; /* +0x3C */
    ResAnmCamValue field_0x40; /* +0x40 */
    ResAnmCamValue field_0x44; /* +0x44 */
    ResAnmCamValue field_0x48; /* +0x48 */
    ResAnmCamValue field_0x4C; /* +0x4C */
    ResAnmCamValue field_0x50; /* +0x50 */
    ResAnmCamValue field_0x54; /* +0x54 */
    ResAnmCamValue field_0x58; /* +0x58 */
} ResAnmCamData; /* size: 0x5C */

/* The camera animation result `fn_8008A28C` fills in. */
typedef struct {
    u32 field_0x00; /* +0x00 */
    u32 field_0x04; /* +0x04 */
    f32 field_0x08; /* +0x08 */
    f32 field_0x0C; /* +0x0C */
    f32 field_0x10; /* +0x10 */
    f32 field_0x14; /* +0x14 */
    f32 field_0x18; /* +0x18 */
    f32 field_0x1C; /* +0x1C */
    f32 field_0x20; /* +0x20 */
    f32 field_0x24; /* +0x24 */
    f32 field_0x28; /* +0x28 */
    f32 field_0x2C; /* +0x2C */
    f32 field_0x30; /* +0x30 */
} ResAnmCamAnmResult; /* size: 0x34 */

/* Forward declarations for the unit's own functions (C linkage: the map spells them `fn_XXXXXXXX`). */
extern "C" {
    void *fn_8008A220(void *self, u32 value);
    void fn_8008A284(u32 *self, u32 value);
    void fn_8008A28C(void *self, ResAnmCamAnmResult *pResult, f32 frame);
    f32 fn_8008A644(ResAnmCamValue *self, f32 frame, u32 flag);
}

/* Sets a sub-resource offset word through the `g3d_resuser_ac.h` constructor.  Nothing calls it, so the
 * linker strips it, as it stripped retail's caller; it is what makes MWCC emit the two inline accessors
 * below at the head of the unit. */
extern "C" void* res_user_data_init(void* self, u32 value) /* untyped: opaque handle - the resource word */
{
    return fn_8008A220(self, value);
}

/* `g3d_resuser_ac.h`'s offset constructor: stores the word, then asserts it is 4-byte aligned. */
extern "C" inline void *fn_8008A220(void *self, u32 value)
{
    fn_8008A284((u32 *)self, value);
    if (value & 3) {
        nw4r::db::Panic("g3d_resuser_ac.h", 87,
            "NW4R:Failed assertion !((u32)p & 0x3)");
    }
    return self;
}

/* Stores the offset word. */
extern "C" inline void fn_8008A284(u32 *self, u32 value)
{
    *self = value;
}

/* Fills in the camera result for one frame from the resolved animation data. */
extern "C" void fn_8008A28C(void *self, ResAnmCamAnmResult *pResult, f32 frame)
{
    ResAnmCamData *res;
    u32 flags;
    s32 camType;

    u32 valid = IS_VALID_PTR(pResult);
    if (!valid) {
        nw4r::db::Panic("g3d_resanmcamera.cpp", 46,
            "NW4R:Pointer Error\npResult(=%p) is not valid pointer.", pResult);
    }

    res = (ResAnmCamData *)fn_800689B0(self);
    flags = res->flags;
    camType = flags & 1;
    pResult->field_0x00 = flags & 3;
    pResult->field_0x04 = res->projType;

    pResult->field_0x08 = fn_8008A644(&res->field_0x20, frame, (flags & 0x20000) != 0);
    pResult->field_0x0C = fn_8008A644(&res->field_0x24, frame, (flags & 0x40000) != 0);
    pResult->field_0x10 = fn_8008A644(&res->field_0x28, frame, (flags & 0x80000) != 0);
    pResult->field_0x14 = fn_8008A644(&res->field_0x2C, frame, (flags & 0x100000) != 0);
    pResult->field_0x18 = fn_8008A644(&res->field_0x30, frame, (flags & 0x200000) != 0);
    pResult->field_0x1C = fn_8008A644(&res->field_0x34, frame, (flags & 0x400000) != 0);

    switch (camType) {
    case 0:
        pResult->field_0x20 = fn_8008A644(&res->field_0x38, frame, (flags & 0x20000000) != 0);
        pResult->field_0x24 = fn_8008A644(&res->field_0x3C, frame, (flags & 0x40000000) != 0);
        pResult->field_0x28 = fn_8008A644(&res->field_0x40, frame, (flags & 0x80000000) != 0);
        break;
    case 1:
        pResult->field_0x20 = fn_8008A644(&res->field_0x44, frame, (flags & 0x02000000) != 0);
        pResult->field_0x24 = fn_8008A644(&res->field_0x48, frame, (flags & 0x04000000) != 0);
        pResult->field_0x28 = fn_8008A644(&res->field_0x4C, frame, (flags & 0x08000000) != 0);
        pResult->field_0x2C = fn_8008A644(&res->field_0x50, frame, (flags & 0x10000000) != 0);
        break;
    default:
        nw4r::db::Panic("g3d_resanmcamera.cpp", 99,
            "NW4R Internal error\nUnknown camera-type (=%d)", camType);
        break;
    }

    switch (res->projType) {
    case 0:
        pResult->field_0x30 = fn_8008A644(&res->field_0x54, frame, (flags & 0x800000) != 0);
        break;
    case 1:
        pResult->field_0x30 = fn_8008A644(&res->field_0x58, frame, (flags & 0x1000000) != 0);
        break;
    default:
        nw4r::db::Panic("g3d_resanmcamera.cpp", 121,
            "NW4R Internal error\nUnknown projection-type (=%d)", res->projType);
        break;
    }
}

/* Evaluates one channel: the constant when its flag is set, else the interpolated key data. */
extern "C" f32 fn_8008A644(ResAnmCamValue *self, f32 frame, u32 flag)
{
    if (flag) {
        return self->value;
    }
    return fn_800898B0((u8 *)self + self->offset, frame);
}
