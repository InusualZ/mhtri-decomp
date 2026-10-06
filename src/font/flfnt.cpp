/*
 * font/flfnt.cpp - the game's debug draw (`disp_beta_tex`, `dbg_drawgraph_init`) and its font/debug-print
 *   console (`flfnt*`, `flKnjMsg*`, `font_*`, `msg_str_gen`, `utf82unicode2`), with the GX pipe helpers.
 * RANGE. .text 0x8005ABD8-0x8005CED0 (59 functions); extab 0x80007328-0x800073C8, extabindex
 *   0x8001F62C-0x8001F71C, .data 0x8058B388-0x8058B410, .bss 0x8066AE60-0x8066AE80, .sdata
 *   0x80791130-0x80791138, .sbss 0x807948D8-0x807948E8, .sdata2 0x80795CF8-0x80795D48.
 * RANGE. Left edge: the registered `g3d/fn_8005AA28.cpp` ends at 0x8005ABD8.  Right edge 0x8005CED0: the
 *   next five functions return the 0x18000-byte `.bss` buffer 0x8066AE80-0x80682E80 that only g3d calc units
 *   call (nw4r g3d work memory, kept in `g3d/g3d_anmchr.cpp`), and from 0x8005CF10 the asserts pass
 *   "g3d_anmchr.cpp".  Every data piece here is referenced only from this range, every one of the next unit's
 *   only from its own; the extab/extabindex records split at the same function.
 * RANGE. Two TUs, not one: `.sdata2` holds 0.0f twice (0x80795CF8, 0x80795D24) and 0x4330000000000000 twice
 *   (0x80795D18, 0x80795D38), and a TU pools a value once.  0x80795CF8-0x80795D20 is read only by
 *   `disp_beta_tex`/`dbg_drawgraph_init`, 0x80795D20-0x80795D48 only by fn_8005BAF4/`flfntFlush`.  The
 *   `.text` seam lies in 0x8005B1B4-0x8005B278 (unproven), so the range stays one unit.
 * FLAGS. the `g3d` lib's `cflags_g3d`, as in the unit this range was cut from (the 100 % rows were measured
 *   under it).
 * NAMES. The map's own names and stems, the stems defined `extern "C"`; the file name `flfnt` is a GUESS from
 *   the `flfnt*` entry points.  `FlFnt` is a GUESS read off the `flfnt*` bodies, its size an approximation;
 *   `getGlyphWidth` (0x8005BF68) is a GUESS (the glyph-width lookup `Network/network_pat_control.cpp` calls);
 *   gx_tex_obj_copy is a GUESS (0x8005C50C: the 0x20-byte copy flfntFlush and the g3d texture-object cache call).
 * RESIDUALS. Unwritten (objdiff scores them zero): flfntStackReset, flKnjMsgNumPtr, flKnjMsgNum, flfntPrintf,
 *   fn_8005B278, fn_8005BA68-fn_8005BD3C, utf82unicode2, fn_8005BF20, getGlyphWidth, fn_8005BFDC, flfntFlush,
 *   fn_8005C4CC-fn_8005C500, font_print, fn_8005C618, font_print_ex, fn_8005C8F0, fn_8005C9B8, fn_8005CA64,
 *   disp_beta_tex, dbg_drawgraph_init, dtor_8005B228.  Partial: fn_8005AED4, fn_8005B7D8, fn_8005C7F0.
 *   flipcheck: `.text` 0x384 of 0x22F8; `.data`, `.bss`, `.sdata`, `.sbss` and `.sdata2` are claimed and not emitted.
 */

#include "types.h"
#include "gx.h"

/* --- the SDK entry points this unit tail-calls (owners are unsplit; declared, never defined) --------- */

extern "C" void GXSetTexCoordGen2(u32, u32, u32, u32, u32, u32);
extern "C" char* strchr(const char*, int);
extern "C" char* strcpy(char*, const char*);

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

extern "C" void gx_tex_obj_copy(MtxBlock8* dst, MtxBlock8* src)
{
    *dst = *src;
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

/* --- the font library's public wrappers ------------------------------------------------------------- */

/* `flfntFlush` and the two lower helpers are defined later in this unit; only `flfntFlush` is written. */
void flfntFlush(void);
extern "C" s32 fn_8005B7D8(char*);
extern "C" void fn_8005BFDC(char*);

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

/* --- small constant helpers ------------------------------------------------------------------------- */

extern "C" u32 fn_8005BF60(void)
{
    return 0;
}
