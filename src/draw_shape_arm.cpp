/*
 * `draw_shape_arm.cpp` - the screen sprite (a half-size EFB copy drawn back over the screen, flat or as a zoom toward a
 * tracked point) and the draw state setters around it.
 *
 * RANGE. `.text` 0x80055EC4..0x80056F24 (18 functions, address order below), `extab` 0x800071E0..0x80007218,
 *   `extabindex` 0x8001F440..0x8001F494, `.sdata` 0x80791100..0x80791110 (the two phase tables), `.sbss`
 *   0x807948C0..0x807948C8 (the TEV scale word), `.sdata2` 0x80795C18..0x80795C48 (the float pool).
 * NAMES. Class 3: `draw_shape_arm` and `draw_shape_state_clear` are the map's; the rest follow the sibling band's scheme
 *   (`fn_80056F24.cpp`'s `draw_sprite_tick`, `<glare|panel|screen>_gx_<SDK inline>`).  GUESS: draw_sprite_init,
 *   draw_sprite_draw, draw_sprite_blend_draw, draw_sprite_set, draw_sprite_reset, draw_sprite_start, draw_sprite_stop,
 *   draw_shape_arm_pos, draw_sprite_track, glare_filter_init, glare_filter_off, draw_sprite_phase_step,
 *   draw_sprite_phase_time, draw_sprite_tev_scale, the `sprite_gx_*` FIFO writers, `DrawSpriteSlot` and its fields; the callee
 *   `screen_size_float_get` (`main.cpp`'s 0x8004030C, proposed in the name registry).
 * FLAGS. `cflags_main`; `#pragma peephole off` for the whole TU (the FIFO writers keep `extsh`/`clrlwi`, as in
 *   `fn_80056F24.cpp`).
 * RESIDUALS. `draw_sprite_track`: retail places the shared "end the zoom" block right after the first check and branches
 *   back to it from the area test and phase 2 (a `goto` shape: 99.31 %); rule 8 leaves the `do { } while (0)` +
 *   `continue` form (93.04 %), whose exit block lands after the switch.  The other 0.7 %: retail loads `size.x` before
 *   the 0.5f constant (operand orders and temporaries measured the same).
 * RESIDUALS. `.sdata2`: retail pads 0.5f to 8 bytes before 0.05f/0.01f (target 0x30, ours 0x2C); a const array or
 *   struct for the pair gives the padding but costs `glare_filter_init` its `lfs sym+4` (97.24 %), so the literals stay.
 *   The pool rows are compiler `@NN` names in our object (relocation names only).
 * RESIDUALS. `.sbss`: `draw_sprite_tev_scale` is one word (retail claim 8 B; only +0 is read).
 * RESIDUALS. `lbl_8066ACF8` is read through `DrawShapeState`, a view of `fn_80056F24.cpp`'s private `GameDrawState`
 *   (and `GlareFilterView` of its `GlareFilter`); one shared header needs that unit's types moved out of its `.cpp`.
 * SHAPES. `draw_sprite_draw`: the texcoord locals declared v1, u0, u1, v0 (register order), the mode test a `switch`
 *   (`cmpwi`), `t = level; t *= k` (operand order); `Mtx44` before `Mtx` (frame order).  `draw_sprite_track` is
 *   compiled with `fp_contract off` (retail keeps `fmuls`+`fsubs`).
 */

#include "types.h"
#include "gx.h"
#include "nw4r/math.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "mh3_pad.h"                 /* VEC3_ctor / copyVec3 (rule 2) */
#include "main.h"                    /* _MH_VEC2 (rule 1) */
#include "screen_size_float_get.h"
#include "MTX/mtx.h"
#include "MTX/mtx44.h"
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
#include "color_rgba_copy.h"
#include "light_mode_set.h"
#include "mh3_pad/Screen_w.h"
#include "ef/pRoot.h"
#include "g3d/scn_root_get_current_camera.h"
#include "g3d/camera_world_to_screen.h"
#include "stage/stg_w.h"             /* get_now_areano (rule 2) */
#include "enemy/ENEMY_WORK.h"
#include "enemy/em_common.h"         /* get_joint_wpos_em (rule 2) */
#include "draw_shape_arm.h"
#include "fn_80056F24/lbl_8066ACF8.h" /* lbl_8066ACF8, owned by fn_80056F24.cpp (rule 2) */

#pragma peephole off

/* The per-phase `level` step and length `draw_sprite_track` loads when it enters the rise and fall phases. */
u8 draw_sprite_phase_step[2] = { 7, 4 };
s32 draw_sprite_phase_time[2] = { 10, 20 };

/* The TEV output scale of the tinted sprite stage (never written in this unit). */
u32 draw_sprite_tev_scale;

/* ---------------------------------------------------------------------------------------------------
 * The screen sprite.
 * ------------------------------------------------------------------------------------------------- */

/* 0x80055EC4 (0x94): clears the sprite and its fixed 0x23000-byte capture buffer. */
extern "C" void draw_sprite_init(DrawSpriteSlot* s) {
    memset(s, 0, 0x38);
    s->tex_buf = (u8*)0x90C60000;
    memset(s->tex_buf, 0, 0x23000);
    DCInvalidateRange(s->tex_buf, 0x23000);
    s->mode = 0;
    s->alpha = 0x80;
    s->level = 0x20;
    s->color.a = 0;
    s->color.b = 0;
    s->color.g = 0;
    s->color.r = 0;
    s->center_x = 0.0f;
    s->center_y = 0.0f;
}

/* 0x80055F58 (0x678): draws the previous frame's copy over the screen, takes this frame's copy and blends it back. */
extern "C" void draw_sprite_draw(DrawSpriteSlot* s) {
    GXColor color;
    GXTexObj tex;
    Mtx44 proj;
    Mtx view;
    GXRenderModeObj* rmode = Rmode;

    if (s->enable == 0) {
        return;
    }
    s->count++;
    color.r = s->color.r;
    color.g = s->color.g;
    color.b = s->color.b;
    color.a = s->alpha;
    GXSetTevColor(1, color);
    GXInitTexObj(&tex, s->tex_buf, (u32)(s16)Screen_w.width >> 1, (u32)(s16)Screen_w.height >> 1, 4, 0, 0, 0);
    GXSetColorUpdate(1);
    GXSetCullMode(0);
    GXSetZMode(0, 7, 0);
    C_MTXOrtho(proj, 0.0f, Screen_w.height_f, 0.0f, (s16)Screen_w.width, 0.0f, -1.0f);
    GXSetProjection(proj, 1);
    PSMTXIdentity(view);
    GXLoadPosMtxImm(view, 0);
    GXSetCurrentMtx(0);
    if (s->count != 1) {
        switch (s->mode) {
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
        GXSetNumTevStages(1);
        GXSetNumChans(0);
        GXSetNumTexGens(1);
        sprite_gx_set_tex_coord_gen(0, 1, 4, 0x3C);
        GXLoadTexObj(&tex, 0);
        GXSetTevDirect(0);
        if (s->mode != 1) {
            GXSetTevOrder(0, 0, 0, 0xFF);
            GXSetTevColorIn(0, 15, 15, 15, 8);
            GXSetTevColorOp(0, 0, 0, 0, 1, 0);
            GXSetTevAlphaIn(0, 7, 7, 7, 1);
            GXSetTevAlphaOp(0, 0, 0, 0, 1, 0);
        } else {
            color.r = s->level;
            color.g = s->level;
            color.b = s->level;
            color.a = s->alpha;
            GXSetTevColor(1, color);
            GXSetTevOrder(0, 0, 0, 0xFF);
            GXSetTevColorIn(0, 2, 15, 15, 8);
            GXSetTevColorOp(0, 1, 0, draw_sprite_tev_scale, 1, 0);
            GXSetTevAlphaIn(0, 7, 7, 7, 1);
            GXSetTevAlphaOp(0, 0, 0, 0, 1, 0);
        }
        GXClearVtxDesc();
        GXSetVtxDesc(9, 1);
        GXSetVtxDesc(13, 1);
        GXSetVtxAttrFmt(0, 9, 0, 3, 0);
        GXSetVtxAttrFmt(0, 13, 1, 4, 0);
        if (s->alpha != 0) {
            GXBegin(0x80, 0, 4);
            switch (s->mode) {
            case 3: {
                f32 t;
                f32 v1;
                f32 u0;
                f32 u1;
                f32 v0;
                f32 dx;
                f32 dy;
                f32 left;
                f32 right;
                f32 bottom;

                t = s->level;
                t *= 0.0009765625f;
                u0 = t;
                v0 = t;
                u1 = 1.0f - t;
                v1 = 1.0f - t;
                dx = s->center_x * t;
                u0 += dx;
                u1 += dx;
                dy = s->center_y * t;
                v0 += dy;
                v1 += dy;
                left = 0.0f;
                right = left + (s16)Screen_w.width;
                bottom = left + Screen_w.height_f;
                sprite_gx_position2s16(left, left);
                sprite_gx_texcoord2f32(u0, v0);
                sprite_gx_position2s16(right, left);
                sprite_gx_texcoord2f32(u1, v0);
                sprite_gx_position2s16(right, bottom);
                sprite_gx_texcoord2f32(u1, v1);
                sprite_gx_position2s16(left, bottom);
                sprite_gx_texcoord2f32(u0, v1);
                break;
            }
            default:
                sprite_gx_position2s16(0, 0);
                sprite_gx_texcoord2f32(0.0f, 0.0f);
                sprite_gx_position2s16(Screen_w.width, 0);
                sprite_gx_texcoord2f32(1.0f, 0.0f);
                sprite_gx_position2s16(Screen_w.width, Screen_w.height_f);
                sprite_gx_texcoord2f32(1.0f, 1.0f);
                sprite_gx_position2s16(0, Screen_w.height_f);
                sprite_gx_texcoord2f32(0.0f, 1.0f);
                break;
            }
            sprite_gx_end();
        }
    }
    GXSetCopyFilter(0, 0, 0, 0);
    GXSetTexCopySrc(0, 0, Screen_w.width, Screen_w.height);
    GXSetTexCopyDst((u32)(s16)Screen_w.width >> 1, (u32)(s16)Screen_w.height >> 1, 4, 1);
    GXCopyTex(s->tex_buf, 0);
    GXSetCopyFilter(rmode->aa, rmode->sample_pattern, 1, rmode->vfilter);
    draw_sprite_blend_draw(s);
}

/* 0x800565D0 (0x4): the empty `GXEnd`. */
extern "C" void sprite_gx_end(void) {
}

/* 0x800565D4 (0x10): writes one texture coordinate pair to the FIFO. */
extern "C" void sprite_gx_texcoord2f32(f32 s, f32 t) {
    GXWGFifo.f32 = s;
    GXWGFifo.f32 = t;
}

/* 0x800565E4 (0x18): writes one signed 16-bit position pair to the FIFO. */
extern "C" void sprite_gx_position2s16(s16 x, s16 y) {
    GXWGFifo.s16 = x;
    GXWGFifo.s16 = y;
}

/* 0x800565FC (0xC): sets texture coordinate generation with no normalisation and the identity post-matrix. */
extern "C" void sprite_gx_set_tex_coord_gen(u32 dst, u32 func, u32 src, u32 mtx) {
    GXSetTexCoordGen2(dst, func, src, mtx, 0, 0x7D);
}

/* 0x80056608 (0x34C): blends the captured copy over the screen with the TEV scale `blend` selects. */
extern "C" void draw_sprite_blend_draw(DrawSpriteSlot* s) {
    GXColor color;
    GXTexObj tex;
    Mtx44 proj;
    Mtx view;

    GXPixModeSync();
    GXInvalidateTexAll();
    GXInitTexObj(&tex, s->tex_buf, (u32)(s16)Screen_w.width >> 1, (u32)(s16)Screen_w.height >> 1, 4, 0, 0, 0);
    GXLoadTexObj(&tex, 0);
    if (s->blend == 0) {
        return;
    }
    color.r = color.g = color.b = s->blend_level;
    color.a = s->blend_alpha;
    GXSetTevColor(1, color);
    GXSetColorUpdate(1);
    GXSetAlphaUpdate(0);
    GXSetCullMode(0);
    GXSetZMode(0, 7, 0);
    C_MTXOrtho(proj, 0.0f, Screen_w.height_f, 0.0f, (s16)Screen_w.width, 0.0f, -1.0f);
    GXSetProjection(proj, 1);
    PSMTXIdentity(view);
    GXLoadPosMtxImm(view, 0);
    GXSetCurrentMtx(0);
    GXSetBlendMode(1, 4, 1, 0);
    GXSetNumTevStages(1);
    GXSetNumChans(1);
    GXSetNumTexGens(1);
    sprite_gx_set_tex_coord_gen(0, 1, 4, 0x3C);
    GXSetTevOrder(0, 0, 0, 0xFF);
    GXSetTevColorIn(0, 15, 2, 12, 8);
    if (s->blend == 1) {
        GXSetTevColorOp(0, 1, 0, 0, 1, 0);
    } else if (s->blend == 2) {
        GXSetTevColorOp(0, 1, 0, 1, 1, 0);
    } else if (s->blend == 3) {
        GXSetTevColorOp(0, 1, 0, 2, 1, 0);
    }
    GXSetTevAlphaIn(0, 7, 7, 7, 1);
    GXSetTevAlphaOp(0, 0, 0, 0, 1, 0);
    GXClearVtxDesc();
    GXSetVtxDesc(9, 1);
    GXSetVtxDesc(13, 1);
    GXSetVtxAttrFmt(0, 9, 0, 2, 0);
    GXSetVtxAttrFmt(0, 13, 1, 4, 0);
    GXBegin(0x80, 0, 4);
    sprite_gx_position2u16(0, 0);
    sprite_gx_texcoord2f32(0.0f, 0.0f);
    sprite_gx_position2u16(Screen_w.width, 0);
    sprite_gx_texcoord2f32(1.0f, 0.0f);
    sprite_gx_position2u16(Screen_w.width, Screen_w.height_f);
    sprite_gx_texcoord2f32(1.0f, 1.0f);
    sprite_gx_position2u16(0, Screen_w.height_f);
    sprite_gx_texcoord2f32(0.0f, 1.0f);
    sprite_gx_end();
}

/* 0x80056954 (0x18): writes one unsigned 16-bit position pair to the FIFO. */
extern "C" void sprite_gx_position2u16(u16 x, u16 y) {
    GXWGFifo.u16 = x;
    GXWGFifo.u16 = y;
}

/* ---------------------------------------------------------------------------------------------------
 * The sprite setters (`lbl_8066ACF8.sprite`).
 * ------------------------------------------------------------------------------------------------- */

/* 0x8005696C (0x60): sets the sprite's blend, zoom centre, tint and timer. */
extern "C" void draw_sprite_set(u8 alpha, u8 mode, u32 level, f32 center_x, f32 center_y, const GXColor* color,
                                s32 timer) {
    DrawSpriteSlot* s = &lbl_8066ACF8.sprite;

    s->alpha = alpha;
    s->mode = mode;
    s->level = level;
    s->center_x = center_x;
    s->center_y = center_y;
    color_rgba_copy((u8*)&s->color, (const u8*)color);
    s->timer = timer;
}

/* 0x800569CC (0x54): returns the sprite to its idle settings. */
extern "C" void draw_sprite_reset(void) {
    DrawSpriteSlot* s = &lbl_8066ACF8.sprite;

    s->enable = 0;
    s->mode = 0;
    s->alpha = 0x80;
    s->level = 0x20;
    s->center_x = 0.0f;
    s->center_y = 0.0f;
    s->color.a = 0;
    s->color.b = 0;
    s->color.g = 0;
    s->color.r = 0;
    s->timer = 0;
    s->enemy = 0;
    s->active = 0;
    s->phase = 0;
}

/* 0x80056A20 (0x1C): turns the sprite on and restarts its frame count. */
extern "C" void draw_sprite_start(void) {
    DrawShapeState* s = &lbl_8066ACF8;

    s->sprite.enable = 1;
    s->sprite.count = 0;
}

/* 0x80056A3C (0x18): turns the sprite off. */
extern "C" void draw_sprite_stop(void) {
    DrawShapeState* s = &lbl_8066ACF8;

    s->sprite.enable = 0;
    s->sprite.count = 0;
}

/* 0x80056A54 (0x30): starts the zoom toward `enemy`'s joint `joint`. */
extern "C" void draw_shape_arm(struct _ENEMY_WORK* enemy, u32 joint, s32 timer) {
    DrawSpriteSlot* s = &lbl_8066ACF8.sprite;

    s->active = 1;
    s->phase = 0;
    s->enemy = enemy;
    s->timer = timer;
    s->joint = joint;
    s->use_pos = 0;
}

/* 0x80056A84 (0x6C): starts the zoom toward the world position `pos` in stage area `area`. */
extern "C" void draw_shape_arm_pos(const nw4r::math::VEC3* pos, s32 timer, u8 area) {
    DrawSpriteSlot* s = &lbl_8066ACF8.sprite;

    s->active = 1;
    s->phase = 0;
    s->enemy = 0;
    s->timer = timer;
    s->joint = 0;
    s->use_pos = 1;
    copyVec3(&s->pos, pos);
    s->area = area;
}

#pragma fp_contract off

/* 0x80056AF0 (0x310): projects the tracked point to the screen and steps the zoom; returns 0 once the zoom has ended. */
extern "C" s32 draw_sprite_track(DrawSpriteSlot* s) {
    GXColor color;
    u32 camera;
    _MH_VEC2 size;
    nw4r::math::VEC3 screen;
    nw4r::math::VEC3 world;
    struct _ENEMY_WORK* enemy = s->enemy;

    VEC3_ctor(&screen);
    VEC3_ctor(&world);
    if (s->active == 0) {
        return 0;
    }
    /* Every `continue` ends the zoom (the loop runs once). */
    do {
        if (s->use_pos == 0 && enemy == 0) {
            continue;
        }
        if (s->use_pos == 0) {
            get_joint_wpos_em(enemy, s->joint, &world);
        } else {
            copyVec3(&world, &s->pos);
        }
        screen_size_float_get(&size);
        camera = scn_root_get_current_camera(pRoot);
        camera_world_to_screen(&camera, &screen, &world);
        if (s->use_pos == 0) {
            if (enemy->area_no != get_now_areano()) {
                continue;
            }
        } else if (s->area != get_now_areano()) {
            continue;
        }
        screen.x -= size.x * 0.5f;
        screen.y -= size.y * 0.5f;
        screen.x /= size.x;
        screen.y /= size.y;
        if (screen.z <= 0.0f || screen.z >= 1.0f) {
            screen.x *= -1.0f;
            screen.y *= -1.0f;
        }
        if (screen.x < -1.0f) {
            screen.x = -1.0f;
        }
        if (screen.y < -1.0f) {
            screen.y = -1.0f;
        }
        if (screen.x > 1.0f) {
            screen.x = 1.0f;
        }
        if (screen.y > 1.0f) {
            screen.y = 1.0f;
        }
        s->center_x = screen.x;
        s->center_y = screen.y;
        switch (s->phase) {
        case 0:
            s->phase = 1;
            draw_sprite_start();
            color.r = 0x80;
            color.g = 0x80;
            color.b = 0x80;
            s->alpha = 0x80;
            s->mode = 3;
            color_rgba_copy((u8*)&s->color, (const u8*)&color);
            s->level = 0x10;
            s->timer = draw_sprite_phase_time[0];
            s->step = draw_sprite_phase_step[0];
            if (s->step < 1) {
                s->step = 1;
            }
            if (s->step > 120) {
                s->step = 120;
            }
            break;
        case 1:
            if (--s->timer <= 0) {
                s->phase = 2;
                s->timer = draw_sprite_phase_time[1];
                s->step = draw_sprite_phase_step[1];
            } else {
                s->level += s->step;
                if (s->level >= 80) {
                    s->level = 80;
                }
            }
            break;
        case 2:
            if (--s->timer <= 0 || s->level == 0) {
                continue;
            }
            if (s->level < s->step) {
                s->level = 0;
            } else {
                s->level -= s->step;
            }
            break;
        }
        return 1;
    } while (0);
    s->enemy = 0;
    s->active = 0;
    s->phase = 0;
    return 0;
}

#pragma fp_contract on

/* 0x80056E00 (0x1C): cancels the zoom and turns the sprite off. */
extern "C" void draw_shape_state_clear(void) {
    DrawSpriteSlot* s = &lbl_8066ACF8.sprite;

    s->enemy = 0;
    s->active = 0;
    s->phase = 0;
    draw_sprite_stop();
}

/* ---------------------------------------------------------------------------------------------------
 * The glare filter (`lbl_8066ACF8.glare`).
 * ------------------------------------------------------------------------------------------------- */

/* 0x80056E1C (0xE8): clears the glare filter and sets its fixed 0x46000-byte texture and default levels. */
extern "C" void glare_filter_init(GlareFilterView* g) {
    memset(g, 0, 0x3C);
    g->width = (s16)Screen_w.width / 2;
    g->height = (s16)Screen_w.height / 2;
    g->tex_size = 0x46000;
    g->tex_buf = (u8*)0x90C83000;
    memset(g->tex_buf, 0, g->tex_size);
    DCInvalidateRange(g->tex_buf, g->tex_size);
    g->color_r = 0xFF;
    g->color_g = 0xFF;
    g->color_b = 0xFF;
    light_mode_set(0, 0, 1);
    g->dark_level = 0x40;
    g->bright_level = 0xC0;
    g->dark_scale = 0.05f;
    g->bright_scale = 0.01f;
    g->capture_buf = lbl_8066ACF8.capture_buf;
}

/* 0x80056F04 (0x20): turns the glare filter off. */
extern "C" void glare_filter_off(void) {
    lbl_8066ACF8.glare.enable = 0;
    light_mode_set(0, 0, 1);
}
