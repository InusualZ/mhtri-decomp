/*
 * g3d/g3d_resanmamblight.c - nw4r g3d ambient-light channel evaluator fn_8008A000 and its pointer, frame-clamp and
 *   offset helpers.
 * RANGE. .text 0x80089F94-0x8008A220 (6 functions); extab, extabindex, .data 0x8058FE90-0x8058FF20 (fn_8008A000
 *   passes "g3d_resanmamblight.cpp"), .sdata2 0x80795EC0-0x80795ED0.  fn_80089F94 cites "g3d_resanmscn_ac.h"
 *   (lbl_8058FF08); fn_8008A204 carries no data reference, so 0x8008A204 is an alternative right edge.
 * NAMES. Map stems.  The retail TU is C++; the source stays `.c` because it is C-idiom (fn_80089F94 passes a
 *   `void*` to a `u32*` parameter), so `fn_80066C8C__FPv` and the `Panic` mangling are spelled verbatim.
 * RESIDUALS. fn_8008A000: two string addresses differ with the `.data` layout, and one instruction more.
 *   fn_8008A204: retail keeps the base word in r0 (`lwz r0,0(r3); add r3,r0,r4`), ours in r3.
 *   flipcheck: `.text` 0x290 of 0x28C; `.data` is 0x8A of 0x90 and its bytes differ.
 * SHAPES. File-scope `#pragma peephole off` and `#pragma fp_contract off`.
 */


#include "types.h"
#include "nw4r/g3d/res_common.h"

#pragma peephole off
#pragma fp_contract off

/* nw4r::db::Panic(const char*, int, const char*, ...) */
extern void Panic__Q24nw4r2dbFPCciPCce(const char *file, int line, const char *msg, ...);

/* The C++ helper `fn_80066C8C` (owner `g3d/fn_80063888.cpp`), spelled with its mangling because this unit is C. */
extern u32 *fn_80066C8C__FPv(void *obj);

/* The resolved animation object `fn_80066C8C__FPv` returns: a type word, a frame count and the
 * channel data at +0x18. */
typedef struct {
    u32 unused_0x00;  /* +0x00 */
    u32 field_0x04;   /* +0x04 */
    u8 pad_0x08[0xC]; /* +0x08 */
    u32 field_0x14;   /* +0x14 */
    u32 field_0x18;   /* +0x18 */
} ResAnmAmbLightData; /* size: 0x1C */

/* The result object fn_8008A000 fills in. */
typedef struct {
    u32 field_0x00; /* +0x00 */
    u32 field_0x04; /* +0x04 */
} ResAnmAmbLightResult; /* size: 0x8 */

/* Forward declarations for the unit's own functions. */
void *fn_80089F94(void *self, u32 value);
void fn_80089FF8(u32 *self, u32 value);
void fn_8008A000(void *arg0, ResAnmAmbLightResult *pResult, f32 frame);
s32 fn_8008A188(u32 *self, s32 flag, f32 frame);
f32 fn_8008A1A8(u16 *count, f32 frame);
s32 fn_8008A204(u32 *self, s32 offset);


/* Stores a pointer and asserts its 4-byte alignment. */
void *fn_80089F94(void *self, u32 value)
{
    fn_80089FF8(self, value);
    if (value & 3) {
        Panic__Q24nw4r2dbFPCciPCce("g3d_resanmscn_ac.h", 40,
            "NW4R:Failed assertion !((u32)p & 0x3)");
    }
    return self;
}

/* Stores a word. */
void fn_80089FF8(u32 *self, u32 value)
{
    *self = value;
}

/* Evaluates an ambient-light channel at a frame. */
void fn_8008A000(void *arg0, ResAnmAmbLightResult *pResult, f32 frame)
{
    ResAnmAmbLightData *res;
    u32 flags;

    u32 valid = IS_VALID_PTR(pResult);
    if (!valid) {
        Panic__Q24nw4r2dbFPCciPCce("g3d_resanmamblight.cpp", 44,
            "NW4R:Pointer Error\npResult(=%p) is not valid pointer.", pResult);
    }
    res = (ResAnmAmbLightData *)fn_80066C8C__FPv(arg0);
    flags = res->field_0x14;
    fn_8008A1A8((u16 *)(fn_8008A204((u32 *)arg0, res->field_0x04) + 0x34), frame);
    pResult->field_0x00 = flags & 3;
    pResult->field_0x04 = fn_8008A188(&res->field_0x18, (flags & 0x80000000) != 0, frame);
}

/* Returns a sub-resource pointer, or evaluates it when it has no own data. */
s32 fn_8008A188(u32 *self, s32 flag, f32 frame)
{
    if (flag) {
        return *self;
    }
    return fn_80089C6C((u32 *)((u8 *)self + *self), frame);
}

/* Clamps a frame into the range of a channel. */
f32 fn_8008A1A8(u16 *count, f32 frame)
{
    if (frame <= 0.0f) {
        return 0.0f;
    }
    if ((f32)*count <= frame) {
        return (f32)*count;
    }
    return frame;
}

/* Returns a resource offset pointer, or null when the offset is zero. */
s32 fn_8008A204(u32 *self, s32 offset)
{
    s32 base = *self;

    if (offset != 0) {
        base += offset;
        return base;
    }
    return 0;
}
