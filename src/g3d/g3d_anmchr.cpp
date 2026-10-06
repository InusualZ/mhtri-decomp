/*
 * g3d/g3d_anmchr.cpp - nw4r g3d character animation (`g3d_anmchr.cpp`) and the game's font/debug-print state
 *   and accessors (`flfnt*`, `font_*`), with the GX pipe helpers and small constant/forwarder helpers.
 * RANGE. .text 0x8005ABD8-0x80063888 (183 functions); extab, extabindex, .data 0x8058B388-0x8058C118, .bss
 *   0x8066AE60-0x80682E80, .sdata 0x80791130-0x80791158, .sbss 0x807948D8-0x807948E8, .sdata2
 *   0x80795CF8-0x80795D68.  The asserts from 0x8005CF10 on pass "g3d_anmchr.cpp" (.data 0x8058B410); the font
 *   accessors' state `lbl_807948D8` and fn_8005B7A0's colour table (0x8058B3D0) sit in the same pools, so they are
 *   this TU's.  The run's edges are a discovery cap, not proven seams.
 * NAMES. The map's own names (`disp_beta_tex`, `dbg_drawgraph_init`, the `flfnt*`/`font_*` manglings) and stems,
 *   the stems defined `extern "C"`; `FlFnt` is a GUESS read off the `flfnt*` bodies, its size an approximation;
 *   `getGlyphWidth` (0x8005BF68) is a GUESS (the glyph-width lookup `Network/network_pat_control.cpp` calls).
 * RESIDUALS. Unwritten (objdiff scores them zero): 104 functions in 36 runs, every function in the range except the
 *   69 at 100 % and these partial ones - fn_8005AED4, fn_8005B7D8, fn_8005C7F0, fn_8005D27C, fn_8005DC30,
 *   fn_8005DCA0, fn_800604DC, fn_8006244C, fn_80062824, fn_800628AC.
 *   flipcheck: `.text` 0x754 of 0x8CB0; `.data`, `.bss`, `.sdata`, `.sbss` and `.sdata2` are claimed and not emitted.
 */

#include "types.h"
#include "gx.h"
#include "fn_8004CAD8.h"   /* fn_80051570's owner header (docs/plan.md 6.5, rule 2) */

/* --- the SDK entry points this unit tail-calls (owners are unsplit; declared, never defined) --------- */

extern "C" void GXSetTexCoordGen2(u32, u32, u32, u32, u32, u32);
extern "C" char* strchr(const char*, int);
extern "C" char* strcpy(char*, const char*);
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

/* --- the unit's own debug-print work block (`lbl_807948D8`, .sbss 0x807948D8, 8 B) ------------------ */

/* The layout is read off the bodies in this range only; the offsets run to 0x168D0, so the tail is
 * padding.  `player_stride` is 0x15000 and the per-glyph slot stride is 0x54 (fn_8005BAB0).  The size is
 * an approximation: nothing written here reaches past +0x168CC. */
typedef struct FlFnt {
    /* +0x00000 */ u8 pad_00000[0x4];
    /* +0x00004 */ u32 mode;          /* flfntFontPuts only runs when this is 1 */
    /* +0x00008 */ u8 pad_00008[0x14];
    /* +0x0001C */ u32 glyph_count;   /* fn_8005BFAC indexes the glyph table below this */
    /* +0x00020 */ u8 pad_00020[0x10];
    /* +0x00030 */ u32 wide_mode;     /* non-zero when the console draws double-width */
    /* +0x00034 */ u8 pad_00034[0x1C];
    /* +0x00050 */ u32 player;        /* selects the 0x15000-byte player region (fn_8005BAB0) */
    /* +0x00054 */ u8 slots[0x15000];
    /* +0x15054 */ u8 pad_15054[0x4];
    /* +0x15058 */ u32 slot_cap;      /* fn_8005BAB0: count >= cap -> no free slot */
    /* +0x1505C */ u32 slot_used;
    /* +0x15060 */ s32 size_x;
    /* +0x15064 */ s32 size_y;
    /* +0x15068 */ s32 pos_x;
    /* +0x1506C */ s32 pos_y;
    /* +0x15070 */ u8 pad_15070[0x8];
    /* +0x15078 */ u32 color;
    /* +0x1507C */ u32 field_1507C;
    /* +0x15080 */ u32 field_15080;
    /* +0x15084 */ u8 pad_15084[0xC];
    /* +0x15090 */ u32 field_15090;
    /* +0x15094 */ s16 field_15094;
    /* +0x15096 */ u8 pad_15096[0x2];
    /* +0x15098 */ s16 field_15098[8];
    /* +0x150A8 */ u8 pad_150A8[0xFE8];
    /* +0x16090 */ u8 pad_16090[0x8];
    /* +0x16098 */ u32 field_16098;
    /* +0x1609C */ u8 pad_1609C[0x4];
    /* +0x160A0 */ s16 field_160A0[0x814];
    /* +0x168C8 */ s16 field_168C8;
    /* +0x168CA */ u8 pad_168CA[0x2];
    /* +0x168CC */ u32 field_168CC;
    /* +0x168D0 */ u8 pad_168D0[0x4];
} FlFnt; /* size: 0x168D4 (approximate) */

extern FlFnt* lbl_807948D8;

/* The 16-entry colour table fn_8005B7A0 indexes (`.data` 0x8058B3D0, 0x40 B). */
extern u32 lbl_8058B3D0[];

/* The glyph metric table (`.data` 0x80580058): a run of 4-byte `{width, advance}` pairs. */
typedef struct GlyphMetric {
    /* +0x00 */ u16 width;
    /* +0x02 */ u16 advance;
} GlyphMetric; /* size: 0x04 */
extern GlyphMetric lbl_80580058[];

/* The UTF-8 decoder walker and the glyph resolvers (`fn_8005BD3C`/`fn_8005BF20` are unwritten). */
extern "C" s32 fn_8005BD3C(char**);
extern "C" u32 fn_8005BF20(u32);
extern "C" u16 fn_8005BFAC(u32);

/* --- bodies, in address order ---------------------------------------------------------------------- */

/* The GX pipe helpers (write-gather-pipe writers; the SDK's own `GXPosition*`/`GXColor*` shapes). */
extern "C" void fn_8005AEC0(void)
{
}

extern "C" void fn_8005AEC4(f32 x, f32 y)
{
    GXWGFifo.f32 = x;
    GXWGFifo.f32 = y;
}

extern "C" void fn_8005AED4(u32 x, u32 y, u32 z, u32 w)
{
    GXWGFifo.u8 = (u8)(x & 0xFF);
    GXWGFifo.u8 = (u8)(y & 0xFF);
    GXWGFifo.u8 = (u8)(z & 0xFF);
    GXWGFifo.u8 = (u8)(w & 0xFF);
}

extern "C" void fn_8005AEFC(f32 x, f32 y, f32 z)
{
    GXWGFifo.f32 = x;
    GXWGFifo.f32 = y;
    GXWGFifo.f32 = z;
}

/* A one-texcoord wrapper that pins GX's normalize flag (0) and the identity post-matrix (125). */
extern "C" void fn_8005AF10(u32 coord, u32 func, u32 src, u32 mtx)
{
    GXSetTexCoordGen2(coord, func, src, mtx, 0, 125);
}

/* The identity helper of the store pair below: returns its argument (the object pointer). */
extern "C" void* fn_8005B224(void* self)
{
    return self;
}

extern "C" void* fn_8005B1E4(void* self, u32 value)
{
    *(u32*)fn_8005B224(self) = value;
    return self;
}

extern "C" void* fn_8005B1B4(void* self, u32 value)
{
    fn_8005B1E4(self, value);
    return self;
}

extern "C" u32 fn_8005B26C(void)
{
    return 0x170E0;
}

/* --- the font/debug-print accessors ---------------------------------------------------------------- */

void flfntSetSize(s32 x, s32 y)
{
    if (lbl_807948D8 == NULL) {
        return;
    }
    lbl_807948D8->size_x = x;
    lbl_807948D8->size_y = y;
}

s16 flfntGetSizeX(void)
{
    if (lbl_807948D8 == NULL) {
        return 0;
    }
    return (s16)lbl_807948D8->size_x;
}

s16 flfntGetSizeY(void)
{
    if (lbl_807948D8 == NULL) {
        return 0;
    }
    return (s16)lbl_807948D8->size_y;
}

void flfntSetPos(s32 x, s32 y)
{
    if (lbl_807948D8 == NULL) {
        return;
    }
    lbl_807948D8->pos_x = x;
    lbl_807948D8->pos_y = y;
}

s16 flfntGetPosX(void)
{
    if (lbl_807948D8 == NULL) {
        return 0;
    }
    return (s16)lbl_807948D8->pos_x;
}

s16 flfntGetPosY(void)
{
    if (lbl_807948D8 == NULL) {
        return 0;
    }
    return (s16)lbl_807948D8->pos_y;
}

void flfntSetColor(u32 color)
{
    if (lbl_807948D8 == NULL) {
        return;
    }
    lbl_807948D8->color = color;
}

/* Sets the colour from the fixed 16-entry palette, ignoring out-of-range and no-state calls. */
extern "C" void fn_8005B7A0(int index)
{
    if (lbl_807948D8 == NULL) {
        return;
    }
    if (index < 0) {
        return;
    }
    if (index >= 16) {
        return;
    }
    lbl_807948D8->color = lbl_8058B3D0[index];
}

extern "C" void fn_8005B988(u32 value)
{
    if (lbl_807948D8 == NULL) {
        return;
    }
    lbl_807948D8->field_1507C = value;
}

/* --- tiny constant/identity helpers ---------------------------------------------------------------- */

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

/* --- the console's glyph metric lookup and the string-length walker --------------------------------- */

/* Returns the advance class of glyph `index`, or 1 when the index is past the loaded table. */
extern "C" u16 fn_8005BFAC(u32 index)
{
    if (index >= lbl_807948D8->glyph_count) {
        return 1;
    }
    return lbl_80580058[index].advance;
}

/* The 8-word (0x20 B) matrix block copy. */
typedef struct MtxBlock8 {
    /* +0x00 */ u32 w[8];
} MtxBlock8; /* size: 0x20 */

extern "C" void fn_8005C50C(MtxBlock8* dst, MtxBlock8* src)
{
    *dst = *src;
}

/* The two-word copy pair the character-animation constructor uses. */
extern "C" void* fn_8005D0CC(u32* dst, u32* src)
{
    fn_8005D118(dst, src);
    fn_8005D114(dst + 1, src + 1);
    return dst;
}

/* Number of console cells a UTF-8 string occupies: 1 per ASCII byte, 2 per wide/kanji codepoint. */
extern "C" s32 fn_8005B7D8(char* s)
{
    FlFnt* state = lbl_807948D8;
    s32 len = 0;
    char* p = s;

    for (;;) {
        s32 c = fn_8005BD3C(&p);

        if (c == 0) {
            break;
        }
        if (c == 10) {
            continue;
        }
        if (state->wide_mode == 0) {
            if (c <= 15) {
                len += 2;
                continue;
            }
            if (c <= 127) {
                len += 1;
                continue;
            }
        }
        len += fn_8005BFAC(fn_8005BF20((u32)c)) == 0 ? 2 : 1;
    }
    return len;
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

/* --- the font library's public wrappers ------------------------------------------------------------- */

/* `flfntFlush` and the two lower helpers are defined later in this unit; only `flfntFlush` is written. */
void flfntFlush(void);
extern "C" s32 fn_8005B7D8(char*);
extern "C" void fn_8005BFDC(char*);
extern "C" void fn_80500CF8(void);

/* Length of the UTF-8 sequence starting at `s` (forwarded to the decoding walker). */
s32 flfntStrLen(char* s)
{
    return fn_8005B7D8(s);
}

/* Puts one string through the flush path when the console is in its single-line mode (state+0x4 == 1). */
void flfntFontPuts(char* s)
{
    if (lbl_807948D8->mode != 1) {
        return;
    }
    fn_8005BFDC(s);
}

/* Locates the cursor (the SDK's `ss` = short, short signature). */
void font_locate(s16 x, s16 y)
{
    flfntSetPos(x, y);
}

void font_set_size(s16 x, s16 y)
{
    flfntSetSize(x, y);
}

/* The palette selector's short-typed entry point. */
extern "C" void fn_8005C7F0(s16 index)
{
    if (index >= 16) {
        return;
    }
    fn_8005B7A0(index);
}

/* The substring search the string helpers call (a thin `strchr`). */
extern "C" char* flfntStrChr(char* s, int c)
{
    return strchr(s, c);
}

void font_flush(void)
{
    flfntFlush();
}

/* Copies the generated message into the caller's buffer (the arguments are swapped for `strcpy`). */
void msg_str_gen(char* src, char* dst)
{
    strcpy(dst, src);
}

/* --- small constant/forwarder helpers --------------------------------------------------------------- */

extern "C" u32 fn_8005BF60(void)
{
    return 0;
}

extern "C" void fn_80060F6C(void)
{
    fn_80500CF8();
}
