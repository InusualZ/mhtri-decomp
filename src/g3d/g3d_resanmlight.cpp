/*
 * nw4r g3d: g3d_resanmlight.cpp - the light-animation channel evaluator and the `ResAnmScn` light
 * channel accessors, `.text` 0x8008F8E4-0x800908FC (31 functions, 0x1018 B).
 *
 * Registered once, at its final home (docs/plan.md 12), from the pooled proposal
 * `g3d/g3d_resanmlight.cpp`.  Naming - evidence class 1, a `__FILE__` string: the range's first
 * body `fn_8008F8E4` passes `.data` 0x80590440 = "g3d_resanmlight.cpp" as its `nw4r::db::Panic` file
 * argument (`python tools/symbols/dumpmap.py lookup 0x80590440`).  The preceding registered unit
 * `g3d/g3d_resanmfog.cpp` ends exactly at 0x8008F8E4 and its header names that string as this unit's
 * first body, so the left seam is proven.
 *
 * The right seam 0x800908FC is the pinned first body of the next TU (`g3d/g3d_resanmscn.cpp`: its
 * `fn_800908FC` cites `g3d_resanmscn.cpp`) and is registered separately by its own worker; the sibling
 * proposals `800916FC`/`80093990`/`800947A4` carve the rest of the discovery cap.  The proposal brief's
 * stated 0x8008F8E4-0x80097D40 cap therefore overlaps those four siblings, so this unit is registered
 * at its own evidenced extent only, 0x8008F8E4-0x800908FC.  (The `.data` pool proves the cap is not one
 * TU: this range cites `g3d_resanmlight.cpp` and the `g3d_reslightset_ac.h`/`g3d_resanmamblight_ac.h`/
 * `g3d_resanmlight_ac.h`/`g3d_resanmfog_ac.h`/`g3d_resanmcamera_ac.h` inlined-assert headers, while
 * 0x800908FC on cites `g3d_resanmscn.cpp`, 0x800916FC cites `g3d_resanmtexsrt.cpp`, 0x80093990 cites
 * `g3d_resfile.cpp` and 0x800947A4 cites `g3d_resmat.cpp`.)
 *
 * Language: C++ - `langcheck.py` says the retail TU is C++ and the bodies carry the mangled
 * `nw4r::db::Panic` assert.  The map symbols carry plain `fn_XXXXXXXX` stems, so they are defined
 * `extern "C"` to keep the bare symbol (playbook 48).
 *
 * Sections: `.text` 0x8008F8E4-0x800908FC, `extab` 0x80009108-0x800091A0 (the unwind records for the
 * frame-using bodies, produced by the lib's `-Cpp_exceptions on`) and `extabindex`
 * 0x80021D68-0x80021E4C.  Both are claimed in splits.txt.  No `.ctors`/`.dtors` word points into the
 * range.
 *
 * Naming note: the symbol map has only `fn_XXXXXXXX` for this range (checked with
 * `python tools/symbols/dumpmap.py lookup 0x8008F8E4` / `0x8008FD04` / `0x800900A4`: all bare
 * `zz_XXXXXXXX_` placeholders in the runtime dump too), so there is no real name to recover and
 * stylelint's rule 7 refuses the landing without this line.
 *
 * Measured (official report metric, `tools/units/recompile.py g3d/g3d_resanmlight.cpp --measure <sym>`),
 * 31/31 bodies over the 80 % bar, unit byte-weighted ~94.3 %:
 *   100.00 % (20): fn_8008FD04, fn_8008FFFC, fn_8009001C, fn_8009004C, fn_800900A4, fn_80090108,
 *                  fn_800901AC, fn_80090210, fn_800902E0, fn_80090380, fn_800903E4, fn_800904B4,
 *                  fn_80090554, fn_800905B8, fn_80090688, fn_80090728, fn_8009078C, fn_8009085C
 *   99.07 % fn_8008F8E4, 99.95 % fn_80090058, 98.92 % fn_8008FE88, 98.57 % fn_80090000
 *   87.24 % fn_80090218/fn_800903EC/fn_800905C0/fn_80090794
 *   84.56 % fn_80090110/fn_800902E4/fn_800904B8/fn_8009068C/fn_80090860
 *
 * Residuals (what still differs; the shape was measured, not guessed):
 *  - The five 0x9C `...GetAnm`-style channel resolvers (fn_80090110/2E4/4B8/68C/860) and the four
 *    0xC8 ones (fn_80090218/3EC/5C0/794) are each one instruction short (152 vs 156 / 196 vs 200 B).
 *    The target keeps `&key` in a second callee-saved register (r31, saving r30 too) and lays the
 *    stack temporaries in ascending order (key 0x10, resolved 0x14, obj 0x18); MWCC here re-materialises
 *    `&key` from the frame and lays them descending.  Every source reshape tried (a named key pointer,
 *    a `key = *(u32*)fn_8006268C(...)` initialiser, reordered locals) keeps the same 84-87 %.
 *  - fn_8008F8E4 (99.07 %) and fn_8008FE88 (98.92 %, 368 vs 372 B) are each a single instruction off:
 *    fn_8008FE88's tail MMCC schedules the bit mask before the array load where the target loads first;
 *    fn_8008F8E4 differs in one register choice in the channel-evaluation block.
 *  - fn_80090000 (98.57 %): the body is instruction-identical but the target keeps `base` in r0
 *    (`lwz r0,0(r3); add r3,r0,r4`), ours in r3; the `base += offset` shape is what forces the branch
 *    (a plain `if/return` is if-converted to a branchless `(base+offset) & -(offset!=0)`).
 *  - fn_80090058 (99.95 %): the two branch arms use different stack slots in the target (0xC and 0x8);
 *    ours share one.  Instruction-identical modulo that.
 */

#include "types.h"
#include "nw4r/g3d/res_common.h"        /* IS_VALID_PTR (rule 1) */
#include "g3d/fn_80063888.h"            /* fn_8006584C/fn_80066E80, owner g3d/fn_80063888.cpp (rule 2) */
#include "g3d/g3d_anmchr.h"             /* fn_8006268C/fn_80062914/fn_80062750/fn_8005B1E4 (rule 2) */
#include "g3d/g3d_resanmamblight.h"     /* fn_8008A188/fn_8008A1A8, owner g3d/g3d_resanmamblight.c */
#include "unsplit/g3d.h"                /* fn_8008A644/fn_80069664/fn_80092330/fn_80463E74 (rule 2) */

#pragma peephole off
#pragma fp_contract off

/* nw4r::db::Panic - the assert failure handler (variadic).  Called through its namespace owner, never
 * its mangled spelling (rule 9). */
namespace nw4r {
namespace db {

void Panic(const char *file, int line, const char *message, ...);

} /* namespace db */
} /* namespace nw4r */

/* The `.data` `__FILE__`/assert strings the retail bodies pass by address (unsplit `.data`). */
extern u8 lbl_80590440[]; /* "g3d_resanmlight.cpp" */
extern u8 lbl_80590454[]; /* "NW4R:Pointer Error\npResult(=%p) is not valid pointer." */
extern u8 lbl_8059048C[]; /* the default-channel assert message */
extern u8 lbl_805904BC[]; /* "NW4R:Pointer Error\npArray(=%p)..." */
extern u8 lbl_80590500[]; /* "g3d_resanm_ac.h" */
extern u8 lbl_80590510[]; /* "NW4R:Failed assertion ..." */
extern u8 lbl_80590550[]; /* "g3d_resanm_ac.h" */
extern u8 lbl_80590560[]; /* "NW4R:Pointer Error\nthis(=%p)..." */
extern u8 lbl_80590598[]; /* "g3d_resanm_ac.h" */
extern u8 lbl_805905A8[]; /* "NW4R:Pointer Error\npArray(=%p)..." */
extern u8 lbl_805905F0[]; /* "g3d_resanm_ac.h" */
extern u8 lbl_80590600[]; /* "NW4R:Pointer Error\nthis(=%p)..." */
extern u8 lbl_80590638[]; /* "g3d_resanm_ac.h" */
extern u8 lbl_80590660[]; /* the light-set channel name record */
extern u8 lbl_80590680[]; /* the unknown channel name record */
extern u8 lbl_805906A0[]; /* the unknown channel name record */
extern u8 lbl_805906C0[]; /* the unknown channel name record */
extern u8 lbl_805906E0[]; /* the unknown channel name record */
extern u8 lbl_80590770[]; /* "NW4R:...IsValid()" (g3d_resanmcamera_ac.h msg) */
extern u8 lbl_80590798[]; /* "g3d_resanmcamera_ac.h" */
extern u8 lbl_805907B0[]; /* assert message (g3d_resanmfog_ac.h) */
extern u8 lbl_805907D8[]; /* "g3d_resanmfog_ac.h" */
extern u8 lbl_805907EC[]; /* assert message (g3d_reslightset_ac.h) */
extern u8 lbl_80590814[]; /* "g3d_reslightset_ac.h" */
extern u8 lbl_8059082C[]; /* assert message (g3d_resanmlight_ac.h) */
extern u8 lbl_80590854[]; /* "g3d_resanmlight_ac.h" */
extern u8 lbl_8059086C[]; /* assert message (g3d_resanmamblight_ac.h) */
extern u8 lbl_80590898[]; /* "g3d_resanmamblight_ac.h" */

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
extern "C" s32 fn_8008FD04(u32 *self, s32 index);
extern "C" s32 fn_8008FE88(u32 *self, s32 index);
extern "C" u32 fn_80090000(u32 *self, s32 offset);
extern "C" u32 *fn_8009001C(u32 *self, u32 *src);
extern "C" void fn_8009004C(u32 *self, u32 *src);
extern "C" u32 fn_80090058(u32 *self, s32 offset);
extern "C" void fn_80090108(u32 *self, u32 value);
extern "C" u32 *fn_800900A4(u32 *self, u32 value);
extern "C" void fn_80090210(u32 *self, u32 value);
extern "C" u32 *fn_800901AC(u32 *self, u32 value);
extern "C" u32 fn_80090218(u32 *self, u32 key);
extern "C" s32 fn_800902E4(u32 *self);
extern "C" void fn_800903E4(u32 *self, u32 value);
extern "C" u32 *fn_80090380(u32 *self, u32 value);
extern "C" u32 fn_800903EC(u32 *self, u32 key);
extern "C" s32 fn_800904B8(u32 *self);
extern "C" void fn_800905B8(u32 *self, u32 value);
extern "C" u32 *fn_80090554(u32 *self, u32 value);
extern "C" u32 fn_800905C0(u32 *self, u32 key);
extern "C" s32 fn_8009068C(u32 *self);
extern "C" void fn_8009078C(u32 *self, u32 value);
extern "C" u32 *fn_80090728(u32 *self, u32 value);
extern "C" u32 fn_80090794(u32 *self, u32 key);
extern "C" s32 fn_80090860(u32 *self);

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
        nw4r::db::Panic((const char *)lbl_80590440, 0x2C, (const char *)lbl_80590454, pResult);
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
            nw4r::db::Panic((const char *)lbl_80590440, 0x8C, (const char *)lbl_8059048C, kind);
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

/* Tests one bit of the packed channel array `*self` at index `index`. */
extern "C" s32 fn_8008FD04(u32 *self, s32 index)
{
    u32 valid = IS_VALID_PTR(self);

    if (!valid) {
        nw4r::db::Panic((const char *)lbl_80590598, 0x55, (const char *)lbl_80590560, self);
    }
    if (*self == 0) {
        nw4r::db::Panic((const char *)lbl_80590550, 0x56, (const char *)lbl_80590510);
    }
    if (index < 0) {
        nw4r::db::Panic((const char *)lbl_80590500, 0x57, (const char *)lbl_805904BC, index, 0);
    }
    return fn_8008FE88((u32 *)((u8 *)self + *self), index);
}

/* Tests one bit of the packed word array `self` at index `index`. */
extern "C" s32 fn_8008FE88(u32 *self, s32 index)
{
    u32 valid = IS_VALID_PTR(self);

    if (!valid) {
        nw4r::db::Panic((const char *)lbl_80590638, 0x45, (const char *)lbl_80590600, self);
    }
    if (index < 0) {
        nw4r::db::Panic((const char *)lbl_805905F0, 0x46, (const char *)lbl_805905A8, index);
    }
    u32 word = self[((u32)index) >> 5];
    u32 bit = 0x80000000u >> (((u32)index) & 31);

    return (word & bit) != 0;
}

/* The global frame thunk (a tail call into the shared frame getter). */
extern "C" f32 fn_8008FFFC(void)
{
    return fn_80463E74();
}

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

/* Copies one word through `self` and returns `self` (nw4r's fluent setter). */
extern "C" u32 *fn_8009001C(u32 *self, u32 *src)
{
    fn_8009004C(self, src);
    return self;
}

/* Copies one word. */
extern "C" void fn_8009004C(u32 *self, u32 *src)
{
    *self = *src;
}

/* Resolves a sub-resource offset and dereferences it. */
extern "C" u32 fn_80090058(u32 *self, s32 offset)
{
    u32 base = *self;
    u32 slot;

    if (offset != 0) {
        return *(u32 *)fn_80062914((void **)&slot, base + offset);
    }
    return *(u32 *)fn_80062914((void **)&slot, 0);
}

/* Stores the resolved light-set pointer and asserts its 4-byte alignment. */
extern "C" u32 *fn_800900A4(u32 *self, u32 value)
{
    fn_80090108(self, value);
    if (value & 3) {
        nw4r::db::Panic((const char *)lbl_80590814, 0x27, (const char *)lbl_805907EC);
    }
    return self;
}

/* Stores a word. */
extern "C" void fn_80090108(u32 *self, u32 value)
{
    *self = value;
}

/* Resolves the `lbl_80590660` channel record through the scene dictionary. */
extern "C" s32 fn_80090110(u32 *self)
{
    u32 slot;
    u32 key;
    u32 resolved;
    u32 obj;
    u32 out;
    s32 found;

    fn_8006268C(&slot, (u32)lbl_80590660);
    key = slot;
    resolved = fn_80090058(self, fn_8006584C(self)->field_0x10);
    fn_8009001C(&obj, &resolved);
    found = fn_80092330(&obj, &key);
    if (found != 0) {
        fn_80062914((void **)&out, found);
        return fn_80069664(&out);
    }
    return 0;
}

/* Stores the resolved ambient-light pointer and asserts its 4-byte alignment. */
extern "C" u32 *fn_800901AC(u32 *self, u32 value)
{
    fn_80090210(self, value);
    if (value & 3) {
        nw4r::db::Panic((const char *)lbl_80590898, 0x25, (const char *)lbl_8059086C);
    }
    return self;
}

/* Stores a word. */
extern "C" void fn_80090210(u32 *self, u32 value)
{
    *self = value;
}

/* Resolves the `lbl_80590680` channel record and reads the entry for `key`. */
extern "C" u32 fn_80090218(u32 *self, u32 key)
{
    u32 slot;
    u32 name;
    u32 resolved;
    u32 obj;
    u32 out;
    s32 found;

    fn_8006268C(&slot, (u32)lbl_80590680);
    name = slot;
    resolved = fn_80090058(self, fn_8006584C(self)->field_0x10);
    fn_8009001C(&obj, &resolved);
    found = fn_80092330(&obj, &name);
    if (found != 0) {
        fn_80062914((void **)&out, found);
        return *(u32 *)fn_800901AC(&obj, fn_80062750(&out, (void *)key));
    }
    return *(u32 *)fn_800901AC(&obj, 0);
}

/* Tail thunk to fn_80090218. */
extern "C" u32 fn_800902E0(u32 *self, u32 key)
{
    return fn_80090218(self, key);
}

/* Resolves the `lbl_80590680` channel record through the scene dictionary. */
extern "C" s32 fn_800902E4(u32 *self)
{
    u32 slot;
    u32 key;
    u32 resolved;
    u32 obj;
    u32 out;
    s32 found;

    fn_8006268C(&slot, (u32)lbl_80590680);
    key = slot;
    resolved = fn_80090058(self, fn_8006584C(self)->field_0x10);
    fn_8009001C(&obj, &resolved);
    found = fn_80092330(&obj, &key);
    if (found != 0) {
        fn_80062914((void **)&out, found);
        return fn_80069664(&out);
    }
    return 0;
}

/* Stores the resolved light pointer and asserts its 4-byte alignment. */
extern "C" u32 *fn_80090380(u32 *self, u32 value)
{
    fn_800903E4(self, value);
    if (value & 3) {
        nw4r::db::Panic((const char *)lbl_80590854, 0x8F, (const char *)lbl_8059082C);
    }
    return self;
}

/* Stores a word. */
extern "C" void fn_800903E4(u32 *self, u32 value)
{
    *self = value;
}

/* Resolves the `lbl_805906A0` channel record and reads the entry for `key`. */
extern "C" u32 fn_800903EC(u32 *self, u32 key)
{
    u32 slot;
    u32 name;
    u32 resolved;
    u32 obj;
    u32 out;
    s32 found;

    fn_8006268C(&slot, (u32)lbl_805906A0);
    name = slot;
    resolved = fn_80090058(self, fn_8006584C(self)->field_0x10);
    fn_8009001C(&obj, &resolved);
    found = fn_80092330(&obj, &name);
    if (found != 0) {
        fn_80062914((void **)&out, found);
        return *(u32 *)fn_80090380(&obj, fn_80062750(&out, (void *)key));
    }
    return *(u32 *)fn_80090380(&obj, 0);
}

/* Tail thunk to fn_800903EC. */
extern "C" u32 fn_800904B4(u32 *self, u32 key)
{
    return fn_800903EC(self, key);
}

/* Resolves the `lbl_805906A0` channel record through the scene dictionary. */
extern "C" s32 fn_800904B8(u32 *self)
{
    u32 slot;
    u32 key;
    u32 resolved;
    u32 obj;
    u32 out;
    s32 found;

    fn_8006268C(&slot, (u32)lbl_805906A0);
    key = slot;
    resolved = fn_80090058(self, fn_8006584C(self)->field_0x10);
    fn_8009001C(&obj, &resolved);
    found = fn_80092330(&obj, &key);
    if (found != 0) {
        fn_80062914((void **)&out, found);
        return fn_80069664(&out);
    }
    return 0;
}

/* Stores the resolved fog pointer and asserts its 4-byte alignment. */
extern "C" u32 *fn_80090554(u32 *self, u32 value)
{
    fn_800905B8(self, value);
    if (value & 3) {
        nw4r::db::Panic((const char *)lbl_805907D8, 0x3E, (const char *)lbl_805907B0);
    }
    return self;
}

/* Stores a word. */
extern "C" void fn_800905B8(u32 *self, u32 value)
{
    *self = value;
}

/* Resolves the `lbl_805906C0` channel record and reads the entry for `key`. */
extern "C" u32 fn_800905C0(u32 *self, u32 key)
{
    u32 slot;
    u32 name;
    u32 resolved;
    u32 obj;
    u32 out;
    s32 found;

    fn_8006268C(&slot, (u32)lbl_805906C0);
    name = slot;
    resolved = fn_80090058(self, fn_8006584C(self)->field_0x10);
    fn_8009001C(&obj, &resolved);
    found = fn_80092330(&obj, &name);
    if (found != 0) {
        fn_80062914((void **)&out, found);
        return *(u32 *)fn_80090554(&obj, fn_80062750(&out, (void *)key));
    }
    return *(u32 *)fn_80090554(&obj, 0);
}

/* Tail thunk to fn_800905C0. */
extern "C" u32 fn_80090688(u32 *self, u32 key)
{
    return fn_800905C0(self, key);
}

/* Resolves the `lbl_805906C0` channel record through the scene dictionary. */
extern "C" s32 fn_8009068C(u32 *self)
{
    u32 slot;
    u32 key;
    u32 resolved;
    u32 obj;
    u32 out;
    s32 found;

    fn_8006268C(&slot, (u32)lbl_805906C0);
    key = slot;
    resolved = fn_80090058(self, fn_8006584C(self)->field_0x10);
    fn_8009001C(&obj, &resolved);
    found = fn_80092330(&obj, &key);
    if (found != 0) {
        fn_80062914((void **)&out, found);
        return fn_80069664(&out);
    }
    return 0;
}

/* Stores the resolved camera pointer and asserts its 4-byte alignment. */
extern "C" u32 *fn_80090728(u32 *self, u32 value)
{
    fn_8009078C(self, value);
    if (value & 3) {
        nw4r::db::Panic((const char *)lbl_80590798, 0x7D, (const char *)lbl_80590770);
    }
    return self;
}

/* Stores a word. */
extern "C" void fn_8009078C(u32 *self, u32 value)
{
    *self = value;
}

/* Resolves the `lbl_805906E0` channel record and reads the entry for `key`. */
extern "C" u32 fn_80090794(u32 *self, u32 key)
{
    u32 slot;
    u32 name;
    u32 resolved;
    u32 obj;
    u32 out;
    s32 found;

    fn_8006268C(&slot, (u32)lbl_805906E0);
    name = slot;
    resolved = fn_80090058(self, fn_8006584C(self)->field_0x10);
    fn_8009001C(&obj, &resolved);
    found = fn_80092330(&obj, &name);
    if (found != 0) {
        fn_80062914((void **)&out, found);
        return *(u32 *)fn_80090728(&obj, fn_80062750(&out, (void *)key));
    }
    return *(u32 *)fn_80090728(&obj, 0);
}

/* Tail thunk to fn_80090794. */
extern "C" u32 fn_8009085C(u32 *self, u32 key)
{
    return fn_80090794(self, key);
}

/* Resolves the `lbl_805906E0` channel record through the scene dictionary. */
extern "C" s32 fn_80090860(u32 *self)
{
    u32 slot;
    u32 key;
    u32 resolved;
    u32 obj;
    u32 out;
    s32 found;

    fn_8006268C(&slot, (u32)lbl_805906E0);
    key = slot;
    resolved = fn_80090058(self, fn_8006584C(self)->field_0x10);
    fn_8009001C(&obj, &resolved);
    found = fn_80092330(&obj, &key);
    if (found != 0) {
        fn_80062914((void **)&out, found);
        return fn_80069664(&out);
    }
    return 0;
}
