/*
 * g3d/g3d_anmchr.cpp - nw4r g3d character animation (`g3d_anmchr.cpp`), led by the g3d work-memory accessors,
 *   with small constant/forwarder helpers.
 * RANGE. .text 0x8005CED0-0x80063888 (124 functions); extab 0x800073C8-0x800076C4, extabindex
 *   0x8001F71C-0x8001FB00, .data 0x8058B410-0x8058C118, .bss 0x8066AE80-0x80682E80, .sdata 0x80791138-0x80791158,
 *   .sdata2 0x80795D48-0x80795D68.  The asserts from 0x8005CF10 on pass "g3d_anmchr.cpp" (.data 0x8058B410).
 * RANGE. Left edge 0x8005CED0: the font/debug-print console before it is `font/flfnt.cpp` (its header has the
 *   seam evidence).  fn_8005CED0-fn_8005CF04 return the 0x18000-byte `.bss` buffer 0x8066AE80-0x80682E80, read
 *   only by g3d calc units; they are nw4r g3d work memory and may be a TU of their own (unproven).  The right
 *   edge is unproven.
 * NAMES. The map's own names and stems, the stems defined `extern "C"`.
 * RESIDUALS. Unwritten (objdiff scores them zero): 75 of the 124 functions, every one except the 42 at 100 % and
 *   these partial ones - fn_8005D27C, fn_8005DC30, fn_8005DCA0, fn_800604DC, fn_8006244C, fn_80062824, fn_800628AC.
 *   flipcheck: `.text` 0x3D0 of 0x69B8; `.data`, `.bss`, `.sdata` and `.sdata2` are claimed and not emitted.
 */

#include "types.h"
#include "fn_8004CAD8.h"   /* fn_80051570's owner header (docs/plan.md 6.5, rule 2) */
#include "nw4r/math_arithmetic.h"   /* nw4r::math::detail::FExp, owner nw4r/math_arithmetic.cpp (rule 2) */

/* --- the SDK entry points this unit tail-calls (owners are unsplit; declared, never defined) --------- */

extern "C" void fn_804C6F40(u32, u32, void*);
extern "C" void fn_804C6D70(void*, u32);
extern "C" void fn_80061EC4(void*, f32);

/* The g3d math constant fn_8006244C scales by (`.sdata` 0x807911E8). */
extern f32 lbl_807911E8;

/* The `.sdata` strings fn_8005D27C / fn_800628AC hand back (`_SDA_BASE_`-relative), and the two
 * absolute constants the blocked-return helpers hand back (`.bss` / `.data`). */
extern u8 lbl_80791148[];
extern u8 lbl_80791150[];
extern u8 lbl_8066AE80[];
extern u8 lbl_8058BF14[];
extern u8 lbl_8056F500[];
extern u8 lbl_8056F538[];

/* --- bodies: the g3d work-memory accessors and tiny constant/identity helpers ---------------------- */

extern "C" u8* fn_8005CED0(void)
{
    return lbl_8066AE80;
}

extern "C" u8* fn_8005CEDC(void)
{
    return lbl_8066AE80 + 0x6000;
}

extern "C" u8* fn_8005CEEC(void)
{
    return lbl_8066AE80;
}

extern "C" u8* fn_8005CEF8(void)
{
    return lbl_8066AE80;
}

extern "C" u8* fn_8005CF04(void)
{
    return lbl_8066AE80;
}

extern "C" u32* fn_8005D0C4(u32** pp)
{
    return *pp;
}

extern "C" void fn_8005D114(u32* a, u32* b)
{
    (void)a;
    (void)b;
}

extern "C" void fn_8005D118(u32* dst, u32* src)
{
    *dst = *src;
}

extern "C" void fn_8005D210(u32* p, u32 v)
{
    *p = v;
}

extern "C" u8* fn_8005D27C(void)
{
    return lbl_80791148;
}

extern "C" void fn_8005D2F0(u32* dst, u32* src)
{
    *dst = *src;
}

extern "C" u32 fn_8005D2FC(u32* p)
{
    return *p != 0;
}

extern "C" void fn_8005D3E0(void)
{
}

extern "C" u32 fn_8005DC24(u32* p)
{
    return *p + 4;
}

extern "C" void fn_8005DC60(u32* p, u32 v)
{
    *p = v;
}

extern "C" void fn_8005DCD0(u32* p, u32 v)
{
    *p = v;
}

/* The two ``current chunk`` getters: stash a fixed descriptor address in a local through the setter,
 * then hand the local back (the setter is opaque, so the reload is real). */
extern "C" u32 fn_8005DC30(void)
{
    u32 value;

    fn_8005DC60(&value, (u32)lbl_8056F500);
    return value;
}

extern "C" u32 fn_8005DCA0(void)
{
    u32 value;

    fn_8005DCD0(&value, (u32)lbl_8056F538);
    return value;
}

/* The two-word copy pair the character-animation constructor uses. */
extern "C" void* fn_8005D0CC(u32* dst, u32* src)
{
    fn_8005D118(dst, src);
    fn_8005D114(dst + 1, src + 1);
    return dst;
}

extern "C" void fn_800600B8(u32* p, u32 v)
{
    p[1] = v;
}

extern "C" u32 fn_800600C0(u32* p)
{
    return p[1];
}

extern "C" u32 fn_800604CC(u32 a, u32 b)
{
    (void)a;
    return b;
}

extern "C" void fn_80060FEC(void)
{
}

extern "C" u32 fn_80061920(u32* p)
{
    return *p;
}

extern "C" u8* fn_80061928(void)
{
    return lbl_8058BF14;
}

extern "C" void fn_80061B30(u32* dst, u32* src)
{
    *dst = *src;
}

extern "C" f32 fn_80062024(f32* p)
{
    return *p;
}

extern "C" void fn_800621DC(f32* p, f32 v)
{
    p[1] = v;
}

extern "C" f32 fn_80062300(f32* p)
{
    return p[1];
}

extern "C" void fn_800626BC(u32* p, u32 v)
{
    *p = v;
}

extern "C" u32 fn_800628A4(u32* p)
{
    return *p;
}

extern "C" u8* fn_800628AC(void)
{
    return lbl_80791150;
}

extern "C" void fn_80062978(u32* p, u32 v)
{
    *p = v;
}

extern "C" void fn_80062D88(u32* dst, u32* src)
{
    *dst = *src;
}

/* --- assignment helpers that return the object (nw4r's fluent setters) ------------------------------ */

extern "C" void* fn_8006268C(void* self, u32 value)
{
    fn_800626BC((u32*)self, value);
    return self;
}

extern "C" void* fn_80061B00(void* self, u32* src)
{
    fn_80061B30((u32*)self, src);
    return self;
}

extern "C" void* fn_80062D58(void* self, u32* src)
{
    fn_80062D88((u32*)self, src);
    return self;
}

extern "C" void* fn_8005D2C0(void* self, u32* src)
{
    fn_8005D2F0((u32*)self, src);
    return self;
}

/* --- more one-instruction accessors ----------------------------------------------------------------- */

extern "C" u32 fn_80061934(u32* p)
{
    return *p != 0;
}

extern "C" u32 fn_800628B4(u32* p)
{
    return *p != 0;
}

/* Returns the word at +0 plus `offset`, or 0 when `offset` is 0 (nw4r's null-safe offset helper). */
extern "C" u32 fn_80062824(u32* p, u32 offset)
{
    u32 base = *p;

    if (offset != 0) {
        return base + offset;
    }
    return 0;
}

/* The align-up helper: (~(align - 1)) & (offset + align - 1). */
extern "C" u32 fn_800604DC(u32 offset, u32 align)
{
    u32 end = offset + align;

    return ~(align - 1) & (end - 1);
}

/* nw4r's four-float vector setter. */
extern "C" void dVector4Set(f32* dst, f32 x, f32 y, f32 z, f32 w)
{
    dst[0] = x;
    dst[1] = y;
    dst[2] = z;
    dst[3] = w;
}

/* --- forwarders into the SDK allocator / matrix helpers --------------------------------------------- */

extern "C" void* fn_80060F70(void* self, u32 a, u32 b)
{
    fn_804C6F40(a, b, self);
    return self;
}

extern "C" void* fn_80060FAC(void* self, u32 a)
{
    fn_804C6D70(self, fn_80051570(a));
    return self;
}

/* The animated-float advance: offset[0] += offset[1] * scale, then the virtual step. */
extern "C" void fn_8006244C(f32* p)
{
    f32 scale = lbl_807911E8;
    f32 value = p[1] * scale;

    fn_80061EC4(p, p[0] + value);
}

/* --- nw4r's ``fn_800626F4``-returns-this pair ------------------------------------------------------- */

extern "C" u32 fn_800626F4(u32 value)
{
    return value;
}

extern "C" u32 fn_800626C4(u32 a, u32 b)
{
    return b + fn_800626F4(a);
}

/* --- forwarders into nw4r math ---------------------------------------------------------------------- */

/* The exp forwarder (nw4r's inline `math::FExp`): a tail call with the argument untouched. */
extern "C" f32 fn_80060F6C(f32 x)
{
    return nw4r::math::detail::FExp(x);
}
