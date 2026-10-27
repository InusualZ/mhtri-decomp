/* The declarations `src/draw_shape.cpp` owns (docs/plan.md 6.5 rule 2): the draw-shape state block's
 * setters.  Created when `enemy/fn_80147CE0.cpp` registered as a consumer - the owner had no header.
 * The signature is the owner's own definition (`extern "C" void draw_shape_arm(u32 a, u32 b, u32 c)`,
 * which stores r3 and r5 as words and `(u8)r4`), so a caller with a pointer first argument casts it.
 */
#ifndef MHTRI_DRAW_SHAPE_H
#define MHTRI_DRAW_SHAPE_H

#include "types.h"
#include "nw4r/math.h"

/* One end of `draw_line_2d`'s line (`_DRAW_POINT_2D` in its mangling): the s16 position and the colour.  size: 0x8 */
typedef struct _DRAW_POINT_2D {
    /* +0x00 */ s16 x;
    /* +0x02 */ s16 y;
    /* +0x04 */ u32 color;
} _DRAW_POINT_2D;

/* One entry of the resource-file descriptor arrays the effect loaders walk: a byte size and the file name. */
typedef struct EftResFile {
    /* +0x00 */ u32 size_0x00;
    /* +0x04 */ char* name_0x04;
} EftResFile; /* size: 0x08 */

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80582F10 / 0x80582F40 / 0x80588EE0 / 0x80589988 - the common resource-file descriptors (one `EftResFile` and a
 * zero terminator each): the quest and lobby common `efm` files and the lobby and quest common `eft` files;
 * `eft_common_load` hands them to the loader. */
extern EftResFile q_comn_efm_file[2];
extern EftResFile lobby_eft_mdl_file[2];
extern EftResFile lb_comn_eft_file[2];
extern EftResFile q_comn_eft_file[2];

/* 0x80582D38 / 0x80582E58 / 0x80582EE0 - the face files `fn_80047398.cpp`'s loaders read: the nine male and the
 * nine female `08/skin/*_faceNNN.tpl` records, and the skin files (`m_skin`, `f_skin`, `facemake`). */
extern EftResFile m_face_file_tbl[9];
extern EftResFile f_face_file_tbl[9];
extern EftResFile skin_file_tbl[3];

/* 0x80055E30 - selects the resource arena index the loaders draw from (and clears the switch byte);
 * `arena_resource_load` picks index 1 for the arena texture pack.  `res_file_ctor` (0x80054FE8) builds a
 * `ResFile` handle from a 32-byte-aligned file address (asserting the alignment) and `res_file_assign`
 * (0x80054FAC) copies one handle over another and returns the destination.  Added with
 * `quest/arenatask.cpp` (rule 2: this unit owns the addresses). */
void set_arena_idx(u8 index);
u32* res_file_ctor(u32* dst, u32 file_data);
u32* res_file_assign(u32* dst, u32* src);
/* 0x80056A54 - arm the draw-shape state block (`lbl_8066ACF8`). */
void draw_shape_arm(u32 a, u32 b, u32 c);
/* 0x80056A84 - the effect step's shape request: the position, the handle and the area byte.  Added
 * with `ef/eft053.cpp`, whose state machine fires it (`fn_80056A84(&pos, 10, area)`). */
void fn_80056A84(const nw4r::math::VEC3* pos, u32 handle, u8 flag);

/* 0x800553B4 - loads the stage's shape textures (GUESS name). */
void draw_shape_stage_load(u8 stage);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
/* 0x80054E04 / 0x80055054 / 0x80055204 - load the item-icon, frame and menu texture packs (C++ scope: the map rows
 * are `itemicon_tex_load__Fv`, `gpframe_tex_load__Fv`, `menu_tex_load__Fv`). */
void itemicon_tex_load(void);
void gpframe_tex_load(void);
void menu_tex_load(void);
#endif

#ifdef __cplusplus
extern "C" {
#endif
/* 0x80055E48 - the resource arena index `set_arena_idx` selected (0 the common packs).  GUESS name. */
u8 arena_idx_get(void);
/* 0x80052760 - resets the shape draw queue (the system core's full reset). */
void draw_shape_reset(void);
/* 0x80053994 / 0x80053FF4 - the shape's per-vertex colour array, and the offset and scale its vertices are drawn
 * with (added with `menu/menu_infomation.cpp`'s sharpness bar). */
void drawshape_set_color_array(const u32* colors);
void drawshape_set_scale_ivec2(const s16* offset, const f32* scale);
/* 0x80054C74 - initialises the GX texture (and TLUT) object of slot `id` and returns both (added with the
 * `ef/system_core.cpp` loading display's draw). */
void drawshape_tex_obj_get(u16 id, struct _GXTexObj** out_tex, struct _GXTlutObj** out_tlut);

/* One map kind's population files: the place file and the free-hunt file.  size: 0x10 */
typedef struct EmPopResFiles {
    /* +0x00 */ u32 size_0x00;
    /* +0x04 */ char* name_0x04;
    /* +0x08 */ u32 size_0x08;
    /* +0x0C */ char* name_0x0C;
} EmPopResFiles;
/* 0x8058AC20 (.data) - per map kind, the population files `enemy/em_model.cpp` loads and releases (GUESS name). */
extern EmPopResFiles* stage_pop_res_tbl[12];
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
struct _mh_ivec2_;
struct _mh_tex_uv_;
/* 0x80053860 / 0x80053ACC - the shape's vertex array and its texture with one uv per vertex (C++ scope: the map
 * rows are `drawshape_set_vertex_array__FPC10_mh_ivec2_` and `drawshape_set_texture_array__FUsPC11_mh_tex_uv_`). */
void drawshape_set_vertex_array(const _mh_ivec2_* verts);
void drawshape_set_texture_array(u16 tex_id, const _mh_tex_uv_* uvs);
#endif

#endif /* MHTRI_DRAW_SHAPE_H */
