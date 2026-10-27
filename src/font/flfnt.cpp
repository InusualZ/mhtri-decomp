/*
 * font/flfnt.cpp - the game's font/debug-print console (`flfnt*`, `flKnjMsg*`, `font_*`, `msg_str_gen`,
 *   `utf82unicode2`), with the GX pipe helpers.
 * RANGE. .text 0x8005B26C-0x8005CED0 (48 functions); extab 0x80007350-0x800073C8, extabindex
 *   0x8001F668-0x8001F71C, .data 0x8058B388-0x8058B410, .bss 0x8066AE60-0x8066AE80, .sdata
 *   0x80791130-0x80791138, .sbss 0x807948D8-0x807948E8, .sdata2 0x80795D20-0x80795D48.
 * RANGE. Left edge 0x8005B26C: `font/dbg_drawgraph.cpp` (the debug draw) ends there - its `.sdata2` pool
 *   0x80795CF8-0x80795D20 repeats this unit's 0.0f, 1.0f and 0x4330000000000000, and the extabindex records
 *   split after the `ut::Color` destructor.  Right edge 0x8005CED0: the next five functions return the
 *   0x18000-byte `.bss` buffer 0x8066AE80-0x80682E80 that only g3d calc units call (nw4r g3d work memory, kept
 *   in `g3d/g3d_anmchr.cpp`), and from 0x8005CF10 the asserts pass "g3d_anmchr.cpp".
 * FLAGS. the `g3d` lib's `cflags_g3d`, as in the unit this range was cut from.
 * NAMES. The `flfnt*`/`font_*`/`flKnjMsg*` entry points are the map's.  GUESS (from each body and its callers):
 *   `font_buffer_size_get`, `font_color_set`, `flfnt_frame_reset`, `getGlyphWidth`, `gx_tex_obj_copy`,
 *   `flfntSetPaletteColor`, `flfntStrCells`, `flfntSetAttr`, `flfntCacheFind`, `flfntGlyphAlloc`, `flfntCacheLoad`,
 *   `flfntGlyphPut`, `flfntUtf8Decode`, `flfntCodeIndex`, `flfntAsciiCode`, `flfntGlyphAdvance`, `flfntDrawText`,
 *   `flfntColorPuts`, the GX helper copies `fntGX*` (the SDK's inline GXEnd/GXTexCoord2f32/
 *   GXColor1u32/GXPosition3f32/GXSetTexCoordGen, emitted out of line), and the data `flfnt_state`,
 *   `font_typed_count`, `flfnt_frame_work`, `flfnt_default_config`, `flfnt_palette`.  `FlFnt` and its field
 *   names are GUESSES read off the bodies.
 * RESIDUALS. `.sbss` holds 8 bytes per object in retail (font_typed_count at +0x8), ours packs them.
 * RESIDUALS. utf82unicode2/flfntUtf8Decode: retail keeps the final continuation-byte test as a branch
 *   (`subi`/`cmplwi 0x3F`/`ble`); every spelling tried either splits the range test or becomes a select.
 *   getGlyphWidth: retail returns without `clrlwi` (an int return), but `Network/network_pat_control.cpp`
 *   measures better with the u16 declaration both units share.  flfntStrCells/flKnjMsgNumPtr: `state`/`len`
 *   take r31/r30 swapped.  flfntDrawText: the second ASCII test re-reads wide_mode (retail jumps straight to
 *   the glyph path).  flfntColorPuts: retail keeps the `extsb` of both colour digits.  font_print_typed: the
 *   multibyte-next '\n' test branches to the '~' test (retail: straight to the newline block).
 *   flfntStackReset: the inner loop count `li r0,0x20` is not hoisted.  font_str_char_count: the byte and
 *   `two` swap r4/r5.
 */

#pragma peephole off
#include "types.h"
#include "gx.h"
#include "stdarg.h"
#include "MTX/mtx.h"
#include "MTX/mtx44.h"
#include "KPAD/kpad.h"
#include "GX/GXRenderModeObj.h"
#include "RVLGX/GXAttr.h"
#include "RVLGX/GXBump.h"
#include "RVLGX/GXFrameBuf.h"
#include "RVLGX/GXGeometry.h"
#include "RVLGX/GXLight.h"
#include "RVLGX/GXMisc.h"
#include "RVLGX/GXPixel.h"
#include "RVLGX/GXTev.h"
#include "RVLGX/GXTexture.h"
#include "RVLGX/GXTransform.h"
#include "OS/OSCache.h"
#include "MSL_C/string.h"
#include "MSL_C/printf.h"
#include "MSL_C/strtoul.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "Rmode.h"
#include "mh3_pad/Screen_w.h"
#include "mh3_pad/system_w.h"
#include "sys_font_code_table.h"
#include "sys_font_decode.h"
#include "rmode_fb_width_get.h"
#include "rmode_efb_height_get.h"
#include "ef/memory_layout_init.h"
#include "ef/tex_palette_bind.h"
#include "g3d/g3d_state.h"
#include "sound/fn_800D7F54.h"
#include "font/flfnt.h"
#include "font/flKnjMsgNum.h"
#include "font/flfntGetSizeX.h"
#include "font/dbg_drawgraph.h"
#include "font/flfnt_frame_reset.h"
#include "font/font_str_char_count.h"
#include "font/font_print_plain.h"
#include "MSL/strlen.h"
#include "font/gx_tex_obj_copy.h"

/* --- the console's types and data ---------------------------------------------------------------- */

/* One queued glyph quad: four corners, their texture coordinates and the colour (flfntGlyphPut fills it,
 * flfntFlush sends it).  size: 0x54 */
typedef struct FlFntGlyph {
    /* +0x00 */ f32 pos[4][3];
    /* +0x30 */ f32 uv[4][2];
    /* +0x50 */ u32 color;
} FlFntGlyph;

/* The console defaults flfnt_frame_reset copies into the work block.  size: 0x48 */
typedef struct FlFntConfig {
    /* +0x00 */ u32 mode;
    /* +0x04 */ u32 size_x;
    /* +0x08 */ u32 size_y;
    /* +0x0C */ u32 field_0x0c;
    /* +0x10 */ u32 field_0x10;
    /* +0x14 */ u32 field_0x14;
    /* +0x18 */ u32 unused_0x18;
    /* +0x1C */ u32 field_0x1c;
    /* +0x20 */ u32 unused_0x20;
    /* +0x24 */ u32 glyph_count;
    /* +0x28 */ u32 field_0x28;
    /* +0x2C */ u32 unused_0x2c;
    /* +0x30 */ u32 field_0x30;
    /* +0x34 */ u32 field_0x34;
    /* +0x38 */ u32 wide_mode;
    /* +0x3C */ u32 cell_w;
    /* +0x40 */ u32 cell_h;
    /* +0x44 */ u32 unused_0x44;
} FlFntConfig;

/* The console's work block, laid out in `system_w`'s font buffer (memset whole by flfnt_frame_reset).
 * size: 0x170D0 */
typedef struct FlFnt {
    /* +0x00000 */ u8 pad_00000[0x4];
    /* +0x00004 */ u32 mode;                  /* flfntFontPuts only draws when this is 1 */
    /* +0x00008 */ s32 init_size_x;
    /* +0x0000C */ s32 init_size_y;
    /* +0x00010 */ u32 field_0x10;
    /* +0x00014 */ u32 field_0x14;
    /* +0x00018 */ u32 field_0x18;
    /* +0x0001C */ u32 glyph_count;           /* the glyphs of `sys_font_code_table` the font image holds */
    /* +0x00020 */ u32 field_0x20;
    /* +0x00024 */ u32 field_0x24;
    /* +0x00028 */ u32 field_0x28;
    /* +0x0002C */ u32 field_0x2c;
    /* +0x00030 */ u32 wide_mode;             /* non-zero: every code goes through the glyph table */
    /* +0x00034 */ u8 pad_00034[0x4];
    /* +0x00038 */ struct TPLPalette* font_tpl[2]; /* the font image per layer */
    /* +0x00040 */ FlFntGlyph* glyph_base;
    /* +0x00044 */ FlFntGlyph* glyph_cur;     /* the current layer's quads */
    /* +0x00048 */ u32 glyph_size;
    /* +0x0004C */ u32 glyph_bytes;
    /* +0x00050 */ s32 layer;
    /* +0x00054 */ FlFntGlyph glyphs[1][1024];
    /* +0x15054 */ u32 tex_bytes;             /* raised by the font image's 0x20000 bytes */
    /* +0x15058 */ u32 glyph_cap;
    /* +0x1505C */ u32 glyph_used;
    /* +0x15060 */ s32 size_x;
    /* +0x15064 */ s32 size_y;
    /* +0x15068 */ s32 pos_x;
    /* +0x1506C */ s32 pos_y;
    /* +0x15070 */ s32 field_0x15070;
    /* +0x15074 */ u32 field_0x15074;
    /* +0x15078 */ u32 color;
    /* +0x1507C */ u32 attr;
    /* +0x15080 */ u32 field_0x15080;
    /* +0x15084 */ s32 initialized;
    /* +0x15088 */ u32 field_0x15088;
    /* +0x1508C */ u32 field_0x1508c;
    /* +0x15090 */ u32 field_0x15090;
    /* +0x15094 */ s16 layer_count[2];        /* quads queued per layer */
    /* +0x15098 */ s16 layer_index[2][1024];  /* the queued quads' indices per layer */
    /* +0x16098 */ s32 cache_count;
    /* +0x1609C */ s32 cache_cap;
    /* +0x160A0 */ u16 cache_code[1024];      /* the glyph codes loaded into the cell cache */
    /* +0x168A0 */ u32 cache_cols;
    /* +0x168A4 */ u32 cache_rows;
    /* +0x168A8 */ u8 pad_168A8[0x8];
    /* +0x168B0 */ u32 field_0x168b0;
    /* +0x168B4 */ u32 field_0x168b4;
    /* +0x168B8 */ u32 cell_w8;
    /* +0x168BC */ u32 cell_h8;
    /* +0x168C0 */ u32 cell_w;
    /* +0x168C4 */ u32 cell_h;
    /* +0x168C8 */ s16 layer_drawn[2];        /* quads already sent per layer */
    /* +0x168CC */ u32 flushed_used;          /* glyph_used at the last flush */
    /* +0x168D0 */ u8 pad_168D0[0x400];
    /* +0x16CD0 */ char print_buf[0x400];     /* flfntPrintf's formatting buffer */
} FlFnt;

FlFntConfig flfnt_default_config = {
    1, 22, 24, 1, 0, 0x100, 0, 4, 2, 0x13F, 0x48, 0x59B8, 0x13F, 2, 1, 24, 24, 0,
};

/* The 16 console colours `~Cnn` and the palette setters select (RGBA). */
u32 flfnt_palette[16] = {
    0xF0F0F0FF, 0x000000FF, 0xFF435DFF, 0x56FF56FF, 0x57FFFFFF, 0xFFFF50FF, 0xFFA461FF, 0xFF84FFFF,
    0xBF6464FF, 0xA0A0A0FF, 0x808080FF, 0xF08200FF, 0x846B5CFF, 0x80212EFF, 0x747EFFFF, 0xFF9ECAFF,
};

u8 flfnt_frame_work[20];

FlFnt* flfnt_state;

/* The typed printer's character count: [0] this call's, [1] the previous call's. */
s16 font_typed_count[2];

/* --- bodies, in address order ---------------------------------------------------------------------- */

/* 0x8005B26C (0xC): the bytes the system core reserves for the console work block. */
extern "C" u32 font_buffer_size_get(void)
{
    return 0x170E0;
}

/* 0x8005B278 (0x250): binds the console to `system_w`'s font buffer and, the first time, builds it. */
extern "C" void flfnt_frame_reset(void)
{
    FlFntConfig* config = &flfnt_default_config;
    SystemWork* sw = &system_w;
    FlFnt* state;

    flfnt_state = (FlFnt*)sw->layout_0x944;
    if (flfnt_state == NULL) {
        memory_layout_init();
        flfnt_state = (FlFnt*)sw->layout_0x944;
    }
    memset(flfnt_frame_work, 0, sizeof(flfnt_frame_work));
    state = flfnt_state;
    if (state->initialized == 1) {
        return;
    }
    memset(state, 0, sizeof(FlFnt));
    state->font_tpl[0] = (struct TPLPalette*)0x81636600;
    sys_font_decode((u16*)state->font_tpl[0]);
    tex_palette_bind(state->font_tpl[0], 254);
    state->tex_bytes += 0x20000;
    state->mode = config->mode;
    state->init_size_x = config->size_x;
    state->init_size_y = config->size_y;
    state->field_0x10 = config->field_0x0c;
    state->field_0x14 = config->field_0x10;
    state->field_0x18 = config->field_0x14;
    state->glyph_count = config->glyph_count;
    state->field_0x20 = config->field_0x28;
    state->field_0x24 = config->field_0x1c;
    state->field_0x28 = config->field_0x30;
    state->field_0x2c = config->field_0x34;
    state->wide_mode = config->wide_mode;
    state->cell_w = config->cell_w;
    state->cell_h = config->cell_h;
    state->glyph_cap = 1024;
    state->glyph_used = 0;
    state->size_x = state->init_size_x;
    state->size_y = state->init_size_y;
    state->pos_x = 0;
    state->pos_y = 0;
    state->field_0x15070 = -2;
    state->field_0x15074 = 0;
    state->color = 0xC0C0C0FF;
    state->attr = 0;
    state->field_0x15080 = 0;
    state->field_0x15088 = 0;
    state->field_0x1508c = 0;
    state->layer = 0;
    state->glyph_base = state->glyphs[0];
    state->glyph_cur = state->glyphs[state->layer];
    state->glyph_size = sizeof(FlFntGlyph);
    state->glyph_bytes = state->glyph_size * 1024;
    memset(state->glyphs, 0, state->glyph_bytes);
    state->cache_count = 0;
    state->cache_cols = 768 / state->cell_w;
    state->cache_rows = 384 / state->cell_h;
    state->cache_cap = state->cache_cols * state->cache_rows;
    state->field_0x168b0 = 96;
    state->field_0x168b4 = 48;
    state->cell_w8 = state->cell_w / 8;
    state->cell_h8 = state->cell_h / 8;
    flfntStackReset();
    state->initialized = 1;
    state->glyph_count = 228;
    state->size_x = 24;
}

/* 0x8005B4C8 (0x1E8): empties both layers' quad queues and the glyph cell cache. */
void flfntStackReset(void)
{
    FlFnt* state = flfnt_state;
    s32 i;
    s32 j;

    if (state == NULL) {
        return;
    }
    for (i = 0; i < 2; i++) {
        state->layer_count[i] = 0;
        state->layer_drawn[i] = 0;
        for (j = 0; j < 1024; j++) {
            state->layer_index[i][j] = -1;
        }
    }
    state->field_0x15090 = 0;
    for (j = 0; j < 1024; j++) {
        state->cache_code[j] = 0;
    }
    state->cache_count = 0;
    state->glyph_used = 0;
    state->flushed_used = 0;
    state->attr = 0;
    state->field_0x15080 = 0;
    state->glyph_cur = state->glyphs[state->layer];
}

/* --- the font/debug-print accessors ---------------------------------------------------------------- */

void flfntSetSize(s32 x, s32 y)
{
    if (flfnt_state == NULL) {
        return;
    }
    flfnt_state->size_x = x;
    flfnt_state->size_y = y;
}

s16 flfntGetSizeX(void)
{
    if (flfnt_state == NULL) {
        return 0;
    }
    return (s16)flfnt_state->size_x;
}

s16 flfntGetSizeY(void)
{
    if (flfnt_state == NULL) {
        return 0;
    }
    return (s16)flfnt_state->size_y;
}

void flfntSetPos(s32 x, s32 y)
{
    if (flfnt_state == NULL) {
        return;
    }
    flfnt_state->pos_x = x;
    flfnt_state->pos_y = y;
}

s16 flfntGetPosX(void)
{
    if (flfnt_state == NULL) {
        return 0;
    }
    return (s16)flfnt_state->pos_x;
}

s16 flfntGetPosY(void)
{
    if (flfnt_state == NULL) {
        return 0;
    }
    return (s16)flfnt_state->pos_y;
}

void flfntSetColor(u32 color)
{
    if (flfnt_state == NULL) {
        return;
    }
    flfnt_state->color = color;
}

/* 0x8005B7A0 (0x38): sets the colour from the 16-entry palette, ignoring out-of-range and no-state calls. */
extern "C" void flfntSetPaletteColor(int index)
{
    if (flfnt_state == NULL) {
        return;
    }
    if (index < 0) {
        return;
    }
    if (index >= 16) {
        return;
    }
    flfnt_state->color = flfnt_palette[index];
}

extern "C" s32 flfntUtf8Decode(u8** cursor);
extern "C" s32 flfntCodeIndex(u32 code);
extern "C" s32 flfntGlyphAdvance(u32 index);

/* 0x8005B7D8 (0x9C): the console cells a UTF-8 string occupies: 1 per narrow character, 2 per wide one. */
extern "C" s32 flfntStrCells(char* s)
{
    FlFnt* state = flfnt_state;
    s32 len = 0;
    char* p = s;

    for (;;) {
        s32 c = flfntUtf8Decode((u8**)&p);

        if (c == 0) {
            break;
        }
        if (c == '\n') {
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
        len += flfntGlyphAdvance(flfntCodeIndex(c)) == 0 ? 2 : 1;
    }
    return len;
}

/* 0x8005B874 (0x4): the cells `s` occupies. */
s32 flfntStrLen(char* s)
{
    return flfntStrCells(s);
}

/* 0x8005B878 (0xB8): the position in `s` after `index` cells, or NULL when the string is shorter. */
char* flKnjMsgNumPtr(char* s, s32 index)
{
    FlFnt* state = flfnt_state;
    s32 len = 0;
    char* p = s;

    for (;;) {
        s32 c;

        if (len >= index) {
            return p;
        }
        c = flfntUtf8Decode((u8**)&p);

        if (c == 0) {
            break;
        }
        if (c == '\n') {
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
        len += flfntGlyphAdvance(flfntCodeIndex(c)) == 0 ? 2 : 1;
    }
    return NULL;
}

/* 0x8005B930 (0x58): the characters of `s`, newlines not counted. */
s32 flKnjMsgNum(char* s)
{
    s32 count = 0;
    char* p = s;

    for (;;) {
        s32 c;

        utf82unicode2((u8*)p);
        c = flfntUtf8Decode((u8**)&p);
        if (c == 0) {
            break;
        }
        if (c == '\n') {
            continue;
        }
        count++;
    }
    return count;
}

/* 0x8005B988 (0x18): stores the console's attribute word. */
extern "C" void flfntSetAttr(u32 value)
{
    if (flfnt_state == NULL) {
        return;
    }
    flfnt_state->attr = value;
}

/* 0x8005B9A0 (0xB0): formats into the console buffer and prints it at the pen. */
void flfntPrintf(char* fmt, ...)
{
    va_list args;
    char* buf;

    if (flfnt_state == NULL) {
        return;
    }
    buf = flfnt_state->print_buf;
    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    flfntFontPuts(buf);
}

extern "C" void flfntDrawText(char* s);

/* Puts one string through the draw path when the console is in its single-line mode (state+0x4 == 1). */
void flfntFontPuts(char* s)
{
    if (flfnt_state->mode != 1) {
        return;
    }
    flfntDrawText(s);
}

/* 0x8005BA68 (0x48): the cell cache slot holding `code`, or -1. */
extern "C" s32 flfntCacheFind(FlFnt* state, u32 code)
{
    s32 i;

    for (i = 0; i < state->cache_count; i++) {
        if ((u16)code == state->cache_code[i]) {
            return i;
        }
    }
    return -1;
}

/* 0x8005BAB0 (0x40): the next free quad of the current layer, or NULL when the queue is full. */
extern "C" FlFntGlyph* flfntGlyphAlloc(FlFnt* state)
{
    if (state->glyph_used >= state->glyph_cap) {
        return NULL;
    }
    return &state->glyphs[state->layer][state->glyph_used];
}

/* 0x8005BAF0 (0x4): loads glyph `code` into cache cell `slot` (nothing to do: the font image is resident). */
extern "C" void flfntCacheLoad(FlFnt* state, s32 slot, u32 code)
{
}

/* 0x8005BAF4 (0x248): queues the quad of glyph `code` at (x, y) in the current colour. */
extern "C" void flfntGlyphPut(s32 x, s32 y, u32 code)
{
    FlFnt* state = flfnt_state;
    FlFntGlyph* glyph = flfntGlyphAlloc(state);
    f32 cell;
    s32 w;
    s32 h;

    if (glyph == NULL) {
        return;
    }
    if (flfntCacheFind(state, code) == -1) {
        s32 slot = state->cache_count;

        if (slot >= state->cache_cap) {
            return;
        }
        state->cache_code[slot] = code;
        state->cache_count++;
        flfntCacheLoad(state, slot, code);
    }
    cell = 0.0458984375f;
    w = state->size_x;
    h = state->size_y;
    glyph->pos[0][0] = x;
    glyph->pos[0][1] = y;
    glyph->pos[0][2] = 0.0f;
    glyph->uv[0][0] = (f32)((code & 0xF) * 32) / 512.0f;
    glyph->uv[0][1] = (f32)((code >> 4) * 32) / 512.0f;
    glyph->pos[1][0] = x + w;
    glyph->pos[1][1] = y;
    glyph->pos[1][2] = 0.0f;
    glyph->uv[1][0] = glyph->uv[0][0] + cell;
    glyph->uv[1][1] = glyph->uv[0][1];
    glyph->pos[2][0] = x + w;
    glyph->pos[2][1] = y + h;
    glyph->pos[2][2] = 0.0f;
    glyph->uv[2][0] = glyph->uv[0][0] + cell;
    glyph->uv[2][1] = glyph->uv[0][1] + cell;
    glyph->pos[3][0] = x;
    glyph->pos[3][1] = y + h;
    glyph->pos[3][2] = 0.0f;
    glyph->uv[3][0] = glyph->uv[0][0];
    glyph->uv[3][1] = glyph->uv[0][1] + cell;
    glyph->color = state->color;
    state->layer_index[state->layer][state->layer_count[state->layer]] = state->glyph_used;
    state->layer_count[state->layer]++;
    state->glyph_used++;
}

/* 0x8005BD3C (0x104): decodes the UTF-8 character at `*cursor` and advances it; -1 on a malformed sequence. */
extern "C" s32 flfntUtf8Decode(u8** cursor)
{
    s32 c;
    s32 c2;
    s32 c3;

    c = **cursor;
    (*cursor)++;
    if (c <= 0x7F) {
        return c;
    }
    if (c >= 0xC0 && c <= 0xDF) {
        c2 = **cursor;
        if (c2 == 0) {
            return -1;
        }
        (*cursor)++;
        if (c2 >= 0x80 && c2 <= 0xBF) {
            return ((c & 0x1F) << 6) | (c2 & 0x3F);
        }
        return -1;
    }
    if (c >= 0xE0 && c <= 0xEF) {
        c2 = **cursor;
        if (c2 == 0) {
            return -1;
        }
        (*cursor)++;
        if (c2 < 0x80 || c2 > 0xBF) {
            return -1;
        }
        c3 = **cursor;
        if (c3 == 0) {
            return -1;
        }
        (*cursor)++;
        if (c3 >= 0x80 && c3 <= 0xBF) {
            return ((c & 0xF) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F);
        }
        return -1;
    }
    return -1;
}

/* 0x8005BE40 (0xE0): the byte length of the UTF-8 character at `str`, 0 when it is malformed. */
s32 utf82unicode2(u8* str)
{
    u8 buf[1024];
    s32 c;

    strcpy((char*)buf, (char*)str);
    c = buf[0];
    if (c <= 0x7F) {
        return 1;
    }
    if (c >= 0xC0 && c <= 0xDF) {
        c = buf[1];
        if (c == 0) {
            return 0;
        }
        if ((u32)(c - 0x80) > 0x3F) {
            return 0;
        }
        return 2;
    }
    if (c >= 0xE0 && c <= 0xEF) {
        c = buf[1];
        if (c == 0) {
            return 0;
        }
        if (c < 0x80 || c > 0xBF) {
            return 0;
        }
        c = buf[2];
        if (c == 0) {
            return 0;
        }
        if ((u32)(c - 0x80) > 0x3F) {
            return 0;
        }
        return 3;
    }
    return 0;
}

/* 0x8005BF20 (0x40): the glyph index of character `code` ('?' for a code the font lacks). */
extern "C" s32 flfntCodeIndex(u32 code)
{
    const u16* entry;
    s32 i;

    for (i = 0, entry = sys_font_code_table; *entry != 0xFFFF; i++, entry += 2) {
        if (*entry == code) {
            return (u16)i;
        }
    }
    return flfntCodeIndex('?');
}

/* 0x8005BF60 (0x8): the glyph of an ASCII code (none: the console draws ASCII from the font image's cells). */
extern "C" u32 flfntAsciiCode(void)
{
    return 0;
}

/* 0x8005BF68 (0x44): the cells of the character at `glyph`: 1 for a narrow one, 2 otherwise. */
extern "C" u16 getGlyphWidth(const char* glyph)
{
    return flfntGlyphAdvance(flfntCodeIndex(flfntUtf8Decode((u8**)&glyph))) == 1 ? 1 : 2;
}

/* 0x8005BFAC (0x30): the advance class of glyph `index` (0 wide, 1 narrow), 1 past the loaded table. */
extern "C" s32 flfntGlyphAdvance(u32 index)
{
    if (index >= flfnt_state->glyph_count) {
        return 1;
    }
    return sys_font_code_table[index * 2 + 1];
}

/* 0x8005BFDC (0x134): queues the quads of `s` at the pen; '\n' returns to the starting column one cell lower. */
extern "C" void flfntDrawText(char* s)
{
    FlFnt* state = flfnt_state;
    char* p = s;
    s32 left = state->pos_x;
    u32 c;
    u32 code;
    u32 shift;

    for (;;) {
        c = flfntUtf8Decode((u8**)&p);
        if (c == 0) {
            break;
        }
        if (c == ' ') {
            shift = 1;
        } else if (c == '\n') {
            state->pos_x = left;
            state->pos_y += state->size_x;
            continue;
        } else {
            do {
                if (state->wide_mode == 0 && c <= 15) {
                    shift = 0;
                    flfntAsciiCode();
                } else if (state->wide_mode == 0 && c <= 127) {
                    shift = 1;
                    flfntAsciiCode();
                } else {
                    code = flfntCodeIndex(c);
                    shift = flfntGlyphAdvance(code);
                    if (code >= state->glyph_count) {
                        break;
                    }
                    if (state->glyph_used >= state->glyph_cap) {
                        return;
                    }
                }
                flfntGlyphPut(state->pos_x, state->pos_y, code);
                state->glyph_used++;
            } while (0);
        }
        state->pos_x += (u32)state->size_x >> shift;
    }
}

extern "C" void fntGXEnd(void);
extern "C" void fntGXTexCoord2f32(f32 s, f32 t);
extern "C" void fntGXColor1u32(u32 color);
extern "C" void fntGXPosition3f32(f32 x, f32 y, f32 z);
extern "C" void fntGXSetTexCoordGen(u32 coord, u32 func, u32 src, u32 mtx);

/* 0x8005C110 (0x3BC): sends the quads queued since the last flush, layer by layer. */
void flfntFlush(void)
{
    FlFnt* state = flfnt_state;
    BOOL pending;
    s32 i;
    s32 j;
    FlFntGlyph* base;
    GXTexObj tex;
    Mtx44 proj;
    Mtx mv;

    if (state == NULL) {
        return;
    }
    if (state->glyph_used == 0) {
        return;
    }
    if (state->glyph_used == state->flushed_used) {
        return;
    }
    pending = FALSE;
    for (i = 0; i < 2; i++) {
        if (state->layer_count[i] > state->layer_drawn[i]) {
            pending = TRUE;
        }
    }
    if (!pending) {
        return;
    }
    dbg_drawgraph_init();
    GXInvalidateTexAll();
    gx_tex_obj_copy(&tex, (GXTexObj*)system_w.tex_0x96c[1].body);
    GXInitTexObjLOD(&tex, 1, 1, 0.0f, 0.0f, 0.0f, 0, 0, 0);
    GXSetBlendMode(1, 4, 5, 0);
    GXSetColorUpdate(1);
    C_MTXOrtho(proj, 0.0f, Screen_w.height_f, 0.0f, Screen_w.width_f, 0.0f, 1.0f);
    GXSetProjection(proj, 1);
    PSMTXIdentity(mv);
    GXLoadPosMtxImm(mv, 0);
    GXSetCurrentMtx(0);
    GXSetCullMode(0);
    GXSetZMode(1, 7, 0);
    GXSetNumTexGens(1);
    fntGXSetTexCoordGen(0, 1, 4, 60);
    DCFlushRange(state->font_tpl[state->layer], 0x20000);
    GXLoadTexObj(&tex, 0);
    GXSetNumTevStages(1);
    GXSetTevDirect(0);
    GXSetTevOp(0, 0);
    GXSetTevOrder(0, 0, 0, 4);
    GXSetNumChans(1);
    GXSetChanCtrl(0, 0, 1, 1, 0, 0, 2);
    GXSetChanCtrl(2, 0, 1, 1, 0, 0, 2);
    GXClearVtxDesc();
    GXSetVtxDesc(9, 1);
    GXSetVtxDesc(11, 1);
    GXSetVtxDesc(13, 1);
    GXSetVtxAttrFmt(0, 9, 1, 4, 0);
    GXSetVtxAttrFmt(0, 11, 1, 5, 0);
    GXSetVtxAttrFmt(0, 13, 1, 4, 0);
    base = state->glyph_cur;
    for (i = 0; i < 2; i++) {
        if (state->layer_count[i] > state->layer_drawn[i]) {
            GXBegin(0x80, 0, (state->layer_count[i] - state->layer_drawn[i]) * 4);
            for (j = state->layer_drawn[i]; j < state->layer_count[i]; j++) {
                FlFntGlyph* glyph = &base[state->layer_index[i][j]];

                fntGXPosition3f32(glyph->pos[0][0], glyph->pos[0][1], glyph->pos[0][2]);
                fntGXColor1u32(glyph->color);
                fntGXTexCoord2f32(glyph->uv[0][0], glyph->uv[0][1]);
                fntGXPosition3f32(glyph->pos[1][0], glyph->pos[1][1], glyph->pos[1][2]);
                fntGXColor1u32(glyph->color);
                fntGXTexCoord2f32(glyph->uv[1][0], glyph->uv[1][1]);
                fntGXPosition3f32(glyph->pos[2][0], glyph->pos[2][1], glyph->pos[2][2]);
                fntGXColor1u32(glyph->color);
                fntGXTexCoord2f32(glyph->uv[2][0], glyph->uv[2][1]);
                fntGXPosition3f32(glyph->pos[3][0], glyph->pos[3][1], glyph->pos[3][2]);
                fntGXColor1u32(glyph->color);
                fntGXTexCoord2f32(glyph->uv[3][0], glyph->uv[3][1]);
            }
            fntGXEnd();
            state->layer_drawn[i] = state->layer_count[i];
        }
    }
    g3d_state_invalidate(0x7FF);
    state->flushed_used = state->glyph_used;
}

/* The GX pipe helpers flfntFlush sends its quads through. */
extern "C" void fntGXEnd(void)
{
}

extern "C" void fntGXTexCoord2f32(f32 s, f32 t)
{
    GXWGFifo.f32 = s;
    GXWGFifo.f32 = t;
}

extern "C" void fntGXColor1u32(u32 color)
{
    GXWGFifo.u32 = color;
}

extern "C" void fntGXPosition3f32(f32 x, f32 y, f32 z)
{
    GXWGFifo.f32 = x;
    GXWGFifo.f32 = y;
    GXWGFifo.f32 = z;
}

extern "C" void fntGXSetTexCoordGen(u32 coord, u32 func, u32 src, u32 mtx)
{
    GXSetTexCoordGen2(coord, func, src, mtx, 0, 125);
}

/* 0x8005C50C (0x44): copies the 0x20-byte texture object `src` into `dst`. */
extern "C" void gx_tex_obj_copy(GXTexObj* dst, const GXTexObj* src)
{
    *dst = *src;
}

/* --- the font library's public wrappers ------------------------------------------------------------- */

/* 0x8005C550 (0xC8): formats and prints at the pen, unless system_w's print mode suppresses it. */
void font_print(s8* fmt, ...)
{
    va_list args;
    char buf[512];

    if (system_w.print_mode_0x23 == 3 && system_w.pause_0x10 != 0) {
        return;
    }
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf) - 1, (char*)fmt, args);
    buf[sizeof(buf) - 1] = '\0';
    flfntPrintf("%s", buf);
}

/* 0x8005C618 (0x1C8): prints `text` at the pen, switching colour at each `~Cnn` code and wrapping one '\n'. */
extern "C" void flfntColorPuts(char* text)
{
    char line[512];
    char rest[512];
    char* tail;
    char* mark;
    char* newline;
    s16 size_x;
    s16 pos_y;
    s16 pos_x;
    s16 left;
    s32 color;

    strcpy(line, text);
    mark = flfntStrChr(line, '~');
    if (mark == NULL) {
        flfntFontPuts(line);
        return;
    }
    size_x = flfntGetSizeX();
    pos_x = flfntGetPosX();
    pos_y = flfntGetPosY();
    left = pos_x;
    do {
        if (mark[1] == 'C') {
            color = (u8)(mark[3] + (mark[2] - '0') * 10 - '0');
            if (color >= 16) {
                strcpy(mark, mark + 4);
                tail = mark;
            } else {
                *mark = '\0';
                tail = line;
                if (flfntStrLen(tail) != 0) {
                    flfntSetPos(pos_x, pos_y);
                    flfntFontPuts(tail);
                    pos_x = flfntGetPosX();
                    pos_y = flfntGetPosY();
                }
                strcpy(tail, mark + 4);
                flfntSetPaletteColor(color);
            }
        } else {
            tail = mark + 1;
        }
        mark = flfntStrChr(tail, '~');
    } while (mark != NULL);
    if (flfntStrLen(line) == 0) {
        return;
    }
    flfntSetPos(pos_x, pos_y);
    newline = flfntStrChr(line, '\n');
    if (newline != NULL) {
        *newline = '\0';
        flfntPrintf(line);
        flfntSetPos(left, (s16)(flfntGetPosY() + size_x));
        if (strlen(newline + 1) == 0) {
            return;
        }
        strcpy(rest, newline + 1);
        flfntFontPuts(rest);
    } else {
        flfntFontPuts(line);
    }
}

/* 0x8005C7E0 (0x4): the substring search the font helpers share (a thin `strchr`). */
extern "C" char* flfntStrChr(char* s, s32 c)
{
    return strchr(s, c);
}

/* 0x8005C7E4 (0xC): moves the pen. */
void font_locate(s16 x, s16 y)
{
    flfntSetPos(x, y);
}

/* 0x8005C7F0 (0x14): the palette selector's short-typed entry point. */
extern "C" void font_color_set(s16 index)
{
    if (index >= 16) {
        return;
    }
    flfntSetPaletteColor(index);
}

void font_set_size(s16 x, s16 y)
{
    flfntSetSize(x, y);
}

/* 0x8005C810 (0xE0): moves the pen, selects palette colour `color` and prints the formatted text. */
void font_print_ex(s16 x, s16 y, s16 color, s8* fmt, ...)
{
    va_list args;
    char buf[512];

    if (system_w.print_mode_0x23 == 3 && system_w.pause_0x10 != 0) {
        return;
    }
    font_locate(x, y);
    font_color_set(color);
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), (char*)fmt, args);
    flfntColorPuts(buf);
}

/* 0x8005C8F0 (0xB4): font_print_ex's unformatted twin: prints `fmt` as it is. */
extern "C" void font_print_plain(s16 x, s16 y, s16 color, const char* fmt, ...)
{
    if (system_w.print_mode_0x23 == 3 && system_w.pause_0x10 != 0) {
        return;
    }
    font_locate(x, y);
    font_color_set(color);
    flfntColorPuts((char*)fmt);
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

/* 0x8005C9B8 (0xAC): the printable characters of `text` (colour codes and newlines skipped). */
extern "C" s32 font_str_char_count(const char* text)
{
    u32 two;
    BOOL lead;
    s32 count = 0;

    while (*text != '\0') {
        if (*text == '~' && text[1] == 'C') {
            text += 4;
        } else if (*text == '\n') {
            text++;
        } else {
            two = TRUE;
            lead = (u8)*text >= 0x80 && (u8)*text <= 0x9F;
            if (!lead) {
                lead = (u8)*text >= 0xE0 && (u8)*text <= 0xFF;
                if (!lead) {
                    two = FALSE;
                }
            }
            if (two == TRUE) {
                text++;
            }
            count++;
            text++;
        }
    }
    return count;
}

/* 0x8005CA64 (0x46C): prints the first `count` characters of `text` at (x, y); 1 once all are out. */
extern "C" s8 font_print_typed(s32 x, s32 y, s16 color, s32 count, const char* text)
{
    char code[8];
    char src[256];
    char out[256];
    s32 total;
    char* end;
    s32 n;
    char* p;
    s32 len;
    s32 i;
    s32 line_h;

    if (flfntStrLen((char*)text) == 0) {
        return 1;
    }
    total = flKnjMsgNum((char*)text);
    if (total == 0) {
        return 1;
    }
    if (count == 0) {
        return 0;
    }
    strcpy(src, text);
    if (count > total) {
        count = total;
    }
    end = flKnjMsgNumPtr(src, count);
    if (end == NULL) {
        return 1;
    }
    font_typed_count[1] = font_typed_count[0];
    flfntSetPos(x, y);
    flfntSetPaletteColor(color);
    flfntGetSizeX();
    line_h = flfntGetSizeY();
    p = src;
    n = 0;
    while (p < end) {
        len = utf82unicode2((u8*)p);
        if (len > 1) {
            for (i = 0; i < len; i++) {
                out[n++] = *p++;
            }
        } else if ((len = utf82unicode2((u8*)&p[1])) > 1 && *p != '\n') {
            out[n++] = *p++;
            for (i = 0; i < len; i++) {
                out[n++] = *p++;
            }
        } else if (*p == '~' && p[1] == 'C') {
            code[0] = p[2];
            code[1] = p[3];
            code[2] = '\0';
            p += 4;
            if (n != 0) {
                out[n] = '\0';
                flfntFontPuts(out);
                n = 0;
                out[0] = '\0';
                flfntSetPos(flfntGetPosX(), y);
            }
            flfntSetPaletteColor(atoi(code));
        } else if (*p == '\n') {
            p++;
            if (n != 0) {
                out[n] = '\0';
                flfntFontPuts(out);
                n = 0;
                out[0] = '\0';
            }
            y += line_h;
            flfntSetPos(x, y);
        } else {
            out[n++] = *p++;
        }
    }
    if (n != 0) {
        out[n] = '\0';
        flfntFontPuts(out);
    }
    font_typed_count[0] = count;
    if (font_typed_count[0] != font_typed_count[1]) {
        sysSE_req(33);
    }
    if (count >= total) {
        return 1;
    }
    return 0;
}
