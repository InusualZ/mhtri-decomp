/*
 * nw4r g3d: g3d_resanmamblight.cpp - the ambient-light channel evaluator, `.text`
 * 0x80089F94-0x8008A220 (6 functions).
 *
 * Re-cut from `auto/800898B0_fn_800898B0.c` (docs/plan.md 12 item 5).  `fn_8008A000` panics with
 * `__FILE__` = `g3d_resanmamblight.cpp` (line 44) and the retail data fragment is
 * 0x8058FE90-0x8058FF20, so the range is the real `g3d_resanmamblight.cpp`.  `fn_80089F94` cites the
 * `g3d_resanmscn_ac.h` header string (lbl_8058FF08); `fn_8008A204` carries no data reference (the
 * report's alternative cut is 0x8008A204) - unpinned, measure to settle.
 *
 * Naming note: the map carries only `fn_XXXXXXXX` names here (docs/plan.md 6.5 rule 7); renaming a
 * symbol needs the map and the source in one edit (playbook 31).
 *
 * Language: langcheck says the retail TU is C++, but the source is C-idiom (`fn_80089F94` passes a
 * `void*` to a `u32*` parameter, which C++ rejects) and this re-cut must not rewrite bodies, so the
 * extension stays `.c`; the C++ conversion rides the language lane.
 * `#pragma peephole off` / `#pragma fp_contract off` are file-scoped.  Registered `Object(NonMatching,
 * ...)` in lib g3d.
 */


#include "types.h"
#include "nw4r/g3d/res_common.h"

#pragma peephole off
#pragma fp_contract off

/* nw4r::db::Panic(const char*, int, const char*, ...) */
extern void Panic__Q24nw4r2dbFPCciPCce(const char *file, int line, const char *msg, ...);

/* nw4r math / resource helpers owned by unsplit units.  Their map names are the C++ manglings, so a C
 * declaration spells the mangled name verbatim - exactly what a C unit calling a C++ function looks
 * like (`fn_80066C8C` is `fn_80066C8C__FPv`, renamed in the map with the unit that calls it,
 * `g3d/g3d_anmscn.cpp`). */
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

/*
 * The pointer-range check nw4r's resource macros expand to: a pointer is valid when it lies in
 * one of the Wii memory regions.  Shared by g3d_resanm.c and g3d_resanmamblight.c, so it lives
 * in nw4r/g3d/res_common.h (rule 1).
 */

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
