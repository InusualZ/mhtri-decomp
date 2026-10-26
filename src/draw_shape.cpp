/*
 * `draw_shape.cpp` - the 2D shape/texture draw layer of the menu/HUD code.
 *
 * RANGE. `.text` 0x8005270C..0x80055EC4: the projection and GX setup helpers, the draw-shape texture slots, the
 *   out-of-line nw4r `ResTex`/`ResPltt` accessors and GX FIFO writers this TU keeps, the 2D line/rectangle draws, the
 *   `drawshape_*` vertex/colour/texture/matrix setters and `drawshape_exec`, the texture-pack loaders and their load
 *   callbacks, the atlas-cell uv helpers, the arena index and the work block's static initialiser.  The 2D capture
 *   sprite and the state setters from 0x80055EC4 are `draw_shape_arm.cpp` (the `.sdata2` pools repeat 0.0f/1.0f, so the
 *   seam is a pool boundary).
 * EVIDENCE. Class 1: every `nw4r::db::Panic` the loaders reach passes the bare source name "draw_shape.cpp"
 *   (`.data` 0x8058178C, one copy, referenced only from this range) with lines rising with the addresses (1982, 2060,
 *   2140, 2592, 2668).  Module: the game-root band (`main` lib, `cflags_main`), next to `mh3_pad.cpp`.
 * FLAGS. The unit's `cflags_main` command line plus `#pragma peephole off` (retail keeps every `clrlwi`+`cmpwi` pair
 *   unfused: 93.06 % with the pass, 96.81 % without) and a closing `#pragma dont_inline on` (the static initialiser then
 *   calls the inline constructor out of line and emits it after itself, as retail does).
 * NAMES. Real: the nw4r accessor manglings, `drawshape_exec`/`drawshape_init`/the `drawshape_set_*` manglings, the
 *   `*_tex_load` manglings, `draw_line_2d`/`draw_rect_2d_tex`, `set_arena_idx_switch`, `__sinit_\draw_shape_cpp`.
 *   GUESS (from the bodies and the callers): `DrawShapeWork` (its constructor `__ct__13DrawShapeWorkFv`),
 *   `draw_shape_reset`, `drawshape_ortho_proj_set`, `drawshape_tex_load`, `drawshape_tev_color_only`,
 *   `drawshape_tev_texture`, `drawshape_gx_texcoord_gen`, `drawshape_tev_stages_set`, `drawshape_vtx_fmt_set`,
 *   `drawshape_gx_setup`, `drawshape_gx_restore`, the `drawshape_gx_*` FIFO writer copies, the `res_*_word_*` handle
 *   copies, `mtx34_struct_copy`, `drawshape_tex_wrap_select`, `drawshape_set_rot_scale`(`_ivec2`),
 *   `drawshape_set_translate`, `drawshape_set_tex_translate`, `drawshape_set_tex_scale`, `drawshape_gx_begin_setup`,
 *   `drawshape_gx_position2f32`, `drawshape_tex_obj_get`, `cockpit_tex_load_cb`, `map_tex_load_cb`,
 *   `quest_fin_tex_load`(`_cb`), `reward_tex_load`, the `*_tex_name_tbl`/`*_file` data names, `itemicon_work_tag`,
 *   `drawshape_work`, `arena_idx`/`arena_idx_switch`, `arena_idx_switch_get`, `atlas_cell20/28/64_uv_get`.
 * RESIDUALS. `.data` (0x80581770..0x8058AF44: the texture-name tables, the file descriptors, the effect resource
 *   tables `draw_shape.h` exports and every string) and `.sdata` (the file descriptors and tags) are not emitted: the
 *   tables are declared here and the strings are literals, so the loaders' string relocations name `@N` objects where
 *   retail names its own (cockpit/gpframe/menu/village loaders 99.6 %).  The `.sbss` words 0x807948A8..0x807948C0 are
 *   not emitted (no code reference).
 *   The nw4r accessors and GX writers are out-of-line definitions in address order; the target's `.data` string order
 *   (the accessors' strings last, in reverse) says they were deferred inline functions (the TU was built with the
 *   inliner off), which this source does not reproduce for them.
 *   Register-only residuals: the seven matrix setters (98.7-99.2 %: r30/r31 swap between the work-block base and the
 *   saved matrix pointer of the apply tail), `atlas_digit/cell20/cell28/cell64_uv_get` (96.1-98.3 %: r6/r7 swap between
 *   the value and the `mulhw` product), `drawshape_tex_obj_get` (98.8 %: the work base and slot offset take r28/r29
 *   where retail has them in r30/r31), `drawshape_tev_stages_set` (99.4 %: the stage pointer and the counter swap
 *   r29/r30), `drawshape_exec` (99.65 %: one vertex loop's pointer and count swap r25/r26), `drawshape_gx_begin_setup`
 *   (99.8 %: r30/r31 swap between the work base and the copied matrix pointer).
 */

#include "types.h"
#include "gx.h"
#include "nw4r/math.h"
#include "MTX/mtx.h"
#include "MTX/mtx44.h"
#include "RVLGX/GXAttr.h"
#include "RVLGX/GXBump.h"
#include "RVLGX/GXGeometry.h"
#include "RVLGX/GXLight.h"
#include "RVLGX/GXTev.h"
#include "RVLGX/GXTexture.h"
#include "RVLGX/GXTransform.h"
#include "g3d/g3d_resshp.h"       /* nw4r::g3d::ResTex / ResPltt (rule 2) */
#include "g3d/g3d_resfile.h"      /* nw4r::g3d::ResFile (rule 2) */
#include "g3d/g3d_state.h"        /* g3d_state_get_render_mode / g3d_state_invalidate (rule 2) */
#include "g3d/res_file_tex_by_index.h"
#include "hud/layout_types.h"     /* _mh_ivec2_ / _mh_tex_uv_ (rule 1) */
#include "camera/camera.h"        /* get_current_view_mtx (rule 2) */
#include "sound/camera_projection_load.h"
#include "mh3_pad.h"              /* cnvt_eur_fname / screen_split_mode_ck (rule 2) */
#include "mh3_pad/Screen_w.h"     /* Screen_w (rule 2) */
#include "ef/work_stack_alloc.h"
#include "ef/load_file_req.h"     /* load_file_req (rule 2) */
#include "ef/fn_800CDB2C.h"       /* load_file */
#include "nw_resource.h"          /* the resource loader entry points (rule 2) */
#include "lobby/lb_quest_screen.h" /* quest_id_get (rule 2) */
#include "quest/quest_entry.h"    /* quest_phase_get (rule 2) */
#include "quest/quest_item_slot.h" /* quest_select_ready_ck (rule 2) */
#include "fn_8004CAD8.h"          /* mtx34_get_ptr / MTX34_ctor / sin_idx_get / anim_tick_cos (rule 2) */
#include "main.h"                 /* _MH_VEC2 (rule 1) */
#include "draw_shape.h"

#pragma peephole off

typedef f32 (*DrawShapeMtxPtr)[4];

/* The per-draw 2D shape work block: the primitive the next `drawshape_exec` draws (kind, width, vertex count and the
 * vertex/colour/uv arrays, with the rectangle setters' own storage), the position and texture matrices, the bound
 * texture/palette handles with their GX objects, and the 33 texture slots the loaders fill.  size: 0x368 */
typedef struct DrawShapeWork {
    /* 0x80055E64 (0x60): constructs the two matrices and the three handles (run by `__sinit_\draw_shape_cpp`). */
    DrawShapeWork() {
        MTX34_ctor(&pos_mtx);
        MTX34_ctor(&tex_mtx);
        res_tex_ctor(&tex, 0);
        res_pltt_ctor(&pltt, 0);
        res_tex_ctor(&tex_0x360, 0);
    }

    /* +0x000 */ u32 flags;              /* 1 3D, 2 textured, 4 pos matrix set, 8 tex matrix set, 0x1000 vertices,
                                            0x2000 colours, 0x10000 f32 vertices, 0x20000 one colour, 0x40000 s16 uvs */
    /* +0x004 */ u8 kind;                /* the GX primitive: point, line, line strip, quads, ... */
    /* +0x005 */ u8 width;               /* the point size / line width */
    /* +0x006 */ u16 count;              /* the vertex count */
    /* +0x008 */ const void* vertices;   /* s16 pairs, f32 pairs or f32 triples, by the flags */
    /* +0x00C */ const u32* colors;
    /* +0x010 */ const void* uvs;        /* f32 pairs, or s16 pairs with flag 0x40000 */
    /* +0x014 */ union {
        _mh_ivec2_ s[4];
        f32 f[4][2];
    } rect;                              /* the rectangle setters' corners */
    /* +0x034 */ u8 pad_0x34[0x10];
    /* +0x044 */ u32 rect_colors[4];
    /* +0x054 */ f32 rect_uvs[4][2];
    /* +0x074 */ MTX34 pos_mtx;
    /* +0x0A4 */ MTX34 tex_mtx;
    /* +0x0D4 */ u32 wrap_s;
    /* +0x0D8 */ u32 wrap_t;
    /* +0x0DC */ u32 tex_offsets;        /* GXSetPointSize / GXSetLineWidth's second argument */
    /* +0x0E0 */ u32 tex;                /* the bound `ResTex` handle */
    /* +0x0E4 */ u32 pltt;               /* the bound `ResPltt` handle */
    /* +0x0E8 */ u8 pad_0xE8[0x10];
    /* +0x0F8 */ GXTexObj tex_objs[8];
    /* +0x1F8 */ GXTlutObj tlut_objs[8];
    /* +0x258 */ u32 slot_tex[0x21];     /* the texture slots' `ResTex` handles */
    /* +0x2DC */ u32 slot_pltt[0x21];    /* their `ResPltt` handles (0 without a palette) */
    /* +0x360 */ u32 tex_0x360;          /* a third handle the constructor clears */
    /* +0x364 */ s8 slot_mode;           /* the last slot set's mode byte: picks the wrap table */
    /* +0x365 */ u8 pad_0x365[3];
} DrawShapeWork;

/* The work block and the arena index pair (`set_arena_idx` sets both, `set_arena_idx_switch` the second). */
DrawShapeWork drawshape_work;
u8 arena_idx;
u8 arena_idx_switch;

/* The texture-name tables the loaders walk (a null name ends each) and the files they load. */
extern char* itemicon_tex_name_tbl[];
/* The work-stack tag the item-icon pack is recorded under (the 9-byte "ITEMICON"). */
extern char itemicon_work_tag[];
extern char* gpframe_tex_name_tbl[];
extern char* menu_tex_name_tbl[];
extern char* cockpit_tex_name_tbl[];
extern char* cockpit_split_tex_name_tbl[];
extern char* village_tex_name_tbl[];
extern char* reward_tex_name_tbl[];
extern EftResFile itemicon_file[];
extern EftResFile gpframe_file[];
extern EftResFile menu_file[];
extern EftResFile map_file_stage9_phase10[];
extern EftResFile reward_file[];
extern EftResFile village_file[];
extern EftResFile cockpit_file_tbl[];
extern EftResFile stage_map_file_tbl[];
extern EftResFile quest_map_file_tbl[];
extern EftResFile quest_fin_file_tbl[];
/* The vertex count `drawshape_init` takes per primitive kind, and the icon atlas columns. */
extern u8 drawshape_kind_vtx_count[8];
extern u8 atlas_icon_cell_tbl[];

/* The TEV stage ids `drawshape_tev_stages_set` walks. */
const u32 drawshape_tev_stage_tbl[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};

static inline nw4r::g3d::ResTex* tex_of(u32* handle) {
    return reinterpret_cast<nw4r::g3d::ResTex*>(handle);
}
static inline nw4r::g3d::ResPltt* pltt_of(u32* handle) {
    return reinterpret_cast<nw4r::g3d::ResPltt*>(handle);
}
static inline const nw4r::g3d::ResFile* file_of(u32* handle) {
    return reinterpret_cast<const nw4r::g3d::ResFile*>(handle);
}
static inline DrawShapeMtxPtr mtx_of(MTX34* m) {
    return (DrawShapeMtxPtr)mtx34_get_ptr(m);
}

extern "C" {
void drawshape_ortho_proj_set(void);
void drawshape_tex_load(u32* tex, u32* pltt, u32 id, u32 wrap_s, u32 wrap_t, u32 flags);
void drawshape_tev_color_only(void);
void drawshape_tev_texture(u32* tex, u32* pltt);
void drawshape_gx_texcoord_gen(s32 dst, s32 func, s32 src, u32 mtx);
void drawshape_tev_stages_set(u8 count);
void drawshape_vtx_fmt_set(u32 flags);
void drawshape_gx_setup(u8 mode, u32* tex, u32* pltt);
MTX34* mtx34_copy(MTX34* dst, const MTX34& src);
void mtx34_struct_copy(MTX34* dst, const MTX34* src);
void drawshape_gx_restore(void);
void drawshape_gx_end(void);
void drawshape_gx_color1u32(u32 color);
void drawshape_gx_position2s16(s16 x, s16 y);
void drawshape_gx_texcoord2f32(f32 u, f32 v);
void drawshape_gx_position3f32(f32 x, f32 y, f32 z);
void drawshape_gx_position2f32(f32 x, f32 y);
void drawshape_tex_wrap_select(u16 id);
void drawshape_set_rot_scale(const f32* pos, const f32* scale, u16 angle);
void drawshape_set_translate(const f32* pos);
void drawshape_set_tex_translate(const f32* pos);
void drawshape_set_tex_scale(const f32* scale, const f32* offset);
void drawshape_gx_begin_setup(void);
void res_tex_word_copy(u32* dst, const u32* src);
void res_tex_word_set(u32* dst, u32 value);
void res_pltt_word_set(u32* dst, u32 value);
void res_pltt_word_copy(u32* dst, const u32* src);
void res_file_word_copy(u32* dst, u32* src);
void res_file_word_set(u32* dst, u32 value);
/* untyped: byte range - the loaded file image */
void cockpit_tex_load_cb(char* name, void* data);
/* untyped: byte range - the loaded file image */
void map_tex_load_cb(char* name, void* data);
/* untyped: byte range - the loaded file image */
void quest_fin_tex_load_cb(char* name, void* data);
}

/* Applies `m` to the position matrix: concatenated onto it once one is set, else copied in. */
static inline void drawshape_pos_mtx_apply(DrawShapeMtxPtr m) {
    if (drawshape_work.flags & 4) {
        PSMTXConcat(m, mtx_of(&drawshape_work.pos_mtx), mtx_of(&drawshape_work.pos_mtx));
    } else {
        PSMTXCopy(m, mtx_of(&drawshape_work.pos_mtx));
        drawshape_work.flags |= 4;
    }
}

/* The same for the texture matrix. */
static inline void drawshape_tex_mtx_apply(DrawShapeMtxPtr m) {
    if (drawshape_work.flags & 8) {
        PSMTXConcat(m, mtx_of(&drawshape_work.tex_mtx), mtx_of(&drawshape_work.tex_mtx));
    } else {
        PSMTXCopy(m, mtx_of(&drawshape_work.tex_mtx));
        drawshape_work.flags |= 8;
    }
}

/* 0x8005270C (0x54): loads the screen-sized 2D orthographic projection. */
extern "C" void drawshape_ortho_proj_set(void) {
    Mtx44 proj;

    g3d_state_get_render_mode();
    C_MTXOrtho(proj, 0.0f, Screen_w.height_f, 0.0f, Screen_w.width_f, -10000.0f, 10000.0f);
    GXSetProjection(proj, 1);
}

/* 0x80052760 (0xE4): clears every texture slot and the slot mode. */
extern "C" void draw_shape_reset(void) {
    u16 i;

    for (i = 0; i < 0x21; i++) {
        drawshape_work.slot_tex[i] = 0;
        drawshape_work.slot_pltt[i] = 0;
    }
    drawshape_work.slot_mode = 0;
}

/* 0x80052844 (0x90): stores a texture (and its palette when colour-indexed) in slot `index`. */
extern "C" void draw_shape_tex_slot_set(const u32* tex, const u32* pltt, u16 index, s8 mode) {
    drawshape_work.slot_tex[index] = (u32)tex_of(const_cast<u32*>(tex))->ptr();
    if (tex_of(const_cast<u32*>(tex))->IsCIFmt() == 1) {
        drawshape_work.slot_pltt[index] = (u32)pltt_of(const_cast<u32*>(pltt))->ptr();
    } else {
        drawshape_work.slot_pltt[index] = 0;
    }
    drawshape_work.slot_mode = mode;
}

/* 0x800528D4 (0x8) */
nw4r::g3d::ResPlttData* nw4r::g3d::ResPltt::ptr() {
    return mpData;
}

/* 0x800528DC (0x34): whether the texture is colour-indexed. */
bool nw4r::g3d::ResTex::IsCIFmt() const {
    return (ref().flag & 1) != 0;
}

namespace nw4r {
namespace g3d {
/* 0x80052910 (0x64) */
NW4R_G3D_RESOURCE_REF_CONST(ResTex, "g3d_restex_ac.h", 38)
/* 0x80052974 (0x8) */
NW4R_G3D_RESOURCE_PTR_CONST(ResTex)
/* 0x8005297C (0x8) */
NW4R_G3D_RESOURCE_CLASS_NAME(ResTex)
/* 0x80052984 (0x14) */
NW4R_G3D_RESOURCE_IS_VALID(ResTex)
/* 0x80052998 (0x8) */
NW4R_G3D_RESOURCE_PTR(ResTex)
}  // namespace g3d
}  // namespace nw4r

/* 0x800529A0 (0x1E4): clears texture slots `first`..`last`. */
extern "C" void draw_shape_tex_slots_clear(u32 first, u32 last) {
    s32 i;

    for (i = (u16)first; i <= (s32)(u16)last; i++) {
        drawshape_work.slot_tex[i] = 0;
        drawshape_work.slot_pltt[i] = 0;
    }
}

/* 0x80052B84 (0x30): copies one texture handle over another and returns the destination. */
extern "C" u32* res_tex_assign(u32* dst, const u32* src) {
    res_tex_word_copy(dst, src);
    return dst;
}

/* 0x80052BB4 (0xC) */
extern "C" void res_tex_word_copy(u32* dst, const u32* src) {
    *dst = *src;
}

/* 0x80052BC0 (0x64): builds a texture handle from a 32-byte-aligned block address. */
/* untyped: opaque handle - the handle word the caller owns */
extern "C" void* res_tex_ctor(void* out, u32 data) {
    res_tex_word_set((u32*)out, data);
    if (data & 0x1F) {
        nw4r::db::Panic("g3d_restex_ac.h", 38, "NW4R:Failed assertion !((u32)p & 0x1f)");
    }
    return out;
}

/* 0x80052C24 (0x8) */
extern "C" void res_tex_word_set(u32* dst, u32 value) {
    *dst = value;
}

/* 0x80052C2C (0x16C): initialises and loads GX texture object `id` (and its TLUT when colour-indexed). */
extern "C" void drawshape_tex_load(u32* tex, u32* pltt, u32 id, u32 wrap_s, u32 wrap_t, u32 flags) {
    u32 tlut_fmt;
    u16 entries;
    GXTexObj* obj = &drawshape_work.tex_objs[id];
    GXTlutObj* tlut = &drawshape_work.tlut_objs[id];
    void* image;
    u16 width;
    u16 height;
    u32 fmt;
    u32 fmt_ci;
    f32 min_lod;
    f32 max_lod;
    u8 mipmap;

    if (tex_of(tex)->IsCIFmt() == 1) {
        if (pltt == NULL || !pltt_of(pltt)->IsValid()) {
            return;
        }
        entries = (u16)pltt_of(pltt)->GetNumEntries();
        tlut_fmt = pltt_of(pltt)->GetFmt();
        GXInitTlutObj(tlut, pltt_of(pltt)->GetPlttData(), tlut_fmt, entries);
        GXLoadTlut(tlut, id);
        tex_of(tex)->GetTexObjCIParam(&image, &width, &height, &fmt_ci, &min_lod, &max_lod, &mipmap);
        GXInitTexObjCI(obj, image, width, height, fmt_ci, wrap_s, wrap_t, mipmap, id);
    } else {
        tex_of(tex)->GetTexObjParam(&image, &width, &height, &fmt, &min_lod, &max_lod, &mipmap);
        GXInitTexObj(obj, image, width, height, fmt, wrap_s, wrap_t, mipmap);
    }
    GXLoadTexObj(obj, id);
}

namespace nw4r {
namespace g3d {
/* 0x80052D98 (0x24) */
u16 ResPltt::GetNumEntries() const {
    return ref().numEntries;
}
/* 0x80052DBC (0x64) */
NW4R_G3D_RESOURCE_REF_CONST(ResPltt, "g3d_respltt_ac.h", 42)
/* 0x80052E20 (0x8) */
NW4R_G3D_RESOURCE_PTR_CONST(ResPltt)
/* 0x80052E28 (0x8) */
NW4R_G3D_RESOURCE_CLASS_NAME(ResPltt)
/* 0x80052E30 (0x24) */
u32 ResPltt::GetFmt() const {
    return ref().fmt;
}
/* 0x80052E54 (0x38): the palette's colour data, or NULL. */
/* untyped: byte range - the colour data */
void* ResPltt::GetPlttData() {
    ResPlttData& r = ref();
    return r.toPlttData != 0 ? (u8*)&r + r.toPlttData : NULL;
}
/* 0x80052E8C (0x64) */
NW4R_G3D_RESOURCE_REF(ResPltt, "g3d_respltt_ac.h", 42)
/* 0x80052EF0 (0x14) */
NW4R_G3D_RESOURCE_IS_VALID(ResPltt)
}  // namespace g3d
}  // namespace nw4r

/* 0x80052F04 (0x4C): one TEV stage passing the vertex colour. */
extern "C" void drawshape_tev_color_only(void) {
    GXSetNumTexGens(0);
    GXSetNumTevStages(1);
    GXSetTevOrder(0, 0xFF, 0xFF, 4);
    GXSetTevOp(0, 4);
}

/* 0x80052F50 (0x74): binds the texture to map 0 and modulates it by the vertex colour. */
extern "C" void drawshape_tev_texture(u32* tex, u32* pltt) {
    drawshape_tex_load(tex, pltt, 0, 0, 0, 0);
    GXSetNumTexGens(1);
    drawshape_gx_texcoord_gen(0, 1, 4, 60);
    GXSetTevOrder(0, 0, 0, 4);
    GXSetTevOp(0, 0);
    drawshape_tev_stages_set(1);
}

/* 0x80052FC4 (0xC): GXSetTexCoordGen (no normalisation, identity post-transform). */
extern "C" void drawshape_gx_texcoord_gen(s32 dst, s32 func, s32 src, u32 mtx) {
    GXSetTexCoordGen2(dst, func, src, mtx, 0, 125);
}

/* 0x80052FD0 (0x9C): sets `count` direct TEV stages with the identity swap table. */
extern "C" void drawshape_tev_stages_set(u8 count) {
    const u32* stage;
    s32 i;

    GXSetTevSwapModeTable(0, 0, 1, 2, 3);
    for (i = 0, stage = drawshape_tev_stage_tbl; i < count; i++, stage++) {
        GXSetTevSwapMode(*stage, 0, 0);
        GXSetTevDirect(*stage);
    }
    GXSetNumTevStages(count);
    GXSetNumIndStages(0);
}

/* 0x8005306C (0xF4): the vertex format: s16, f32 xy or f32 xyz positions, a colour, and f32 uvs when textured. */
extern "C" void drawshape_vtx_fmt_set(u32 flags) {
    GXClearVtxDesc();
    GXSetVtxDesc(9, 1);
    GXSetVtxDesc(11, 1);
    if ((flags & 1) == 0) {
        if ((flags & 0x10000) == 0) {
            GXSetVtxAttrFmt(0, 9, 0, 3, 0);
        } else {
            GXSetVtxAttrFmt(0, 9, 0, 4, 0);
        }
    } else {
        GXSetVtxAttrFmt(0, 9, 1, 4, 0);
    }
    GXSetVtxAttrFmt(0, 11, 1, 5, 0);
    if (flags & 2) {
        GXSetVtxDesc(13, 1);
        GXSetVtxAttrFmt(0, 13, 1, 4, 0);
    }
}

/* 0x80053160 (0x17C): the GX state of an immediate 2D (or, with mode bit 0, camera-space) draw. */
extern "C" void drawshape_gx_setup(u8 mode, u32* tex, u32* pltt) {
    MTX34 mtx;
    MTX34_ctor(&mtx);

    if ((mode & 1) == 0) {
        drawshape_ortho_proj_set();
        PSMTXIdentity(mtx_of(&mtx));
        GXLoadPosMtxImm(mtx_of(&mtx), 0);
    } else {
        camera_projection_load();
        mtx34_copy(&mtx, get_current_view_mtx());
        GXLoadPosMtxImm(mtx_of(&mtx), 0);
        PSMTXInvXpose(mtx_of(&mtx), mtx_of(&mtx));
        GXLoadNrmMtxImm(mtx_of(&mtx), 0);
    }
    GXSetCurrentMtx(0);
    GXSetCullMode(0);
    GXSetClipMode(0);
    GXSetAlphaCompare(7, 0, 1, 7, 0);
    drawshape_vtx_fmt_set(mode);
    GXSetNumChans(1);
    GXSetChanCtrl(0, 0, 1, 1, 0, 0, 2);
    GXSetChanCtrl(2, 0, 1, 1, 0, 0, 2);
    if ((mode & 2) == 0) {
        drawshape_tev_color_only();
    } else {
        drawshape_tev_texture(tex, pltt);
    }
}

/* 0x800532DC (0x30): copies a 3x4 matrix and returns the destination. */
extern "C" MTX34* mtx34_copy(MTX34* dst, const MTX34& src) {
    mtx34_struct_copy(dst, &src);
    return dst;
}

/* 0x8005330C (0x64) */
extern "C" void mtx34_struct_copy(MTX34* dst, const MTX34* src) {
    *dst = *src;
}

/* 0x80053370 (0x30): restores the TEV count, the camera projection and the cached g3d state. */
extern "C" void drawshape_gx_restore(void) {
    GXSetNumTevStages(1);
    camera_projection_load();
    g3d_state_invalidate(0x7FF);
}

/* 0x800533A0 (0x4): GXEnd. */
extern "C" void drawshape_gx_end(void) {
}

/* 0x800533A4 (0xC): GXColor1u32. */
extern "C" void drawshape_gx_color1u32(u32 color) {
    GXWGFifo.u32 = color;
}

/* 0x800533B0 (0x18): GXPosition2s16. */
extern "C" void drawshape_gx_position2s16(s16 x, s16 y) {
    GXWGFifo.s16 = x;
    GXWGFifo.s16 = y;
}

/* 0x800533C8 (0x90): draws a 6-pixel line between two coloured points. */
void draw_line_2d(_DRAW_POINT_2D* a, _DRAW_POINT_2D* b) {
    drawshape_gx_setup(0, NULL, NULL);
    GXSetLineWidth(6, 0);
    GXBegin(0xA8, 0, 2);
    drawshape_gx_position2s16(a->x, a->y);
    drawshape_gx_color1u32(a->color);
    drawshape_gx_position2s16(b->x, b->y);
    drawshape_gx_color1u32(b->color);
    drawshape_gx_end();
    drawshape_gx_restore();
}

/* 0x80053458 (0x10): GXTexCoord2f32. */
extern "C" void drawshape_gx_texcoord2f32(f32 u, f32 v) {
    GXWGFifo.f32 = u;
    GXWGFifo.f32 = v;
}

namespace nw4r {
namespace g3d {
/* 0x80053468 (0x24) */
u16 ResTex::GetHeight() const {
    return ref().height;
}
/* 0x8005348C (0x24) */
u16 ResTex::GetWidth() const {
    return ref().width;
}
}  // namespace g3d
}  // namespace nw4r

/* 0x800534B0 (0x64): builds a palette handle from a 32-byte-aligned block address. */
/* untyped: opaque handle - the handle word the caller owns */
extern "C" void* res_pltt_ctor(void* out, u32 data) {
    res_pltt_word_set((u32*)out, data);
    if (data & 0x1F) {
        nw4r::db::Panic("g3d_respltt_ac.h", 42, "NW4R:Failed assertion !((u32)p & 0x1f)");
    }
    return out;
}

/* 0x80053514 (0x8) */
extern "C" void res_pltt_word_set(u32* dst, u32 value) {
    *dst = value;
}

/* 0x8005351C (0xA0): draws `rect` as one coloured quad. */
extern "C" void draw_rect_fill(DrawRect* rect) {
    drawshape_gx_setup(0, NULL, NULL);
    GXBegin(0x80, 0, 4);
    drawshape_gx_position2s16(rect->x0, rect->y0);
    drawshape_gx_color1u32(rect->color);
    drawshape_gx_position2s16(rect->x1, rect->y0);
    drawshape_gx_color1u32(rect->color);
    drawshape_gx_position2s16(rect->x1, rect->y1);
    drawshape_gx_color1u32(rect->color);
    drawshape_gx_position2s16(rect->x0, rect->y1);
    drawshape_gx_color1u32(rect->color);
    drawshape_gx_end();
    drawshape_gx_restore();
}

/* 0x800535BC (0x1B4): draws `rect` as one textured quad, its texel coordinates scaled to the texture size. */
void draw_rect_2d_tex(_DRAW_RECT_2D_TEX* rect, nw4r::g3d::ResTex* tex, nw4r::g3d::ResPltt* pltt) {
    _MH_VEC2 uv0;
    _MH_VEC2 uv1;

    drawshape_gx_setup(2, reinterpret_cast<u32*>(tex), reinterpret_cast<u32*>(pltt));
    f32 su = 1.0f / tex->GetWidth();
    f32 sv = 1.0f / tex->GetHeight();
    uv0.x = rect->u0 * su;
    uv0.y = rect->v0 * sv;
    uv1.x = rect->u1 * su;
    uv1.y = rect->v1 * sv;
    GXBegin(0x80, 0, 4);
    drawshape_gx_position2s16(rect->x, rect->y);
    drawshape_gx_color1u32(rect->color);
    drawshape_gx_texcoord2f32(uv0.x, uv0.y);
    drawshape_gx_position2s16(rect->end_x, rect->y);
    drawshape_gx_color1u32(rect->color);
    drawshape_gx_texcoord2f32(uv1.x, uv0.y);
    drawshape_gx_position2s16(rect->end_x, rect->end_y);
    drawshape_gx_color1u32(rect->color);
    drawshape_gx_texcoord2f32(uv1.x, uv1.y);
    drawshape_gx_position2s16(rect->x, rect->end_y);
    drawshape_gx_color1u32(rect->color);
    drawshape_gx_texcoord2f32(uv0.x, uv1.y);
    drawshape_gx_end();
    drawshape_gx_restore();
}

/* 0x80053770 (0x88): draws `rect` with the texture of slot `id`, when the slot holds one. */
extern "C" void draw_rect_2d_tex_by_id(const _DRAW_RECT_2D_TEX* rect, u16 id) {
    u32 tex;
    u32 pltt;

    res_tex_ctor(&tex, drawshape_work.slot_tex[id]);
    res_pltt_ctor(&pltt, drawshape_work.slot_pltt[id]);
    if (tex_of(&tex)->IsValid()) {
        draw_rect_2d_tex(const_cast<_DRAW_RECT_2D_TEX*>(rect), tex_of(&tex), pltt_of(&pltt));
    }
}

/* 0x800537F8 (0x14): GXPosition3f32. */
extern "C" void drawshape_gx_position3f32(f32 x, f32 y, f32 z) {
    GXWGFifo.f32 = x;
    GXWGFifo.f32 = y;
    GXWGFifo.f32 = z;
}

/* 0x8005380C (0x54): starts a shape of primitive `kind` with `count` vertices (0xFFFF: the kind's own count). */
void drawshape_init(u8 kind, u16 count) {
    drawshape_work.kind = kind;
    drawshape_work.flags = 0;
    if (count == 0xFFFF) {
        u16 n = drawshape_kind_vtx_count[kind];
        drawshape_work.count = n;
        if (n == 0) {
            return;
        }
    } else {
        drawshape_work.count = count;
    }
    drawshape_work.width = 6;
}

/* 0x80053860 (0x1C): takes an s16 vertex array. */
void drawshape_set_vertex_array(const _mh_ivec2_* verts) {
    drawshape_work.flags |= 0x1000;
    drawshape_work.vertices = verts;
}

/* 0x8005387C (0x50): takes an s16 rectangle as the four vertices. */
void drawshape_set_vertex_rect(s16 x, s16 y, s16 w, s16 h) {
    drawshape_work.flags |= 0x1000;
    drawshape_work.rect.s[0].x = drawshape_work.rect.s[3].x = x;
    drawshape_work.rect.s[1].x = drawshape_work.rect.s[2].x = x + w;
    drawshape_work.rect.s[0].y = drawshape_work.rect.s[1].y = y;
    drawshape_work.rect.s[2].y = drawshape_work.rect.s[3].y = y + h;
    drawshape_work.vertices = drawshape_work.rect.s;
}

/* 0x800538CC (0x20): takes an f32 vertex array. */
extern "C" void drawshape_set_vertex_array_f32(const f32* verts) {
    drawshape_work.flags |= 0x10000 | 0x1000;
    drawshape_work.vertices = verts;
}

/* 0x800538EC (0x4C): takes an f32 rectangle as the four vertices. */
void drawshape_set_vertex_rect(f32 x, f32 y, f32 w, f32 h) {
    drawshape_work.flags |= 0x10000 | 0x1000;
    drawshape_work.rect.f[0][0] = drawshape_work.rect.f[3][0] = x;
    drawshape_work.rect.f[1][0] = drawshape_work.rect.f[2][0] = x + w;
    drawshape_work.rect.f[0][1] = drawshape_work.rect.f[1][1] = y;
    drawshape_work.rect.f[2][1] = drawshape_work.rect.f[3][1] = y + h;
    drawshape_work.vertices = drawshape_work.rect.f;
}

/* 0x80053938 (0x28): one colour for every vertex. */
void drawshape_set_flat_color(u32 color) {
    drawshape_work.flags |= 0x20000 | 0x2000;
    drawshape_work.rect_colors[0] = color;
    drawshape_work.colors = drawshape_work.rect_colors;
}

/* 0x80053960 (0x34): one colour per rectangle corner. */
extern "C" void drawshape_set_corner_colors(u32 c0, s32 c1, s32 c2, u32 c3) {
    drawshape_work.flags = (drawshape_work.flags & ~0x20000) | 0x2000;
    drawshape_work.rect_colors[0] = c0;
    drawshape_work.rect_colors[1] = c1;
    drawshape_work.rect_colors[2] = c2;
    drawshape_work.rect_colors[3] = c3;
    drawshape_work.colors = drawshape_work.rect_colors;
}

/* 0x80053994 (0x20): one colour per vertex from `colors`. */
extern "C" void drawshape_set_color_array(const u32* colors) {
    drawshape_work.flags = (drawshape_work.flags & ~0x20000) | 0x2000;
    drawshape_work.colors = colors;
}

/* 0x800539B4 (0xDC): picks the texture wrap modes of slot `id` and marks the shape textured. */
extern "C" void drawshape_tex_wrap_select(u16 id) {
    drawshape_work.wrap_s = drawshape_work.wrap_t = 0;
    if ((u8)drawshape_work.slot_mode == 0) {
        switch (id) {
        case 3:
        case 6:
        case 32:
            drawshape_work.wrap_s = 1;
            break;
        case 4:
            drawshape_work.wrap_t = 1;
            break;
        }
    } else {
        switch (id) {
        case 10:
        case 11:
        case 21:
        case 24:
        case 26:
            drawshape_work.wrap_s = 1;
            break;
        case 12:
        case 22:
            drawshape_work.wrap_t = 1;
            break;
        }
    }
    drawshape_work.flags |= 2;
}

/* 0x80053A90 (0x30): copies one palette handle over another and returns the destination. */
extern "C" u32* res_pltt_assign(u32* dst, const u32* src) {
    res_pltt_word_copy(dst, src);
    return dst;
}

/* 0x80053AC0 (0xC) */
extern "C" void res_pltt_word_copy(u32* dst, const u32* src) {
    *dst = *src;
}

/* 0x80053ACC (0x9C): binds slot `id`'s texture with an s16 texel uv array. */
void drawshape_set_texture_array(u16 id, const _mh_tex_uv_* uvs) {
    u32 tex;
    u32 pltt;

    drawshape_tex_wrap_select(id);
    drawshape_work.flags |= 0x40000;
    res_tex_assign(&drawshape_work.tex, (const u32*)res_tex_ctor(&tex, drawshape_work.slot_tex[id]));
    res_pltt_assign(&drawshape_work.pltt, (const u32*)res_pltt_ctor(&pltt, drawshape_work.slot_pltt[id]));
    drawshape_work.uvs = uvs;
}

/* 0x80053B68 (0x190): binds slot `id`'s texture with the texel rectangle `uv0`..`uv1`. */
void drawshape_set_texture_rect(u16 id, const _mh_tex_uv_* uv0, const _mh_tex_uv_* uv1) {
    u32 tex;
    u32 pltt;

    drawshape_tex_wrap_select(id);
    res_tex_assign(&drawshape_work.tex, (const u32*)res_tex_ctor(&tex, drawshape_work.slot_tex[id]));
    res_pltt_assign(&drawshape_work.pltt, (const u32*)res_pltt_ctor(&pltt, drawshape_work.slot_pltt[id]));
    f32 su = 1.0f / tex_of(&drawshape_work.tex)->GetWidth();
    f32 sv = 1.0f / tex_of(&drawshape_work.tex)->GetHeight();
    drawshape_work.rect_uvs[0][0] = drawshape_work.rect_uvs[3][0] = uv0->u * su;
    drawshape_work.rect_uvs[1][0] = drawshape_work.rect_uvs[2][0] = uv1->u * su;
    drawshape_work.rect_uvs[0][1] = drawshape_work.rect_uvs[1][1] = uv0->v * sv;
    drawshape_work.rect_uvs[2][1] = drawshape_work.rect_uvs[3][1] = uv1->v * sv;
    drawshape_work.uvs = drawshape_work.rect_uvs;
}

/* 0x80053CF8 (0x14): copies an f32 pair. */
extern "C" void drawshape_copy_vec2(f32* dst, const f32* src) {
    dst[0] = src[0];
    dst[1] = src[1];
}

/* 0x80053D0C (0x90): binds slot `id`'s texture with an f32 uv array. */
extern "C" void drawshape_set_texture_array_f32(u16 id, const f32* uvs) {
    u32 tex;
    u32 pltt;

    drawshape_tex_wrap_select(id);
    res_tex_assign(&drawshape_work.tex, (const u32*)res_tex_ctor(&tex, drawshape_work.slot_tex[id]));
    res_pltt_assign(&drawshape_work.pltt, (const u32*)res_pltt_ctor(&pltt, drawshape_work.slot_pltt[id]));
    drawshape_work.uvs = uvs;
}

/* 0x80053D9C (0xDC): scales the shape by `scale` about `pos`. */
void drawshape_set_scale(const _MH_VEC2* pos, const _MH_VEC2* scale) {
    Mtx trans;
    Mtx m;

    PSMTXScale(m, scale->x, scale->y, 1.0f);
    m[0][3] = pos->x;
    m[1][3] = pos->y;
    PSMTXTrans(trans, -pos->x, -pos->y, 0.0f);
    PSMTXConcat(m, trans, m);
    drawshape_pos_mtx_apply(m);
}

/* 0x80053E78 (0x148): rotates by `angle` and scales by `scale` about `pos`. */
extern "C" void drawshape_set_rot_scale(const f32* pos, const f32* scale, u16 angle) {
    Mtx trans;
    Mtx m;

    PSMTXRotTrig(m, 'z', sin_idx_get(angle), anim_tick_cos(angle));
    m[0][0] *= scale[0];
    m[1][0] *= scale[0];
    m[0][1] *= scale[1];
    m[1][1] *= scale[1];
    m[0][3] = pos[0];
    m[1][3] = pos[1];
    PSMTXTrans(trans, -pos[0], -pos[1], 0.0f);
    PSMTXConcat(m, trans, m);
    drawshape_pos_mtx_apply(m);
}

/* 0x80053FC0 (0x34): rotates by `angle` and scales uniformly about `pos`. */
extern "C" void drawshape_set_tex_scale_uniform(const f32* pos, u16 angle, f32 scale) {
    f32 s[2];

    s[0] = s[1] = scale;
    drawshape_set_rot_scale(pos, s, angle);
}

/* 0x80053FF4 (0x64): scales about an s16 position. */
extern "C" void drawshape_set_scale_ivec2(const s16* pos, const f32* scale) {
    _MH_VEC2 p;

    p.x = pos[0];
    p.y = pos[1];
    drawshape_set_scale(&p, (const _MH_VEC2*)scale);
}

/* 0x80054058 (0x78): rotates and scales uniformly about an s16 position. */
extern "C" void drawshape_set_rot_scale_ivec2(const s16* pos, u16 angle, f32 scale) {
    f32 p[2];
    f32 s[2];

    p[0] = pos[0];
    p[1] = pos[1];
    s[0] = s[1] = scale;
    drawshape_set_rot_scale(p, s, angle);
}

/* 0x800540D0 (0xA8): translates the shape by `pos`. */
extern "C" void drawshape_set_translate(const f32* pos) {
    Mtx m;

    PSMTXTrans(m, pos[0], pos[1], 0.0f);
    drawshape_pos_mtx_apply(m);
}

/* 0x80054178 (0x64): translates the shape by an s16 position. */
extern "C" void drawshape_set_offset_ivec2(s16* pos) {
    f32 p[2];

    p[0] = pos[0];
    p[1] = pos[1];
    drawshape_set_translate(p);
}

/* 0x800541DC (0x30): scales uniformly, then moves to `offset`. */
extern "C" void drawshape_set_offset_uniform(const f32* offset, f32 scale) {
    f32 s[2];

    s[0] = s[1] = scale;
    drawshape_set_offset_f32(s, offset);
}

/* 0x8005420C (0xBC): scales by `scale`, then moves to `offset`. */
extern "C" void drawshape_set_offset_f32(const f32* scale, const f32* offset) {
    Mtx m;

    PSMTXScale(m, scale[0], scale[1], 1.0f);
    m[0][3] = offset[0];
    m[1][3] = offset[1];
    drawshape_pos_mtx_apply(m);
}

/* 0x800542C8 (0xA8): translates the texture coordinates by `pos`. */
extern "C" void drawshape_set_tex_translate(const f32* pos) {
    Mtx m;

    PSMTXTrans(m, pos[0], pos[1], 0.0f);
    drawshape_tex_mtx_apply(m);
}

/* 0x80054370 (0xBC): scales the texture coordinates by `scale`, then moves them by `offset`. */
extern "C" void drawshape_set_tex_scale(const f32* scale, const f32* offset) {
    Mtx m;

    PSMTXScale(m, scale[0], scale[1], 1.0f);
    m[0][3] = offset[0];
    m[1][3] = offset[1];
    drawshape_tex_mtx_apply(m);
}

/* 0x8005442C (0x30): scales the texture coordinates uniformly, then moves them by `offset`. */
extern "C" void drawshape_set_tex_offset_uniform(const f32* offset, f32 scale) {
    f32 s[2];

    s[0] = s[1] = scale;
    drawshape_set_tex_scale(s, offset);
}

/* 0x8005445C (0x124): rotates and scales the texture coordinates, then moves them by `offset`. */
extern "C" void drawshape_set_tex_offset_f32(const f32* scale, u16 angle, const f32* offset) {
    Mtx m;

    PSMTXRotTrig(m, 'z', sin_idx_get(angle), anim_tick_cos(angle));
    m[0][0] *= scale[0];
    m[1][0] *= scale[0];
    m[0][1] *= scale[1];
    m[1][1] *= scale[1];
    m[0][3] = offset[0];
    m[1][3] = offset[1];
    drawshape_tex_mtx_apply(m);
}

/* 0x80054580 (0x228): the GX state of the shape `drawshape_exec` draws. */
extern "C" void drawshape_gx_begin_setup(void) {
    MTX34 mtx;
    MTX34_ctor(&mtx);

    if ((drawshape_work.flags & 1) == 0) {
        if (drawshape_work.flags & 4) {
            PSMTXCopy(mtx_of(&drawshape_work.pos_mtx), mtx_of(&mtx));
        } else {
            PSMTXIdentity(mtx_of(&mtx));
        }
        drawshape_ortho_proj_set();
        GXLoadPosMtxImm(mtx_of(&mtx), 0);
    } else {
        camera_projection_load();
        mtx34_copy(&mtx, get_current_view_mtx());
        GXLoadPosMtxImm(mtx_of(&mtx), 0);
        PSMTXInvXpose(mtx_of(&mtx), mtx_of(&mtx));
        GXLoadNrmMtxImm(mtx_of(&mtx), 0);
    }
    GXSetCurrentMtx(0);
    GXSetCullMode(0);
    GXSetClipMode(0);
    GXSetAlphaCompare(7, 0, 1, 7, 0);
    drawshape_vtx_fmt_set(drawshape_work.flags);
    GXSetNumChans(1);
    GXSetChanCtrl(0, 0, 1, 1, 0, 0, 2);
    GXSetChanCtrl(2, 0, 1, 1, 0, 0, 2);
    if ((drawshape_work.flags & 2) == 0) {
        drawshape_tev_color_only();
        return;
    }
    drawshape_tex_load(&drawshape_work.tex, &drawshape_work.pltt, 0, drawshape_work.wrap_s, drawshape_work.wrap_t,
                       drawshape_work.flags);
    GXSetNumTexGens(1);
    if ((drawshape_work.flags & 8) == 0) {
        drawshape_gx_texcoord_gen(0, 1, 4, 60);
    } else {
        GXLoadTexMtxImm(mtx_of(&drawshape_work.tex_mtx), 54, 0);
        drawshape_gx_texcoord_gen(0, 0, 4, 54);
    }
    GXSetTevOrder(0, 0, 0, 4);
    GXSetTevOp(0, 0);
    drawshape_tev_stages_set(1);
}

/* 0x800547A8 (0x4BC): draws the shape the setters described, when it has vertices and colours. */
void drawshape_exec(void) {
    if ((drawshape_work.flags & 0x3000) != 0x3000) {
        return;
    }
    drawshape_gx_begin_setup();
    switch (drawshape_work.kind) {
    case 0:
        GXSetPointSize(drawshape_work.width, drawshape_work.tex_offsets);
        GXBegin(0xB8, 0, drawshape_work.count);
        break;
    case 1:
        GXSetLineWidth(drawshape_work.width, drawshape_work.tex_offsets);
        GXBegin(0xA8, 0, drawshape_work.count);
        break;
    case 2:
        GXBegin(0x90, 0, drawshape_work.count);
        break;
    case 3:
        GXBegin(0x80, 0, drawshape_work.count);
        break;
    case 4:
        GXSetLineWidth(drawshape_work.width, drawshape_work.tex_offsets);
        GXBegin(0xB0, 0, drawshape_work.count);
        break;
    case 5:
        GXBegin(0x98, 0, drawshape_work.count);
        break;
    case 6:
        GXBegin(0xA0, 0, drawshape_work.count);
        break;
    default:
        return;
    }

    const u32* color = drawshape_work.colors;
    s32 color_step = (drawshape_work.flags & 0x20000) == 0;
    s32 n;
    if ((drawshape_work.flags & 0x40000) == 0) {
        const _MH_VEC2* uv = (const _MH_VEC2*)drawshape_work.uvs;
        if ((drawshape_work.flags & 1) == 0) {
            if ((drawshape_work.flags & 0x10000) == 0) {
                const _mh_ivec2_* v = (const _mh_ivec2_*)drawshape_work.vertices;
                for (n = drawshape_work.count; n > 0; n--, v++, color += color_step, uv++) {
                    drawshape_gx_position2s16(v->x, v->y);
                    drawshape_gx_color1u32(*color);
                    if (drawshape_work.flags & 2) {
                        drawshape_gx_texcoord2f32(uv->x, uv->y);
                    }
                }
            } else {
                const _MH_VEC2* v = (const _MH_VEC2*)drawshape_work.vertices;
                for (n = drawshape_work.count; n > 0; n--, v++, color += color_step, uv++) {
                    drawshape_gx_position2f32(v->x, v->y);
                    drawshape_gx_color1u32(*color);
                    if (drawshape_work.flags & 2) {
                        drawshape_gx_texcoord2f32(uv->x, uv->y);
                    }
                }
            }
        } else {
            const nw4r::math::VEC3* v = (const nw4r::math::VEC3*)drawshape_work.vertices;
            for (n = drawshape_work.count; n > 0; n--, v++, color += color_step, uv++) {
                drawshape_gx_position3f32(v->x, v->y, v->z);
                drawshape_gx_color1u32(*color);
                if (drawshape_work.flags & 2) {
                    drawshape_gx_texcoord2f32(uv->x, uv->y);
                }
            }
        }
    } else {
        const _mh_tex_uv_* uv = (const _mh_tex_uv_*)drawshape_work.uvs;
        f32 su = 1.0f / tex_of(&drawshape_work.tex)->GetWidth();
        f32 sv = 1.0f / tex_of(&drawshape_work.tex)->GetHeight();
        if ((drawshape_work.flags & 1) == 0) {
            if ((drawshape_work.flags & 0x10000) == 0) {
                const _mh_ivec2_* v = (const _mh_ivec2_*)drawshape_work.vertices;
                for (n = drawshape_work.count; n > 0; n--, v++, color += color_step, uv++) {
                    drawshape_gx_position2s16(v->x, v->y);
                    drawshape_gx_color1u32(*color);
                    drawshape_gx_texcoord2f32(uv->u * su, uv->v * sv);
                }
            } else {
                const _MH_VEC2* v = (const _MH_VEC2*)drawshape_work.vertices;
                for (n = drawshape_work.count; n > 0; n--, v++, color += color_step, uv++) {
                    drawshape_gx_position2f32(v->x, v->y);
                    drawshape_gx_color1u32(*color);
                    drawshape_gx_texcoord2f32(uv->u * su, uv->v * sv);
                }
            }
        } else {
            const nw4r::math::VEC3* v = (const nw4r::math::VEC3*)drawshape_work.vertices;
            for (n = drawshape_work.count; n > 0; n--, v++, color += color_step, uv++) {
                drawshape_gx_position3f32(v->x, v->y, v->z);
                drawshape_gx_color1u32(*color);
                drawshape_gx_texcoord2f32(uv->u * su, uv->v * sv);
            }
        }
    }
    drawshape_gx_end();
    drawshape_gx_restore();
}

/* 0x80054C64 (0x10): GXPosition2f32. */
extern "C" void drawshape_gx_position2f32(f32 x, f32 y) {
    GXWGFifo.f32 = x;
    GXWGFifo.f32 = y;
}

/* 0x80054C74 (0x190): initialises the GX texture (and TLUT) object of slot `id` and returns both. */
extern "C" void drawshape_tex_obj_get(u16 id, GXTexObj** out_tex, GXTlutObj** out_tlut) {
    u32 tlut_fmt;
    u16 entries;
    void* image;
    u16 width;
    u16 height;
    u32 fmt;
    u32 fmt_ci;
    f32 min_lod;
    f32 max_lod;
    u8 mipmap;
    u32 tex;
    u32 pltt;

    drawshape_tex_wrap_select(id);
    res_tex_ctor(&tex, drawshape_work.slot_tex[id]);
    res_pltt_ctor(&pltt, drawshape_work.slot_pltt[id]);
    *out_tex = drawshape_work.tex_objs;
    *out_tlut = drawshape_work.tlut_objs;
    drawshape_work.wrap_s = drawshape_work.wrap_t = 0;

    if (tex_of(&tex)->IsCIFmt() == 1) {
        if (!pltt_of(&pltt)->IsValid()) {
            *out_tlut = NULL;
            return;
        }
        entries = (u16)pltt_of(&pltt)->GetNumEntries();
        tlut_fmt = pltt_of(&pltt)->GetFmt();
        GXInitTlutObj(*out_tlut, pltt_of(&pltt)->GetPlttData(), tlut_fmt, entries);
        GXLoadTlut(*out_tlut, 0);
        tex_of(&tex)->GetTexObjCIParam(&image, &width, &height, &fmt_ci, &min_lod, &max_lod, &mipmap);
        GXInitTexObjCI(*out_tex, image, width, height, fmt_ci, drawshape_work.wrap_s, drawshape_work.wrap_t, mipmap,
                       0);
    } else {
        *out_tlut = NULL;
        tex_of(&tex)->GetTexObjParam(&image, &width, &height, &fmt, &min_lod, &max_lod, &mipmap);
        GXInitTexObj(*out_tex, image, width, height, fmt, drawshape_work.wrap_s, drawshape_work.wrap_t, mipmap);
    }
}

/* 0x80054E04 (0x1A8): loads the item-icon texture pack into slots 0..2. */
void itemicon_tex_load(void) {
    char path[128];
    EftResFile* desc = itemicon_file;
    u32 file;
    u32 tex;
    u32 pltt;
    u32 file_tmp;
    u32 tex_tmp;
    u32 pltt_tmp;
    u32 buf;
    u32 i;

    cnvt_eur_fname(path, desc->name_0x04);
    if (work_stack_find(itemicon_work_tag) == -1) {
        buf = work_stack_alloc(desc->size_0x00, 0, itemicon_work_tag);
    } else {
        return;
    }
    set_arena_idx(0);
    if (buf != 0) {
        if (load_file(path, buf, desc->size_0x00) != 0) {
            res_file_ctor(&file, 0);
            res_tex_ctor(&tex, 0);
            res_pltt_ctor(&pltt, 0);
            nwAddResource(desc->name_0x04, (void*)buf);
            ResEntry* entry = ckResourceName(desc->name_0x04);
            if (entry != NULL) {
                res_file_assign(&file, res_file_ctor(&file_tmp, (u32)entry->data));
                for (i = 0; i <= 2; i++) {
                    if (itemicon_tex_name_tbl[i] == NULL) {
                        break;
                    }
                    tex_tmp = file_of(&file)->GetResTex(itemicon_tex_name_tbl[i]);
                    res_tex_assign(&tex, &tex_tmp);
                    if (tex_of(&tex)->IsCIFmt()) {
                        pltt_tmp = file_of(&file)->GetResPltt(itemicon_tex_name_tbl[i]);
                        res_pltt_assign(&pltt, &pltt_tmp);
                    }
                    draw_shape_tex_slot_set(&tex, &pltt, i, 0);
                }
                return;
            }
        }
        work_stack_free(buf);
    }
    nw4r::db::Panic("draw_shape.cpp", 1982, "NW4R:Failed assertion 0");
    draw_shape_tex_slots_clear(0, 2);
}

/* 0x80054FAC (0x30): copies one file handle over another and returns the destination. */
extern "C" u32* res_file_assign(u32* dst, u32* src) {
    res_file_word_copy(dst, src);
    return dst;
}

/* 0x80054FDC (0xC) */
extern "C" void res_file_word_copy(u32* dst, u32* src) {
    *dst = *src;
}

/* 0x80054FE8 (0x64): builds a file handle from a 32-byte-aligned file address. */
extern "C" u32* res_file_ctor(u32* dst, u32 file_data) {
    res_file_word_set(dst, file_data);
    if (file_data & 0x1F) {
        nw4r::db::Panic("g3d_resfile_ac.h", 60, "NW4R:Failed assertion !((u32)p & 0x1f)");
    }
    return dst;
}

/* 0x8005504C (0x8) */
extern "C" void res_file_word_set(u32* dst, u32 value) {
    *dst = value;
}

/* 0x80055054 (0x1B0): loads the frame texture pack into slots 3..6. */
void gpframe_tex_load(void) {
    char path[128];
    EftResFile* desc = gpframe_file;
    u32 file;
    u32 tex;
    u32 pltt;
    u32 file_tmp;
    u32 tex_tmp;
    u32 pltt_tmp;
    u32 buf;
    u32 i;

    cnvt_eur_fname(path, desc->name_0x04);
    if (work_stack_find("GPFRAME") == -1) {
        buf = work_stack_alloc(desc->size_0x00, 0, "GPFRAME");
    } else {
        return;
    }
    set_arena_idx(0);
    if (buf != 0) {
        if (load_file(path, buf, desc->size_0x00) != 0) {
            res_file_ctor(&file, 0);
            res_tex_ctor(&tex, 0);
            res_pltt_ctor(&pltt, 0);
            nwAddResource(desc->name_0x04, (void*)buf);
            ResEntry* entry = ckResourceName(desc->name_0x04);
            if (entry != NULL) {
                res_file_assign(&file, res_file_ctor(&file_tmp, (u32)entry->data));
                for (i = 0; i <= 3; i++) {
                    u32 slot = i + 3;
                    if (gpframe_tex_name_tbl[i] == NULL) {
                        break;
                    }
                    tex_tmp = file_of(&file)->GetResTex(gpframe_tex_name_tbl[i]);
                    res_tex_assign(&tex, &tex_tmp);
                    if (tex_of(&tex)->IsCIFmt()) {
                        pltt_tmp = file_of(&file)->GetResPltt(gpframe_tex_name_tbl[i]);
                        res_pltt_assign(&pltt, &pltt_tmp);
                    }
                    draw_shape_tex_slot_set(&tex, &pltt, slot, 0);
                }
                return;
            }
        }
        work_stack_free(buf);
    }
    nw4r::db::Panic("draw_shape.cpp", 2060, "NW4R:Failed assertion 0");
    draw_shape_tex_slots_clear(3, 6);
}

/* 0x80055204 (0x1B0): loads the menu texture pack into slots 7..12. */
void menu_tex_load(void) {
    char path[128];
    EftResFile* desc = menu_file;
    u32 file;
    u32 tex;
    u32 pltt;
    u32 file_tmp;
    u32 tex_tmp;
    u32 pltt_tmp;
    u32 buf;
    u32 i;

    cnvt_eur_fname(path, desc->name_0x04);
    if (work_stack_find("MENU") == -1) {
        buf = work_stack_alloc(desc->size_0x00, 0, "MENU");
    } else {
        return;
    }
    set_arena_idx(0);
    if (buf != 0) {
        if (load_file(path, buf, desc->size_0x00) != 0) {
            res_file_ctor(&file, 0);
            res_tex_ctor(&tex, 0);
            res_pltt_ctor(&pltt, 0);
            nwAddResource(desc->name_0x04, (void*)buf);
            ResEntry* entry = ckResourceName(desc->name_0x04);
            if (entry != NULL) {
                res_file_assign(&file, res_file_ctor(&file_tmp, (u32)entry->data));
                for (i = 0; i <= 5; i++) {
                    u32 slot = i + 7;
                    if (menu_tex_name_tbl[i] == NULL) {
                        break;
                    }
                    tex_tmp = file_of(&file)->GetResTex(menu_tex_name_tbl[i]);
                    res_tex_assign(&tex, &tex_tmp);
                    if (tex_of(&tex)->IsCIFmt()) {
                        pltt_tmp = file_of(&file)->GetResPltt(menu_tex_name_tbl[i]);
                        res_pltt_assign(&pltt, &pltt_tmp);
                    }
                    draw_shape_tex_slot_set(&tex, &pltt, slot, 0);
                }
                return;
            }
        }
        work_stack_free(buf);
    }
    nw4r::db::Panic("draw_shape.cpp", 2140, "NW4R:Failed assertion 0");
    draw_shape_tex_slots_clear(7, 12);
}

/* 0x800553B4 (0x180): requests the cockpit texture pack and the stage's map texture (slot 19). */
extern "C" void draw_shape_stage_load(u8 stage) {
    char path[128];
    EftResFile* file;
    u32 buf;

    set_arena_idx(0);
    file = &cockpit_file_tbl[screen_split_mode_ck()];
    cnvt_eur_fname(path, file->name_0x04);
    buf = work_stack_alloc(file->size_0x00, 0, "COCKPIT");
    if (buf != 0) {
        load_file_req(path, buf, file->size_0x00, (u32)cockpit_tex_load_cb, 0, NULL);
    }
    if (stage != 0xFF) {
        file = &stage_map_file_tbl[stage];
        if (quest_select_ready_ck()) {
            switch ((u16)quest_id_get()) {
            case 1:
                file = &quest_map_file_tbl[0];
                break;
            case 2:
                file = &quest_map_file_tbl[1];
                break;
            case 3:
                file = &quest_map_file_tbl[2];
                break;
            }
        } else if (stage == 9 && (s32)quest_phase_get() == 10) {
            file = map_file_stage9_phase10;
        }
        cnvt_eur_fname(path, file->name_0x04);
        buf = work_stack_alloc(file->size_0x00, 0, "MAP");
        if (buf != 0) {
            load_file_req(path, buf, file->size_0x00, (u32)map_tex_load_cb, 0, NULL);
        }
    } else {
        draw_shape_tex_slots_clear(19, 19);
    }
}

/* 0x80055534 (0x140): the cockpit pack's load callback: fills slots 13..18. */
/* untyped: byte range - the loaded file image */
extern "C" void cockpit_tex_load_cb(char* name, void* data) {
    u32 file;
    u32 tex;
    u32 pltt;
    u32 file_tmp;
    u32 tex_tmp;
    u32 pltt_tmp;
    char** names;
    u32 i;

    res_file_ctor(&file, 0);
    res_tex_ctor(&tex, 0);
    res_pltt_ctor(&pltt, 0);
    if (screen_split_mode_ck()) {
        names = cockpit_split_tex_name_tbl;
    } else {
        names = cockpit_tex_name_tbl;
    }
    nwAddResource(name, data);
    ResEntry* entry = ckResourceName(name);
    if (entry != NULL) {
        res_file_assign(&file, res_file_ctor(&file_tmp, (u32)entry->data));
        for (i = 0; i <= 5; i++) {
            u32 slot = i + 13;
            if (names[i] == NULL) {
                break;
            }
            tex_tmp = file_of(&file)->GetResTex(names[i]);
            res_tex_assign(&tex, &tex_tmp);
            if (tex_of(&tex)->IsCIFmt()) {
                pltt_tmp = file_of(&file)->GetResPltt(names[i]);
                res_pltt_assign(&pltt, &pltt_tmp);
            }
            draw_shape_tex_slot_set(&tex, &pltt, slot, 0);
        }
    } else {
        draw_shape_tex_slots_clear(13, 18);
    }
}

/* 0x80055674 (0xFC): the map texture's load callback: fills slot 19 with the file's first texture. */
/* untyped: byte range - the loaded file image */
extern "C" void map_tex_load_cb(char* name, void* data) {
    u32 file;
    u32 tex;
    u32 pltt;
    u32 file_tmp;
    u32 tex_tmp;
    u32 pltt_tmp;

    res_file_ctor(&file, 0);
    res_tex_ctor(&tex, 0);
    res_pltt_ctor(&pltt, 0);
    nwAddResource(name, data);
    ResEntry* entry = ckResourceName(name);
    if (entry != NULL) {
        res_file_assign(&file, res_file_ctor(&file_tmp, (u32)entry->data));
        tex_tmp = res_file_tex_by_index(&file, 0);
        res_tex_assign(&tex, &tex_tmp);
        if (tex_of(&tex)->IsCIFmt()) {
            pltt_tmp = res_file_pltt_by_index(&file, 0);
            res_pltt_assign(&pltt, &pltt_tmp);
        }
        draw_shape_tex_slot_set(&tex, &pltt, 19, 0);
        return;
    }
    draw_shape_tex_slots_clear(19, 19);
}

/* 0x80055770 (0xC8): requests the quest-finish texture of result kind 4, 6 or 7 (slot 20), freeing the map's. */
extern "C" void quest_fin_tex_load(u8 kind) {
    char path[128];
    EftResFile* file;
    s32 index;
    u32 buf;

    switch (kind) {
    case 4:
        index = 0;
        break;
    case 6:
        index = 1;
        break;
    case 7:
        index = 2;
        break;
    default:
        return;
    }
    s32 handle = work_stack_find("MAP");
    if (handle >= 0) {
        work_stack_free_index(handle);
    }
    file = &quest_fin_file_tbl[index];
    cnvt_eur_fname(path, file->name_0x04);
    buf = work_stack_alloc(file->size_0x00, 0, "QUE_FIN");
    if (buf != 0) {
        load_file_req(path, buf, file->size_0x00, (u32)quest_fin_tex_load_cb, 0, NULL);
    }
}

/* 0x80055838 (0xFC): the quest-finish texture's load callback: fills slot 20 with the file's first texture. */
/* untyped: byte range - the loaded file image */
extern "C" void quest_fin_tex_load_cb(char* name, void* data) {
    u32 file;
    u32 tex;
    u32 pltt;
    u32 file_tmp;
    u32 tex_tmp;
    u32 pltt_tmp;

    res_file_ctor(&file, 0);
    res_tex_ctor(&tex, 0);
    res_pltt_ctor(&pltt, 0);
    nwAddResource(name, data);
    ResEntry* entry = ckResourceName(name);
    if (entry != NULL) {
        res_file_assign(&file, res_file_ctor(&file_tmp, (u32)entry->data));
        tex_tmp = res_file_tex_by_index(&file, 0);
        res_tex_assign(&tex, &tex_tmp);
        if (tex_of(&tex)->IsCIFmt()) {
            pltt_tmp = res_file_pltt_by_index(&file, 0);
            res_pltt_assign(&pltt, &pltt_tmp);
        }
        draw_shape_tex_slot_set(&tex, &pltt, 20, 0);
        return;
    }
    draw_shape_tex_slots_clear(20, 20);
}

/* 0x80055934 (0x190): loads the village texture pack into slots 26..32. */
void village_tex_load(void) {
    char path[128];
    EftResFile* desc = village_file;
    u32 file;
    u32 tex;
    u32 pltt;
    u32 file_tmp;
    u32 tex_tmp;
    u32 pltt_tmp;
    u32 buf;
    u32 i;

    buf = work_stack_alloc(desc->size_0x00, 0, "VILLAGE");
    cnvt_eur_fname(path, desc->name_0x04);
    if (buf != 0) {
        if (load_file(path, buf, desc->size_0x00) != 0) {
            res_file_ctor(&file, 0);
            res_tex_ctor(&tex, 0);
            res_pltt_ctor(&pltt, 0);
            nwAddResource(desc->name_0x04, (void*)buf);
            ResEntry* entry = ckResourceName(desc->name_0x04);
            if (entry != NULL) {
                res_file_assign(&file, res_file_ctor(&file_tmp, (u32)entry->data));
                for (i = 0; i <= 6; i++) {
                    u32 slot = i + 26;
                    if (village_tex_name_tbl[i] == NULL) {
                        break;
                    }
                    tex_tmp = file_of(&file)->GetResTex(village_tex_name_tbl[i]);
                    res_tex_assign(&tex, &tex_tmp);
                    if (tex_of(&tex)->IsCIFmt()) {
                        pltt_tmp = file_of(&file)->GetResPltt(village_tex_name_tbl[i]);
                        res_pltt_assign(&pltt, &pltt_tmp);
                    }
                    draw_shape_tex_slot_set(&tex, &pltt, slot, 0);
                }
                return;
            }
        }
        work_stack_free(buf);
    }
    nw4r::db::Panic("draw_shape.cpp", 2592, "NW4R:Failed assertion 0");
    draw_shape_tex_slots_clear(26, 32);
}

/* 0x80055AC4 (0x198): loads the reward texture pack into resident memory and slots 21..25. */
extern "C" void reward_tex_load(void) {
    char path[128];
    EftResFile* desc = reward_file;
    u32 file;
    u32 tex;
    u32 pltt;
    u32 file_tmp;
    u32 tex_tmp;
    u32 pltt_tmp;
    u32 i;

    cnvt_eur_fname(path, desc->name_0x04);
    s32 slot = pull_res_mem(desc->name_0x04, desc->size_0x00, 1);
    if (slot >= 0) {
        void* buf = getResMemAdrs(slot);
        if (load_file(path, (u32)buf, desc->size_0x00) != 0) {
            res_file_ctor(&file, 0);
            res_tex_ctor(&tex, 0);
            res_pltt_ctor(&pltt, 0);
            nwAddResource(desc->name_0x04, buf);
            ResEntry* entry = ckResourceName(desc->name_0x04);
            if (entry != NULL) {
                res_file_assign(&file, res_file_ctor(&file_tmp, (u32)entry->data));
                for (i = 0; i <= 4; i++) {
                    u32 slot = i + 21;
                    if (reward_tex_name_tbl[i] == NULL) {
                        break;
                    }
                    tex_tmp = file_of(&file)->GetResTex(reward_tex_name_tbl[i]);
                    res_tex_assign(&tex, &tex_tmp);
                    if (tex_of(&tex)->IsCIFmt()) {
                        pltt_tmp = file_of(&file)->GetResPltt(reward_tex_name_tbl[i]);
                        res_pltt_assign(&pltt, &pltt_tmp);
                    }
                    draw_shape_tex_slot_set(&tex, &pltt, slot, 0);
                }
                return;
            }
        }
        push_res_mem(slot);
    }
    nw4r::db::Panic("draw_shape.cpp", 2668, "NW4R:Failed assertion 0");
    draw_shape_tex_slots_clear(21, 25);
}

/* 0x80055C5C (0x68): the two-digit atlas cell of `value` (38-pixel cells, ones across and tens down). */
extern "C" void atlas_digit_uv_get(_mh_tex_uv_* uv0, _mh_tex_uv_* uv1, u8 value) {
    s16 u = (value % 10) * 38;
    uv0->u = u;
    uv0->v = (value / 10) * 38;
    uv1->u = u + 38;
    uv1->v = uv0->v + 38;
}

/* 0x80055CC4 (0x5C): the 28-pixel atlas cell of `value` (eight per row), offset down by the caller's v. */
extern "C" void atlas_cell28_uv_get(_mh_tex_uv_* uv0, _mh_tex_uv_* uv1, u8 value) {
    s16 u = (value % 8) * 28;
    uv0->u = u;
    uv0->v = uv0->v + (value >> 3) * 28;
    uv1->u = u + 28;
    uv1->v = uv0->v + 28;
}

/* 0x80055D20 (0x6C): the 20-pixel atlas cell of `value` (twelve per row, from the second row). */
extern "C" void atlas_cell20_uv_get(_mh_tex_uv_* uv0, _mh_tex_uv_* uv1, u8 value) {
    s16 u = (value % 12) * 20;
    uv0->u = u;
    uv0->v = (value / 12 + 1) * 20;
    uv1->u = u + 20;
    uv1->v = uv0->v + 20;
}

/* 0x80055D8C (0x3C): the 20-pixel icon cell of `value`, its column read from the icon table. */
extern "C" void atlas_icon_uv_get(_mh_tex_uv_* uv0, _mh_tex_uv_* uv1, u8 value) {
    s16 u = atlas_icon_cell_tbl[value] * 20;
    uv0->u = u;
    uv0->v = 0;
    uv1->u = u + 20;
    uv1->v = 20;
}

/* 0x80055DC8 (0x68): the 64-pixel atlas cell of `value` (twelve per row). */
extern "C" void atlas_cell64_uv_get(_mh_tex_uv_* uv0, _mh_tex_uv_* uv1, u8 value) {
    s16 u = (value % 12) << 6;
    uv0->u = u;
    uv0->v = (value / 12) << 6;
    uv1->u = u + 64;
    uv1->v = uv0->v + 64;
}

/* 0x80055E30 (0x10): selects the resource arena the loaders draw from and clears the switch. */
extern "C" void set_arena_idx(u8 index) {
    arena_idx = index;
    arena_idx_switch = 0;
}

/* 0x80055E40 (0x8) */
void set_arena_idx_switch(u8 index) {
    arena_idx_switch = index;
}

/* 0x80055E48 (0x8) */
extern "C" u8 arena_idx_get(void) {
    return arena_idx;
}

/* 0x80055E50 (0x8) */
extern "C" u8 arena_idx_switch_get(void) {
    return arena_idx_switch;
}


/* The static initialiser this TU ends with calls the work block's constructor out of line, and the constructor follows
 * it (0x80055E58, 0x80055E64): the inliner is off when MWCC emits `__sinit_\draw_shape_cpp`. */
#pragma dont_inline on
