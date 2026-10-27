/*
 * The character face render unit: it loads the male/female face, skin and `facemake` TPL files, binds them as GX
 * texture objects in `face_work`, keeps a per-slot face/eye/makeup state there and renders each slot's face into its
 * two texture images with an orthographic GX pass (`face_draw`), plus the scene-camera accessors and the face-sprite
 * buffers `userdata_item.cpp` fills.
 *
 * RANGE. `.text` 0x80047398..0x80048964 (30 functions, address order below); `.data` 0x80581158..0x80581180 (the two
 *   face-number runs); `.sdata` 0x80790EE8..0x80790EF0 (`face_no_tbl`); `.sdata2` 0x80795B70..0x80795BA8 (the float
 *   pool); `.bss` 0x806694E8..0x806699B8 (`face_work`); `.sbss` 0x80794870..0x80794878 (`face_buf_tbl`).
 * FLAGS. The `main` lib's `cflags_main` (`-Cpp_exceptions on`: the target carries one extab record per framed function),
 *   plus a file-wide `#pragma peephole off`: retail keeps every `clrlwi` before a narrow store or index (the GX writers,
 *   the accessors; turning the pass on fuses them away).
 * SHAPES. `FaceWork* work = &face_work` at function entry where retail hoists the base before the first branch; the
 *   loaders index `tex[ctx]` (a direct `tex[N]` is CSE'd with the NULL check, retail reloads it); `face_work_init` sets
 *   the two face-file buffers in a 2-trip loop (written out, the constants fold and the function grows 20 bytes);
 *   `face_draw`'s quad UVs come from float locals (`base`/`span`), not literals, so the `fadds` survive.
 * NAMES. No `__FILE__` string and no runtime-dump name reach the range, so the file keeps the map's stem.  GUESS (from
 *   each body and its callers): every `face_*` row, `face_buf_size_get`, `face_no_tbl`/`face_no_m`/`face_no_f`,
 *   `faceGX*` (the SDK's `GXPosition2s16`/`GXColor4u8`/`GXTexCoord2f32`/`GXEnd`/`GXSetTexCoordGen` emitted out of
 *   line, named in the `dbgGX*`/`fntGX*` scheme), `m_face_file_tbl`/`f_face_file_tbl`/`skin_file_tbl` (draw_shape's).
 * RESIDUALS. Register allocation only: `face_draw` (f28/f29/f27 hold 1.0/0.25/0.125 in retail, ours 1.0 in f29;
 *   the makeup block re-derives `work + i` through r4 where retail reads r27); `face_tex_bind` (retail work=r29,
 *   sex=r27, i=r31; ours work=r31, sex=r28); `face_buf_reset`, 8 bytes short (retail hoists the unrolled loop's trip
 *   count `li r28,2` out of the outer loop and saves r28, ours rematerialises `li r0,2` per pass).
 */

#include "types.h"
#include "gx.h"
#include "MTX/mtx.h"                       /* Mtx, Vec, PSMTXIdentity */
#include "MTX/mtx44.h"                     /* Mtx44, C_MTXOrtho */
#include "KPAD/kpad.h"                     /* Vec2 */
#include "TPL/tpl.h"                       /* TPLPalette, TPLBind, TPLGet */
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
#include "Runtime.PPCEABI.H/memset.h"      /* owned by Runtime.PPCEABI.H/memset.c (rule 2) */
#include "g3d/g3d_scnroot.h"               /* nw4r::g3d::ScnRoot */
#include "g3d/g3d_state.h"                 /* g3d_state_invalidate */
#include "g3d/scn_root_get_current_camera.h"
#include "ef/pRoot.h"                      /* pRoot (nw_resource.cpp) */
#include "ef/fn_800CDB2C.h"                /* my_player_no, load_file (ef/system_core.cpp) */
#include "ef/load_file_req.h"              /* load_file_req (ef/system_core.cpp) */
#include "mh3_pad/Screen_w.h"              /* Screen_w (mh3_pad.cpp) */
#include "mh3_pad/system_w.h"              /* system_w (mh3_pad.cpp) */
#include "mh3_pad/disp_beta.h"             /* disp_beta (mh3_pad.cpp) */
#include "font/dbg_drawgraph.h"           /* dbg_drawgraph_init (font/dbg_drawgraph.cpp) */
#include "draw_shape.h"                    /* EftResFile, the face/skin file tables (draw_shape.cpp) */
#include "face_spr.h"                      /* FaceSprBuf */

/* The retail object keeps the unfused `clrlwi` before every narrow store and index (playbook: peephole). */
#pragma peephole off

/* This unit's own symbols, declared once in its headers (rule 2). */
#include "fn_80047398.h"
#include "scene_current_camera_get.h"
#include "scene_current_camera_set.h"
#include "face_tex_image_get.h"
#include "face_work_init.h"
#include "face_tex_request.h"
#include "face_tex_ready_ck.h"
#include "face_draw.h"
#include "frame_draw_end_hook.h"
#include "face_buf_tbl.h"
#include "face_buf_reset.h"

/* The face work block: per face slot (ten of them) the draw state and the two texture images the face is rendered
 * into, plus the three loaded TPL files and the GX texture objects bound from them.  size: 0x4D0 */
typedef struct FaceWork {
    /* +0x000 */ u8 bound;                 /* set by face_tex_bind once the textures are bound; the loads clear it */
    /* +0x001 */ u8 pad_0x001[3];
    /* +0x004 */ TPLPalette* tex[3];       /* the skin file, the face file and `facemake` */
    /* +0x010 */ u8 state[3];              /* per file: 0 idle, 1 loading, 2 loaded */
    /* +0x013 */ u8 enabled[10];
    /* +0x01D */ u8 select[10];            /* the face slot `face_select_set` picks */
    /* +0x027 */ u8 pad_0x027;
    /* +0x028 */ GXTexObj texobj[33];      /* 0..7 the two face files' images (4 each), 8.. the facemake images */
    /* +0x448 */ u8 face_type[10];
    /* +0x452 */ u8 bind_face[4];          /* the face types face_tex_bind bound: {type, type + 1, type, type + 1} */
    /* +0x456 */ u8 eye[10];               /* 0..100, the blend into the next face type */
    /* +0x460 */ u8* image_face[10];       /* the rendered face (0x10000 bytes each) */
    /* +0x488 */ u8* image_makeup[10];     /* the rendered makeup half */
    /* +0x4B0 */ u8 dirty[10];             /* set by every writer; cleared when the slot is drawn */
    /* +0x4BA */ s8 makeup_color[10];
    /* +0x4C4 */ u8 makeup[10];
    /* +0x4CE */ u8 pad_0x4CE[2];
} FaceWork;

extern "C" {
void face_tex_load_done(u32 result, u32 dst, u32 size, u32* ctx);
void face_select_on(u8 slot, u8 face);
void face_tex_reload(u8 sex, u8 index);
void faceGXEnd(void);
void faceGXTexCoord2f32(f32 s, f32 t);
void faceGXColor4u8(u32 r, u32 g, u32 b, u32 a);
void faceGXPosition2s16(s32 x, s32 y);
void faceGXSetTexCoordGen(u32 coord, u32 func, u32 src, u32 mtx);
void face_buf_init(void);
}

/* --- the unit's own data ----------------------------------------------------------------------------------- */

/* The face numbers `face_tex_reload` maps a face index to, per sex. */
u16 face_no_m[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8};
u16 face_no_f[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8};
u16* face_no_tbl[2] = {face_no_m, face_no_f};

FaceWork face_work;
FaceSprBuf* face_buf_tbl[2];

/* Draws one textured quad at (x, y) of w x h, white with alpha `alpha`, texels (s, t)..(s + sw, t + th). */
static inline void face_quad_draw(s16 x, s16 y, s16 w, s16 h, u8 alpha, f32 s, f32 t, f32 sw, f32 th)
{
    GXBegin(0x80, 0, 4);
    faceGXPosition2s16(x, y);
    faceGXColor4u8(255, 255, 255, alpha);
    faceGXTexCoord2f32(s, t);
    faceGXPosition2s16(x + w, y);
    faceGXColor4u8(255, 255, 255, alpha);
    faceGXTexCoord2f32(s + sw, t);
    faceGXPosition2s16(x + w, y + h);
    faceGXColor4u8(255, 255, 255, alpha);
    faceGXTexCoord2f32(s + sw, t + th);
    faceGXPosition2s16(x, y + h);
    faceGXColor4u8(255, 255, 255, alpha);
    faceGXTexCoord2f32(s, t + th);
    faceGXEnd();
}

/* --- bodies, in address order ------------------------------------------------------------------------------ */

/* 0x80047398 (0x5C): the scene root's current camera (the local player's own one in a split screen). */
/* untyped: opaque handle - the scene root's camera word (`ScnRoot::GetCamera` returns it as an int) */
void* scene_current_camera_get(void)
{
    if (Screen_w.split_mode != 0) {
        return (void*)((nw4r::g3d::ScnRoot*)pRoot)
            ->GetCamera(Screen_w.camera_view.player_camera_0x50[(s8)my_player_no()]);
    }
    return (void*)scn_root_get_current_camera(pRoot);
}

/* 0x800473F4 (0x7C): makes camera `index` current (and records it as the local player's in a split screen). */
void scene_current_camera_set(u8 index)
{
    if (Screen_w.split_mode != 0) {
        if ((s8)my_player_no() == 0) {
            Screen_w.camera_view.player_camera_0x50[0] = index + 8;
        } else {
            Screen_w.camera_view.player_camera_0x50[1] = index + 16;
        }
    }
    ((nw4r::g3d::ScnRoot*)pRoot)->SetCurrentCamera(index);
}

/* 0x80047470 (0x38): slot `slot`'s rendered face (`which` 0) or makeup (`which` 1) image. */
u8* face_tex_image_get(u8 slot, s32 which)
{
    FaceWork* work = &face_work;

    if (which == 0) {
        return work->image_face[slot];
    }
    return work->image_makeup[slot];
}

/* 0x800474A8 (0x170): clears the face work and hands out the file buffers and the per-slot image buffers. */
void face_work_init(void)
{
    u8* addr = (u8*)0x90CC9000;
    s32 i;

    memset(&face_work, 0, sizeof(FaceWork));
    for (i = 0; i < 2; i++) {
        if (face_work.tex[i] == NULL) {
            face_work.tex[i] = (TPLPalette*)addr;
        }
        addr += 0x80C0;
    }
    face_work.tex[2] = (TPLPalette*)addr;
    addr += 0x32460;
    for (i = 0; i < 10; i++) {
        if (face_work.image_face[i] == NULL) {
            face_work.image_face[i] = addr;
        }
        if (face_work.image_makeup[i] == NULL) {
            face_work.image_makeup[i] = addr + 0x10000;
        }
        addr += 0x20000;
    }
}

/* 0x80047618 (0x1C): the load_file_req completion: marks the file whose index the context word holds as loaded. */
void face_tex_load_done(u32 result, u32 dst, u32 size, u32* ctx)
{
    face_work.state[*ctx] = 2;
}

/* 0x80047634 (0x14C): loads face `face` of sex `sex` and the facemake file (`mode` 1: asynchronously). */
void face_tex_load(u8 sex, u32 face, u8 mode)
{
    FaceWork* work = &face_work;
    u32 ctx = 1;
    EftResFile* file;

    work->bound = 0;
    work->state[1] = 0;
    file = &(sex == 0 ? m_face_file_tbl : f_face_file_tbl)[face];
    if (file->size_0x00 != 0 && work->tex[1] != NULL) {
        work->state[1] = 1;
        if (mode == 1) {
            load_file_req(file->name_0x04, (u32)work->tex[ctx], file->size_0x00, (u32)face_tex_load_done, 1, &ctx);
        } else {
            load_file(file->name_0x04, (u32)work->tex[ctx], file->size_0x00);
            work->state[ctx] = 2;
        }
        ctx = 2;
        file = &skin_file_tbl[2];
        work->state[2] = 1;
        if (mode == 1) {
            load_file_req(file->name_0x04, (u32)work->tex[ctx], file->size_0x00, (u32)face_tex_load_done, 1, &ctx);
        } else {
            load_file(file->name_0x04, (u32)work->tex[ctx], file->size_0x00);
            work->state[ctx] = 2;
        }
    }
}

/* 0x80047780 (0xC0): loads the skin file of sex `sex` (`mode` 1: asynchronously). */
void face_parts_tex_load(u8 sex, s32 part, u8 mode)
{
    FaceWork* work = &face_work;
    u32 ctx = 0;
    EftResFile* file;

    work->bound = 0;
    work->state[0] = 0;
    file = skin_file_tbl;
    file += sex;
    if (file->size_0x00 != 0 && work->tex[0] != NULL) {
        work->state[0] = 1;
        if (mode == 1) {
            load_file_req(file->name_0x04, (u32)work->tex[ctx], file->size_0x00, (u32)face_tex_load_done, 1, &ctx);
        } else {
            load_file(file->name_0x04, (u32)work->tex[ctx], file->size_0x00);
            work->state[ctx] = 2;
        }
    }
}

/* 0x80047840 (0x44): selects face `face` for slot `slot` and shows it (0xFF: no change). */
void face_select_on(u8 slot, u8 face)
{
    if (face != 0xFF) {
        face_select_set(slot, face);
        face_draw_on(slot);
    }
}

/* 0x80047884 (0x6C): synchronously reloads sex `sex`'s face number `index` and its skin, and binds them. */
void face_tex_reload(u8 sex, u8 index)
{
    if (sex != 0xFF) {
        face_tex_load(sex, face_no_tbl[sex][index], 0);
        face_parts_tex_load(sex, 0, 0);
        face_tex_bind(sex);
    }
}

/* 0x800478F0 (0x200): once all three files are loaded, binds their images as GX texture objects. */
void face_tex_bind(u32 sex)
{
    FaceWork* work = &face_work;
    TPLPalette* pal;
    TPLImageHeader* image;
    GXTexObj* obj;
    s32 j;
    s32 i;

    if (work->state[0] != 2 || work->state[1] != 2 || work->state[2] != 2) {
        return;
    }
    if (work->bound != 0) {
        return;
    }
    for (j = 0; j < 2; j++) {
        pal = work->tex[j];
        TPLBind(pal);
        for (i = 0; i < pal->numDescriptors; i++) {
            image = TPLGet(pal, i)->textureHeader;
            obj = &work->texobj[i % 4 + j * 4];
            GXInitTexObj(obj, image->data, image->width, image->height, image->format, image->wrapS, image->wrapT, 0);
            GXInitTexObjLOD(obj, 0, 1, 0.0f, 0.0f, 0.0f, 0, 0, 0);
        }
    }
    pal = work->tex[j];
    TPLBind(pal);
    obj = &work->texobj[8];
    for (i = 0; i < pal->numDescriptors; i++) {
        image = TPLGet(pal, i)->textureHeader;
        GXInitTexObj(obj, image->data, image->width, image->height, image->format, 0, 0, 0);
        GXInitTexObjLOD(obj, 0, 1, 0.0f, 0.0f, 0.0f, 0, 0, 0);
        obj++;
    }
    work->bind_face[0] = work->face_type[sex];
    work->bind_face[1] = work->face_type[sex] + 1;
    work->bind_face[2] = work->bind_face[0];
    work->bind_face[3] = work->bind_face[1];
    work->bound++;
    g3d_state_invalidate(0x7FF);
    GXSetNumIndStages(0);
}

/* 0x80047AF0 (0x1C): shows slot `slot`'s face. */
void face_draw_on(u8 slot)
{
    face_work.enabled[slot] = 1;
}

/* 0x80047B0C (0x1C): hides slot `slot`'s face. */
void face_draw_off(u8 slot)
{
    face_work.enabled[slot] = 0;
}

/* 0x80047B28 (0x18): slot `slot`'s eye blend (0..100). */
u8 face_eye_get(u8 slot)
{
    return face_work.eye[slot];
}

/* 0x80047B40 (0x18): slot `slot`'s face type. */
u8 face_type_get(u8 slot)
{
    return face_work.face_type[slot];
}

/* 0x80047B58 (0x88): steps slot `slot`'s eye blend forward; at 100 it moves on to the next face type (up to 2). */
void face_type_next(u8 slot)
{
    FaceWork* work = &face_work;

    if (work->eye[slot] < 100) {
        work->eye[slot]++;
        if (work->eye[slot] == 100) {
            if (work->face_type[slot] < 2) {
                work->face_type[slot]++;
                work->eye[slot] = 0;
            }
        }
        work->dirty[slot] = 1;
    } else if (work->face_type[slot] < 2) {
        work->face_type[slot]++;
        work->eye[slot] = 0;
        work->dirty[slot] = 1;
    }
}

/* 0x80047BE0 (0x88): steps slot `slot`'s eye blend back; at 0 it moves back to the previous face type. */
void face_type_prev(u8 slot)
{
    FaceWork* work = &face_work;

    if (work->eye[slot] != 0) {
        work->eye[slot]--;
        if (work->eye[slot] == 0) {
            if (work->face_type[slot] != 0) {
                work->face_type[slot]--;
                work->eye[slot] = 100;
            }
        }
        work->dirty[slot] = 1;
    } else if (work->face_type[slot] != 0) {
        work->face_type[slot]--;
        work->eye[slot] = 100;
        work->dirty[slot] = 1;
    }
}

/* 0x80047C68 (0x20): selects face slot `face` for slot `slot`. */
void face_select_set(u8 slot, u8 face)
{
    face_work.select[slot] = face;
    face_work.dirty[slot] = 1;
}

/* 0x80047C88 (0x24): sets slot `slot`'s makeup and makeup colour. */
void face_makeup_set(u8 slot, u8 makeup, s8 color)
{
    face_work.makeup[slot] = makeup;
    face_work.makeup_color[slot] = color;
    face_work.dirty[slot] = 1;
}

/* 0x80047CAC (0x8C): loads sex `sex`'s face `face` asynchronously and shows it in slot `slot` (`color` 0xFF: none). */
void face_tex_request(u8 slot, u8 sex, u8 face, u8 style, u8 color)
{
    FaceWork* work = &face_work;

    if (color != 0xFF) {
        face_tex_load(sex, face, 1);
        face_parts_tex_load(sex, 0, 1);
        work->face_type[slot] = style;
        work->eye[slot] = color;
        work->enabled[slot] = 1;
        work->dirty[slot] = 1;
    }
}

/* 0x80047D38 (0x34): 1 when slot `slot` is hidden or has been drawn since its last change. */
s32 face_tex_ready_ck(u8 slot)
{
    if (face_work.enabled[slot] == 0) {
        return 1;
    }
    return face_work.dirty[slot] == 0;
}

/* 0x80047D6C (0x9F8): renders every shown, changed slot's face into its two images, then clears the screen. */
void face_draw(void)
{
    s32 i;
    FaceWork* work = &face_work;
    Mtx44 proj;
    Mtx mv;
    Vec pos;
    Vec2 size;
    GXColor color;
    u8 alpha;
    f32 base;
    f32 span;
    f32 s;
    f32 t;

    if (face_work.bound == 1) {
        for (i = 0; i < 10; i++) {
            if (work->enabled[i] == 0 || work->dirty[i] == 0) {
                continue;
            }
            work->dirty[i] = 0;
            dbg_drawgraph_init();
            GXInvalidateTexAll();
            GXSetColorUpdate(1);
            C_MTXOrtho(proj, 0.0f, (s16)Screen_w.height, 0.0f, (s16)Screen_w.width, 0.0f, 1.0f);
            GXSetProjection(proj, 1);
            PSMTXIdentity(mv);
            GXLoadPosMtxImm(mv, 0);
            GXSetCurrentMtx(0);
            GXSetCullMode(0);
            GXSetZMode(1, 7, 0);
            GXSetNumTexGens(1);
            faceGXSetTexCoordGen(0, 1, 4, 60);
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
            base = 0.0f;
            span = 1.0f;
            GXLoadTexObj(&work->texobj[work->face_type[i]], 0);
            face_quad_draw(0, 0, 128, 128, 255, base, base, span, span);
            GXLoadTexObj(&work->texobj[work->face_type[i] + 4], 0);
            face_quad_draw(256, 0, 128, 128, 255, base, base, span, span);

            alpha = 255.0f * (work->eye[i] / 100.0f);
            GXSetBlendMode(1, 4, 5, 0);
            GXLoadTexObj(&work->texobj[work->face_type[i] + 1], 0);
            face_quad_draw(0, 0, 128, 128, alpha, base, base, span, span);
            GXLoadTexObj(&work->texobj[work->face_type[i] + 5], 0);
            face_quad_draw(256, 0, 128, 128, alpha, base, base, span, span);

            if (work->makeup_color[i] > 0) {
                GXSetBlendMode(1, 4, 5, 0);
                GXLoadTexObj(&(work->texobj + 8)[work->makeup_color[i] + work->select[i] * 12], 0);
                face_quad_draw(256, 0, 128, 128, 255, base, base, span, span);
            }

            s = (f32)(work->makeup[i] % 4 * 32) / 128.0f;
            t = (f32)(work->makeup[i] / 4 * 16 + work->select[i] * 64) / 128.0f;
            GXLoadTexObj(&work->texobj[8], 0);
            GXSetBlendMode(1, 1, 0, 0);
            face_quad_draw(352, 112, 32, 16, 255, s, t, 0.25f, 0.125f);

            GXDrawDone();
            GXSetTexCopySrc(0, 0, 128, 128);
            GXSetTexCopyDst(128, 128, 6, 0);
            DCInvalidateRange(work->image_face[i], 0x10000);
            GXCopyTex(work->image_face[i], 0);
            GXSetTexCopySrc(256, 0, 128, 128);
            GXSetTexCopyDst(128, 128, 6, 0);
            DCInvalidateRange(work->image_makeup[i], 0x10000);
            GXCopyTex(work->image_makeup[i], 0);
            GXPixModeSync();
        }
        GXInvalidateTexAll();
    }
    pos.x = 0.0f;
    pos.y = 0.0f;
    pos.z = 0.0f;
    size.x = 640.0f;
    size.y = 480.0f;
    *(u32*)&color = system_w.field_0x7bc.value;
    disp_beta(&pos, &size, color);
    g3d_state_invalidate(0x7FF);
    GXSetNumIndStages(0);
}

/* --- the GX pipe helpers (the SDK's write-gather-pipe writers, emitted out of line) ------------------------ */

/* 0x80048764 (0x4): ends a primitive (nothing to flush). */
void faceGXEnd(void)
{
}

/* 0x80048768 (0x10): writes one texture coordinate pair. */
void faceGXTexCoord2f32(f32 s, f32 t)
{
    GXWGFifo.f32 = s;
    GXWGFifo.f32 = t;
}

/* 0x80048778 (0x28): writes one RGBA8 vertex colour. */
void faceGXColor4u8(u32 r, u32 g, u32 b, u32 a)
{
    GXWGFifo.u8 = r;
    GXWGFifo.u8 = g;
    GXWGFifo.u8 = b;
    GXWGFifo.u8 = a;
}

/* 0x800487A0 (0x18): writes one s16 2D position. */
void faceGXPosition2s16(s32 x, s32 y)
{
    GXWGFifo.s16 = x;
    GXWGFifo.s16 = y;
}

/* 0x800487B8 (0xC): one texcoord generator with GX's normalize flag off and the identity post-matrix (125). */
void faceGXSetTexCoordGen(u32 coord, u32 func, u32 src, u32 mtx)
{
    GXSetTexCoordGen2(coord, func, src, mtx, 0, 125);
}

/* 0x800487C4 (0x4): the empty hook the frame draw calls last. */
void frame_draw_end_hook(void)
{
}

/* 0x800487C8 (0x8): the size of one face-sprite buffer (the system core lays out two). */
u32 face_buf_size_get(void)
{
    return sizeof(FaceSprBuf);
}

/* 0x800487D0 (0x5C): carves the two face-sprite buffers out of the system memory layout and clears them. */
void face_buf_init(void)
{
    face_buf_tbl[0] = (FaceSprBuf*)system_w.layout_0x940;
    face_buf_tbl[1] = face_buf_tbl[0] + 1;
    memset(face_buf_tbl[0], 0, sizeof(FaceSprBuf));
    memset(face_buf_tbl[1], 0, sizeof(FaceSprBuf));
}

/* 0x8004882C (0x138): clears both face-sprite buffers and marks every texture run free (-1). */
void face_buf_reset(void)
{
    s32 i;
    s32 j;

    for (i = 0; i < 2; i++) {
        memset(face_buf_tbl[i], 0, sizeof(FaceSprBuf));
        for (j = 0; j < 0x20; j++) {
            face_buf_tbl[i]->tex[j] = -1;
        }
    }
}
