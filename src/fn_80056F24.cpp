/*
 * `fn_80056F24.cpp` - the game-root screen fade / filter / glare band.
 *
 * RANGE. `.text` 0x80056F24..0x80059550 (59 symbols), `extab` 0x80007218..0x800072D0, `extabindex`
 *   0x8001F494..0x8001F5A8, `.ctors` 0x8056F2CC..0x8056F2D0 (the `game_draw_sinit` static-init word), `.data`
 *   0x8058B178..0x8058B220 (the fade step table), `.bss` 0x8066AC88..0x8066AE48 (the fade slots and the draw
 *   state block), `.sdata2` 0x80795C48..0x80795CA0.
 * RANGE. Several TUs, not one: the out-of-line GX inline family (`GXEnd`, `GXTexCoord2f32`, `GXColor4u8`,
 *   `GXPosition2s16`, `GXSetTexCoordGen`) is emitted three times (0x80057290, 0x80057DB8, 0x80058B50) and the
 *   `.sdata2` pool repeats 0.0f/1.0f/0.5f in four runs (0x80795C48 glare, 0x80795C58 panel, 0x80795C68 fade
 *   draw, 0x80795C70 screen fade).  Candidate seams: 0x800578B4 (glare | panel), 0x80058030 (panel | fade
 *   slots), 0x800584B0 (fade slots | screen filter).  Filed as a seam request; the bodies stay here until it
 *   is ruled.
 * NAMES. Class 1 (`__FILE__`) and class 2 (runtime dump: `FUN_`/`zz_` placeholders) fail; the map's own
 *   `GlareFilter_on`, `fade_set`, `fade_reset`, `get_fade_stat`, `filter_reset`, `setFilterPrio` are real.
 * NAMES. GUESS (from each body and its callers): filter_panel_on, filter_flag_clear, filter_panel_flag_clear,
 *   glare_filter_draw, glare_filter_capture, filter_panel_draw, filter_panel_upper_set,
 *   filter_panel_lower_set, fade_update, fade_draw_screen, game_draw_calc, screen_capture_copy,
 *   screen_capture_draw, screen_copy_tex_capture, screen_fade_draw, fade_work, fade_step_table and the
 *   `.sdata2` pool names (glare_/panel_/fade_/screen_ + value); filter_panel_init, filter_panel_reset,
 *   fade_work_clear, fade_color_get, screen_size_get, capture_size_get, game_draw_init,
 *   screen_fade_alpha_update, draw_sprite_tick, screen_fade_start, screen_fade_ease, game_draw_sinit,
 *   game_draw_state_ctor, draw_sprite_slot_ctor; the GX inline copies `<glare|panel|screen>_gx_<SDK inline>`.
 * FLAGS. `cflags_main`; `#pragma peephole off` for the whole TU (the FIFO writers keep `clrlwi`/`extsh`).
 * RESIDUALS. The `.sdata2` pool is read through `extern` declarations of the map's rows, so the object emits
 *   no `.sdata2` of its own (target-extra 0x58 B) until the seam is redrawn and each TU emits its own pool.
 * RESIDUALS. `glare_filter_capture`'s unsigned-to-float conversion constant comes from our pool, not the
 *   target's `glare_f64_u32_bias` (a relocation name difference only).
 * RESIDUALS. `screen_tex_draw`/`screen_capture_draw`: retail extends the quad width/height once into a
 *   register reused by the later `screen_gx_position2s16` calls; ours extends at each call (s16/s32 locals,
 *   s16/s32 writer parameters, inline writers all measured the same).
 * RESIDUALS. `glare_filter_capture`: the same lazy `extsh` of `width`/`height` inside the halving loop.
 * RESIDUALS. `filter_panel_draw`: retail's frame is 0x10 B larger and its two strips use disjoint saved
 *   registers; `fade_update`/`fade_set`: volatile register assignment only (declaration orders measured).
 * RESIDUALS. `filter_panel_upper_set`/`_lower_set`: `fmuls` operand order (height * scale; the swapped
 *   spelling measures lower).
 */

#include "types.h"
#include "gx.h"
#include "unsplit/unknown.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "main.h"    /* _MH_VEC2 / get_ScreenSize / fn_8004030C (rule 2) */
#include "copyVec2.h"
#include "MTX/mtx.h"
#include "MTX/mtx44.h"
#include "KPAD/kpad.h"
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
#include "OS/DCInvalidateRange.h"
#include "gx/GXRenderModeObj.h"
#include "Rmode.h"
#include "disp_beta.h"
#include "font/flfntGetSizeX.h"  /* dbg_drawgraph_init (rule 2) */
#include "pad_hold_ck.h"
#include "screen_split_ck.h"
#include "color_rgba_copy.h"
#include "mh3_pad/Screen_w.h"
#include "ef/GameMode_ck.h"
#include "ef/sys_quest_move.h"
#include "ef/loading_disp_flag_get.h"
#include "lobby/lobby_w.h"
#include "g3d/g3d_state.h"       /* g3d_state_invalidate (rule 2) */
#include "stage/stage_light_update.h"
#include "hud/cockpit_quest.h"     /* quest_marker_draw_sub (rule 2) */
#include "fade_draw_upper.h"
#include "fade_draw_lower.h"
#include "screen_tex_draw.h"
#include "layout_overlay_draw.h"
#include "math_sin_deg.h"

/* The peephole pass is off for this whole TU: the retail FIFO writers keep the narrowing the pass would
 * fold away (`clrlwi r0,r3,24; stb r0,-0x8000(r3)` in `glare_gx_color4u8`/`screen_gx_color4u8`, `extsh r0,r3,16` in
 * `glare_gx_position2s16`/`screen_gx_position2s16`/`panel_gx_position2s16`), and they are the only functions in the range whose shape
 * depends on it.  The same per-unit device `ef/eft002.cpp` and `ef/eft004.cpp` use (playbook 39). */
#pragma peephole off

/* ---------------------------------------------------------------------------------------------------
 * The work blocks.
 * ------------------------------------------------------------------------------------------------- */

/* The fade table entry `fade_step_table` (0x18 B): the colour at +0x04, the from/to alpha at +0x08/+0x0C
 * and the per-tick step at +0x10.  `fade_set` reads all four; only the unread head is padding. */
typedef struct FadeStep {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ u32 color_0x04;
    /* +0x08 */ s32 from_0x08;
    /* +0x0C */ s32 to_0x0C;
    /* +0x10 */ s32 step_0x10;
    /* +0x14 */ u32 pad_0x14;
} FadeStep; /* size: 0x18 */

extern "C" FadeStep fade_step_table[];

/* `fade_work` (.bss, 0x70 B): the four fade slots, one array per field (`fade_work_clear` zeroes all 0x70,
 * `fade_update` walks the four `state` entries). size: 0x70 */
typedef struct FadeWork {
    /* +0x00 */ u32 color[4];   /* ARGB, the top byte is the alpha the walk eases */
    /* +0x10 */ s32 state[4];   /* 0 idle, 1 fade-in, 2 fade-out, 3 finished */
    /* +0x20 */ u8 pad_0x20[0x10];
    /* +0x30 */ s32 ticks[4];   /* the ticks left */
    /* +0x40 */ s32 tbl[4];     /* the `fade_step_table` index `fade_set` was called with */
    /* +0x50 */ s32 target[4];  /* the alpha the slot eases to */
    /* +0x60 */ s32 step[4];    /* the per-tick alpha step */
} FadeWork; /* size: 0x70 */

extern "C" FadeWork fade_work;

/* The glare filter at +0x74 of the draw state: a luminance-thresholded, repeatedly halved copy of the
 * screen blended back over it (`glare_filter_capture` builds it, `glare_filter_draw` blends it). size: 0x3C */
typedef struct GlareFilter {
    /* +0x00 */ u8 enable;
    /* +0x01 */ u8 pad_0x01;
    /* +0x02 */ u8 color_r;
    /* +0x03 */ u8 color_g;
    /* +0x04 */ u8 color_b;
    /* +0x05 */ u8 blend_src;      /* GXSetBlendMode source factor */
    /* +0x06 */ u8 blend_dst;      /* GXSetBlendMode destination factor */
    /* +0x07 */ u8 pad_0x07;
    /* +0x08 */ u8* tex_buf;       /* the glare texture */
    /* +0x0C */ u8 pad_0x0C[0x04];
    /* +0x10 */ s32 reduce_count;  /* how many times the copy is halved */
    /* +0x14 */ u16 tex_width;     /* the texture size the last capture left */
    /* +0x16 */ u16 tex_height;
    /* +0x18 */ u32 shift;         /* the luminance right shift */
    /* +0x1C */ s16 width;         /* the first pass's quad size */
    /* +0x1E */ s16 height;
    /* +0x20 */ u8 pad_0x20[0x08];
    /* +0x28 */ u32 dark_level;    /* below it the luminance is scaled down */
    /* +0x2C */ u32 bright_level;  /* above it the luminance is scaled up */
    /* +0x30 */ f32 dark_scale;
    /* +0x34 */ f32 bright_scale;
    /* +0x38 */ u8 pad_0x38[0x04];
} GlareFilter; /* size: 0x3C */

/* The 2D gradient panel at +0xB0 of the draw state: two strips of six vertex colours (upper and lower
 * screen half), each with its own height scale. size: 0x44 */
typedef struct FilterPanel {
    /* +0x00 */ u8 enable;
    /* +0x01 */ u8 blend;          /* 1 additive, 2 subtractive, else alpha */
    /* +0x02 */ u8 pad_0x02[0x02];
    /* +0x04 */ u32 upper_color[6];
    /* +0x1C */ f32 upper_scale;
    /* +0x20 */ f32 lower_scale;
    /* +0x24 */ s16 upper_height;  /* screen height * upper_scale */
    /* +0x26 */ s16 lower_height;  /* screen height * lower_scale */
    /* +0x28 */ u8 pad_0x28[0x04];
    /* +0x2C */ u32 lower_color[6];
} FilterPanel; /* size: 0x44 */

/* The sprite slot at +0x3C of the draw state (`draw_sprite_tick` delegates to its owner and ticks the
 * count).  Its owner type lives in the neighbouring `draw_shape_arm.cpp` registration; this is the partial
 * view this range touches. size: 0x38 */
typedef struct DrawSpriteSlot {
    /* +0x00 */ u8 pad_0x00[0x18];
    /* +0x18 */ s32 count;
    /* +0x1C */ u8 pad_0x1C[0x1C];
} DrawSpriteSlot; /* size: 0x38 */

/* A draw layer's callback: it takes the layer's own block. */
/* untyped: caller-owned payload - each layer's own block (sprite slot, glare filter, panel) */
typedef void (*DrawLayerFunc)(void* block);

/* The screen-fade/filter state block `lbl_8066ACF8` (.bss, 0x14C B; `game_draw_init` memsets all of it).
 * Only the bytes this range touches are named. size: 0x14C */
typedef struct GameDrawState {
    /* +0x000 */ u8 pad_0x00[0x01];
    /* +0x001 */ u8 layer_count;
    /* +0x002 */ u8 layer_mask;          /* bit n enables layer n */
    /* +0x003 */ u8 pad_0x03[0x01];
    /* +0x004 */ u8 layer_order[6];      /* the draw order; `setFilterPrio` swaps entries 2 and 3 */
    /* +0x00A */ u8 pad_0x0A[0x02];
    /* +0x00C */ DrawLayerFunc layer_draw[6];
    /* +0x024 */ void* layer_arg[6];
    /* +0x03C */ DrawSpriteSlot sprite_0x3C; /* the sprite slot `draw_sprite_tick` ticks */
    /* +0x074 */ GlareFilter glare;
    /* +0x0B0 */ FilterPanel panel;
    /* +0x0F4 */ u8 pad_0x0F4[0x28];
    /* +0x11C */ u8 filter_prio;         /* `setFilterPrio` stores it */
    /* +0x11D */ u8 pad_0x11D[0x03];
    /* +0x120 */ _MH_VEC2 capture_size;  /* the EFB copy size */
    /* +0x128 */ _MH_VEC2 screen_size;
    /* +0x130 */ u32 capture_buf_size;
    /* +0x134 */ u8* capture_buf;        /* the per-frame EFB copy */
    /* +0x138 */ u8* screen_tex;         /* the held screen image */
    /* +0x13C */ u8 fade_phase;          /* screen-fade phase: 0 idle, 1..3 easing */
    /* +0x13D */ GXColor fade_color;     /* `a` is the eased alpha */
    /* +0x141 */ u8 pad_0x141[0x03];
    /* +0x144 */ f32 fade_value;         /* the angle `screen_fade_ease` eases toward zero */
    /* +0x148 */ f32 field_0x148;
} GameDrawState; /* size: 0x14C */

extern "C" GameDrawState lbl_8066ACF8;

/* The `.sdata2` pool words this range reads (the map's rows; see RESIDUALS). */
extern "C" {
extern f32 glare_f32_zero;        /* 0.0f */
extern f32 glare_f32_one;         /* 1.0f */
extern f32 panel_f32_half;        /* 0.5f */
extern f32 panel_f32_zero;        /* 0.0f */
extern f32 panel_f32_one;         /* 1.0f */
extern f32 fade_f32_zero;         /* 0.0f */
extern f32 fade_f32_half;         /* 0.5f */
extern f32 screen_f32_0_6;        /* 0.6f */
extern f32 screen_f32_zero;       /* 0.0f */
extern f32 screen_f32_one;        /* 1.0f */
extern f32 screen_f32_255;        /* 255.0f */
extern f32 screen_f32_deg_to_idx; /* 0.7111111f = 256 / 360 */
extern f32 screen_fade_f32_floor; /* 25.0f */
extern f32 screen_fade_f32_start; /* 90.0f */
extern f32 screen_fade_f32_start_long; /* 50.0f */
extern f32 screen_fade_f32_step1; /* 18.0f */
extern f32 screen_fade_f32_step2; /* 5.0f */
extern f32 screen_fade_f32_step3; /* 0.65f */
}

/* ---------------------------------------------------------------------------------------------------
 * Declarations.  The `fn_*` callees are unsplit band neighbours, the `GX*` symbols are the SDK's, and
 * `SinFIdx` is called through its owner namespace (rule 9).
 * ------------------------------------------------------------------------------------------------- */

namespace nw4r { namespace math { f32 SinFIdx(f32); } }

extern "C" {
s32   fn_80056AF0(void* p);
void  fn_80056A3C(s32 count);
}

/* The band neighbours below are plain `fn_` thunks: the target object references them by their plain map
 * name, so they carry C linkage. */
extern "C" {
void fn_80055EC4(void* block);
void fn_80056E1C(void* block);
void fn_80055F58(void);
void fn_800569CC(void);
void fn_80056F04(void);
}

extern "C" {
void glare_filter_capture(GlareFilter* glare);
void filter_panel_draw(FilterPanel* panel);
void screen_capture_copy(GameDrawState* s, u8 force);
void screen_capture_draw(GameDrawState* s, u8 mode);
void screen_fade_draw(GameDrawState* s);
void screen_fade_alpha_update(GameDrawState* s);
void draw_sprite_tick(GameDrawState* s);
void screen_size_get(_MH_VEC2* dst);
void capture_size_get(_MH_VEC2* dst);
u32 fade_color_get(s32 slot);
}
s32 get_fade_stat(s32 slot);

/* The FIFO writer families, defined below in address order. */
extern "C" {
void glare_gx_end(void);
void glare_gx_texcoord2f32(f32 x, f32 y);
void glare_gx_color4u8(u32 r, u32 g, u32 b, u32 a);
void glare_gx_position2s16(s16 x, s16 y);
void glare_gx_set_tex_coord_gen(u32 a, u32 b, u32 c, u32 d);
void panel_gx_end(void);
void panel_gx_color1u32(s32 v);
void panel_gx_position2s16(s32 x, s32 y);
void screen_gx_end(void);
void screen_gx_texcoord2f32(f32 x, f32 y);
void screen_gx_color4u8(u32 r, u32 g, u32 b, u32 a);
void screen_gx_position2s16(s32 x, s32 y);
void screen_gx_set_tex_coord_gen(u32 a, u32 b, u32 c, u32 d);
}

/* ---------------------------------------------------------------------------------------------------
 * The glare filter.
 * ------------------------------------------------------------------------------------------------- */

/* 0x80056F24 (0x36C): the glare layer: refreshes the screen copy, rebuilds the glare texture and blends it
 * over the screen in the glare colour. */
extern "C" void glare_filter_draw(GlareFilter* glare) {
    GXTexObj tex;
    Mtx44 proj;
    Mtx view;
    _MH_VEC2 capture;
    _MH_VEC2 screen;
    f32 one;
    u8 r;
    u8 g;
    u8 b;

    if (pad_hold_ck() == 0 && lbl_8066ACF8.filter_prio != 0) {
        screen_capture_copy(&lbl_8066ACF8, 0);
    }
    if (glare->enable != 0) {
        glare_filter_capture(glare);
        screen_capture_draw(&lbl_8066ACF8, 0);
        capture_size_get(&capture);
        screen_size_get(&screen);
        GXInitTexObj(&tex, glare->tex_buf, glare->tex_width, glare->tex_height, 6, 0, 0, 0);
        GXInitTexObjLOD(&tex, 1, 1, glare_f32_zero, glare_f32_zero, glare_f32_zero, 0, 0, 0);
        C_MTXOrtho(proj, glare_f32_zero, screen.y, glare_f32_zero, screen.x, glare_f32_zero, glare_f32_one);
        GXSetProjection(proj, 1);
        PSMTXIdentity(view);
        GXLoadPosMtxImm(view, 0);
        GXSetCurrentMtx(0);
        GXSetCullMode(0);
        GXSetZMode(1, 7, 0);
        GXSetNumTexGens(1);
        glare_gx_set_tex_coord_gen(0, 1, 4, 0x3C);
        GXLoadTexObj(&tex, 0);
        GXSetNumTevStages(1);
        GXSetTevDirect(0);
        GXSetTevOp(0, 0);
        GXSetTevOrder(0, 0, 0, 4);
        GXSetNumChans(1);
        GXSetChanCtrl(4, 0, 0, 1, 0, 0, 2);
        GXClearVtxDesc();
        GXSetVtxDesc(9, 1);
        GXSetVtxDesc(11, 1);
        GXSetVtxDesc(13, 1);
        GXSetVtxAttrFmt(0, 9, 0, 3, 0);
        GXSetVtxAttrFmt(0, 11, 1, 5, 0);
        GXSetVtxAttrFmt(0, 13, 1, 4, 0);
        GXSetBlendMode(1, glare->blend_src, glare->blend_dst, 0);
        one = glare_f32_one;
        r = glare->color_r;
        g = glare->color_g;
        b = glare->color_b;
        GXBegin(0x80, 0, 4);
        glare_gx_position2s16(0, 0);
        glare_gx_color4u8(r, g, b, 0xFF);
        glare_gx_texcoord2f32(glare_f32_zero, glare_f32_zero);
        glare_gx_position2s16((s16)screen.x, 0);
        glare_gx_color4u8(r, g, b, 0xFF);
        glare_gx_texcoord2f32(glare_f32_one, glare_f32_zero);
        glare_gx_position2s16((s16)screen.x, (s16)screen.y);
        glare_gx_color4u8(r, g, b, 0xFF);
        glare_gx_texcoord2f32(glare_f32_one, one);
        glare_gx_position2s16(0, (s16)screen.y);
        glare_gx_color4u8(r, g, b, 0xFF);
        glare_gx_texcoord2f32(glare_f32_zero, one);
        glare_gx_end();
    }
}

/* ---------------------------------------------------------------------------------------------------
 * The FIFO writers (one store-width each; the same family the other `auto` units carry).
 * ------------------------------------------------------------------------------------------------- */

extern "C" void glare_gx_end(void) {
}

extern "C" void glare_gx_texcoord2f32(f32 x, f32 y) {
    GXWGFifo.f32 = x;
    GXWGFifo.f32 = y;
}

extern "C" void glare_gx_color4u8(u32 r, u32 g, u32 b, u32 a) {
    GXWGFifo.u8 = r;
    GXWGFifo.u8 = g;
    GXWGFifo.u8 = b;
    GXWGFifo.u8 = a;
}

extern "C" void glare_gx_position2s16(s16 x, s16 y) {
    GXWGFifo.s16 = x;
    GXWGFifo.s16 = y;
}

/* The six-argument TEV coordgen wrapper (the caller supplies args 1-4, the two selectors are fixed). */
extern "C" void glare_gx_set_tex_coord_gen(u32 a, u32 b, u32 c, u32 d) {
    GXSetTexCoordGen2(a, b, c, d, 0, 0x7D);
}

/* 0x800572F0 (0x520): builds the glare texture: copies the screen down `reduce_count` times at half size,
 * then rewrites every texel of the last copy as its thresholded, scaled luminance. */
extern "C" void glare_filter_capture(GlareFilter* glare) {
    GXTexObj tex;
    Mtx44 proj;
    Mtx view;
    _MH_VEC2 capture;
    _MH_VEC2 screen;
    u32 argb;
    s32 i;
    s32 y;
    s32 x;
    u16 dst_w;
    u16 dst_h;
    u16 src_w;
    u16 src_h;
    s16 w;
    s16 h;
    u32 lum;

    capture_size_get(&capture);
    screen_size_get(&screen);
    w = glare->width;
    h = glare->height;
    GXSetCopyFilter(0, NULL, 0, NULL);
    src_w = w * 2;
    src_h = h * 2;
    dst_w = w;
    dst_h = h;
    for (i = 0; i < glare->reduce_count; i++) {
        GXSetTexCopySrc(0, 0, src_w, src_h);
        GXSetTexCopyDst(dst_w, dst_h, 6, 1);
        DCInvalidateRange(glare->tex_buf, dst_w * dst_h * 4);
        GXCopyTex(glare->tex_buf, 0);
        GXPixModeSync();
        GXInvalidateTexAll();
        GXInitTexObj(&tex, glare->tex_buf, dst_w, dst_h, 6, 0, 0, 0);
        GXInitTexObjLOD(&tex, 1, 1, glare_f32_zero, glare_f32_zero, glare_f32_zero, 0, 0, 0);
        C_MTXOrtho(proj, glare_f32_zero, screen.y, glare_f32_zero, screen.x, glare_f32_zero, glare_f32_one);
        GXSetProjection(proj, 1);
        PSMTXIdentity(view);
        GXLoadPosMtxImm(view, 0);
        GXSetCurrentMtx(0);
        GXSetCullMode(0);
        GXSetZMode(1, 7, 0);
        GXSetNumTexGens(1);
        glare_gx_set_tex_coord_gen(0, 1, 4, 0x3C);
        GXLoadTexObj(&tex, 0);
        GXSetNumTevStages(1);
        GXSetTevDirect(0);
        GXSetTevOp(0, 0);
        GXSetTevOrder(0, 0, 0, 4);
        GXSetNumChans(1);
        GXSetChanCtrl(4, 0, 0, 1, 0, 0, 2);
        GXClearVtxDesc();
        GXSetVtxDesc(9, 1);
        GXSetVtxDesc(11, 1);
        GXSetVtxDesc(13, 1);
        GXSetVtxAttrFmt(0, 9, 0, 3, 0);
        GXSetVtxAttrFmt(0, 11, 1, 5, 0);
        GXSetVtxAttrFmt(0, 13, 1, 4, 0);
        GXSetBlendMode(1, 1, 0, 0);
        GXBegin(0x80, 0, 4);
        glare_gx_position2s16(0, 0);
        glare_gx_color4u8(0xFF, 0xFF, 0xFF, 0xFF);
        glare_gx_texcoord2f32(glare_f32_zero, glare_f32_zero);
        glare_gx_position2s16(w, 0);
        glare_gx_color4u8(0xFF, 0xFF, 0xFF, 0xFF);
        glare_gx_texcoord2f32(glare_f32_one, glare_f32_zero);
        glare_gx_position2s16(w, h);
        glare_gx_color4u8(0xFF, 0xFF, 0xFF, 0xFF);
        glare_gx_texcoord2f32(glare_f32_one, glare_f32_one);
        glare_gx_position2s16(0, h);
        glare_gx_color4u8(0xFF, 0xFF, 0xFF, 0xFF);
        glare_gx_texcoord2f32(glare_f32_zero, glare_f32_one);
        glare_gx_end();
        src_w /= 2u;
        src_h /= 2u;
        dst_w /= 2u;
        dst_h /= 2u;
        w /= 2;
        h /= 2;
    }
    GXDrawDone();
    for (y = 0; y < src_h; y++) {
        for (x = 0; x < src_w; x++) {
            GXPeekARGB(x, y, &argb);
            lum = (((argb & 0xFF0000) >> 16) * 77 + ((argb & 0xFF00) >> 8) * 150 + (argb & 0xFF) * 29) >> 8;
            if (lum < glare->dark_level) {
                lum = (u8)(lum - (u32)((f32)lum * glare->dark_scale));
            }
            if (lum > glare->bright_level) {
                lum += (u32)((f32)lum * glare->bright_scale);
            }
            if (lum > 0xFF) {
                lum = 0xFF;
            }
            lum >>= glare->shift;
            argb = lum | (lum << 8) | (lum << 16) | 0xFF000000;
            GXPokeARGB(x, y, argb);
        }
    }
    GXSetTexCopySrc(0, 0, src_w, src_h);
    GXSetTexCopyDst(src_w, src_h, 6, 0);
    DCInvalidateRange(glare->tex_buf, src_w * src_h * 4);
    GXCopyTex(glare->tex_buf, 0);
    GXInvalidateTexAll();
    GXSetCopyFilter(Rmode->aa, Rmode->sample_pattern, 1, Rmode->vfilter);
    glare->tex_width = src_w;
    glare->tex_height = src_h;
}

/* ---------------------------------------------------------------------------------------------------
 * Glare-filter setters.
 * ------------------------------------------------------------------------------------------------- */

/* `light_mode_set`: the glare blend factors, the halving count and the luminance shift. */
extern "C" void light_mode_set(u8 arg0, u8 arg1, u8 arg2) {
    GameDrawState* s = &lbl_8066ACF8;
    if (arg0 == 0) {
        s->glare.blend_dst = 1;
        s->glare.blend_src = 1;
    } else {
        s->glare.blend_src = 1;
        s->glare.blend_dst = 3;
    }
    if (arg1 == 0) {
        s->glare.reduce_count = 3;
    } else {
        s->glare.reduce_count = 2;
    }
    if (arg2 == 0) {
        s->glare.shift = 1;
        return;
    }
    s->glare.shift = 2;
}

/* `light_mode_rgb_set`: the glare colour. */
extern "C" void light_mode_rgb_set(u8 r, u8 g, u8 b) {
    lbl_8066ACF8.glare.color_r = r;
    lbl_8066ACF8.glare.color_g = g;
    lbl_8066ACF8.glare.color_b = b;
}

void GlareFilter_on(void) {
    lbl_8066ACF8.glare.enable = 1;
}

/* The two panel initialisers: clear the 0x44 B panel and seed the scale/height pair from the screen
 * height.  `filter_panel_init` takes the panel, `filter_panel_reset` uses the one inside the state block. */
extern "C" void filter_panel_init(FilterPanel* p) {
    memset(p, 0, 0x44);
    f32 half = panel_f32_half;
    p->upper_scale = half;
    p->upper_height = (s16)(Screen_w.height_f * half);
    p->lower_scale = half;
    p->lower_height = (s16)(Screen_w.height_f * half);
}

extern "C" void filter_panel_reset(void) {
    FilterPanel* p = &lbl_8066ACF8.panel;
    memset(p, 0, 0x44);
    f32 half = panel_f32_half;
    p->upper_scale = half;
    p->upper_height = (s16)(Screen_w.height_f * half);
    p->lower_scale = half;
    p->lower_height = (s16)(Screen_w.height_f * half);
}

/* 0x800579A4 (0x414): the panel layer: two eight-vertex gradient quads over the upper and (in split-screen)
 * lower half of the screen. */
extern "C" void filter_panel_draw(FilterPanel* panel) {
    Mtx44 proj;
    Mtx view;
    _MH_VEC2 screen;
    s16 width;
    s16 upper;
    s16 mid;
    u32 c0;
    u32 c1;
    u32 c2;
    u32 c3;
    u32 c4;
    u32 c5;

    if (panel->enable != 0) {
        get_ScreenSize(&screen);
        width = (s16)screen.x;
        upper = panel->upper_height;
        if (screen_split_ck() == 0) {
            mid = (s16)Screen_w.height_f;
        } else {
            mid = (s16)(Screen_w.height_f * panel_f32_half);
        }
        switch (panel->blend) {
        case 1:
            GXSetBlendMode(1, 4, 1, 0);
            break;
        case 2:
            GXSetBlendMode(3, 4, 5, 0);
            break;
        default:
            GXSetBlendMode(1, 4, 5, 0);
            break;
        }
        GXSetColorUpdate(1);
        GXSetAlphaCompare(7, 0, 1, 7, 0);
        C_MTXOrtho(proj, panel_f32_zero, screen.y, panel_f32_zero, screen.x, panel_f32_zero, panel_f32_one);
        GXSetProjection(proj, 1);
        PSMTXIdentity(view);
        GXLoadPosMtxImm(view, 0);
        GXSetCurrentMtx(0);
        GXSetCullMode(0);
        GXSetZMode(1, 7, 0);
        GXSetNumTexGens(0);
        GXSetNumTevStages(1);
        GXSetTevDirect(0);
        GXSetTevOp(0, 4);
        GXSetTevOrder(0, 0xFF, 0xFF, 4);
        GXSetNumChans(1);
        GXSetChanCtrl(4, 0, 1, 1, 0, 0, 2);
        GXClearVtxDesc();
        GXSetVtxDesc(9, 1);
        GXSetVtxDesc(11, 1);
        GXSetVtxAttrFmt(0, 9, 0, 3, 0);
        GXSetVtxAttrFmt(0, 11, 1, 5, 0);
        c0 = panel->upper_color[0];
        c1 = panel->upper_color[1];
        c2 = panel->upper_color[2];
        c3 = panel->upper_color[3];
        c4 = panel->upper_color[4];
        c5 = panel->upper_color[5];
        GXBegin(0x80, 0, 8);
        panel_gx_position2s16(0, upper);
        panel_gx_color1u32(c2);
        panel_gx_position2s16(0, 0);
        panel_gx_color1u32(c0);
        panel_gx_position2s16(width, 0);
        panel_gx_color1u32(c1);
        panel_gx_position2s16(width, upper);
        panel_gx_color1u32(c3);
        panel_gx_position2s16(0, mid);
        panel_gx_color1u32(c4);
        panel_gx_position2s16(0, upper);
        panel_gx_color1u32(c2);
        panel_gx_position2s16(width, upper);
        panel_gx_color1u32(c3);
        panel_gx_position2s16(width, mid);
        panel_gx_color1u32(c5);
        panel_gx_end();
        if (screen_split_ck() != 0) {
            s16 lower_width = (s16)screen.x;
            s16 top = (s16)(Screen_w.height_f * panel_f32_half);
            s16 lower = top + panel->lower_height;
            s16 bottom = (s16)Screen_w.height_f;
            c0 = panel->lower_color[0];
            c1 = panel->lower_color[1];
            c2 = panel->lower_color[2];
            c3 = panel->lower_color[3];
            c4 = panel->lower_color[4];
            c5 = panel->lower_color[5];
            GXBegin(0x80, 0, 8);
            panel_gx_position2s16(0, lower);
            panel_gx_color1u32(c2);
            panel_gx_position2s16(0, top);
            panel_gx_color1u32(c0);
            panel_gx_position2s16(lower_width, top);
            panel_gx_color1u32(c1);
            panel_gx_position2s16(lower_width, lower);
            panel_gx_color1u32(c3);
            panel_gx_position2s16(0, bottom);
            panel_gx_color1u32(c4);
            panel_gx_position2s16(0, lower);
            panel_gx_color1u32(c2);
            panel_gx_position2s16(lower_width, lower);
            panel_gx_color1u32(c3);
            panel_gx_position2s16(lower_width, bottom);
            panel_gx_color1u32(c5);
            panel_gx_end();
        }
    }
}

extern "C" void panel_gx_end(void) {
}

extern "C" void panel_gx_color1u32(s32 v) {
    GXWGFifo.s32 = v;
}

extern "C" void panel_gx_position2s16(s32 x, s32 y) {
    GXWGFifo.s16 = x;
    GXWGFifo.s16 = y;
}

/* 0x80057DE0 (0x114): sets the upper strip's colours from `start` (at most up to entry 5) and, for a
 * positive `scale`, its height scale. */
extern "C" void filter_panel_upper_set(s32 start, s32 count, u32* colors, f32 scale) {
    FilterPanel* p = &lbl_8066ACF8.panel;
    s32 i;
    if (!(scale <= panel_f32_zero)) {
        p->upper_scale = scale;
        if (screen_split_ck() == 0) {
            p->upper_height = (s16)(Screen_w.height_f * p->upper_scale);
        } else {
            p->upper_height = (s16)(Screen_w.height_f * panel_f32_half * p->upper_scale);
        }
    }
    if (0 <= start && start <= 5) {
        for (i = start; i < start + count; i++) {
            if (i > 5) {
                break;
            }
            p->upper_color[i] = *colors++;
        }
    }
}

/* 0x80057EF4 (0x114): the same for the lower strip. */
extern "C" void filter_panel_lower_set(s32 start, s32 count, u32* colors, f32 scale) {
    FilterPanel* p = &lbl_8066ACF8.panel;
    s32 i;
    if (!(scale <= panel_f32_zero)) {
        p->lower_scale = scale;
        if (screen_split_ck() == 0) {
            p->lower_height = (s16)(Screen_w.height_f * p->lower_scale);
        } else {
            p->lower_height = (s16)(Screen_w.height_f * panel_f32_half * p->lower_scale);
        }
    }
    if (0 <= start && start <= 5) {
        for (i = start; i < start + count; i++) {
            if (i > 5) {
                break;
            }
            p->lower_color[i] = *colors++;
        }
    }
}

/* ---------------------------------------------------------------------------------------------------
 * The fade slots.
 * ------------------------------------------------------------------------------------------------- */

/* `fn_80058008`/`filter_panel_flag_clear`: the panel enable flag. */
extern "C" void filter_panel_on(void) {
    lbl_8066ACF8.panel.enable = 1;
}

extern "C" void filter_panel_flag_clear(void) {
    lbl_8066ACF8.panel.enable = 0;
}

/* Zero all four fade slots. */
extern "C" void fade_work_clear(void) {
    memset(&fade_work, 0, 0x70);
}

/* `fade_set(slot, tableIndex)`: arm slot `slot` from the `fade_step_table` entry `tableIndex`.  The
 * coloured alpha the walk eases is the table's +0x04; the tick count is the alpha span divided by |step|,
 * rounded up. */
void fade_set(s32 slot, s32 tableIndex) {
    FadeWork* fw = &fade_work;
    FadeStep* step = &fade_step_table[tableIndex];
    fw->tbl[slot] = tableIndex;
    fw->color[slot] = step->color_0x04;
    s32 span = step->to_0x0C - step->from_0x08;
    if (span < 0) {
        span -= span * 2;
        fw->state[slot] = 2;
    } else {
        fw->state[slot] = 1;
    }
    fw->step[slot] = step->step_0x10;
    s32 abs_ticks = fw->step[slot];
    if (abs_ticks < 0) {
        abs_ticks = -abs_ticks;
    }
    fw->ticks[slot] = span / abs_ticks;
    if (span > abs_ticks * fw->ticks[slot]) {
        fw->ticks[slot] += 1;
    }
    fw->target[slot] = step->to_0x0C;
}

/* `fade_reset(slot)`: clear the six fields of one slot. */
void fade_reset(s32 slot) {
    fade_work.color[slot] = 0;
    fade_work.ticks[slot] = 0;
    fade_work.state[slot] = 0;
    fade_work.tbl[slot] = 0;
    fade_work.target[slot] = 0;
    fade_work.step[slot] = 0;
}

/* `fade_done_ck`: idle test for a slot. */
extern "C" u32 fade_done_ck(u8 slot) {
    return fade_work.state[slot] == 0;
}

/* 0x80058140 (0x11C): steps every running fade slot's alpha toward its target and ends it when its ticks
 * run out. */
extern "C" void fade_update(void) {
    FadeWork* fw;
    s32 i;
    s32 alpha;
    if (system_w.field_0x05 == 0) {
        fw = &fade_work;
        for (i = 0; i < 4; i++) {
            switch (fw->state[i]) {
            case 1:
                alpha = ((fw->color[i] & 0xFF000000) >> 24) + fw->step[i];
                if (alpha >= fw->target[i]) {
                    alpha = fw->target[i];
                    fw->ticks[i] = 0;
                }
                fw->ticks[i]--;
                if (fw->ticks[i] < 0) {
                    fw->ticks[i] = 0;
                    fw->state[i] = 3;
                }
                fw->color[i] &= 0xFFFFFF;
                fw->color[i] |= alpha << 24;
                break;
            case 2:
                alpha = ((fw->color[i] & 0xFF000000) >> 24) + fw->step[i];
                if (alpha <= fw->target[i]) {
                    alpha = fw->target[i];
                    fw->ticks[i] = 0;
                }
                fw->ticks[i]--;
                if (fw->ticks[i] < 0) {
                    fw->ticks[i] = 0;
                    fw->state[i] = 0;
                }
                fw->color[i] &= 0xFFFFFF;
                fw->color[i] |= alpha << 24;
                break;
            }
        }
    }
}

/* `fade_color_get`: the slot's packed ARGB colour. */
extern "C" u32 fade_color_get(s32 slot) {
    return fade_work.color[slot];
}

s32 get_fade_stat(s32 slot) {
    return fade_work.state[slot];
}

/* 0x80058288 (0xC0): fade slot 0 over the whole screen, then the quest marker while loading. */
extern "C" void fade_draw_screen(void) {
    Vec pos;
    Vec2 size;
    _MH_VEC2 screen;
    GXColor color;
    u32 argb;

    if (get_fade_stat(0) != 0) {
        get_ScreenSize(&screen);
        dbg_drawgraph_init();
        pos.z = fade_f32_zero;
        pos.y = fade_f32_zero;
        pos.x = fade_f32_zero;
        size.x = screen.x;
        size.y = screen.y;
        argb = fade_color_get(0);
        color.a = (argb & 0xFF000000) >> 24;
        color.r = (argb & 0xFF0000) >> 16;
        color.g = (argb & 0xFF00) >> 8;
        color.b = argb;
        disp_beta(&pos, &size, color);
    }
    if (loading_disp_flag_get() != 0) {
        quest_marker_draw_sub();
    }
}

/* 0x80058348 (0xB4): fade slot 1 over the upper half of the screen. */
void fade_draw_upper(void) {
    Vec pos;
    Vec2 size;
    _MH_VEC2 screen;
    GXColor color;
    u32 argb;

    if (get_fade_stat(1) != 0) {
        get_ScreenSize(&screen);
        dbg_drawgraph_init();
        pos.z = fade_f32_zero;
        pos.y = fade_f32_zero;
        pos.x = fade_f32_zero;
        size.x = screen.x;
        size.y = screen.y * fade_f32_half;
        argb = fade_color_get(1);
        color.a = (argb & 0xFF000000) >> 24;
        color.r = (argb & 0xFF0000) >> 16;
        color.g = (argb & 0xFF00) >> 8;
        color.b = argb;
        disp_beta(&pos, &size, color);
    }
}

/* 0x800583FC (0xB4): fade slot 2 over the lower half of the screen. */
void fade_draw_lower(void) {
    Vec pos;
    Vec2 size;
    _MH_VEC2 screen;
    GXColor color;
    u32 argb;
    f32 half;

    if (get_fade_stat(2) != 0) {
        get_ScreenSize(&screen);
        dbg_drawgraph_init();
        pos.z = fade_f32_zero;
        pos.x = fade_f32_zero;
        half = screen.y * fade_f32_half;
        pos.y = half;
        size.x = screen.x;
        size.y = half;
        argb = fade_color_get(2);
        color.a = (argb & 0xFF000000) >> 24;
        color.r = (argb & 0xFF0000) >> 16;
        color.g = (argb & 0xFF00) >> 8;
        color.b = argb;
        disp_beta(&pos, &size, color);
    }
}

/* ---------------------------------------------------------------------------------------------------
 * The filter/glare dispatch helpers.
 * ------------------------------------------------------------------------------------------------- */

/* `filter_reset`: re-run the glare teardown and the panel reset. */
void filter_reset(void) {
    fn_800569CC();
    fn_80056F04();
    filter_panel_reset();
}

/* The two `Screen_w` vector copies out of the state block. */
extern "C" void screen_size_get(_MH_VEC2* dst) {
    copyVec2(&dst->x, &lbl_8066ACF8.screen_size.x);
}

extern "C" void capture_size_get(_MH_VEC2* dst) {
    copyVec2(&dst->x, &lbl_8066ACF8.capture_size.x);
}

/* `game_draw_init`: initialise the whole draw/filter state block `lbl_8066ACF8` - the capture-buffer pair,
 * the three layers and their callbacks. */
extern "C" void game_draw_init(void) {
    GameDrawState* s = &lbl_8066ACF8;
    memset(s, 0, 0x14C);
    fn_8004030C(&s->capture_size);
    get_ScreenSize(&s->screen_size);
    s->capture_buf_size = 0x118000;
    s->capture_buf = (u8*)0x90B48000;
    s->layer_order[0] = 0;
    s->screen_tex = (u8*)0x90E30000;
    s->layer_count = 4;
    s->layer_mask = 1;
    s->layer_mask |= 2;
    s->layer_mask |= 4;
    s->layer_mask |= 8;
    s->layer_draw[1] = (DrawLayerFunc)fn_80055F58;
    s->layer_draw[2] = (DrawLayerFunc)glare_filter_draw;
    s->layer_draw[3] = (DrawLayerFunc)filter_panel_draw;
    s->layer_arg[1] = &s->sprite_0x3C;
    s->layer_arg[2] = &s->glare;
    s->layer_arg[3] = &s->panel;
    fn_80055EC4(&s->sprite_0x3C);
    fn_80056E1C(&s->glare);
    filter_panel_init(&s->panel);
    s->layer_order[1] = 1;
    s->layer_order[2] = 2;
    s->layer_order[3] = 3;
    s->field_0x148 = screen_f32_0_6;
}

/* `setFilterPrio`: store the priority (`screen_fade_draw` reads it back) and swap the glare and panel
 * layers in the draw order around it. */
void setFilterPrio(u8 prio) {
    lbl_8066ACF8.filter_prio = prio;
    if (prio == 0) {
        lbl_8066ACF8.layer_order[2] = 2;
        lbl_8066ACF8.layer_order[3] = 3;
    } else {
        lbl_8066ACF8.layer_order[3] = 2;
        lbl_8066ACF8.layer_order[2] = 3;
    }
}

/* 0x8005863C (0xCC): the per-frame update: screen size, stage light, screen fade, sprite tick, and the
 * requested copy of the screen image once the menu and the fade allow it. */
extern "C" void game_draw_calc(void) {
    GameDrawState* s = &lbl_8066ACF8;
    SysQuestMove* move;
    if (system_w.field_0x30 == 0 && system_w.field_0x05 == 0) {
        get_ScreenSize(&s->screen_size);
        stage_light_update();
        screen_fade_alpha_update(s);
        draw_sprite_tick(s);
        if (system_w.field_0x868 == 1) {
            move = (SysQuestMove*)get_move_work_adrs(0);
            if ((move == NULL || move->menu_lock_0x112 == 0) && get_fade_stat(0) != 2) {
                memcpy(s->screen_tex, s->capture_buf, 0x118000);
                system_w.field_0x868 = 2;
            }
        }
    }
}

/* 0x80058708 (0x104): copies the EFB into the capture buffer (unconditionally, or with `force` only while
 * the screen fade is idle or in phase 2). */
extern "C" void screen_capture_copy(GameDrawState* s, u8 force) {
    if (s->capture_buf != NULL && (force == 0 || s->fade_phase == 0 || s->fade_phase == 2)) {
        GXSetCopyFilter(0, NULL, 0, NULL);
        GXSetTexCopySrc(0, 0, (u16)s->capture_size.x, (u16)s->capture_size.y);
        GXSetTexCopyDst((u16)s->capture_size.x, (u16)s->capture_size.y, 6, 0);
        DCInvalidateRange(s->capture_buf, s->capture_buf_size);
        GXCopyTex(s->capture_buf, 0);
        GXPixModeSync();
        GXSetCopyFilter(Rmode->aa, Rmode->sample_pattern, 1, Rmode->vfilter);
    }
}

/* 0x8005880C (0x344): draws the capture buffer over the screen (dimmed while the pad hold is on; mode 1
 * subtracts it). */
extern "C" void screen_capture_draw(GameDrawState* s, u8 mode) {
    GXTexObj tex;
    Mtx44 proj;
    Mtx view;
    u8 shade;
    s16 w;
    s16 h;

    if (s->capture_buf != NULL) {
        shade = pad_hold_ck() == 0 ? 0xFF : 0x78;
        GXInitTexObj(&tex, s->capture_buf, (u16)s->capture_size.x, (u16)s->capture_size.y, 6, 0, 0, 0);
        GXInitTexObjLOD(&tex, 1, 1, screen_f32_zero, screen_f32_zero, screen_f32_zero, 0, 0, 0);
        C_MTXOrtho(proj, screen_f32_zero, s->screen_size.y, screen_f32_zero, s->screen_size.x, screen_f32_zero,
                   screen_f32_one);
        GXSetProjection(proj, 1);
        PSMTXIdentity(view);
        GXLoadPosMtxImm(view, 0);
        GXSetCurrentMtx(0);
        GXSetCullMode(0);
        GXSetZMode(1, 7, 0);
        GXSetNumTexGens(1);
        screen_gx_set_tex_coord_gen(0, 1, 4, 0x3C);
        GXLoadTexObj(&tex, 0);
        GXSetNumTevStages(1);
        GXSetTevDirect(0);
        GXSetTevOp(0, 0);
        GXSetTevOrder(0, 0, 0, 4);
        GXSetNumChans(1);
        GXSetChanCtrl(4, 0, 0, 1, 0, 0, 2);
        GXClearVtxDesc();
        GXSetVtxDesc(9, 1);
        GXSetVtxDesc(11, 1);
        GXSetVtxDesc(13, 1);
        GXSetVtxAttrFmt(0, 9, 0, 3, 0);
        GXSetVtxAttrFmt(0, 11, 1, 5, 0);
        GXSetVtxAttrFmt(0, 13, 1, 4, 0);
        if (mode == 1) {
            GXSetBlendMode(3, 2, 2, 0);
        } else {
            GXSetBlendMode(1, 1, 0, 0);
        }
        w = (s16)s->screen_size.x;
        h = (s16)s->screen_size.y;
        GXBegin(0x80, 0, 4);
        screen_gx_position2s16(0, 0);
        screen_gx_color4u8(shade, shade, shade, 0xFF);
        screen_gx_texcoord2f32(screen_f32_zero, screen_f32_zero);
        screen_gx_position2s16(w, 0);
        screen_gx_color4u8(shade, shade, shade, 0xFF);
        screen_gx_texcoord2f32(screen_f32_one, screen_f32_zero);
        screen_gx_position2s16(w, h);
        screen_gx_color4u8(shade, shade, shade, 0xFF);
        screen_gx_texcoord2f32(screen_f32_one, screen_f32_one);
        screen_gx_position2s16(0, h);
        screen_gx_color4u8(shade, shade, shade, 0xFF);
        screen_gx_texcoord2f32(screen_f32_zero, screen_f32_one);
        screen_gx_end();
    }
}

extern "C" void screen_gx_end(void) {
}

extern "C" void screen_gx_texcoord2f32(f32 x, f32 y) {
    GXWGFifo.f32 = x;
    GXWGFifo.f32 = y;
}

extern "C" void screen_gx_color4u8(u32 r, u32 g, u32 b, u32 a) {
    GXWGFifo.u8 = r;
    GXWGFifo.u8 = g;
    GXWGFifo.u8 = b;
    GXWGFifo.u8 = a;
}

extern "C" void screen_gx_position2s16(s32 x, s32 y) {
    GXWGFifo.s16 = x;
    GXWGFifo.s16 = y;
}

extern "C" void screen_gx_set_tex_coord_gen(u32 a, u32 b, u32 c, u32 d) {
    GXSetTexCoordGen2(a, b, c, d, 0, 0x7D);
}

/* `system_copy_filter_request`/`system_copy_filter_arm`/`system_copy_filter_clear`: the copy-filter handshake with the system block. */
extern "C" void system_copy_filter_request(void) {
    if (system_w.field_0x868 != 1) {
        system_w.field_0x868 = 1;
    }
}

/* 0x80058BD0 (0x318): draws the held screen image over the screen (dimmed when `dim` is set). */
void screen_tex_draw(u8 dim) {
    GXTexObj tex;
    Mtx44 proj;
    Mtx view;
    u8 shade;
    s16 w;
    s16 h;
    GameDrawState* s = &lbl_8066ACF8;

    if (s->screen_tex != NULL) {
        shade = dim != 0 ? 0xC8 : 0xFF;
        GXInitTexObj(&tex, s->screen_tex, (u16)s->capture_size.x,
                     (u16)s->capture_size.y, 6, 0, 0, 0);
        GXInitTexObjLOD(&tex, 1, 1, screen_f32_zero, screen_f32_zero, screen_f32_zero, 0, 0, 0);
        C_MTXOrtho(proj, screen_f32_zero, s->screen_size.y, screen_f32_zero, s->screen_size.x,
                   screen_f32_zero, screen_f32_one);
        GXSetProjection(proj, 1);
        PSMTXIdentity(view);
        GXLoadPosMtxImm(view, 0);
        GXSetCurrentMtx(0);
        GXSetCullMode(0);
        GXSetZMode(1, 7, 0);
        GXSetNumTexGens(1);
        screen_gx_set_tex_coord_gen(0, 1, 4, 0x3C);
        GXLoadTexObj(&tex, 0);
        GXSetNumTevStages(1);
        GXSetTevDirect(0);
        GXSetTevOp(0, 0);
        GXSetTevOrder(0, 0, 0, 4);
        GXSetNumChans(1);
        GXSetChanCtrl(4, 0, 0, 1, 0, 0, 2);
        GXClearVtxDesc();
        GXSetVtxDesc(9, 1);
        GXSetVtxDesc(11, 1);
        GXSetVtxDesc(13, 1);
        GXSetVtxAttrFmt(0, 9, 0, 3, 0);
        GXSetVtxAttrFmt(0, 11, 1, 5, 0);
        GXSetVtxAttrFmt(0, 13, 1, 4, 0);
        GXSetBlendMode(1, 1, 0, 0);
        w = (s16)s->screen_size.x;
        h = (s16)s->screen_size.y;
        GXBegin(0x80, 0, 4);
        screen_gx_position2s16(0, 0);
        screen_gx_color4u8(shade, shade, shade, 0xFF);
        screen_gx_texcoord2f32(screen_f32_zero, screen_f32_zero);
        screen_gx_position2s16(w, 0);
        screen_gx_color4u8(shade, shade, shade, 0xFF);
        screen_gx_texcoord2f32(screen_f32_one, screen_f32_zero);
        screen_gx_position2s16(w, h);
        screen_gx_color4u8(shade, shade, shade, 0xFF);
        screen_gx_texcoord2f32(screen_f32_one, screen_f32_one);
        screen_gx_position2s16(0, h);
        screen_gx_color4u8(shade, shade, shade, 0xFF);
        screen_gx_texcoord2f32(screen_f32_zero, screen_f32_one);
        screen_gx_end();
    }
}

extern "C" void system_copy_filter_arm(void) {
    if (system_w.field_0x868 == 2) {
        system_w.pad_0x869[0] = 1;
    }
}

extern "C" void system_copy_filter_clear(void) {
    system_w.field_0x868 = 0;
    system_w.pad_0x869[0] = 0;
}

/* 0x80058F20 (0xDC): copies the EFB into the held screen image. */
extern "C" void screen_copy_tex_capture(void) {
    GXSetCopyFilter(0, NULL, 0, NULL);
    GXSetTexCopySrc(0, 0, (u16)lbl_8066ACF8.capture_size.x, (u16)lbl_8066ACF8.capture_size.y);
    GXSetTexCopyDst((u16)lbl_8066ACF8.capture_size.x, (u16)lbl_8066ACF8.capture_size.y, 6, 0);
    DCInvalidateRange(lbl_8066ACF8.screen_tex, lbl_8066ACF8.capture_buf_size);
    GXCopyTex(lbl_8066ACF8.screen_tex, 0);
    GXPixModeSync();
    GXSetCopyFilter(Rmode->aa, Rmode->sample_pattern, 1, Rmode->vfilter);
}

/* 0x80058FFC (0x140): the screen fade: a flat fade-colour rectangle, cross-faded with the capture buffer in
 * phase 2. */
extern "C" void screen_fade_draw(GameDrawState* s) {
    Vec pos;
    Vec2 size;
    GXColor color;

    switch (s->fade_phase) {
    case 0:
        if (s->filter_prio == 0) {
            screen_capture_copy(s, 0);
        }
        break;
    case 2:
        color_rgba_copy(&color.r, &s->fade_color.r);
        screen_capture_copy(s, 1);
        pos.z = screen_f32_zero;
        pos.y = screen_f32_zero;
        pos.x = screen_f32_zero;
        size.x = s->screen_size.x;
        size.y = s->screen_size.y;
        dbg_drawgraph_init();
        color.a = 0xFF;
        disp_beta(&pos, &size, color);
        screen_capture_draw(s, 1);
        color.a = 0xFF - s->fade_color.a;
        disp_beta(&pos, &size, color);
        screen_capture_copy(s, 0);
        break;
    default:
        pos.z = screen_f32_zero;
        pos.y = screen_f32_zero;
        pos.x = screen_f32_zero;
        size.x = s->screen_size.x;
        size.y = s->screen_size.y;
        dbg_drawgraph_init();
        disp_beta(&pos, &size, s->fade_color);
        screen_capture_copy(s, 0);
        break;
    }
}

/* ---------------------------------------------------------------------------------------------------
 * The eased screen-fade alpha.
 * ------------------------------------------------------------------------------------------------- */

/* `screen_fade_alpha_update`: advance the eased alpha for an active fade, or finish it when the value is spent. */
extern "C" void screen_fade_alpha_update(GameDrawState* s) {
    if (s->fade_phase >= 2) {
        if (s->fade_value <= screen_f32_zero) {
            s->fade_phase = 0;
        } else {
            s->fade_color.a = (u8)(screen_f32_255 * math_sin_deg(s->fade_value));
        }
    }
}

/* `math_sin_deg`: the sine ease, `SinFIdx(0.7111111f * x)` (the owner namespace, rule 9). */
extern "C" f32 math_sin_deg(f32 x) {
    return nw4r::math::SinFIdx(screen_f32_deg_to_idx * x);
}

/* `draw_sprite_tick`: tick the sprite slot at +0x3C while its owner says it is not finished. */
extern "C" void draw_sprite_tick(GameDrawState* s) {
    DrawSpriteSlot* slot = &s->sprite_0x3C;
    if (fn_80056AF0(slot) != 1) {
        s32 remaining = slot->count;
        if (remaining <= 0) {
            fn_80056A3C(remaining);
        } else {
            slot->count = remaining - 1;
        }
    }
}

/* 0x8005920C (0x168): draws the enabled layers in their order (only layer 2 while the pad hold shows the
 * capture), after the screen fade, then resets the g3d state. */
void layout_overlay_draw(void) {
    GameDrawState* s = &lbl_8066ACF8;
    s32 i;
    u8 layer;
    DrawLayerFunc draw;

    if (GameMode_ck() == 2 && lobby_w.field_0x12C == 1) {
        return;
    }
    dbg_drawgraph_init();
    if (pad_hold_ck() == 1) {
        screen_capture_draw(s, 0);
        for (i = 1; i < 6; i++) {
            layer = s->layer_order[i];
            if (layer == 2 && (s->layer_mask & (1 << layer)) != 0) {
                draw = s->layer_draw[layer];
                if (draw != NULL) {
                    draw(s->layer_arg[layer]);
                }
            }
        }
    } else if (system_w.pad_0x869[0] == 0 || system_w.field_0x868 != 2) {
        screen_fade_draw(s);
        for (i = 1; i < 6; i++) {
            layer = s->layer_order[i];
            if (layer != 0 && (s->layer_mask & (1 << layer)) != 0) {
                draw = s->layer_draw[layer];
                if (draw != NULL) {
                    draw(s->layer_arg[layer]);
                }
            }
        }
    }
    g3d_state_invalidate(0x7FF);
    GXSetNumIndStages(0);
}

/* `screen_fade_start`: start (arg 0..2) or restart the screen fade.  A fade that is already easing (phase 1
 * or 2), or a finished phase-3 fade still above its floor, is left alone. */
extern "C" void screen_fade_start(s32 arg0) {
    GameDrawState* s = &lbl_8066ACF8;
    if (arg0 == 0) {
        if ((u8)(s->fade_phase + 0xFF) <= 1) {
            return;
        }
        if (s->fade_phase == 3 && s->fade_value >= screen_fade_f32_floor) {
            return;
        }
    }
    switch (arg0) {
    case 0:
        s->fade_phase = 1;
        s->fade_value = screen_fade_f32_start;
        break;
    case 1:
        s->fade_phase = 2;
        s->fade_value = screen_fade_f32_start;
        break;
    case 2:
        s->fade_phase = 3;
        s->fade_value = screen_fade_f32_start_long;
        break;
    }
    s->fade_color.a = 0xFF;
    s->fade_color.b = 0xFF;
    s->fade_color.g = 0xFF;
    s->fade_color.r = 0xFF;
}

/* `screen_fade_ease`: ease the fade value down by the phase's step until it reaches the floor. */
extern "C" void screen_fade_ease(void) {
    GameDrawState* s = &lbl_8066ACF8;
    switch (s->fade_phase) {
    case 1:
        if (s->fade_value > screen_f32_zero) {
            s->fade_value -= screen_fade_f32_step1;
        }
        break;
    case 2:
        if (s->fade_value > screen_f32_zero) {
            s->fade_value -= screen_fade_f32_step2;
        }
        break;
    case 3:
        if (s->fade_value > screen_f32_zero) {
            s->fade_value -= screen_fade_f32_step3;
        }
        break;
    default:
        if (s->fade_value > screen_f32_zero) {
            s->fade_value -= screen_f32_one;
        }
        break;
    }
}

extern "C" void filter_flag_clear(void) {
    lbl_8066ACF8.fade_phase = 0;
}

/* The sub-object chain `game_draw_sinit` walks: the global state's +0x3C block, whose +0x28 node is the
 * `VEC3_ctor` target.  Only the node's base is used. size: 0x3C */
typedef struct CtorBlock {
    /* +0x00 */ u8 pad_0x00[0x28];
    /* +0x28 */ nw4r::math::VEC3 node_0x28;
    /* +0x34 */ u8 pad_0x34[0x08];
} CtorBlock; /* size: 0x3C */

extern "C" void* game_draw_state_ctor(GameDrawState* s);
extern "C" void* draw_sprite_slot_ctor(CtorBlock* self);

/* The constructor chain `game_draw_sinit` runs over the global state (retail emits the sinit first, then
 * the two constructors): the +0x3C sub-object's own +0x28 node is handed to `VEC3_ctor`. */
extern "C" void game_draw_sinit(void) {
    game_draw_state_ctor(&lbl_8066ACF8);
}

extern "C" void* game_draw_state_ctor(GameDrawState* s) {
    draw_sprite_slot_ctor((CtorBlock*)&s->sprite_0x3C);
    return s;
}

extern "C" void* draw_sprite_slot_ctor(CtorBlock* self) {
    VEC3_ctor(&self->node_0x28);
    return self;
}
