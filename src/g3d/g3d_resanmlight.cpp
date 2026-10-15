/*
 * g3d/g3d_resanmlight.cpp - nw4r g3d light-animation channel evaluator (fn_8008F8E4), its `g3d_resanm_ac.h`
 *   bool-channel accessors and the frame and offset helpers.
 * RANGE. .text 0x8008F8E4-0x8009001C (5 functions); extab, extabindex, .data 0x80590440-0x80590660.  The left seam
 *   is "g3d_resanmlight.cpp" (.data 0x80590440); the right one is `g3d/g3d_resanmscn.cpp`'s: fn_8009001C onward
 *   read that TU's `ResName` records (0x80590660) and `_ac.h` strings, and extabindex 0x80021D8C opens it.
 * NAMES. Map stems, defined `extern "C"` (the dump answers `zz_` placeholders).
 * RESIDUALS. fn_8008F8E4: register colouring only - retail holds the resource in r30, the flags in r29, the light
 *   kind in r28 and the second-colour flag in r27; ours r29/r28/r27/r30 (declaration permutations reach no closer
 *   than one more instruction).
 *   flipcheck: `.text` is the one row above; `.data` byte-identical to the 0x208 bytes the object emits (the
 *   0x18-byte tail is fill before `g3d/g3d_resanmscn.cpp`'s 32-aligned name records).
 * SHAPES. fn_8008FE88/fn_8008FD04 are `inline` under `#pragma dont_inline on`, defined in that order: their asserts'
 *   strings are the inline tail (weak `@STRING@` copies, reversed), and their weak copies follow fn_8008F8E4.
 *   `base += offset` in fn_80090000 keeps the branch (a plain `if`/`return` if-converts to branchless code), and
 *   `#pragma peephole on` keeps its base word in r0.
 */

#include "types.h"
#include "nw4r/g3d/res_common.h"        /* IS_VALID_PTR (rule 1) */
#include "g3d/g3d_anmscn.h"            /* fn_8006584C/fn_80066E80, owner g3d/g3d_anmscn.cpp (rule 2) */
#include "g3d/g3d_rescommon.h"          /* nw4r::g3d::ResDic / ResName (rule 2) */
#include "g3d/g3d_anmchr.h"             /* fn_8006268C/fn_80062914/fn_80062750 (rule 2) */
#include "font/flfnt.h"                 /* fn_8005B1E4 (rule 2) */
#include "g3d/g3d_resanmamblight.h"     /* fn_8008A188/fn_8008A1A8, owner g3d/g3d_resanmamblight.c */
#include "g3d/g3d_anmshp.h" /* ResDic::GetNumData's owner (rule 2) */
#include "g3d/g3d_resanmtexsrt.h" /* fn_80092330 (rule 2) */
#include "MSL_C/alloc.h" /* floorf (rule 2) */
#include "g3d/g3d_resanmcamera.h" /* fn_8008A644 (rule 2) */

#pragma pool_data off

#pragma peephole off
#pragma fp_contract off

/* nw4r::db::Panic - the assert failure handler (variadic).  Called through its namespace owner, never
 * its mangled spelling (rule 9). */
namespace nw4r {
namespace db {

void Panic(const char *file, int line, const char *message, ...);

} /* namespace db */
} /* namespace nw4r */

/* The result record the light evaluator fills in (only the words this unit writes are named). */
typedef struct {
    /* +0x00 */ u32 typeWord;
    /* +0x04 */ u32 flags;
    /* +0x08 */ f32 nearChannel;
    /* +0x0C */ f32 midChannel;
    /* +0x10 */ f32 farChannel;
    /* +0x14 */ f32 field_0x14;
    /* +0x18 */ f32 field_0x18;
    /* +0x1C */ f32 field_0x1C;
    /* +0x20 */ u32 inlineColor;
    /* +0x24 */ u32 field_0x24;
    /* +0x28 */ f32 field_0x28;
    /* +0x2C */ f32 field_0x2C;
    /* +0x30 */ u32 field_0x30;
    /* +0x34 */ f32 field_0x34;
    /* +0x38 */ u32 inlineColor2;
    /* +0x3C */ f32 field_0x3C;
} ResAnmLightResult; /* size: 0x40 */

/* Forward declarations for the unit's own functions (defined below in address order). */
extern "C" f32 fn_8008FFFC(void);
extern "C" u32 fn_80090000(u32 *self, s32 offset);

/* The `g3d_resanm_ac.h` bool-channel accessors: `inline`, called out of line (`dont_inline`), defined in the
 * order their strings are created (the inline tail is emitted in reverse). */
#pragma dont_inline on

/* Tests one bit of the packed word array `self` at index `index`. */
extern "C" inline s32 fn_8008FE88(u32 *self, s32 index)
{
    u32 valid = IS_VALID_PTR(self);

    if (!valid) {
        nw4r::db::Panic("g3d_resanm_ac.h", 0x45, "NW4R:Pointer Error\npData(=%p) is not valid pointer.", self);
    }
    if (index < 0) {
        nw4r::db::Panic("g3d_resanm_ac.h", 0x46, "iframeOffset is out of bounds(%d)\n%d <= iframeOffset not satisfied.", index, 0);
    }
    u32 word = self[((u32)index) >> 5];
    u32 bit = 0x80000000u >> (((u32)index) & 31);

    return (word & bit) != 0;
}

/* Tests one bit of the packed channel array `*self` at index `index`. */
extern "C" inline s32 fn_8008FD04(u32 *self, s32 index)
{
    u32 valid = IS_VALID_PTR(self);

    if (!valid) {
        nw4r::db::Panic("g3d_resanm_ac.h", 0x55, "NW4R:Pointer Error\npData(=%p) is not valid pointer.", self);
    }
    if (*self == 0) {
        nw4r::db::Panic("g3d_resanm_ac.h", 0x56, "NW4R:Failed assertion pData->toResBoolAnmFramesData != 0");
    }
    if (index < 0) {
        nw4r::db::Panic("g3d_resanm_ac.h", 0x57, "iframeOffset is out of bounds(%d)\n%d <= iframeOffset not satisfied.", index, 0);
    }
    return fn_8008FE88((u32 *)((u8 *)self + *self), index);
}

/* Evaluates the light animation at `frame` into `pResult`. */
extern "C" void fn_8008F8E4(void *arg0, ResAnmLightResult *pResult, f32 frame)
{
    ResAnmLightConfig *res;
    u32 flags;
    f32 clamped;
    s32 kind;
    s32 second;
    s32 useSecond;
    u32 valid = IS_VALID_PTR(pResult);

    if (!valid) {
        nw4r::db::Panic("g3d_resanmlight.cpp", 0x2C, "NW4R:Pointer Error\npResult(=%p) is not valid pointer.", pResult);
    }
    res = fn_80066E80(arg0);
    flags = res->flags;
    clamped = fn_8008A1A8((u16 *)(fn_80090000((u32 *)arg0, res->field_0x04) + 0x34), frame);
    pResult->flags = flags & 0x3F;
    if ((flags & 0x00800000) == 0) {
        if (fn_8008FD04(&res->dataOffset, (s32)fn_8008FFFC()) != 0) {
            pResult->flags |= 0x4;
        }
    }
    if ((pResult->flags & 0x4) != 0) {
        kind = flags & 3;
        second = (flags & 0x8) != 0;
        useSecond = (kind != 0) || (second != 0);
        pResult->nearChannel = fn_8008A644(&res->channel_0x24, frame, (flags & 0x00080000) != 0);
        pResult->midChannel = fn_8008A644(&res->channel_0x28, frame, (flags & 0x00100000) != 0);
        pResult->farChannel = fn_8008A644(&res->channel_0x2C, frame, (flags & 0x00200000) != 0);
        fn_8005B1E4(&pResult->inlineColor,
            fn_8008A188(&res->channel_0x30, clamped, (flags & 0x00400000) != 0));
        if (useSecond) {
            pResult->field_0x14 = fn_8008A644(&res->channel_0x34, frame, (flags & 0x01000000) != 0);
            pResult->field_0x18 = fn_8008A644(&res->channel_0x38, frame, (flags & 0x02000000) != 0);
            pResult->field_0x1C = fn_8008A644(&res->channel_0x3C, frame, (flags & 0x04000000) != 0);
        }
        switch (kind) {
        case 0:
            pResult->field_0x24 = res->field_0x40;
            pResult->field_0x28 = fn_8008A644(&res->channel_0x44, frame, (flags & 0x10000000) != 0);
            pResult->field_0x2C = fn_8008A644(&res->channel_0x48, frame, (flags & 0x20000000) != 0);
            break;
        case 1:
            break;
        case 2:
            pResult->field_0x24 = res->field_0x40;
            pResult->field_0x30 = res->field_0x4C;
            pResult->field_0x34 = fn_8008A644(&res->channel_0x50, frame, (flags & 0x08000000) != 0);
            pResult->field_0x28 = fn_8008A644(&res->channel_0x44, frame, (flags & 0x10000000) != 0);
            pResult->field_0x2C = fn_8008A644(&res->channel_0x48, frame, (flags & 0x20000000) != 0);
            break;
        default:
            nw4r::db::Panic("g3d_resanmlight.cpp", 0x8C, "NW4R Internal error\nUnknown light-type (=%d)", kind);
            break;
        }
        if (second) {
            fn_8005B1E4(&pResult->inlineColor2,
                fn_8008A188(&res->channel_0x54, clamped, (flags & 0x40000000) != 0));
            pResult->field_0x3C = fn_8008A644(&res->channel_0x58, frame, (flags & 0x80000000) != 0);
            pResult->typeWord = res->field_0x14;
        }
    }
}

/* The global frame thunk (a tail call into the shared frame getter). */
extern "C" f32 fn_8008FFFC(void)
{
    return floorf();
}

#pragma peephole on
/* Resolves a sub-resource offset against the object base; a zero offset means "none". */
extern "C" u32 fn_80090000(u32 *self, s32 offset)
{
    u32 base = *self;

    if (offset != 0) {
        base += offset;
        return base;
    }
    return 0;
}
#pragma peephole off
