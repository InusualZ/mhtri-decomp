/*
 * g3d/g3d_state.cpp - the nw4r g3d `G3DState` cluster: GX state writes and the camera matrix table the
 *   `mtxID < NUM_CAMERA` asserts guard.
 * RANGE. .text 0x80084630-0x80088E24 (166 functions); extab, extabindex, .ctors 0x8056F2D4-0x8056F2D8, .rodata
 *   0x8056F6E0-0x8056F710, .data 0x8058F750-0x8058FCE8 (opens on "g3d_state.cpp"), .bss 0x80682E80-0x80688420,
 *   .sdata 0x80791238-0x80791258, .sbss 0x807948F0-0x80794910, .sdata2 0x80795E70-0x80795E98.  Left seam:
 *   `g3d/g3d_scnroot.cpp`'s `.text` ends at 0x80084630 (fn_80084630 is the first function citing "g3d_state.cpp") and
 *   its `.data` fragment ends at 0x8058F74A; the right edge is `g3d/g3d_resvtx.cpp`, whose
 *   fragment opens at 0x8058FCE8.
 * NAMES. Map stems (`fn_`, no mangling to derive from) for the small accessors;
 *   mtx34_inverse is a GUESS (0x800883C4: the 3x4 matrix inverse its body computes).  The material-state cache functions are GUESSES from their
 *   bodies and nw4r's G3DState: g3d_ind_mtx_op_init is a GUESS, g3d_ind_mtx_op_load is a GUESS,
 *   g3d_ind_mtx_op_set is a GUESS (the three indirect matrices),
 *   g3d_tex_coord_scale_load is a GUESS, g3d_state_set_mat_misc is a GUESS, g3d_zcomp_cache_set is a GUESS,
 *   g3d_state_load_tex_obj is a GUESS, g3d_tex_obj_cache_load is a GUESS, g3d_tex_obj_equal is a GUESS,
 *   g3d_state_load_tlut_obj is a GUESS, g3d_tlut_obj_cache_load is a GUESS, g3d_state_set_gen_mode is a GUESS,
 *   g3d_gen_mode_cache_set is a GUESS, g3d_state_load_tev is a GUESS, g3d_tex_coord_scale_set_tex_maps is a GUESS,
 *   g3d_gen_mode_cache_load is a GUESS, g3d_gd_set_gen_mode is a GUESS, g3d_tev_cache_update is a GUESS,
 *   g3d_state_load_mat_pix is a GUESS, g3d_state_load_mat_ind_mtx_dl is a GUESS, g3d_state_load_mat_ind_mtx is a
 *   GUESS; the globals g3d_state_gen_mode_cache, g3d_state_tex_coord_scale_cache, g3d_state_tex_obj_cache,
 *   g3d_state_tlut_obj_cache, g3d_state_dl_dirty, g3d_state_cached_tev, g3d_state_zcomp_cache and
 *   g3d_state_cull_mode_hw are GUESSES from their readers.
 *   g3d_gen_mode_cache_load_full is a GUESS, g3d_gd_set_chan_ctrl_lights is a GUESS,
 *   g3d_gd_set_chan_ctrl_unlit is a GUESS, g3d_gd_set_chan_amb_color is a GUESS,
 *   g3d_state_load_tex_coord_gen is a GUESS, g3d_bits_clear is a GUESS, g3d_bits_set is a GUESS,
 *   g3d_bits_test is a GUESS, g3d_swap_f32 is a GUESS, g3d_f32_ref is a GUESS,
 *   g3d_tex_mtx_func_type_set is a GUESS, g3d_tex_proj_mtx_mode is a GUESS, g3d_tex_proj_build is a GUESS,
 *   g3d_vtx_desc_cache_update is a GUESS, g3d_vtx_desc_copy is a GUESS, g3d_vtx_desc_equal is a GUESS,
 *   g3d_current_mtx_reset_clear is a GUESS, g3d_current_mtx_reset is a GUESS, g3d_tex_mtx_current_set is a GUESS,
 *   g3d_view_mtx_arrays_has_pos is a GUESS, g3d_state_set_view_mtx_arrays is a GUESS,
 *   g3d_view_mtx_arrays_set is a GUESS, g3d_view_mtx_arrays_pos_mtx is a GUESS, g3d_state_get_nrm_mtx is a GUESS,
 *   g3d_view_mtx_arrays_nrm_mtx is a GUESS, g3d_state_get_ind_mtx_hook is a GUESS,
 *   g3d_state_view_mtx_arrays is a GUESS, g3d_state_current_mtx_reset is a GUESS,
 *   g3d_state_ind_mtx_hook is a GUESS (from their bodies, their callers and nw4r's G3DState: the texture projection
 *   table, the view position/normal/texture matrix arrays, the vertex-description cache).
 *   g3d_fog_table_invalidate is a GUESS, g3d_state_get_camera_mtx is a GUESS,
 *   g3d_camera_table_camera_mtx is a GUESS, g3d_state_get_inv_camera_mtx is a GUESS,
 *   g3d_camera_table_inv_camera_mtx is a GUESS, g3d_state_get_camera_mtx_at is a GUESS,
 *   g3d_camera_table_camera_mtx_at is a GUESS, g3d_state_get_proj_tex_mtx is a GUESS,
 *   g3d_state_get_proj_tex_mtx_at is a GUESS, g3d_camera_table_proj_tex_mtx_at is a GUESS,
 *   g3d_state_get_env_tex_mtx is a GUESS, g3d_state_get_render_mode is a GUESS, g3d_state_invalidate is a GUESS,
 *   g3d_dl_dirty_set is a GUESS, g3d_view_mtx_arrays_clear is a GUESS, g3d_zcomp_cache_invalidate is a GUESS,
 *   g3d_tex_mtx_flags_clear is a GUESS, g3d_tex_mtx_func_types_clear is a GUESS,
 *   g3d_vtx_desc_cache_invalidate is a GUESS, g3d_vtx_desc_cache_clear is a GUESS,
 *   g3d_gen_mode_cache_reset is a GUESS, g3d_tev_cache_invalidate is a GUESS,
 *   g3d_tlut_obj_cache_invalidate is a GUESS, g3d_tex_coord_scale_invalidate is a GUESS,
 *   g3d_tex_obj_cache_invalidate is a GUESS, g3d_state_camera_table is a GUESS, g3d_state_fog_table is a GUESS,
 *   g3d_state_light_table is a GUESS, g3d_state_render_mode is a GUESS, g3d_state_vtx_desc_cache is a GUESS,
 *   g3d_state_tex_mtx_func_types is a GUESS, g3d_state_tex_mtx_flags is a GUESS,
 *   g3d_camera_table_proj_tex_mtx is a GUESS, g3d_camera_table_env_tex_mtx is a GUESS,
 *   g3d_zcomp_cache_clear is a GUESS (nw4r's G3DState camera table, Invalidate and its resetters;
 *   g3d_state_invalidate's 0x7FF argument is INVALIDATE_ALL).
 *   g3d_state_load_mat_tev_color is a GUESS (the TEV colour display-list load); g3d_tex_proj_identity is a GUESS,
 *   g3d_distance is a GUESS, g3d_distance_random_access is a GUESS, g3d_find_s32 is a GUESS, g3d_find_s8 is a
 *   GUESS (std::distance/std::find shapes), g3d_state_load_mat_chan is a GUESS, g3d_fog_ref is a GUESS,
 *   g3d_state_load_fog is a GUESS, g3d_fog_table_load is a GUESS, g3d_state_set_light_setting is a GUESS,
 *   g3d_state_get_light_obj is a GUESS, g3d_state_get_light_set_entry is a GUESS, g3d_state_load_light_set is a
 *   GUESS, g3d_light_table_set_setting is a GUESS, g3d_light_table_load_light_set is a GUESS,
 *   g3d_state_load_shp_pre_prim is a GUESS, g3d_state_set_camera is a GUESS, g3d_camera_table_set_camera is a GUESS (nw4r's G3DState light, fog and
 *   camera entry points, by their bodies and callers); G3dLightTable is a GUESS, G3dCameraTable is a GUESS.
 *   G3dIndMtxCallback is a GUESS, G3dIndMtxCallbackStd is a GUESS (nw4r's G3DState IndMtxOp and its standard
 *   implementation: the two vtables' slots, the ITM asserts and the normal-map matrices); its members
 *   Exec is a GUESS, Reset is a GUESS, SetNrmMapMtx is a GUESS (nw4r's IndMtxOp slot names, by their bodies).
 *   g3d_tex_proj_env, g3d_tex_proj_projection and g3d_tex_proj_env_spec are GUESSES (the environment, projection
 *   and specular-environment map projections: entries 1/3, 2 and 4 of the texture projection table);
 *   g3d_tex_env_scale_mtx is a GUESS (their 0.5 scale/offset matrix); g3d_state_set_fog, g3d_fog_table_set_fog,
 *   g3d_state_set_render_mode, g3d_state_load_tex_srt and g3d_state_load_shp_prim are GUESSES (by their bodies);
 *   g3d_state_tex_proj_table, g3d_state_nrm_mtx_scratch(_init) and g3d_state_ind_mtx_callback_std are GUESSES;
 *   G3dLightSetting, G3dFogData, G3dTevCache, G3dCurrentMtxReset, G3dTexMtxFlags and G3dTexMtxFuncTypes are
 *   GUESSES (the types of the light setting, the fog data and the scalar file-scope objects).  The callees' names
 *   light_obj_*, light_setting_*, amb_light_obj_copy, fog_*, camera_get_*, vec3_negate, vec3_to_vec and
 *   g3d_calc_tex_mtx are GUESSES (by their bodies and this unit's calls).
 *   ResGenMode's GXGet*, ResTev's ref/ptr/GetClassName, ResMatChan::ptr and ResTexSrt's
 *   ref/ptr/GetTexMtxMode/GetTexSrtFlag/IsIdentityTexMtx/IsExist, ResShp::IsVtxAttrEnabled
 *   are nw4r's members.
 * RESIDUALS. Every function is written.  G3dLightObj, G3dLightSetting, G3dFog and G3dFogData are this unit's views
 *   of the records g3d_light.cpp, g3d_fog.cpp and g3d_anmscn.cpp define; the owners' headers declare the callees
 *   over them by forward declaration.
 *   g3d_light_table_set_setting, g3d_light_table_load_light_set: register order only (retail numbers the table,
 *     setting and loop registers differently).
 *   g3d_fog_table_load: the fog handle and the zero colour take each other's stack slot (8/12).
 *   G3dRandomAccessTag: its inline constructor is empty on purpose (the tag temporary stays uninitialised, and
 *     retail copies an uninitialised byte).
 *   __ct__14G3dCameraTableFv: register choice only (retail alternates r30/r31 through the member array loops).
 *   G3dIndMtxCallback, G3dIndMtxCallbackStd: the constructors' and destructors' empty bodies are complete (the
 *     compiler emits the vtable stores, the base constructor call and the deleting tail).
 *   G3dIndMtxCallbackStd::SetNrmMapMtx: saves one register more than retail (`_savegpr_25` for `_savegpr_26`, the
 *     index stays in r4 in retail) and reuses the light direction it loaded for the first row where retail reloads
 *     it for setVec3 (tried: every store through `mtx[idx]`, a light-direction reference, the operand order; scoped
 *     common_subs/propagation/lifetimes/scheduling/optimization_level pragmas - none better; `peephole on` is the best).
 *   The map splits 0x800884AC, 0x80088560 and 0x80088700 out of their callers' rows: each is a static function its
 *     caller tail-branches into, and dtk's analysis had merged it into the caller.
 *   g3d_tex_obj_cache_load, g3d_tlut_obj_cache_load: retail recomputes the copied entry's address (`slwi`/`mulli`)
 *     where ours reuses the compare's offset register; tlut also swaps the index and TLUT registers.
 *   extab: G3dLightTable's constructor record carries retail's cleanup actions (the light setting and the light
 *     object array destroyed on unwind), so the members are C++ objects with destructors there; ours calls
 *     light_setting_ctor/__construct_array explicitly and has none.  Reproducing it needs the g3d_light.cpp
 *     constructors and destructors under their C++ manglings (a rename in that Matching unit).  The other extab
 *     records differ only where the register saves do (SetNrmMapMtx, the callback constructor's range).
 * SHAPES. `#pragma fp_contract off` around g3d_state_load_mat_chan: retail keeps `fmuls` + `fadds` for the scaled
 *   ambient.  File-scope `#pragma pool_data off`: retail gives each assert string its own `lis`/`addi`.
 *   g3d_state_load_tex_obj/g3d_state_load_tlut_obj compile under `opt_dead_assignments off` (it forms the by-value
 *   copy's address before the cache's, as retail); SetNrmMapMtx under `peephole on`.
 *   File-scope `#pragma peephole off`: retail keeps every `rlwinm`/`clrlwi` + `cmpwi` and unfused `clrlwi` + `slwi`
 *   pairs.  The generation-mode word is `(ind << 16 | cull << 14) | ((tev - 1) << 10 | (gens | chans << 4))`, that
 *   grouping.
 *   Function order: the definitions are in retail's address order, except the inline `_ac.h` accessors and the
 *   inline helpers retail emits as weak copies: they are defined first, under `#pragma dont_inline on` (playbook
 *   163), and their callers are compiled under `dont_inline` too (a push/pop per caller), so each copy lands right
 *   after its first caller in reverse first-use order.  Their definition order (IsExist, ResTexSrt::ref, the
 *   ResGenMode getters, ResMatChan::ref, ResTev::ref, g3d_fog_ref) gives retail's weak `@STRING@` tail after the
 *   vtables; the texture-projection helpers are `static inline` so their strings stay in the TU's own pool.  The
 *   `_ac.h` ref accessors pass `__FUNCTION__` (an `.sdata` object created ahead of the TU's own data).
 *   The calc_tex_mtx arguments are evaluated right to left, so GetTexSrtFlag's first use precedes GetTexMtxMode's.
 *   g3d_tex_obj_cache_load / g3d_tlut_obj_cache_load index the compared and the copied entry by the `_GXTexMapID`
 *   / `_GXTlut` cast of the loop counter and pass the resource's object first.
 *   The file-scope objects are defined at the end, in retail's `.bss`/`.sbss`/`.sdata` order (the normal-matrix
 *   scratch and its guard between the view matrix arrays and the projection table), with inline constructors and
 *   a closing `#pragma dont_inline on`: MWCC's static initialiser then calls each constructor out of line and emits
 *   the weak copies after it in reverse order, as retail.  G3dIndMtxCallbackStd's destructor body is visible and
 *   empty, so its object gets no `__register_global_object` (retail has none).
 */

#include "types.h"
#include "nw4r/math.h"
#include "gx.h"              /* GXWGFifo, the 0xCC008000 write window (rule 1) */
#include "g3d/g3d_resmat.h"   /* nw4r::g3d::ResGenMode (rule 2) */
#include "g3d/g3d_anmchr.h"    /* TypeObj::GetTypeName and operator==, owned by g3d/g3d_anmchr.cpp (rule 2) */
#include "g3d/g3d_anmscn.h"   /* the cluster declarations, owned by g3d/g3d_anmscn.cpp (rule 2) */
#include "fn_8004CAD8.h"       /* mtx34_identity/MTX34_ctor, owned by fn_8004CAD8.cpp (rule 2) */
#include "g3d/g3d_camera.h"    /* fn_80075390..fn_80075620, owned by g3d/g3d_camera.cpp (rule 2) */
#include "g3d/g3d_draw1mat1shp.h" /* GDWriteXFCmd / g3d_gd_set_chan_mat_color / FPow (rule 2) */
#include "g3d/g3d_scninline.h" /* the scene inline copies, placement new (rule 2) */
#include "g3d/g3d_scnobj.h"     /* nw4r::g3d::ScnGroup, owned by g3d/g3d_scnobj.cpp (rule 2) */
#include "g3d/g3d_resshp.h"     /* nw4r::g3d::ResTev (rule 2) */
#include "g3d/g3d_state.h"
#include "g3d/g3d_ind_mtx_callback.h" /* G3dIndMtxOp, G3dIndMtxCallback (rule 1) */
#include "gx/fn_8009AA78.h"     /* GDSetTexCoordScale2, GDSetGenMode2 (rule 2) */
#include "gx/GDSetIndTexMtx.h"  /* GDSetIndTexMtx, GDResetCurrentMtx (rule 2) */
#include "draw_shape/mtx34_copy.h" /* mtx34_copy (rule 2) */
#include "g3d/g3d_gpu.h"         /* GDSetCurrentMtx, GDLoadTexMtxImm3x3, Array8, Mat33 (rule 2) */
#include "RVLGX/GXAttr.h"
#include "RVLGX/GXLight.h"
#include "g3d/g3d_calcview.h"    /* mtx34_concat (rule 2) */
#include "mh3_pad.h"             /* setVec3 (rule 2) */
#include "nw4r/fn_805012C4.h"    /* nw4r::math::MTX34Zero (rule 2) */
#include "font/gx_tex_obj_copy.h" /* gx_tex_obj_copy (rule 2) */
#include "RVLGX/GXTexture.h"
#include "Runtime.PPCEABI.H/CPlusLibPPC.h" /* __construct_array (rule 2) */
#include "g3d/g3d_fog.h"          /* fog_ctor (rule 2) */
#include "RVLGX/GXPixel.h"        /* GXSetFog (rule 2) */
#include "MSL_C/alloc.h"           /* memcmp (rule 2) */
#include "Runtime.PPCEABI.H/memset.h" /* memset (rule 2) */
#include "MTX/vec.h"              /* C_VECHalfAngle, and PSMTXInverse through MTX/mtx.h (rule 2) */

#pragma peephole off
#pragma pool_data off
#include "RVLGX/GXSetZCompLoc.h"

/* ------------------------------------------------------------------------------------------------ */
/* externs: the SDK and the neighbouring units this one calls (the map owns their names)             */
/* ------------------------------------------------------------------------------------------------ */

namespace nw4r {
namespace db {

/* `Panic(const char* pFile, int line, const char* pFmt, ...)` - the trailing `e` in the map's mangling
 * is MWCC's vararg marker. */
void Panic(const char* pFile, int line, const char* pFmt, ...);

}  // namespace db
}  // namespace nw4r

/* The SDK entry points the target reaches with their own `lis`/`addi`; mtx34_identity/MTX34_ctor and
 * fn_80075390..fn_80075620 come from their owners' headers above. */
extern "C" void fn_80501658(void* p);

#include "g3d/g3d_light.h"       /* light_obj_*, light_setting_*, amb_light_obj_copy (rule 2) */
#include "g3d/g3d_draw.h"        /* light_obj_is_point_light, light_obj_is_specular_light, vec3_negate (rule 2) */
#include "g3d/g3d_dcc.h"         /* g3d_calc_tex_mtx (rule 2) */
#include "g3d/light_obj_is_enable.h" /* light_obj_is_enable, light_setting_get_*, fog_is_valid (rule 2) */
#include "g3d/g3d_camera_types.h" /* nw4r::g3d::Camera::GetCameraMtx (rule 2) */
#include "gx/GXRenderModeObj.h"   /* GXRenderModeObj (rule 1) */
#include "hud/mtx34_inverse_affine.h" /* mtx34_to_mtx33 (rule 2) */
#include "main.h"                 /* render_mode_copy (rule 2) */
#include "RVLGX/GXTransform.h"     /* GXLoadTexMtxImm (rule 2) */

/* The nw4r resource pointer assert: `ptr` must fall in one of the seven mapped Wii memory ranges (the six
 * materialised BOOLs are retail's shape). */
#define G3D_STATE_POINTER_ASSERT(ptr, line, msg)                                               \
    {                                                                                          \
        BOOL ok1_ = TRUE, ok2_ = TRUE, ok3_ = TRUE, ok4_ = TRUE, ok5_ = TRUE, ok6_ = TRUE;      \
        u32 top_ = (u32)(ptr) & 0xFF000000u;                                                    \
        if (!(top_ == 0x80000000u) && !(((u32)(ptr) & 0xFF800000u) == 0x81000000u))             \
            ok6_ = FALSE;                                                                        \
        if (!ok6_ && !(((u32)(ptr) & 0xF8000000u) == 0x90000000u))                              \
            ok5_ = FALSE;                                                                        \
        if (!ok5_ && !(top_ == 0xC0000000u))                                                    \
            ok4_ = FALSE;                                                                        \
        if (!ok4_ && !(((u32)(ptr) & 0xFF800000u) == 0xC1000000u))                              \
            ok3_ = FALSE;                                                                        \
        if (!ok3_ && !(((u32)(ptr) & 0xF8000000u) == 0xD0000000u))                              \
            ok2_ = FALSE;                                                                        \
        if (!ok2_ && !(((u32)(ptr) & 0xFFFFC000u) == 0xE0000000u))                              \
            ok1_ = FALSE;                                                                        \
        if (!ok1_)                                                                              \
            nw4r::db::Panic("g3d_state.cpp", line, msg, (ptr));                                     \
    }

/* ------------------------------------------------------------------------------------------------ */
/* types                                                                                             */
/* ------------------------------------------------------------------------------------------------ */

/* The 4-byte-head resource handle `fn_8008569C`/`fn_800856CC` hand back the interior pointer of:
 * a `ResCommon<T>`-shaped wrapper whose payload starts one word in. */
struct G3DResRef {
    /* +0x00 */ u32 mUnk00;
    /* +0x04 */ u32 mPayload;
}; /* size: 0x8 (approximation - only the two words are touched by this unit) */

/* One word/byte of the state the small accessors below read. */
struct StateWord {
    /* +0x00 */ u32 mWord;
}; /* size: 0x4 (approximation) */

/* The iterator category the distance helper dispatches on (an empty tag passed by value).  size: 0x1 */
struct G3dRandomAccessTag {
    G3dRandomAccessTag() {}
};

/* The display-list dirty flag (`g3d_state_dl_dirty`): set when a cached state changed, read-and-cleared by the next
 * display-list call.  size: 0x4 */
struct G3dDlDirty {
    G3dDlDirty();
    /* +0x00 */ bool dirty;
    /* +0x01 */ u8 pad_0x01[3];
};

struct StateByte {
    /* +0x00 */ u8 mByte;
}; /* size: 0x1 (approximation) */

struct StateByte3 {
    /* +0x00 */ u8 pad_00[3];
    /* +0x03 */ u8 mByte;
}; /* size: 0x4 (approximation) */

struct StateByte1 {
    /* +0x00 */ u8 pad_00[1];
    /* +0x01 */ u8 mByte;
}; /* size: 0x2 (approximation) */

/* The three-word value `fn_80086138` copies and `fn_80086154` compares. */
struct StateWord3 {
    /* +0x00 */ u32 mWords[3];
}; /* size: 0xC (approximation) */

/* One 4-byte `{u16, u16}` pair of the table at +0x04 (`fn_80085F3C`): a texture's width and height. */
struct StatePair {
    /* +0x00 */ u16 mA;
    /* +0x02 */ u16 mB;
}; /* size: 0x4 */

/* One texture-coordinate scale the cache writes (`g3d_tex_coord_scale_load`). */
struct StateCoordScale {
    /* +0x00 */ u16 s;
    /* +0x02 */ u16 t;
    /* +0x04 */ u8 pad_0x04[4];
}; /* size: 0x8 */

/* The texture-coordinate scale cache (`g3d_state_tex_coord_scale_cache`): the loaded textures' sizes, the scales
 * derived from them and the TEV's coordinate-to-texture-map table.  size: 0x74 */
struct StatePairTable {
    StatePairTable();
    /* +0x00 */ u32 mFlags;
    /* +0x04 */ StatePair mPairs[8];
    /* +0x24 */ StateCoordScale scale[8];
    /* +0x64 */ union {
        u8 texMapID[8];
        u32 texMapWord[2];
    };
    /* +0x6C */ u8 pad_0x6C[8];
};

/* The texture-object cache (`g3d_state_tex_obj_cache`): the loaded objects and a valid bit per map.
 * size: 0x120 */
struct G3dTexObjCache {
    G3dTexObjCache();
    /* +0x000 */ GXTexObj texObj[8];
    /* +0x100 */ u8 mMask;
    /* +0x101 */ u8 pad_0x101[0x1F];
};

/* The TLUT-object cache (`g3d_state_tlut_obj_cache`): the loaded objects and a valid bit per slot.  size: 0x64 */
struct G3dTlutObjCache {
    G3dTlutObjCache();
    /* +0x00 */ GXTlutObj tlut[8];
    /* +0x60 */ u16 validMask;
    /* +0x62 */ u8 pad_0x62[2];
};

/* The state object's flag word carrier, read by `fn_800856A4`/`fn_800856B8`. */
struct StateFlags {
    /* +0x00 */ u32 mFlags;
}; /* size: 0x4 (approximation) */

/* One light object of the light table (`fn_8008540C` constructs 0x80 of them, `fn_800856D4` indexes them).
 * size: 0x44 */
struct G3dLightObj {
    /* +0x00 */ u8 mData[0x44];
};

/* One light set of the light table: the light objects it selects and its ambient light (-1: none).  size: 0xC */
struct G3dLightSet {
    /* +0x00 */ s8 idxLight[8];
    /* +0x08 */ s8 idxAmbLight;
    /* +0x09 */ u8 pad_0x09[3];
};

/* The light setting a light table starts with (`light_setting_ctor` builds it over the table's buffers): the
 * counts and the light objects, ambient colours and light sets.  size: 0x10 */
struct G3dLightSetting {
    /* +0x00 */ u16 numLightObj;
    /* +0x02 */ u16 numLightSet;
    /* +0x04 */ G3dLightObj* pLightObj;
    /* +0x08 */ GXColor* pAmbLight;
    /* +0x0C */ G3dLightSet* pLightSet;
};

/* The light table (`g3d_state_light_table`): the light setting's header, the loaded light set and the masks and
 * light indices it produced, the 128 light objects and the 128 light sets.  size: 0x2A2C */
struct G3dLightTable {
    G3dLightTable();
    /* +0x0000 */ G3dLightSetting setting;
    /* +0x0010 */ s32 loadedLightSet;      /* -1: none (`fn_80085478`); selects the row `fn_80085B54` reads */
    /* +0x0014 */ u32 maskDiffColor;
    /* +0x0018 */ u32 maskDiffAlpha;
    /* +0x001C */ u32 maskSpecColor;
    /* +0x0020 */ u32 maskSpecAlpha;
    /* +0x0024 */ s8 loadedLightIdx[8];    /* -1: none */
    /* +0x002C */ G3dLightObj lightObj[0x80];
    /* +0x222C */ GXColor ambLight[0x80];
    /* +0x242C */ G3dLightSet lightSet[0x80];
};

/* The object `fn_80085344` sets the validity byte of. */
struct StateValidFlag {
    /* +0x00 */ u8 mValid;
}; /* size: 0x1 (approximation) */

/* ------------------------------------------------------------------------------------------------ */
/* The material-state caches: the indirect matrices, the z-compare location, the texture/TLUT object  */
/* caches, the generation mode and the TEV/pixel/indirect display lists                              */
/* ------------------------------------------------------------------------------------------------ */

/* The cached TEV (`g3d_state_cached_tev`): the TEV block last loaded.  size: 0x4 */
struct G3dTevCache {
    G3dTevCache();
    /* +0x00 */ u32 tev;
};

/* The cached z-compare location (`lbl_80794908`).  size: 0x8 */
struct G3dZCompCache {
    G3dZCompCache();
    /* +0x00 */ u32 flags;
    /* +0x04 */ u8 beforeTex;
};

/* The cached generation mode (`lbl_80682E80`): the four counts and the cull mode `ResGenMode` holds, and the
 * dirty/valid flags.  size: 0xC */
struct G3dGenModeCache {
    G3dGenModeCache();
    /* +0x00 */ u8 numTexGens;
    /* +0x01 */ u8 numChans;
    /* +0x02 */ u8 numTevStages;
    /* +0x03 */ u8 numIndStages;
    /* +0x04 */ s32 cullMode;
    /* +0x08 */ u32 flags;
};

extern "C" {

extern G3dGenModeCache g3d_state_gen_mode_cache;
extern StatePairTable g3d_state_tex_coord_scale_cache;
extern G3dTexObjCache g3d_state_tex_obj_cache;
extern G3dTlutObjCache g3d_state_tlut_obj_cache;
extern G3dDlDirty g3d_state_dl_dirty;
extern G3dTevCache g3d_state_cached_tev;
extern G3dZCompCache g3d_state_zcomp_cache;
extern const u8 g3d_state_cull_mode_hw[4];
/* The hardware cull mode of each GXCullMode (front and back swapped). */
const u8 g3d_state_cull_mode_hw[4] = {0, 2, 1, 3};

} /* extern "C" */

/* A scene-dependent texture projection: builds `pMtx` from camera `refCamera` and light `refLight`. */
typedef void (*G3dTexProjFunc)(nw4r::math::MTX34* pMtx, s8 refCamera, s8 refLight);

/* One entry of the texture-projection table: the function that builds the projection matrix and the
 * texture-matrix mode the projection needs.  size: 0x8 */
struct G3dTexProjEntry {
    /* +0x00 */ G3dTexProjFunc func;
    /* +0x04 */ s32 mtxMode;
};

/* The texture-projection table, indexed by map mode.  size: 0x800 */
struct G3dTexProjTable {
    G3dTexProjTable();
    /* +0x000 */ G3dTexProjEntry entry[0x100];
};

/* The shape's vertex description the pre-primitive display list was last loaded with.  size: 0xC */
struct G3dVtxDescCache {
    G3dVtxDescCache();
    /* +0x00 */ u32 word[3];
};

/* Whether the current matrix indices are the reset ones (`g3d_state_current_mtx_reset`).  size: 0x1 */
struct G3dCurrentMtxReset {
    G3dCurrentMtxReset();
    /* +0x00 */ u8 reset;
};

/* The texture matrices loaded as the identity, a bit per matrix (`g3d_state_tex_mtx_flags`).  size: 0x4 */
struct G3dTexMtxFlags {
    G3dTexMtxFlags();
    /* +0x00 */ u32 flags;
};

/* The eight texture matrices' scene-dependent function types (`g3d_state_tex_mtx_func_types`).  size: 0x20 */
struct G3dTexMtxFuncTypes {
    G3dTexMtxFuncTypes();
    /* +0x00 */ s32 funcType[8];
};

/* The view matrix arrays the indexed position/normal/texture loads read (`g3d_state_view_mtx_arrays`).
 * size: 0xC */
struct G3dViewMtxArrays {
    G3dViewMtxArrays();
    /* +0x00 */ nw4r::math::MTX34* pViewPosMtx;
    /* +0x04 */ Mat33* pViewNrmMtx;
    /* +0x08 */ nw4r::math::MTX34* pViewTexMtx;
};

extern "C" {

extern G3dViewMtxArrays g3d_state_view_mtx_arrays;
extern G3dCurrentMtxReset g3d_state_current_mtx_reset;
extern G3dIndMtxCallback* g3d_state_ind_mtx_hook;
extern s8 g3d_state_nrm_mtx_scratch_init;   /* the scratch matrix below is constructed */
extern Mat33 g3d_state_nrm_mtx_scratch;     /* g3d_view_mtx_arrays_nrm_mtx's result without a normal array */

} /* extern "C" */

/* ------------------------------------------------------------------------------------------------ */
/* The camera matrix table and the state invalidation                                               */
/* ------------------------------------------------------------------------------------------------ */

/* The camera matrix table (`g3d_state_camera_table`): a cached inverse of the current camera matrix and, per camera,
 * its view matrix, its projection and the two texture projection matrices built from it.  size: 0x1A34 */
struct G3dCameraTable {
    G3dCameraTable();

    /* +0x0000 */ u16 flags;            /* bit 0: invCameraMtx is the current camera's inverse */
    /* +0x0002 */ u16 currentCamera;
    /* +0x0004 */ nw4r::math::MTX34 invCameraMtx;
    /* +0x0034 */ nw4r::math::MTX34 cameraMtx[32];
    /* +0x0634 */ nw4r::math::MTX44 projMtx[32];
    /* +0x0E34 */ nw4r::math::MTX34 projTexMtx[32];
    /* +0x1434 */ nw4r::math::MTX34 envTexMtx[32];
};

/* The fog data a `G3dFog` handle points at: `GXSetFog`'s arguments and the range adjustment `fog_set_gp` hands to
 * `GXSetFogRangeAdj`.  size: 0x30 */
struct G3dFogData {
    /* +0x00 */ u32 fogType;
    /* +0x04 */ f32 startZ;
    /* +0x08 */ f32 endZ;
    /* +0x0C */ f32 nearZ;
    /* +0x10 */ f32 farZ;
    /* +0x14 */ GXColor color;
    /* +0x18 */ u8 adjEnable;
    /* +0x19 */ u8 pad_0x19;
    /* +0x1A */ u16 adjCenter;
    /* +0x1C */ u16 adjTable[10];
};

/* The fog table (`g3d_state_fog_table`): the valid flags, the loaded fog's index and the 32 fogs.  size: 0x608 */
struct G3dFogTable {
    G3dFogTable();
    /* +0x000 */ u32 flags;
    /* +0x004 */ s32 fogIdx;
    /* +0x008 */ G3dFogData fog[32];
};

extern "C" {

extern G3dCameraTable g3d_state_camera_table;
extern G3dFogTable g3d_state_fog_table;
extern G3dLightTable g3d_state_light_table;
extern GXRenderModeObj g3d_state_render_mode;
extern G3dVtxDescCache g3d_state_vtx_desc_cache;
extern G3dTexMtxFuncTypes g3d_state_tex_mtx_func_types;
extern G3dTexMtxFlags g3d_state_tex_mtx_flags;
extern G3dTexProjTable g3d_state_tex_proj_table;

/* Whether `idx` names one of the 32 cameras. */
static inline BOOL g3d_camera_idx_valid(u32 idx) {
    BOOL valid = FALSE;
    if (idx <= 31) {
        valid = TRUE;
    }
    return valid;
}

} /* extern "C" */

/* ------------------------------------------------------------------------------------------------ */
/* The default indirect-matrix callback                                                             */
/* ------------------------------------------------------------------------------------------------ */

/* The callback `g3d_state_ind_mtx_hook` points at by default: up to three normal-map matrices built from a light
 * direction and the model's normal matrix, handed to the indirect-matrix operation when enabled.  size: 0x98 */
class G3dIndMtxCallbackStd : public G3dIndMtxCallback {
public:
    G3dIndMtxCallbackStd();
    virtual void Exec(G3dIndMtxOp* pOp);
    virtual ~G3dIndMtxCallbackStd();
    virtual void Reset();
    virtual void SetNrmMapMtx(_GXIndTexMtxID id, const nw4r::math::VEC3* pLightDir,
                              const nw4r::math::MTX34* pNrmMtx,
                              nw4r::g3d::ResMatMiscData::IndirectMethod method);

    /* +0x04 */ u8 enabled[3];   /* per GX_ITM_0..GX_ITM_2: mtx[] holds a matrix to load */
    /* +0x07 */ u8 pad_0x07;
    /* +0x08 */ nw4r::math::MTX34 mtx[3];
};

extern "C" {

/* The environment map's scale and offset: texture coordinates from the [-1, 1] view directions. */
static const nw4r::math::MTX34 g3d_tex_env_scale_mtx = {{
    {0.5f, 0.0f, 0.0f, 0.5f},
    {0.0f, -0.5f, 0.0f, 0.5f},
    {0.0f, 0.0f, 0.0f, 1.0f},
}};

} /* extern "C" */

/* The light setting `g3d_state_set_light_setting` copies in; only used through the pointer. */
struct G3dLightSetting;

/* ------------------------------------------------------------------------------------------------ */
/* The functions in address order; the `_ac.h` accessors and the helpers retail emits as weak copies */
/* come first, so each copy follows its first caller                                               */
/* ------------------------------------------------------------------------------------------------ */

extern "C" {

void g3d_tex_proj_env(nw4r::math::MTX34* pM, s8 refCamera, s8 refLight);
void g3d_tex_proj_projection(nw4r::math::MTX34* pM, s8 refCamera, s8 refLight);
void g3d_tex_proj_env_spec(nw4r::math::MTX34* pM, s8 refCamera, s8 refLight);
void g3d_tex_proj_identity(nw4r::math::MTX34* pM, s8 refCamera, s8 refLight);
G3dIndMtxOp* g3d_ind_mtx_op_init(G3dIndMtxOp* pSelf, nw4r::g3d::ResMatIndMtxAndScale ind);
void g3d_ind_mtx_op_load(G3dIndMtxOp* pSelf);
void g3d_ind_mtx_op_set(G3dIndMtxOp* pSelf, s32 id, nw4r::math::MTX34* pMtx);
void fn_80085344(StateValidFlag* pSelf);
void g3d_tex_coord_scale_load(StatePairTable* pSelf, u8 count);
void fn_80085478(G3dLightTable* pSelf);
void g3d_light_table_set_setting(G3dLightTable* pSelf, const G3dLightSetting* pSetting);
u32* fn_8008569C(G3DResRef* pSelf);
BOOL fn_800856A4(StateFlags* pSelf);
BOOL fn_800856B8(StateFlags* pSelf);
u32* fn_800856CC(G3DResRef* pSelf);
G3dLightObj* fn_800856D4(G3dLightTable* pSelf, u32 idx);
void g3d_light_table_load_light_set(G3dLightTable* pSelf, s32 idx, u32* pMaskDiffColor, u32* pMaskDiffAlpha,
                                    u32* pMaskSpecColor, u32* pMaskSpecAlpha, GXColor* pAmb);
s32 g3d_distance(const s8* first, const s8* last);
s32 g3d_distance_random_access(const s8* first, const s8* last, G3dRandomAccessTag);
s8* g3d_find_s32(s8* p, s8* pEnd, const s32& value);
s8* g3d_find_s8(s8* p, s8* pEnd, const s8& delim);
u8 fn_80085B54(G3dLightTable* pSelf, u32 byteIdx);
void g3d_camera_table_set_camera(G3dCameraTable* pSelf, const nw4r::g3d::Camera& camera, u32 id, bool bUpdate);
void g3d_state_set_mat_misc(nw4r::g3d::ResMatMisc misc);
void g3d_zcomp_cache_set(G3dZCompCache* pSelf, u8 beforeTex);
void g3d_state_load_tex_obj(nw4r::g3d::ResTexObj texObj);
void g3d_tex_obj_cache_load(G3dTexObjCache* pSelf, const nw4r::g3d::ResTexObj texObj);
void fn_80085F3C(StatePairTable* pSelf, u32 idx, u16 a, u16 b);
BOOL g3d_tex_obj_equal(const GXTexObj* pA, const GXTexObj* pB);
void g3d_state_load_tlut_obj(nw4r::g3d::ResTlutObj tlutObj);
void g3d_tlut_obj_cache_load(G3dTlutObjCache* pSelf, nw4r::g3d::ResTlutObj tlutObj);
void fn_80086118(G3dTexObjCache* pSelf, u32 bit);
void fn_80086138(StateWord3* pDst, const StateWord3* pSrc);
BOOL fn_80086154(const StateWord3* pA, const StateWord3* pB);
void g3d_state_set_gen_mode(nw4r::g3d::ResGenMode genMode);
void g3d_gen_mode_cache_set(G3dGenModeCache* pSelf, nw4r::g3d::ResGenMode genMode);
void g3d_state_load_tev(nw4r::g3d::ResTev tev);
void fn_80086610(StatePairTable* pSelf, u8 arg);
u8 fn_80086640(StateByte* pSelf);
void g3d_tex_coord_scale_set_tex_maps(StatePairTable* pSelf, nw4r::g3d::ResTev tev);
bool fn_80086770(G3dDlDirty* pSelf);
void fn_800867A0(G3dDlDirty* pSelf);
void g3d_gen_mode_cache_load(G3dGenModeCache* pSelf);
void g3d_gd_set_gen_mode(u8 numTexGens, u8 numChans, u8 numTevStages, u8 numIndStages, u32 cullMode);
void fn_800868A0(u32 value);
void fn_800868D8(u32 value);
void fn_800868E4(u32 value);
BOOL g3d_tev_cache_update(u32* pCached, nw4r::g3d::ResTev tev);
void g3d_state_load_mat_pix(nw4r::g3d::ResMatPix pix);
void g3d_state_load_mat_tev_color(nw4r::g3d::ResMatTevColor tevColor);
void g3d_state_load_mat_ind_mtx_dl(nw4r::g3d::ResMatIndMtxAndScale ind);
u8 fn_80086AA0(StateByte3* pSelf);
void g3d_state_load_mat_ind_mtx(nw4r::g3d::ResMatIndMtxAndScale ind, G3dIndMtxCallback* pCallback);
void g3d_state_load_mat_chan(const nw4r::g3d::ResMatChan chan, u32 maskColor0, u32 maskAlpha0, u32 maskColor1,
                             u32 maskAlpha1, GXColor amb, BOOL bLightOff);
inline void g3d_gen_mode_cache_load_full(G3dGenModeCache* pSelf);
inline u8 fn_80086FFC(StateByte1* pSelf);
inline void g3d_gd_set_chan_ctrl_lights(u32 chan, u32 ctrl, u32 lightMask);
inline void g3d_gd_set_chan_ctrl_unlit(u32 chan, u32 ctrl, u32 lightMask);
inline void g3d_gd_set_chan_amb_color(u32 chan, GXColor color);
void g3d_state_load_tex_coord_gen(nw4r::g3d::ResMatTexCoordGen texCoordGen);
void g3d_state_load_tex_srt(const nw4r::g3d::ResTexSrt srt);
inline void g3d_bits_clear(u32* pBits, u32 bit);
inline void g3d_bits_set(u32* pBits, u32 bit);
inline BOOL g3d_bits_test(const u32* pBits, u32 bit);
inline void g3d_swap_f32(f32* pA, f32* pB);
inline f32* g3d_f32_ref(f32* p);
static inline void g3d_tex_mtx_func_type_set(s32* pFuncTypes, u32 idx, s32 funcType);
static inline s32 g3d_tex_proj_mtx_mode(G3dTexProjTable* pSelf, u32 idx);
static inline void g3d_tex_proj_build(G3dTexProjTable* pSelf, u32 idx, nw4r::math::MTX34* pOut, s8 refCamera, s8 refLight);
void g3d_state_load_shp_pre_prim(const nw4r::g3d::ResShp shp);
bool g3d_vtx_desc_cache_update(G3dVtxDescCache* pSelf, const G3dVtxDescCache* pDesc);
void g3d_vtx_desc_copy(G3dVtxDescCache* pDst, const G3dVtxDescCache* pSrc);
BOOL g3d_vtx_desc_equal(const G3dVtxDescCache* pA, const G3dVtxDescCache* pB);
void g3d_state_load_shp_prim(const nw4r::g3d::ResShp shp, const nw4r::math::MTX34* pViewPos,
                             const nw4r::math::MTX34* pViewNrm);
void g3d_current_mtx_reset_clear(u8* pFlag);
void g3d_current_mtx_reset(u8* pFlag);
void g3d_tex_mtx_current_set(const s32* pModes, s32 mtxIdx);
BOOL g3d_view_mtx_arrays_has_pos(const G3dViewMtxArrays* pSelf);
void g3d_state_set_view_mtx_arrays(nw4r::math::MTX34* pPos, Mat33* pNrm, nw4r::math::MTX34* pTex);
void g3d_view_mtx_arrays_set(G3dViewMtxArrays* pSelf, nw4r::math::MTX34* pPos, Mat33* pNrm,
                             nw4r::math::MTX34* pTex);
nw4r::math::MTX34* g3d_view_mtx_arrays_pos_mtx(G3dViewMtxArrays* pSelf, u32 idx);
Mat33* g3d_state_get_nrm_mtx(u32 idx);
Mat33* g3d_view_mtx_arrays_nrm_mtx(G3dViewMtxArrays* pSelf, u32 idx);
G3dIndMtxCallback* g3d_state_get_ind_mtx_hook(void);
void g3d_state_set_fog(G3dFog fog, u32 idx);
void g3d_fog_table_set_fog(G3dFogTable* pSelf, G3dFog fog, u32 idx);
inline void g3d_fog_table_invalidate(G3dFogTable* pSelf);
inline G3dFogData* g3d_fog_ref(const G3dFog* pFog);
void g3d_state_load_fog(s32 idx);
void g3d_fog_table_load(G3dFogTable* pSelf, s32 idx);
void g3d_state_set_light_setting(const G3dLightSetting* pSetting);
G3dLightObj* g3d_state_get_light_obj(u32 idx);
s8 g3d_state_get_light_set_entry(u32 idx);
void g3d_state_load_light_set(s32 idx, u32* pMaskDiffColor, u32* pMaskDiffAlpha, u32* pMaskSpecColor,
                              u32* pMaskSpecAlpha, GXColor* pAmb);
void g3d_state_set_camera(const nw4r::g3d::Camera& camera, u32 id, bool bUpdate);
nw4r::math::MTX34* g3d_state_get_camera_mtx(void);
nw4r::math::MTX34* g3d_camera_table_camera_mtx(G3dCameraTable* pSelf);
nw4r::math::MTX34* g3d_state_get_inv_camera_mtx(void);
nw4r::math::MTX34* g3d_camera_table_inv_camera_mtx(G3dCameraTable* pSelf);
u32 mtx34_inverse(MTX34* pOut, const MTX34* pSrc);
nw4r::math::MTX34* g3d_state_get_camera_mtx_at(u32 idx);
nw4r::math::MTX34* g3d_camera_table_camera_mtx_at(G3dCameraTable* pSelf, u32 idx);
nw4r::math::MTX34* g3d_state_get_proj_tex_mtx(void);
static nw4r::math::MTX34* g3d_camera_table_proj_tex_mtx(G3dCameraTable* pSelf);
nw4r::math::MTX34* g3d_state_get_proj_tex_mtx_at(u32 idx);
nw4r::math::MTX34* g3d_camera_table_proj_tex_mtx_at(G3dCameraTable* pSelf, u32 idx);
nw4r::math::MTX34* g3d_state_get_env_tex_mtx(void);
static nw4r::math::MTX34* g3d_camera_table_env_tex_mtx(G3dCameraTable* pSelf);
void g3d_state_set_render_mode(const GXRenderModeObj* pRenderMode);
struct RenderModeObj* g3d_state_get_render_mode(void);
void g3d_state_invalidate(u32 flag);
void g3d_dl_dirty_set(G3dDlDirty* pSelf);
void g3d_view_mtx_arrays_clear(G3dViewMtxArrays* pSelf);
void g3d_zcomp_cache_invalidate(G3dZCompCache* pSelf);
static void g3d_zcomp_cache_clear(G3dZCompCache* pSelf);
void g3d_tex_mtx_flags_clear(u32* pFlags);
void g3d_tex_mtx_func_types_clear(s32* pFuncTypes);
void g3d_vtx_desc_cache_invalidate(G3dVtxDescCache* pSelf);
void g3d_vtx_desc_cache_clear(G3dVtxDescCache* pSelf);
void g3d_gen_mode_cache_reset(G3dGenModeCache* pSelf);
void g3d_tev_cache_invalidate(u32* pCached);
void g3d_tlut_obj_cache_invalidate(G3dTlutObjCache* pSelf);
void g3d_tex_coord_scale_invalidate(StatePairTable* pSelf);
void g3d_tex_obj_cache_invalidate(G3dTexObjCache* pSelf);

} /* extern "C" */

/* The inline accessors and helpers, defined ahead of their callers and kept out of line: each weak copy lands after
 * its first caller, and their strings come out in reverse definition order after the vtables. */
#pragma push
#pragma dont_inline on

/* 0x800878DC (0x9C): whether coordinate `id` has a transform. */
inline BOOL nw4r::g3d::ResTexSrt::IsExist(u32 id) const {
    if (!IsValid()) {
        nw4r::db::Panic("g3d_resmat_ac.h", 0x71, "NW4R:Failed assertion IsValid()");
    }
    if (IsValid()) {
        return ((1 << (id * 4)) & ptr()->flagTexSrt) != 0;
    }
    return FALSE;
}

/* 0x80087870 (0x64): returns the texture-SRT block, panicking on a NULL handle. */
inline const nw4r::g3d::ResTexSrtData& nw4r::g3d::ResTexSrt::ref() const {
    if (!IsValid()) {
        nw4r::db::Panic("g3d_resmat_ac.h", 0x6B, "%s::%s: Object not valid.", GetClassName(), __FUNCTION__);
    }
    return *ptr();
}

/* 0x800878D4 (0x8): returns the texture-SRT block. */
inline const nw4r::g3d::ResTexSrtData* nw4r::g3d::ResTexSrt::ptr() const {
    return static_cast<const ResTexSrtData*>(mpData);
}

/* 0x8008749C (0x24): returns the texture-matrix mode. */
inline u32 nw4r::g3d::ResTexSrt::GetTexMtxMode() const {
    return ref().texMtxMode;
}

/* 0x800874C0 (0x3C): returns coordinate `id`'s four transform flag bits. */
inline u32 nw4r::g3d::ResTexSrt::GetTexSrtFlag(u32 id) const {
    return (ref().flagTexSrt >> (id * 4)) & 0xF;
}

/* 0x800874FC (0x84): whether coordinate `id` has no transform and an identity effect matrix. */
inline BOOL nw4r::g3d::ResTexSrt::IsIdentityTexMtx(u32 id) const {
    BOOL result = FALSE;
    if (((ref().flagTexSrt >> (id * 4)) & 0xF) == 0xF && (ref().effect[id].misc_flag & 1) != 0) {
        result = TRUE;
    }
    return result;
}

extern "C" {

/* 0x800873E4 (0x1C): clears bit `bit` of the word. */
inline void g3d_bits_clear(u32* pBits, u32 bit) {
    *pBits &= ~(1 << bit);
}

/* 0x80087400 (0x18): sets bit `bit` of the word. */
inline void g3d_bits_set(u32* pBits, u32 bit) {
    *pBits |= 1 << bit;
}

/* 0x80087418 (0x20): whether bit `bit` of the word is set. */
inline BOOL g3d_bits_test(const u32* pBits, u32 bit) {
    return (*pBits & (1 << bit)) != 0;
}

/* 0x80087498 (0x4): returns its argument. */
inline f32* g3d_f32_ref(f32* p) {
    return p;
}

/* 0x80087438 (0x60): swaps the two floats. */
inline void g3d_swap_f32(f32* pA, f32* pB) {
    f32 tmp = *g3d_f32_ref(pA);
    *pA = *g3d_f32_ref(pB);
    *pB = *g3d_f32_ref(&tmp);
}

/* 0x80087580 (0x90): records texture matrix `idx`'s scene-dependent function type. */
static inline void g3d_tex_mtx_func_type_set(s32* pFuncTypes, u32 idx, s32 funcType) {
    if (idx >= 8) {
        nw4r::db::Panic("g3d_state.cpp", 0x363, "NW4R:Failed assertion idx >= 0 && idx < 8");
    }
    if (funcType >= 4) {
        nw4r::db::Panic("g3d_state.cpp", 0x364,
                        "NW4R:Failed assertion funcType < MAX_SCNDEPENDENT_TEXMTX_FUNCTYPE");
    }
    pFuncTypes[idx] = funcType;
}

/* 0x80087610 (0x70): returns the texture-matrix mode projection `idx` needs. */
static inline s32 g3d_tex_proj_mtx_mode(G3dTexProjTable* pSelf, u32 idx) {
    if (idx >= 0x100) {
        nw4r::db::Panic("g3d_state.cpp", 0x469, "NW4R:Failed assertion idx < NUM_SCNDEPENDENT_TEXMTX_FUNCTYPE");
    }
    if (idx >= 0x100) {
        idx = 0;
    }
    return pSelf->entry[idx].mtxMode;
}

/* 0x80087680 (0x1F0): builds projection `idx`'s matrix into `pOut` from camera `refCamera` and light `refLight`. */
static inline void g3d_tex_proj_build(G3dTexProjTable* pSelf, u32 idx, nw4r::math::MTX34* pOut, s8 refCamera, s8 refLight) {
    if (idx >= 0x100) {
        nw4r::db::Panic("g3d_state.cpp", 0x459, "NW4R:Failed assertion idx < NUM_SCNDEPENDENT_TEXMTX_FUNCTYPE");
    }
    if (refCamera >= 0x20) {
        nw4r::db::Panic("g3d_state.cpp", 0x45A, "NW4R:Failed assertion ref_camera < NUM_CAMERA");
    }
    if (refLight >= 0x80) {
        nw4r::db::Panic("g3d_state.cpp", 0x45B, "NW4R:Failed assertion ref_light < NUM_LIGHT");
    }
    G3D_STATE_POINTER_ASSERT(pOut, 0x45C, "NW4R:Pointer Error\npOut(=%p) is not valid pointer.");
    if (idx >= 0x100) {
        idx = 0;
    }
    if (refCamera >= 0x20) {
        refCamera = -1;
    }
    if (refLight >= 0x80) {
        refLight = -1;
    }
    if (pOut != NULL) {
        pSelf->entry[idx].func(pOut, refCamera, refLight);
    }
}

} /* extern "C" */

/* 0x8008631C (0x74): returns the generation mode's cullMode, 3 for an empty handle. */
inline _GXCullMode nw4r::g3d::ResGenMode::GXGetCullMode() const {
    if (!IsValid()) {
        nw4r::db::Panic("g3d_resmat_ac.h", 0xDF, "NW4R:Failed assertion IsValid()");
    }
    if (IsValid()) {
        return (_GXCullMode)ptr()->cullMode;
    }
    return (_GXCullMode)3;
}

/* 0x80086390 (0x8): returns the generation-mode block. */
inline const nw4r::g3d::ResGenModeData* nw4r::g3d::ResGenMode::ptr() const {
    return mpData;
}

/* 0x80086398 (0x74): returns the generation mode's nInds, 0 for an empty handle. */
inline u8 nw4r::g3d::ResGenMode::GXGetNumIndStages() const {
    if (!IsValid()) {
        nw4r::db::Panic("g3d_resmat_ac.h", 0xD8, "NW4R:Failed assertion IsValid()");
    }
    if (IsValid()) {
        return ptr()->nInds;
    }
    return 0;
}

/* 0x8008640C (0x74): returns the generation mode's nTevs, 0 for an empty handle. */
inline u8 nw4r::g3d::ResGenMode::GXGetNumTevStages() const {
    if (!IsValid()) {
        nw4r::db::Panic("g3d_resmat_ac.h", 0xD1, "NW4R:Failed assertion IsValid()");
    }
    if (IsValid()) {
        return ptr()->nTevs;
    }
    return 0;
}

/* 0x80086480 (0x74): returns the generation mode's nChans, 0 for an empty handle. */
inline u8 nw4r::g3d::ResGenMode::GXGetNumChans() const {
    if (!IsValid()) {
        nw4r::db::Panic("g3d_resmat_ac.h", 0xCA, "NW4R:Failed assertion IsValid()");
    }
    if (IsValid()) {
        return ptr()->nChans;
    }
    return 0;
}

/* 0x800864F4 (0x74): returns the generation mode's nTexGens, 0 for an empty handle. */
inline u8 nw4r::g3d::ResGenMode::GXGetNumTexGens() const {
    if (!IsValid()) {
        nw4r::db::Panic("g3d_resmat_ac.h", 0xC3, "NW4R:Failed assertion IsValid()");
    }
    if (IsValid()) {
        return ptr()->nTexGens;
    }
    return 0;
}

/* 0x80087078 (0x64): returns the channel block, panicking on a NULL handle. */
inline const nw4r::g3d::ResMatChanData& nw4r::g3d::ResMatChan::ref() const {
    if (!IsValid()) {
        nw4r::db::Panic("g3d_resmat_ac.h", 0x1D1, "%s::%s: Object not valid.", GetClassName(), __FUNCTION__);
    }
    return *ptr();
}

/* 0x800870DC (0x8): returns the channel block. */
inline const nw4r::g3d::ResMatChanData* nw4r::g3d::ResMatChan::ptr() const {
    return mpData;
}

extern "C" {

/* 0x80086F94 (0x68): writes the cached generation mode in full when it is complete and either half is stale. */
inline void g3d_gen_mode_cache_load_full(G3dGenModeCache* pSelf) {
    if ((pSelf->flags & 4) != 0 && (pSelf->flags & 3) != 3) {
        GDSetGenMode2(pSelf->numTexGens, pSelf->numChans, pSelf->numTevStages, pSelf->numIndStages,
                      pSelf->cullMode);
        pSelf->flags |= 3;
    }
}

inline u8 fn_80086FFC(StateByte1* pSelf) {
    return pSelf->mByte;
}

/* 0x80087004 (0x30): writes a colour channel's control word with its light mask (XF 0x100E + chan). */
inline void g3d_gd_set_chan_ctrl_lights(u32 chan, u32 ctrl, u32 lightMask) {
    ctrl = (ctrl & ~0x783C) | ((lightMask & 0xF) << 2);
    ctrl |= ((lightMask >> 4) & 0xF) << 11;
    GDWriteXFCmd((chan & 3) + 0x100E, ctrl);
}

/* 0x80087034 (0x30): writes the same word with the lighting enable bit cleared too. */
inline void g3d_gd_set_chan_ctrl_unlit(u32 chan, u32 ctrl, u32 lightMask) {
    ctrl = (ctrl & ~0x783E) | ((lightMask & 0xF) << 2);
    ctrl |= ((lightMask >> 4) & 0xF) << 11;
    GDWriteXFCmd((chan & 3) + 0x100E, ctrl);
}

/* 0x80087064 (0x14): writes a colour channel's ambient colour (XF 0x100A + chan). */
inline void g3d_gd_set_chan_amb_color(u32 chan, GXColor color) {
    GDWriteXFCmd((chan & 1) + 0x100A, *(const u32*)&color);
}

} /* extern "C" */

/* 0x800866FC (0x64): returns the TEV block, panicking on a NULL handle. */
inline const nw4r::g3d::ResTevData& nw4r::g3d::ResTev::ref() const {
    if (!IsValid()) {
        nw4r::db::Panic("g3d_restev_ac.h", 0x25, "%s::%s: Object not valid.", GetClassName(), __FUNCTION__);
    }
    return *ptr();
}

/* 0x80086760 (0x8): returns the TEV block. */
inline const nw4r::g3d::ResTevData* nw4r::g3d::ResTev::ptr() const {
    return mpData;
}

/* 0x80086768 (0x8): returns the class name. */
inline const char* nw4r::g3d::ResTev::GetClassName() {
    return "ResTev";
}

extern "C" {

/* 0x8008812C (0xC): forgets which fogs are loaded. */
inline void g3d_fog_table_invalidate(G3dFogTable* pSelf) {
    pSelf->flags = 0;
}

/* 0x80088138 (0x54): returns the fog's data, asserting the handle is set. */
inline G3dFogData* g3d_fog_ref(const G3dFog* pFog) {
    if (pFog->mpData == NULL) {
        nw4r::db::Panic("g3d_rescommon_ac.h", 0x8F, "NW4R:Pointer must not be NULL (mpData)");
    }
    return pFog->mpData;
}

} /* extern "C" */
#pragma pop

extern "C" {

/* 0x80084630 (0x3E4): the environment map projection: seen from camera `refCamera` when it is set, otherwise
 * along light `refLight`'s direction (its position for a point light), else the current environment matrix. */
void g3d_tex_proj_env(nw4r::math::MTX34* pM, s8 refCamera, s8 refLight) {
    G3D_STATE_POINTER_ASSERT(pM, 0x31, "NW4R:Pointer Error\npM(=%p) is not valid pointer.");
    if (pM != NULL) {
        s32 lightIdx = g3d_state_get_light_set_entry(refLight);
        if ((u8)refCamera <= 31) {
            nw4r::math::MTX34* pInvCamera = g3d_state_get_inv_camera_mtx();
            mtx34_concat(pM, g3d_state_get_camera_mtx_at(refCamera), pInvCamera);
            pM->m[0][3] = pM->m[1][3] = pM->m[2][3] = 0.0f;
            mtx34_concat(pM, g3d_state_get_env_tex_mtx(), pM);
        } else if ((u8)lightIdx <= 127 && light_obj_is_enable((const nw4r::g3d::LightObj*)g3d_state_get_light_obj(lightIdx))) {
            nw4r::math::VEC3 dir;
            nw4r::math::VEC3 up;
            nw4r::math::VEC3 xAxis;
            nw4r::math::VEC3 yAxis;
            VEC3_ctor(&dir);
            VEC3_ctor(&up);
            VEC3_ctor(&xAxis);
            VEC3_ctor(&yAxis);
            G3dLightObj* pLight = g3d_state_get_light_obj(lightIdx);
            light_obj_get_light_dir((const nw4r::g3d::LightObj*)pLight, &dir);
            nw4r::math::MTX34* pInvCamera = g3d_state_get_inv_camera_mtx();
            if (light_obj_is_point_light(pLight) || (0.0f == dir.x && 0.0f == dir.y && 0.0f == dir.z)) {
                light_obj_get_light_pos((const nw4r::g3d::LightObj*)pLight, &dir);
                mtx34_rotate_vec3(&dir, pInvCamera, &dir);
                nw4r::math::VEC3 neg;
                vec3_negate(&neg, &dir);
                copyVec3(&dir, &neg);
                if (0.0f == dir.x && 0.0f == dir.y && 0.0f == dir.z) {
                    dir.y = -1.0f;
                }
            } else {
                mtx34_rotate_vec3(&dir, pInvCamera, &dir);
            }
            vec3_normalize_into(&dir, &dir);
            if (abs_f32(dir.x) < 0.000001f && abs_f32(dir.z) < 0.000001f) {
                up.x = up.y = 0.0f;
                if (dir.y <= 0.0f) {
                    up.z = -1.0f;
                } else {
                    up.z = 1.0f;
                }
            } else {
                up.x = up.z = 0.0f;
                up.y = 1.0f;
            }
            vec3_cross((f32*)&xAxis, (const f32*)&dir, (const f32*)&up);
            vec3_normalize_into(&xAxis, &xAxis);
            vec3_cross((f32*)&yAxis, (const f32*)&xAxis, (const f32*)&dir);
            pM->m[0][0] = xAxis.x;
            pM->m[0][1] = xAxis.y;
            pM->m[0][2] = xAxis.z;
            pM->m[1][0] = yAxis.x;
            pM->m[1][1] = yAxis.y;
            pM->m[1][2] = yAxis.z;
            pM->m[2][0] = -dir.x;
            pM->m[2][1] = -dir.y;
            pM->m[2][2] = -dir.z;
            mtx34_concat(pM, pM, pInvCamera);
            pM->m[0][3] = pM->m[1][3] = pM->m[2][3] = 0.0f;
            mtx34_concat(pM, &g3d_tex_env_scale_mtx, pM);
        } else {
            mtx34_copy_ps(pM, g3d_state_get_env_tex_mtx());
        }
    }
}

/* 0x80084A14 (0x188): the projection map: camera `refCamera`'s texture projection seen from the current camera,
 * else the current camera's own. */
void g3d_tex_proj_projection(nw4r::math::MTX34* pM, s8 refCamera, s8 refLight) {
    G3D_STATE_POINTER_ASSERT(pM, 0xA8, "NW4R:Pointer Error\npM(=%p) is not valid pointer.");
    if (pM != NULL) {
        if ((u8)refCamera <= 31) {
            nw4r::math::MTX34* pInvCamera = g3d_state_get_inv_camera_mtx();
            mtx34_concat(pM, g3d_state_get_camera_mtx_at(refCamera), pInvCamera);
            mtx34_concat(pM, g3d_state_get_proj_tex_mtx_at(refCamera), pM);
        } else {
            mtx34_copy_ps(pM, g3d_state_get_proj_tex_mtx());
        }
    }
}

/* 0x80084B9C (0x4A0): the specular environment map: oriented along the half vector between the view direction and
 * light `refLight` (or camera `refCamera`'s), else the current environment matrix. */
void g3d_tex_proj_env_spec(nw4r::math::MTX34* pM, s8 refCamera, s8 refLight) {
    G3D_STATE_POINTER_ASSERT(pM, 0xC7, "NW4R:Pointer Error\npM(=%p) is not valid pointer.");
    if (pM != NULL) {
        nw4r::math::VEC3 half;
        nw4r::math::VEC3 up;
        nw4r::math::VEC3 eye;
        nw4r::math::VEC3 dir;
        nw4r::math::VEC3 camDir;
        nw4r::math::VEC3 xAxis;
        nw4r::math::VEC3 yAxis;
        nw4r::math::VEC3 negPos;
        nw4r::math::VEC3 negDir;
        VEC3_ctor(&half);
        VEC3_ctor(&up);
        VEC3_ctor(&eye);
        nw4r::math::MTX34* pCamera = g3d_state_get_camera_mtx();
        eye.x = -pCamera->m[2][0];
        eye.y = -pCamera->m[2][1];
        eye.z = -pCamera->m[2][2];
        nw4r::math::MTX34* pInvCamera = g3d_state_get_inv_camera_mtx();
        s32 lightIdx = g3d_state_get_light_set_entry(refLight);
        if ((u8)lightIdx <= 127 && light_obj_is_enable((const nw4r::g3d::LightObj*)g3d_state_get_light_obj(lightIdx))) {
            VEC3_ctor(&dir);
            G3dLightObj* pLight = g3d_state_get_light_obj(lightIdx);
            light_obj_get_light_dir((const nw4r::g3d::LightObj*)pLight, &dir);
            if (light_obj_is_point_light(pLight) || (0.0f == dir.x && 0.0f == dir.y && 0.0f == dir.z)) {
                light_obj_get_light_pos((const nw4r::g3d::LightObj*)pLight, &dir);
                mtx34_rotate_vec3(&dir, pInvCamera, &dir);
                vec3_negate(&negPos, &dir);
                copyVec3(&dir, &negPos);
                if (0.0f == dir.x && 0.0f == dir.y && 0.0f == dir.z) {
                    dir.y = -1.0f;
                }
            } else {
                mtx34_rotate_vec3(&dir, pInvCamera, &dir);
            }
            if (!light_obj_is_specular_light(pLight)) {
                C_VECHalfAngle(vec3_to_vec(&eye), vec3_to_vec(&dir), vec3_to_vec(&half));
            } else {
                vec3_negate(&negDir, &dir);
                copyVec3(&half, &negDir);
            }
            if (abs_f32(half.x) < 0.000001f && abs_f32(half.z) < 0.000001f) {
                up.x = up.y = 0.0f;
                if (half.y <= 0.0f) {
                    up.z = -1.0f;
                } else {
                    up.z = 1.0f;
                }
            } else {
                up.x = up.z = 0.0f;
                up.y = 1.0f;
            }
        } else if ((u8)refCamera <= 31) {
            nw4r::math::MTX34* pRefCamera = g3d_state_get_camera_mtx_at(refCamera);
            up.x = pRefCamera->m[1][0];
            up.y = pRefCamera->m[1][1];
            up.z = pRefCamera->m[1][2];
            setVec3(&camDir, -pRefCamera->m[2][0], -pRefCamera->m[2][1], -pRefCamera->m[2][2]);
            C_VECHalfAngle(vec3_to_vec(&eye), vec3_to_vec(&camDir), vec3_to_vec(&half));
        } else {
            mtx34_copy_ps(pM, g3d_state_get_env_tex_mtx());
            return;
        }
        vec3_normalize_into(&half, &half);
        VEC3_ctor(&xAxis);
        VEC3_ctor(&yAxis);
        vec3_cross((f32*)&xAxis, (const f32*)&up, (const f32*)&half);
        vec3_normalize_into(&xAxis, &xAxis);
        vec3_cross((f32*)&yAxis, (const f32*)&half, (const f32*)&xAxis);
        pM->m[0][0] = xAxis.x;
        pM->m[0][1] = xAxis.y;
        pM->m[0][2] = xAxis.z;
        pM->m[1][0] = yAxis.x;
        pM->m[1][1] = yAxis.y;
        pM->m[1][2] = yAxis.z;
        pM->m[2][0] = half.x;
        pM->m[2][1] = half.y;
        pM->m[2][2] = half.z;
        mtx34_concat(pM, pM, pInvCamera);
        pM->m[0][3] = pM->m[1][3] = pM->m[2][3] = 0.0f;
        mtx34_concat(pM, g3d_state_get_env_tex_mtx(), pM);
    }
}

/* 0x8008503C (0x124): the projection of a map mode with no scene dependency: the identity. */
void g3d_tex_proj_identity(nw4r::math::MTX34* pM, s8 refCamera, s8 refLight) {
    G3D_STATE_POINTER_ASSERT(pM, 0x140, "NW4R:Pointer Error\npM(=%p) is not valid pointer.");
    if (pM != NULL) {
        mtx34_identity(pM);
    }
}

/* Reads the material's three indirect matrices into the operation, one flag bit per matrix found. */
G3dIndMtxOp* g3d_ind_mtx_op_init(G3dIndMtxOp* pSelf, nw4r::g3d::ResMatIndMtxAndScale ind) {
    nw4r::math::MTX34* pMtx = pSelf->mtx;
    do {
        MTX34_ctor(pMtx);
        pMtx++;
    } while (pMtx < &pSelf->mtx[3]);
    pSelf->flags = 0;
    if (ind.GXGetIndTexMtx(GX_ITM_0, &pSelf->mtx[0])) {
        pSelf->flags |= 1;
    }
    if (ind.GXGetIndTexMtx(GX_ITM_1, &pSelf->mtx[1])) {
        pSelf->flags |= 2;
    }
    if (ind.GXGetIndTexMtx(GX_ITM_2, &pSelf->mtx[2])) {
        pSelf->flags |= 4;
    }
    return pSelf;
}

/* Writes the operation's flagged matrices to the display list. */
void g3d_ind_mtx_op_load(G3dIndMtxOp* pSelf) {
    if (pSelf->flags & 1) {
        GDSetIndTexMtx(0, (const f32*)&pSelf->mtx[0]);
    }
    if (pSelf->flags & 2) {
        GDSetIndTexMtx(3, (const f32*)&pSelf->mtx[1]);
    }
    if (pSelf->flags & 4) {
        GDSetIndTexMtx(6, (const f32*)&pSelf->mtx[2]);
    }
}

/* Replaces the operation's matrix `id` (1..3) and flags it. */
void g3d_ind_mtx_op_set(G3dIndMtxOp* pSelf, s32 id, nw4r::math::MTX34* pMtx) {
    if (id == 1) {
        mtx34_copy(&pSelf->mtx[0], pMtx);
        pSelf->flags |= 1;
    } else if (id == 2) {
        mtx34_copy(&pSelf->mtx[1], pMtx);
        pSelf->flags |= 2;
    } else if (id == 3) {
        mtx34_copy(&pSelf->mtx[2], pMtx);
        pSelf->flags |= 4;
    }
}

void fn_80085344(StateValidFlag* pSelf) {
    pSelf->mValid = 1;
}

/* Writes the texture-coordinate scales of the first `count` coordinates from the sizes of their textures. */
void g3d_tex_coord_scale_load(StatePairTable* pSelf, u8 count) {
    u8 i;
    for (i = 0; i < count; i++) {
        u8 map = pSelf->texMapID[i];
        if (map != 0xFF) {
            pSelf->scale[i].s = pSelf->mPairs[map].mA;
            pSelf->scale[i].t = pSelf->mPairs[pSelf->texMapID[i]].mB;
            GDSetTexCoordScale2(i, pSelf->scale[i].s, 0, 0, pSelf->scale[i].t, 0, 0);
        }
    }
    pSelf->mFlags |= 1;
}

/* 0x8008540C (0x6C): constructs the light table: its light setting over the table's own buffers, the 128 light
 * objects, and no light set loaded. */
G3dLightTable::G3dLightTable() {
    light_setting_ctor(this, (s32)lightObj, (s32)ambLight, 0x80, (s32)lightSet, 0x80);
    __construct_array(lightObj, (void*)light_obj_ctor, (void*)light_obj_dtor, sizeof(G3dLightObj), 0x80);
    fn_80085478(this);
}

/* Re-initialise one camera-state entry: the -1 markers and the cleared words. */
void fn_80085478(G3dLightTable* pSelf) {
    pSelf->loadedLightSet = -1;
    pSelf->maskDiffColor = 0;
    pSelf->maskDiffAlpha = 0;
    pSelf->maskSpecColor = 0;
    pSelf->maskSpecAlpha = 0;
    pSelf->loadedLightIdx[7] = -1;
    pSelf->loadedLightIdx[6] = -1;
    pSelf->loadedLightIdx[5] = -1;
    pSelf->loadedLightIdx[4] = -1;
    pSelf->loadedLightIdx[3] = -1;
    pSelf->loadedLightIdx[2] = -1;
    pSelf->loadedLightIdx[1] = -1;
    pSelf->loadedLightIdx[0] = -1;
}

/* 0x800854B8 (0x1E4): copies `pSetting` into the table, moves its lights into view space and loads the first eight
 * enabled lights into the GX light slots. */
void g3d_light_table_set_setting(G3dLightTable* pSelf, const G3dLightSetting* pSetting) {
    BOOL bResult = ((nw4r::g3d::LightSetting*)&pSelf->setting)->Import(*(const nw4r::g3d::LightSetting*)pSetting);
    if (!bResult) {
        nw4r::db::Panic("g3d_state.cpp", 0x501, "NW4R:Failed assertion bResult");
    }
    nw4r::math::MTX34* pCamera = g3d_state_get_camera_mtx();
    G3D_STATE_POINTER_ASSERT(pCamera, 0x504, "NW4R:Pointer Error\npCamera(=%p) is not valid pointer.");
    ((nw4r::g3d::LightSetting*)&pSelf->setting)
        ->ApplyViewMtx(*pCamera, light_setting_get_num_light_obj((const nw4r::g3d::LightSetting*)pSetting));
    fn_80085478(pSelf);
    u32 numLightObj = light_setting_get_num_light_obj((const nw4r::g3d::LightSetting*)&pSelf->setting);
    u32 slot = 0;
    for (u32 i = 0; slot < 8 && i < numLightObj; i++) {
        G3dLightObj* pLight =
            &((G3dLightObj*)light_setting_get_light_obj_array((const nw4r::g3d::LightSetting*)&pSelf->setting))[i];
        if (light_obj_is_enable((const nw4r::g3d::LightObj*)pLight)) {
            pSelf->loadedLightIdx[slot] = (s8)i;
            GXLoadLightObjImm((const GXLightObj*)fn_8008569C((G3DResRef*)pLight), 1 << slot);
            slot++;
        }
    }
}

u32* fn_8008569C(G3DResRef* pSelf) {
    return &pSelf->mPayload;
}

/* `!(flags & 0x20)` - the target's `rlwinm`/`cntlzw`/`srwi` triple, so the masked value is
 * materialised first (inlining the test lets MWCC fold it to `extrwi`/`xori`). */
BOOL fn_800856A4(StateFlags* pSelf) {
    u32 masked = pSelf->mFlags & 0x20;
    return masked == 0 ? TRUE : FALSE;
}

/* `!(flags & 0x10)`, the same shape as fn_800856A4. */
BOOL fn_800856B8(StateFlags* pSelf) {
    u32 masked = pSelf->mFlags & 0x10;
    if (masked == 0) {
        return TRUE;
    }
    return FALSE;
}

u32* fn_800856CC(G3DResRef* pSelf) {
    return &pSelf->mPayload;
}

/* The bounds-tested table accessor.  The target puts the NULL path last, so the guard is spelled
 * `if (idx <= 0x7F) { return ...; } return NULL;`. */
G3dLightObj* fn_800856D4(G3dLightTable* pSelf, u32 idx) {
    if (idx <= 0x7F) {
        return &pSelf->lightObj[idx];
    }
    return NULL;
}

#pragma push
#pragma dont_inline on
/* 0x800856F4 (0x3D8): loads light set `idx` into the GX light slots (keeping lights already loaded) and returns its
 * four light masks and its ambient colour; past the table, no lights and a white ambient. */
void g3d_light_table_load_light_set(G3dLightTable* pSelf, s32 idx, u32* pMaskDiffColor, u32* pMaskDiffAlpha,
                                    u32* pMaskSpecColor, u32* pMaskSpecAlpha, GXColor* pAmb) {
    if ((u32)idx > 127) {
        if (pMaskDiffColor != NULL) {
            *pMaskDiffColor = 0;
        }
        if (pMaskDiffAlpha != NULL) {
            *pMaskDiffAlpha = 0;
        }
        if (pMaskSpecColor != NULL) {
            *pMaskSpecColor = 0;
        }
        if (pMaskSpecAlpha != NULL) {
            *pMaskSpecAlpha = 0;
        }
        if (pAmb != NULL) {
            pAmb->r = pAmb->g = pAmb->b = pAmb->a = 0xFF;
        }
        return;
    }
    if (pAmb != NULL) {
        s32 idxAmb = pSelf->lightSet[idx].idxAmbLight;
        if (idxAmb < 0) {
            pAmb->r = pAmb->g = pAmb->b = pAmb->a = 0xFF;
        } else {
            *(nw4r::g3d::AmbLightObj*)pAmb = *(const nw4r::g3d::AmbLightObj*)&pSelf->ambLight[idxAmb];
        }
    }
    if (pSelf->loadedLightSet == idx) {
        if (pMaskDiffColor != NULL) {
            *pMaskDiffColor = pSelf->maskDiffColor;
        }
        if (pMaskDiffAlpha != NULL) {
            *pMaskDiffAlpha = pSelf->maskDiffAlpha;
        }
        if (pMaskSpecColor != NULL) {
            *pMaskSpecColor = pSelf->maskSpecColor;
        }
        if (pMaskSpecAlpha != NULL) {
            *pMaskSpecAlpha = pSelf->maskSpecAlpha;
        }
        return;
    }
    G3dLightSet* pSet = &pSelf->lightSet[idx];
    pSelf->maskDiffColor = 0;
    pSelf->maskDiffAlpha = 0;
    pSelf->maskSpecColor = 0;
    pSelf->maskSpecAlpha = 0;
    pSelf->loadedLightSet = idx;
    for (u32 i = 0; i < 8; i++) {
        s8 lightID = pSet->idxLight[i];
        if (lightID < 0 || !light_obj_is_enable((const nw4r::g3d::LightObj*)&pSelf->lightObj[lightID])) {
            continue;
        }
        u32 lightMask;
        s8* pLoaded = g3d_find_s8(pSelf->loadedLightIdx, &pSelf->loadedLightIdx[8], lightID);
        if (pLoaded == &pSelf->loadedLightIdx[8]) {
            s8* pFree = g3d_find_s32(pSelf->loadedLightIdx, &pSelf->loadedLightIdx[8], -1);
            if (pFree != &pSelf->loadedLightIdx[8]) {
                lightMask = 1 << g3d_distance(pSelf->loadedLightIdx, pFree);
                GXLoadLightObjImm((const GXLightObj*)fn_800856CC((G3DResRef*)&pSelf->lightObj[lightID]), lightMask);
                *pFree = lightID;
            } else {
                u32 j;
                for (j = 0; j < 8; j++) {
                    if (g3d_find_s8(pSet->idxLight, &pSet->idxLight[8], pSelf->loadedLightIdx[j]) ==
                        &pSet->idxLight[8]) {
                        break;
                    }
                }
                if (j == 8) {
                    nw4r::db::Panic("g3d_state.cpp", 0x60E, "NW4R:Failed assertion j != 8");
                }
                lightMask = 1 << j;
                GXLoadLightObjImm((const GXLightObj*)fn_800856CC((G3DResRef*)&pSelf->lightObj[lightID]), lightMask);
                pSelf->loadedLightIdx[j] = lightID;
            }
        } else {
            lightMask = 1 << g3d_distance(pSelf->loadedLightIdx, pLoaded);
        }
        if (light_obj_is_specular_light(&pSelf->lightObj[lightID])) {
            if (fn_800856B8((StateFlags*)&pSelf->lightObj[lightID])) {
                pSelf->maskSpecColor |= lightMask;
            }
            if (fn_800856A4((StateFlags*)&pSelf->lightObj[lightID])) {
                pSelf->maskSpecAlpha |= lightMask;
            }
        } else {
            if (fn_800856B8((StateFlags*)&pSelf->lightObj[lightID])) {
                pSelf->maskDiffColor |= lightMask;
            }
            if (fn_800856A4((StateFlags*)&pSelf->lightObj[lightID])) {
                pSelf->maskDiffAlpha |= lightMask;
            }
        }
    }
    if (pMaskDiffColor != NULL) {
        *pMaskDiffColor = pSelf->maskDiffColor;
    }
    if (pMaskDiffAlpha != NULL) {
        *pMaskDiffAlpha = pSelf->maskDiffAlpha;
    }
    if (pMaskSpecColor != NULL) {
        *pMaskSpecColor = pSelf->maskSpecColor;
    }
    if (pMaskSpecAlpha != NULL) {
        *pMaskSpecAlpha = pSelf->maskSpecAlpha;
    }
}
#pragma pop

/* 0x80085ACC (0x2C): returns the number of bytes from `first` to `last`. */
s32 g3d_distance(const s8* first, const s8* last) {
    return g3d_distance_random_access(first, last, G3dRandomAccessTag());
}

/* 0x80085AF8 (0x8): returns the number of bytes from `first` to `last` (the random-access form). */
s32 g3d_distance_random_access(const s8* first, const s8* last, G3dRandomAccessTag) {
    return last - first;
}

/* The two byte-scan loops of the state's key tables: walk `p` up to `pEnd` while the byte does not
 * match, and return where it stopped. */
s8* g3d_find_s32(s8* p, s8* pEnd, const s32& value) {
    while (p != pEnd && *p != value) {
        p++;
    }
    return p;
}

s8* g3d_find_s8(s8* p, s8* pEnd, const s8& delim) {
    while (p != pEnd && *p != delim) {
        p++;
    }
    return p;
}

u8 fn_80085B54(G3dLightTable* pSelf, u32 byteIdx) {
    return pSelf->lightSet[pSelf->loadedLightSet].idxLight[byteIdx];
}

} /* extern "C" */

/* 0x80085B6C (0x104): constructs the camera table: every matrix the identity, camera 0 current. */
G3dCameraTable::G3dCameraTable() {
    MTX34_ctor(&invCameraMtx);
    nw4r::math::MTX34* pMtx = cameraMtx;
    do {
        MTX34_ctor(pMtx);
        pMtx++;
    } while (pMtx < &cameraMtx[32]);
    nw4r::math::MTX44* pProj = projMtx;
    do {
        MTX44_ctor(pProj);
        pProj++;
    } while (pProj < &projMtx[32]);
    pMtx = projTexMtx;
    do {
        MTX34_ctor(pMtx);
        pMtx++;
    } while (pMtx < &projTexMtx[32]);
    pMtx = envTexMtx;
    do {
        MTX34_ctor(pMtx);
        pMtx++;
    } while (pMtx < &envTexMtx[32]);
    flags = 0;
    currentCamera = 0;
    mtx34_identity(&invCameraMtx);
    for (u32 i = 0; i < 32; i++) {
        mtx34_identity(&cameraMtx[i]);
        nw4r::math::MTX44Identity(&projMtx[i]);
        mtx34_identity(&projTexMtx[i]);
        mtx34_identity(&envTexMtx[i]);
    }
}

extern "C" {

/* 0x80085C70 (0xDC): records camera `id`'s view, projection and texture projection matrices from `camera`, making it
 * the current camera when `bUpdate`. */
void g3d_camera_table_set_camera(G3dCameraTable* pSelf, const nw4r::g3d::Camera& camera, u32 id, bool bUpdate) {
    if (!g3d_camera_idx_valid(id)) {
        nw4r::db::Panic("g3d_state.cpp", 0x6C5, "NW4R:Failed assertion mtxID < NUM_CAMERA && mtxID >= 0");
    }
    if (id <= 31) {
        camera.GetCameraMtx(&pSelf->cameraMtx[id]);
        camera_get_projection_mtx(&camera, &pSelf->projMtx[id]);
        camera_get_projection_tex_mtx(&camera, &pSelf->projTexMtx[id]);
        camera_get_environment_tex_mtx(&camera, &pSelf->envTexMtx[id]);
        if (bUpdate) {
            pSelf->currentCamera = id;
            pSelf->flags &= ~1;
        }
    }
}

/* Caches the material's z-compare location. */
void g3d_state_set_mat_misc(nw4r::g3d::ResMatMisc misc) {
    if (misc.IsValid()) {
        g3d_zcomp_cache_set(&g3d_state_zcomp_cache, misc.GXGetZCompLoc());
    }
}

/* Sets the z-compare location when it changed and marks the display list dirty. */
void g3d_zcomp_cache_set(G3dZCompCache* pSelf, u8 beforeTex) {
    if ((pSelf->flags & 1) == 0 || pSelf->beforeTex != beforeTex) {
        pSelf->flags |= 1;
        pSelf->beforeTex = beforeTex;
        GXSetZCompLoc(beforeTex);
        fn_80085344((StateValidFlag*)&g3d_state_dl_dirty);
    }
}

#pragma push
#pragma opt_dead_assignments off
/* Loads the material's texture objects through the cache. */
void g3d_state_load_tex_obj(nw4r::g3d::ResTexObj texObj) {
    if (texObj.IsValid()) {
        g3d_tex_obj_cache_load(&g3d_state_tex_obj_cache, texObj);
        fn_80085344((StateValidFlag*)&g3d_state_dl_dirty);
    }
}
#pragma pop

/* 0x80085E48 (0xF4): loads every texture object of the material that differs from the cached one and records its
 * size for the coordinate scales. */
void g3d_tex_obj_cache_load(G3dTexObjCache* pSelf, const nw4r::g3d::ResTexObj texObj) {
    u32 i;
    for (i = 0; i < 8; i++) {
        if (texObj.IsValidTexObj((_GXTexMapID)i)) {
            const GXTexObj* pObj = texObj.GetTexObj((_GXTexMapID)i);
            u8 bit = 1 << i;
            if ((pSelf->mMask & bit) == 0 || !g3d_tex_obj_equal(pObj, &pSelf->texObj[(_GXTexMapID)i])) {
                pSelf->mMask |= bit;
                gx_tex_obj_copy(&pSelf->texObj[(_GXTexMapID)i], pObj);
                GXLoadTexObj(pObj, i);
                u16 height = GXGetTexObjHeight(pObj);
                fn_80085F3C(&g3d_state_tex_coord_scale_cache, i, GXGetTexObjWidth(pObj), height);
            }
        }
    }
}

/* Store the pair into the table at `idx` and mark the table dirty (`(flags | 2) & ~1`). */
void fn_80085F3C(StatePairTable* pSelf, u32 idx, u16 a, u16 b) {
    pSelf->mPairs[idx].mA = a;
    pSelf->mPairs[idx].mB = b;
    pSelf->mFlags = (pSelf->mFlags | 2) & ~1;
}

/* Whether two texture objects are the same eight words. */
BOOL g3d_tex_obj_equal(const GXTexObj* pA, const GXTexObj* pB) {
    u32 equal = 0;
    if (pA->dummy[0] == pB->dummy[0] && pA->dummy[1] == pB->dummy[1] && pA->dummy[2] == pB->dummy[2] &&
        pA->dummy[3] == pB->dummy[3] && pA->dummy[4] == pB->dummy[4] && pA->dummy[5] == pB->dummy[5] &&
        pA->dummy[6] == pB->dummy[6] && pA->dummy[7] == pB->dummy[7]) {
        equal = 1;
    }
    return equal;
}

#pragma push
#pragma opt_dead_assignments off
/* Loads the material's TLUT objects through the cache. */
void g3d_state_load_tlut_obj(nw4r::g3d::ResTlutObj tlutObj) {
    if (tlutObj.IsValid()) {
        g3d_tlut_obj_cache_load(&g3d_state_tlut_obj_cache, tlutObj);
        fn_80085344((StateValidFlag*)&g3d_state_dl_dirty);
    }
}
#pragma pop

/* Loads each valid TLUT that is not already cached, and invalidates the texture cached for that slot. */
void g3d_tlut_obj_cache_load(G3dTlutObjCache* pSelf, nw4r::g3d::ResTlutObj tlutObj) {
    u32 i = 0;
    do {
        if (tlutObj.IsValidTlut((_GXTlut)i)) {
            const GXTlutObj* pTlut = static_cast<const nw4r::g3d::ResTlutObj&>(tlutObj).GetTlut((_GXTlut)i);
            u16 bit = 1 << i;
            if ((pSelf->validMask & bit) == 0 || !fn_80086154((const StateWord3*)pTlut,
                                                             (const StateWord3*)&pSelf->tlut[(_GXTlut)i])) {
                pSelf->validMask |= bit;
                fn_80086138((StateWord3*)&pSelf->tlut[(_GXTlut)i], (const StateWord3*)pTlut);
                GXLoadTlut(pTlut, i);
                fn_80086118(&g3d_state_tex_obj_cache, i);
            }
        }
        i++;
    } while (i < 8);
}

void fn_80086118(G3dTexObjCache* pSelf, u32 bit) {
    pSelf->mMask &= ~(1 << bit);
}

void fn_80086138(StateWord3* pDst, const StateWord3* pSrc) {
    *pDst = *pSrc;
}

BOOL fn_80086154(const StateWord3* pA, const StateWord3* pB) {
    u32 equal = 0;
    if (pA->mWords[0] == pB->mWords[0] && pA->mWords[1] == pB->mWords[1] &&
        pA->mWords[2] == pB->mWords[2]) {
        equal = 1;
    }
    return equal;
}

/* Caches the material's generation mode. */
void g3d_state_set_gen_mode(nw4r::g3d::ResGenMode genMode) {
    if (genMode.IsValid()) {
        g3d_gen_mode_cache_set(&g3d_state_gen_mode_cache, genMode);
    }
}

#pragma push
#pragma dont_inline on
/* Copies each changed count or the cull mode into the cache, clearing the matching loaded flags. */
void g3d_gen_mode_cache_set(G3dGenModeCache* pSelf, nw4r::g3d::ResGenMode genMode) {
    if (pSelf->numTexGens != genMode.GXGetNumTexGens()) {
        pSelf->numTexGens = genMode.GXGetNumTexGens();
        pSelf->flags &= ~3;
    }
    if (pSelf->numChans != genMode.GXGetNumChans()) {
        pSelf->numChans = genMode.GXGetNumChans();
        pSelf->flags &= ~3;
    }
    if (pSelf->numTevStages != genMode.GXGetNumTevStages()) {
        pSelf->numTevStages = genMode.GXGetNumTevStages();
        pSelf->flags &= ~1;
    }
    if (pSelf->numIndStages != genMode.GXGetNumIndStages()) {
        pSelf->numIndStages = genMode.GXGetNumIndStages();
        pSelf->flags &= ~1;
    }
    if (pSelf->cullMode != genMode.GXGetCullMode()) {
        pSelf->cullMode = genMode.GXGetCullMode();
        pSelf->flags &= ~1;
    }
    if ((pSelf->flags & 4) == 0) {
        pSelf->flags = (pSelf->flags & ~3) | 4;
    }
}
#pragma pop

/* Loads the material's TEV display list unless it is the one already loaded. */
void g3d_state_load_tev(nw4r::g3d::ResTev tev) {
    if (tev.IsValid()) {
        if (!g3d_tev_cache_update(&g3d_state_cached_tev.tev, tev)) {
            g3d_gen_mode_cache_load(&g3d_state_gen_mode_cache);
            tev.CallDisplayList(fn_80086770(&g3d_state_dl_dirty));
            g3d_tex_coord_scale_set_tex_maps(&g3d_state_tex_coord_scale_cache, tev);
            fn_80086610(&g3d_state_tex_coord_scale_cache, fn_80086640((StateByte*)&g3d_state_gen_mode_cache));
        }
    }
}

/* The state's dirty-flush hook: only flush when the pending flags say so and the new value differs. */
void fn_80086610(StatePairTable* pSelf, u8 arg) {
    if ((pSelf->mFlags & 2) != 0 && (pSelf->mFlags & 1) == 0 && arg != 0) {
        g3d_tex_coord_scale_load(pSelf, arg);
    }
}

u8 fn_80086640(StateByte* pSelf) {
    return pSelf->mByte;
}

#pragma push
#pragma dont_inline on
/* Copies the TEV's per-coordinate texture map table into the scale cache when it changed. */
void g3d_tex_coord_scale_set_tex_maps(StatePairTable* pSelf, nw4r::g3d::ResTev tev) {
    const nw4r::g3d::ResTevData& r = tev.ref();
    const u32* pMaps = (const u32*)r.texMapID;
    if (((u32)pMaps & 3) != 0) {
        nw4r::db::Panic("g3d_state.cpp", 0x249, "NW4R:Failed assertion ((u32)x & 0x3) == 0");
    }
    if ((pSelf->mFlags & 2) == 0 || pMaps[0] != pSelf->texMapWord[0] || pMaps[1] != pSelf->texMapWord[1]) {
        pSelf->texMapWord[0] = pMaps[0];
        pSelf->texMapWord[1] = pMaps[1];
        pSelf->mFlags = (pSelf->mFlags | 2) & ~1;
    }
}
#pragma pop

/* Read the byte and clear it, returning what it held. */
bool fn_80086770(G3dDlDirty* pSelf) {
    bool value = pSelf->dirty;
    fn_800867A0(pSelf);
    return value;
}

void fn_800867A0(G3dDlDirty* pSelf) {
    pSelf->dirty = false;
}

/* Writes the cached generation mode when it is complete but not yet loaded. */
void g3d_gen_mode_cache_load(G3dGenModeCache* pSelf) {
    if ((pSelf->flags & 4) != 0 && (pSelf->flags & 3) == 2) {
        g3d_gd_set_gen_mode(pSelf->numTexGens, pSelf->numChans, pSelf->numTevStages, pSelf->numIndStages,
                            pSelf->cullMode);
        pSelf->flags |= 1;
    }
}

/* Writes the BP generation-mode register (behind its mask command). */
void g3d_gd_set_gen_mode(u8 numTexGens, u8 numChans, u8 numTevStages, u8 numIndStages, u32 cullMode) {
    fn_800868A0(0xFE07FC3F);
    fn_800868A0(((numIndStages << 16) | (g3d_state_cull_mode_hw[cullMode] << 14)) |
                (((numTevStages - 1) << 10) | (numTexGens | (numChans << 4))));
}

/* The `0x61`-tagged pipe command followed by its word. */
void fn_800868A0(u32 value) {
    fn_800868E4(0x61);
    fn_800868D8(value);
}

/* The two write-gather-pipe stores (the window lives in `gx.h`, rule 1). */
void fn_800868D8(u32 value) {
    GXWGFifo.u32 = value;
}

void fn_800868E4(u32 value) {
    u8 narrow = value;
    GXWGFifo.u8 = narrow;
}

#pragma push
#pragma dont_inline on
/* Records `tev` as the loaded TEV, returning whether it already was. */
BOOL g3d_tev_cache_update(u32* pCached, nw4r::g3d::ResTev tev) {
    if (!tev.IsValid()) {
        nw4r::db::Panic("g3d_state.cpp", 0x3B2, "NW4R:Failed assertion rhs.IsValid()");
    }
    u32 cur = (u32)tev.ptr();
    if (cur == *pCached) {
        return TRUE;
    }
    *pCached = (u32)tev.ptr();
    return FALSE;
}
#pragma pop

/* Calls the material's pixel display list. */
void g3d_state_load_mat_pix(nw4r::g3d::ResMatPix pix) {
    if (pix.IsValid()) {
        g3d_gen_mode_cache_load(&g3d_state_gen_mode_cache);
        pix.CallDisplayList(fn_80086770(&g3d_state_dl_dirty));
    }
}

/* 0x800869D4 (0x54): calls the material's TEV colour display list. */
void g3d_state_load_mat_tev_color(nw4r::g3d::ResMatTevColor tevColor) {
    if (tevColor.IsValid()) {
        g3d_gen_mode_cache_load(&g3d_state_gen_mode_cache);
        tevColor.CallDisplayList(fn_80086770(&g3d_state_dl_dirty));
    }
}

/* Calls the material's indirect-matrix display list for the cached number of indirect stages. */
void g3d_state_load_mat_ind_mtx_dl(nw4r::g3d::ResMatIndMtxAndScale ind) {
    if (ind.IsValid()) {
        g3d_gen_mode_cache_load(&g3d_state_gen_mode_cache);
        ind.CallDisplayList(fn_80086AA0((StateByte3*)&g3d_state_gen_mode_cache), fn_80086770(&g3d_state_dl_dirty));
    }
}

u8 fn_80086AA0(StateByte3* pSelf) {
    return pSelf->mByte;
}

/* Loads the material's indirect matrices: the display list, then the operation the callback may edit. */
void g3d_state_load_mat_ind_mtx(nw4r::g3d::ResMatIndMtxAndScale ind, G3dIndMtxCallback* pCallback) {
    if (ind.IsValid()) {
        G3dIndMtxOp op;
        g3d_state_load_mat_ind_mtx_dl(ind);
        g3d_ind_mtx_op_init(&op, ind);
        pCallback->Exec(&op);
        g3d_ind_mtx_op_load(&op);
    }
}

#pragma fp_contract off
#pragma push
#pragma dont_inline on
/* 0x80086B2C (0x468): loads the material's channel colours and controls: the material colours, the ambient colours
 * scaled by `amb`, and the light masks (unlit when `bLightOff`); the second channel only when two are in use. */
void g3d_state_load_mat_chan(const nw4r::g3d::ResMatChan chan, u32 maskColor0, u32 maskAlpha0, u32 maskColor1,
                             u32 maskAlpha1, GXColor amb, BOOL bLightOff) {
    if (chan.IsValid()) {
        const nw4r::g3d::ResMatChanData& data = chan.ref();
        u32 flag = data.chan[0].flag;
        if (flag & 1) {
            if (flag & 2) {
                g3d_gd_set_chan_mat_color(4, data.chan[0].matColor);
            } else {
                g3d_gd_set_chan_mat_color(0, data.chan[0].matColor);
            }
        } else if (flag & 2) {
            g3d_gd_set_chan_mat_color(2, data.chan[0].matColor);
        }
        GXColor ambColor;
        ambColor.r = amb.r * data.chan[0].ambColor.r * (1.0f / 255.0f) + 0.5f;
        ambColor.g = amb.g * data.chan[0].ambColor.g * (1.0f / 255.0f) + 0.5f;
        ambColor.b = amb.b * data.chan[0].ambColor.b * (1.0f / 255.0f) + 0.5f;
        ambColor.a = amb.a * data.chan[0].ambColor.a * (1.0f / 255.0f) + 0.5f;
        flag = data.chan[0].flag;
        if (flag & 4) {
            if (flag & 8) {
                g3d_gd_set_chan_amb_color(4, ambColor);
            } else {
                g3d_gd_set_chan_amb_color(0, ambColor);
            }
        } else if (flag & 8) {
            g3d_gd_set_chan_amb_color(2, ambColor);
        }
        if (bLightOff) {
            if (data.chan[0].flag & 0x10) {
                g3d_gd_set_chan_ctrl_unlit(0, data.chan[0].paramChanCtrlC, maskColor0);
            }
            if (data.chan[0].flag & 0x20) {
                g3d_gd_set_chan_ctrl_unlit(2, data.chan[0].paramChanCtrlA, maskAlpha0);
            }
        } else {
            if (data.chan[0].flag & 0x10) {
                g3d_gd_set_chan_ctrl_lights(0, data.chan[0].paramChanCtrlC, maskColor0);
            }
            if (data.chan[0].flag & 0x20) {
                g3d_gd_set_chan_ctrl_lights(2, data.chan[0].paramChanCtrlA, maskAlpha0);
            }
        }
        if (fn_80086FFC((StateByte1*)&g3d_state_gen_mode_cache) == 2) {
            const nw4r::g3d::ResMatChanData& data1 = chan.ref();
            flag = data1.chan[1].flag;
            if (flag & 1) {
                if (flag & 2) {
                    g3d_gd_set_chan_mat_color(5, data1.chan[1].matColor);
                } else {
                    g3d_gd_set_chan_mat_color(1, data1.chan[1].matColor);
                }
            } else if (flag & 2) {
                g3d_gd_set_chan_mat_color(3, data1.chan[1].matColor);
            }
            flag = data1.chan[1].flag;
            if (flag & 4) {
                if (flag & 8) {
                    g3d_gd_set_chan_amb_color(5, data1.chan[1].ambColor);
                } else {
                    g3d_gd_set_chan_amb_color(1, data1.chan[1].ambColor);
                }
            } else if (flag & 8) {
                g3d_gd_set_chan_amb_color(3, data1.chan[1].ambColor);
            }
            if (bLightOff) {
                if (data1.chan[1].flag & 0x10) {
                    g3d_gd_set_chan_ctrl_unlit(1, data1.chan[1].paramChanCtrlC, maskColor1);
                }
                if (data1.chan[1].flag & 0x20) {
                    g3d_gd_set_chan_ctrl_unlit(3, data1.chan[1].paramChanCtrlA, maskAlpha1);
                }
            } else {
                if (data1.chan[1].flag & 0x10) {
                    g3d_gd_set_chan_ctrl_lights(1, data1.chan[1].paramChanCtrlC, maskColor1);
                }
                if (data1.chan[1].flag & 0x20) {
                    g3d_gd_set_chan_ctrl_lights(3, data1.chan[1].paramChanCtrlA, maskAlpha1);
                }
            }
        } else {
            g3d_gd_set_chan_ctrl_lights(1, 0, 0);
            g3d_gd_set_chan_ctrl_lights(3, 0, 0);
        }
        g3d_gen_mode_cache_load_full(&g3d_state_gen_mode_cache);
    }
}
#pragma pop
#pragma fp_contract on

#pragma push
#pragma dont_inline on
/* 0x800870E4 (0x78): calls the material's texture-coordinate generation display list for the cached number of
 * coordinates. */
void g3d_state_load_tex_coord_gen(nw4r::g3d::ResMatTexCoordGen texCoordGen) {
    if (texCoordGen.IsValid()) {
        bool dirty = fn_80086770(&g3d_state_dl_dirty);
        texCoordGen.CallDisplayList(fn_80086640((StateByte*)&g3d_state_gen_mode_cache), dirty);
        g3d_gen_mode_cache_load_full(&g3d_state_gen_mode_cache);
    }
}
#pragma pop

#pragma push
#pragma dont_inline on
/* 0x8008715C (0x288): loads the material's eight texture matrices: each coordinate's projection, effect matrix and
 * SRT transform, an identity matrix only once while it stays the identity. */
void g3d_state_load_tex_srt(const nw4r::g3d::ResTexSrt srt) {
    nw4r::math::MTX34 m;
    MTX34_ctor(&m);
    for (u32 i = 0; i < 8; i++) {
        if (srt.IsExist(i)) {
            BOOL bIdentity = TRUE;
            const nw4r::g3d::TexMtxEffect& effect = srt.ref().effect[i];
            mtx34_identity(&m);
            if (effect.map_mode != 0) {
                g3d_tex_proj_build(&g3d_state_tex_proj_table, effect.map_mode, &m, effect.ref_camera,
                                   effect.ref_light);
                g3d_tex_mtx_func_type_set(g3d_state_tex_mtx_func_types.funcType, i,
                                          g3d_tex_proj_mtx_mode(&g3d_state_tex_proj_table, effect.map_mode));
                bIdentity = FALSE;
            } else {
                g3d_tex_mtx_func_type_set(g3d_state_tex_mtx_func_types.funcType, i, 0);
            }
            if (!srt.IsIdentityTexMtx(i)) {
                if (bIdentity) {
                    if (!(effect.misc_flag & 1)) {
                        bIdentity = FALSE;
                        mtx34_copy_ps(&m, &effect.effectMtx);
                    }
                } else {
                    mtx34_concat(&m, &effect.effectMtx, &m);
                }
                if (effect.map_mode == 0) {
                    g3d_calc_tex_mtx(&m, bIdentity, &srt.ref().texSrt[i], srt.GetTexSrtFlag(i), srt.GetTexMtxMode());
                } else {
                    nw4r::math::MTX34 srtMtx;
                    MTX34_ctor(&srtMtx);
                    g3d_calc_tex_mtx(&srtMtx, TRUE, &srt.ref().texSrt[i], srt.GetTexSrtFlag(i), srt.GetTexMtxMode());
                    g3d_swap_f32(&srtMtx.m[0][2], &srtMtx.m[0][3]);
                    g3d_swap_f32(&srtMtx.m[1][2], &srtMtx.m[1][3]);
                    mtx34_concat(&m, &srtMtx, &m);
                }
                bIdentity = FALSE;
            }
            if (bIdentity) {
                if (!g3d_bits_test(&g3d_state_tex_mtx_flags.flags, i)) {
                    nw4r::math::MTX34 identity;
                    MTX34_ctor(&identity);
                    g3d_bits_set(&g3d_state_tex_mtx_flags.flags, i);
                    mtx34_identity(&identity);
                    GXLoadTexMtxImm((const f32(*)[4])mtx34_get_ptr(&identity), 0x40 + i * 3, 0);
                }
            } else {
                g3d_bits_clear(&g3d_state_tex_mtx_flags.flags, i);
                GXLoadTexMtxImm((const f32(*)[4])mtx34_get_ptr(&m), 0x40 + i * 3, 0);
            }
        } else {
            g3d_tex_mtx_func_type_set(g3d_state_tex_mtx_func_types.funcType, i, 0);
        }
    }
}
#pragma pop

#pragma push
#pragma dont_inline on
/* 0x80087978 (0x9C): loads the shape's pre-primitive display list, telling it whether its vertex description is
 * already loaded. */
void g3d_state_load_shp_pre_prim(const nw4r::g3d::ResShp shp) {
    if (shp.IsValid()) {
        fn_80086610(&g3d_state_tex_coord_scale_cache, fn_80086640((StateByte*)&g3d_state_gen_mode_cache));
        g3d_gen_mode_cache_load_full(&g3d_state_gen_mode_cache);
        bool same = g3d_vtx_desc_cache_update(&g3d_state_vtx_desc_cache,
                                              (const G3dVtxDescCache*)shp.ref().vtxDesc);
        shp.CallPrePrimitiveDisplayList(fn_80086770(&g3d_state_dl_dirty), same);
    }
}
#pragma pop

/* 0x80087A14 (0x60): records `pDesc` as the loaded vertex description, returning whether it already was. */
bool g3d_vtx_desc_cache_update(G3dVtxDescCache* pSelf, const G3dVtxDescCache* pDesc) {
    if (g3d_vtx_desc_equal(pDesc, pSelf)) {
        return true;
    }
    g3d_vtx_desc_copy(pSelf, pDesc);
    return false;
}

/* 0x80087A74 (0x1C): copies the vertex description. */
void g3d_vtx_desc_copy(G3dVtxDescCache* pDst, const G3dVtxDescCache* pSrc) {
    *pDst = *pSrc;
}

/* 0x80087A90 (0x40): whether the two vertex descriptions are the same three words. */
BOOL g3d_vtx_desc_equal(const G3dVtxDescCache* pA, const G3dVtxDescCache* pB) {
    BOOL equal = FALSE;
    if (pA->word[0] == pB->word[0] && pA->word[1] == pB->word[1] && pA->word[2] == pB->word[2]) {
        equal = TRUE;
    }
    return equal;
}

#pragma push
#pragma dont_inline on
/* 0x80087AD0 (0x1B0): loads the shape's matrices (the given view matrices, the indexed view matrix arrays, or none for
 * a multi-matrix shape, which resets the current matrices) and calls its primitive display list. */
void g3d_state_load_shp_prim(const nw4r::g3d::ResShp shp, const nw4r::math::MTX34* pViewPos,
                             const nw4r::math::MTX34* pViewNrm) {
    if (shp.IsValid()) {
        fn_80086610(&g3d_state_tex_coord_scale_cache, fn_80086640((StateByte*)&g3d_state_gen_mode_cache));
        u32 curMtx = shp.ref().curMtxIdx;
        u32 bMultiMtx = curMtx >> 31;
        if (!bMultiMtx) {
            if (pViewPos != NULL) {
                GXLoadPosMtxImm((const f32(*)[4])mtx34_const_ptr((u32)pViewPos), 0);
                if (shp.IsVtxAttrEnabled(10)) {
                    if (pViewNrm != NULL) {
                        GXLoadNrmMtxImm((const f32(*)[4])mtx34_const_ptr((u32)pViewNrm), 0);
                    } else {
                        GXLoadNrmMtxImm((const f32(*)[4])mtx34_const_ptr((u32)pViewPos), 0);
                    }
                }
            } else {
                if (!g3d_view_mtx_arrays_has_pos(&g3d_state_view_mtx_arrays)) {
                    nw4r::db::Panic("g3d_state.cpp", 0x893, "NW4R:Failed assertion sPosNrmArrayState.IsValid()");
                }
                GXLoadPosMtxIndx(curMtx, 0);
                if (shp.IsVtxAttrEnabled(10)) {
                    GXLoadNrmMtxIndx3x3(curMtx, 0);
                }
            }
            g3d_tex_mtx_current_set(g3d_state_tex_mtx_func_types.funcType, curMtx);
        } else {
            g3d_current_mtx_reset(&g3d_state_current_mtx_reset.reset);
            if (!g3d_view_mtx_arrays_has_pos(&g3d_state_view_mtx_arrays)) {
                nw4r::db::Panic("g3d_state.cpp", 0x8A2, "NW4R:Failed assertion sPosNrmArrayState.IsValid()");
            }
        }
        g3d_gen_mode_cache_load_full(&g3d_state_gen_mode_cache);
        shp.CallPrimitiveDisplayList(fn_80086770(&g3d_state_dl_dirty));
        if (bMultiMtx) {
            g3d_current_mtx_reset_clear(&g3d_state_current_mtx_reset.reset);
        }
    }
}
#pragma pop

/* 0x80087C80 (0xC): marks the current-matrix indices as no longer the reset ones. */
void g3d_current_mtx_reset_clear(u8* pFlag) {
    *pFlag = 0;
}

/* 0x80087C8C (0x40): resets the current matrix indices once. */
void g3d_current_mtx_reset(u8* pFlag) {
    if (*pFlag == 0) {
        GDResetCurrentMtx();
        *pFlag = 1;
    }
}

/* 0x80087CCC (0xD0): loads the normal matrix `mtxIdx` as each texgen's matrix where its mode asks for one and
 * points the texgens at them. */
void g3d_tex_mtx_current_set(const s32* pModes, s32 mtxIdx) {
    u32 setting[8];
    BOOL allIdentity = TRUE;
    if (mtxIdx >= 0) {
        const s32* pMode;
        u32 texMtx;
        u32* pSetting;
        u32 i = 0;
        pMode = pModes;
        texMtx = 0x1E;
        pSetting = setting;
        do {
            if (*pMode == 1) {
                *pSetting = texMtx;
                GDLoadTexMtxImm3x3(g3d_state_get_nrm_mtx((u16)mtxIdx), *pSetting);
                allIdentity = FALSE;
            } else if (*pMode == 2) {
                *pSetting = 0;
                allIdentity = FALSE;
            } else {
                *pSetting = 0x3C;
            }
            pMode++;
            texMtx += 3;
            pSetting++;
            i++;
        } while (i < 8);
        if (allIdentity) {
            g3d_current_mtx_reset(&g3d_state_current_mtx_reset.reset);
            return;
        }
        GDSetCurrentMtx((Array8*)setting);
        g3d_current_mtx_reset_clear(&g3d_state_current_mtx_reset.reset);
    }
}

/* 0x80087D9C (0x14): whether the view position matrix array is set. */
BOOL g3d_view_mtx_arrays_has_pos(const G3dViewMtxArrays* pSelf) {
    return pSelf->pViewPosMtx != NULL;
}

} /* extern "C" */

/* 0x80087DB0 (0x48): whether the shape carries GX vertex attribute `attr`. */
bool nw4r::g3d::ResShp::IsVtxAttrEnabled(u32 attr) const {
    return (ref().vtxAttrFlags & (1 << attr)) != 0;
}

extern "C" {

/* 0x80087DF8 (0x20): points the indexed loads at the three view matrix arrays. */
void g3d_state_set_view_mtx_arrays(nw4r::math::MTX34* pPos, Mat33* pNrm, nw4r::math::MTX34* pTex) {
    g3d_view_mtx_arrays_set(&g3d_state_view_mtx_arrays, pPos, pNrm, pTex);
}

/* 0x80087E18 (0x78): stores the three view matrix arrays and hands the set ones to GX. */
void g3d_view_mtx_arrays_set(G3dViewMtxArrays* pSelf, nw4r::math::MTX34* pPos, Mat33* pNrm,
                             nw4r::math::MTX34* pTex) {
    pSelf->pViewPosMtx = pPos;
    pSelf->pViewNrmMtx = pNrm;
    pSelf->pViewTexMtx = pTex;
    if (pPos != NULL) {
        GXSetArray(0x15, pPos, 0x30);
    }
    if (pSelf->pViewNrmMtx != NULL) {
        GXSetArray(0x16, pSelf->pViewNrmMtx, 0x24);
    }
    if (pSelf->pViewTexMtx != NULL) {
        GXSetArray(0x17, pSelf->pViewTexMtx, 0x30);
    }
}

/* 0x80087E90 (0x78): returns view position matrix `idx`. */
nw4r::math::MTX34* g3d_view_mtx_arrays_pos_mtx(G3dViewMtxArrays* pSelf, u32 idx) {
    if (pSelf->pViewPosMtx == NULL) {
        nw4r::db::Panic("g3d_state.cpp", 0x3EE, "NW4R:Pointer must not be NULL (mpViewPosMtxArray)");
    }
    if (pSelf->pViewPosMtx != NULL) {
        return &pSelf->pViewPosMtx[idx];
    }
    return NULL;
}

/* 0x80087F08 (0x10): returns view normal matrix `idx`. */
Mat33* g3d_state_get_nrm_mtx(u32 idx) {
    return g3d_view_mtx_arrays_nrm_mtx(&g3d_state_view_mtx_arrays, idx);
}

/* 0x80087F18 (0x90): returns view normal matrix `idx`: from the normal matrix array when it is set, else the
 * rotation part of view position matrix `idx`, built into a scratch matrix. */
Mat33* g3d_view_mtx_arrays_nrm_mtx(G3dViewMtxArrays* pSelf, u32 idx) {
    if (pSelf->pViewNrmMtx != NULL) {
        return &pSelf->pViewNrmMtx[idx];
    }
    if (g3d_state_nrm_mtx_scratch_init == 0) {
        MTX33_ctor(&g3d_state_nrm_mtx_scratch);
        g3d_state_nrm_mtx_scratch_init = 1;
    }
    mtx34_to_mtx33((nw4r::math::MTX33*)&g3d_state_nrm_mtx_scratch, g3d_view_mtx_arrays_pos_mtx(pSelf, idx));
    return &g3d_state_nrm_mtx_scratch;
}

} /* extern "C" */

/* 0x80087FA8 (0x5C): destroys the callback. */
G3dIndMtxCallbackStd::~G3dIndMtxCallbackStd() {
}

/* 0x80088004 (0x44): destroys the callback base. */
G3dIndMtxCallback::~G3dIndMtxCallback() {
}

extern "C" {

/* 0x80088048 (0x8): returns the hook the indirect-matrix builder calls. */
G3dIndMtxCallback* g3d_state_get_ind_mtx_hook(void) {
    return g3d_state_ind_mtx_hook;
}

/* 0x80088050 (0x38): copies `fog` into fog `idx` of the fog table. */
void g3d_state_set_fog(G3dFog fog, u32 idx) {
    g3d_fog_table_set_fog(&g3d_state_fog_table, fog, idx);
}

#pragma push
#pragma dont_inline on
/* 0x80088088 (0xA4): copies `fog` into fog `idx`, forgetting the loaded fog when it changes the one loaded. */
void g3d_fog_table_set_fog(G3dFogTable* pSelf, G3dFog fog, u32 idx) {
    if (idx <= 31 && fog_is_valid((const nw4r::g3d::Fog*)&fog)) {
        if (pSelf->fogIdx == (s32)idx) {
            if (memcmp(g3d_fog_ref(&fog), &pSelf->fog[idx], sizeof(G3dFogData)) == 0) {
                return;
            }
            g3d_fog_table_invalidate(pSelf);
        }
        ((const nw4r::g3d::Fog*)&fog)->CopyTo(&pSelf->fog[idx]);
    }
}
#pragma pop

/* 0x8008818C (0x10): loads fog `idx` of the fog table. */
void g3d_state_load_fog(s32 idx) {
    g3d_fog_table_load(&g3d_state_fog_table, idx);
}

/* 0x8008819C (0xB4): sets fog `idx` in GX unless it is the one loaded; past the table, no fog. */
void g3d_fog_table_load(G3dFogTable* pSelf, s32 idx) {
    G3dFog fog;
    u32 flags = pSelf->flags;
    if ((flags & 1) && (flags & 2) && idx == pSelf->fogIdx) {
        return;
    }
    if ((u32)idx > 31) {
        GXColor color = {0, 0, 0, 0};
        GXSetFog(0, 0.0f, 0.0f, 0.0f, 0.0f, color);
    } else {
        ((const nw4r::g3d::Fog*)fog_ctor((s32)&fog, &pSelf->fog[idx]))->SetGP();
    }
    pSelf->flags |= 3;
    pSelf->fogIdx = idx;
}

/* 0x80088250 (0x10): copies the light setting into the state and loads its lights. */
void g3d_state_set_light_setting(const G3dLightSetting* pSetting) {
    g3d_light_table_set_setting(&g3d_state_light_table, pSetting);
}

/* 0x80088260 (0x10): returns light object `idx`, NULL past the table. */
G3dLightObj* g3d_state_get_light_obj(u32 idx) {
    return fn_800856D4(&g3d_state_light_table, idx);
}

/* 0x80088270 (0x30): returns byte `idx` of the loaded light set (a light index, -1 for none). */
s8 g3d_state_get_light_set_entry(u32 idx) {
    return fn_80085B54(&g3d_state_light_table, idx);
}

/* 0x800882A0 (0x5C): loads light set `idx` and returns its four light masks and its ambient colour. */
void g3d_state_load_light_set(s32 idx, u32* pMaskDiffColor, u32* pMaskDiffAlpha, u32* pMaskSpecColor,
                              u32* pMaskSpecAlpha, GXColor* pAmb) {
    g3d_light_table_load_light_set(&g3d_state_light_table, idx, pMaskDiffColor, pMaskDiffAlpha, pMaskSpecColor,
                                   pMaskSpecAlpha, pAmb);
}

/* 0x800882FC (0x20): records camera `id`'s matrices from `camera`, making it current when `bUpdate`. */
void g3d_state_set_camera(const nw4r::g3d::Camera& camera, u32 id, bool bUpdate) {
    g3d_camera_table_set_camera(&g3d_state_camera_table, camera, id, bUpdate);
}

/* 0x8008831C (0xC): returns the current camera's matrix. */
nw4r::math::MTX34* g3d_state_get_camera_mtx(void) {
    return g3d_camera_table_camera_mtx(&g3d_state_camera_table);
}

/* 0x80088328 (0x14): returns the current camera's matrix. */
nw4r::math::MTX34* g3d_camera_table_camera_mtx(G3dCameraTable* pSelf) {
    return &pSelf->cameraMtx[pSelf->currentCamera];
}

/* 0x8008833C (0xC): returns the inverse of the current camera's matrix. */
nw4r::math::MTX34* g3d_state_get_inv_camera_mtx(void) {
    return g3d_camera_table_inv_camera_mtx(&g3d_state_camera_table);
}

/* 0x80088348 (0x7C): returns the inverse of the current camera's matrix, computing it on first use. */
nw4r::math::MTX34* g3d_camera_table_inv_camera_mtx(G3dCameraTable* pSelf) {
    if ((pSelf->flags & 1) == 0) {
        if (!mtx34_inverse(&pSelf->invCameraMtx, g3d_camera_table_camera_mtx(pSelf))) {
            nw4r::db::Panic("g3d_state.cpp", 0x69B, "NW4R:Failed assertion result");
        }
        pSelf->flags |= 1;
    }
    return &pSelf->invCameraMtx;
}

/* 0x800883C4 (0x48): inverts `pSrc` into `pOut`; 0 when `pSrc` is singular. */
u32 mtx34_inverse(MTX34* pOut, const MTX34* pSrc) {
    MTX34* pDst = (MTX34*)mtx34_get_ptr(pOut);
    return PSMTXInverse(((const MTX34*)mtx34_const_ptr((u32)pSrc))->m, pDst->m);
}

/* 0x8008840C (0x10): returns camera `idx`'s matrix. */
nw4r::math::MTX34* g3d_state_get_camera_mtx_at(u32 idx) {
    return g3d_camera_table_camera_mtx_at(&g3d_state_camera_table, idx);
}

/* 0x8008841C (0x84): returns camera `idx`'s matrix, NULL past the table. */
nw4r::math::MTX34* g3d_camera_table_camera_mtx_at(G3dCameraTable* pSelf, u32 idx) {
    if (!g3d_camera_idx_valid(idx)) {
        nw4r::db::Panic("g3d_state.cpp", 0x66A, "NW4R:Failed assertion mtxID < NUM_CAMERA && mtxID >= 0");
    }
    if (idx <= 31) {
        return &pSelf->cameraMtx[idx];
    }
    return NULL;
}

/* 0x800884A0 (0xC): returns the current camera's texture projection matrix. */
nw4r::math::MTX34* g3d_state_get_proj_tex_mtx(void) {
    return g3d_camera_table_proj_tex_mtx(&g3d_state_camera_table);
}

/* 0x800884AC (0x14): returns the current camera's texture projection matrix. */
static nw4r::math::MTX34* g3d_camera_table_proj_tex_mtx(G3dCameraTable* pSelf) {
    return &pSelf->projTexMtx[pSelf->currentCamera];
}

/* 0x800884C0 (0x10): returns camera `idx`'s texture projection matrix. */
nw4r::math::MTX34* g3d_state_get_proj_tex_mtx_at(u32 idx) {
    return g3d_camera_table_proj_tex_mtx_at(&g3d_state_camera_table, idx);
}

/* 0x800884D0 (0x84): returns camera `idx`'s texture projection matrix, NULL past the table. */
nw4r::math::MTX34* g3d_camera_table_proj_tex_mtx_at(G3dCameraTable* pSelf, u32 idx) {
    if (!g3d_camera_idx_valid(idx)) {
        nw4r::db::Panic("g3d_state.cpp", 0x67E, "NW4R:Failed assertion mtxID < NUM_CAMERA && mtxID >= 0");
    }
    if (idx <= 31) {
        return &pSelf->projTexMtx[idx];
    }
    return NULL;
}

/* 0x80088554 (0xC): returns the current camera's environment texture matrix. */
nw4r::math::MTX34* g3d_state_get_env_tex_mtx(void) {
    return g3d_camera_table_env_tex_mtx(&g3d_state_camera_table);
}

/* 0x80088560 (0x14): returns the current camera's environment texture matrix. */
static nw4r::math::MTX34* g3d_camera_table_env_tex_mtx(G3dCameraTable* pSelf) {
    return &pSelf->envTexMtx[pSelf->currentCamera];
}

/* 0x80088574 (0x10): copies `pRenderMode` into the render mode the state keeps. */
void g3d_state_set_render_mode(const GXRenderModeObj* pRenderMode) {
    render_mode_copy(&g3d_state_render_mode, (GXRenderModeObj*)pRenderMode);
}

/* 0x80088584 (0xC): returns the render mode the state keeps. */
struct RenderModeObj* g3d_state_get_render_mode(void) {
    return (struct RenderModeObj*)&g3d_state_render_mode;
}

#pragma push
#pragma dont_inline on
/* 0x80088590 (0x14C): forgets the cached state the `flag` bits select, so the next loads write it again. */
void g3d_state_invalidate(u32 flag) {
    if (flag & 1) {
        g3d_tex_obj_cache_invalidate(&g3d_state_tex_obj_cache);
        g3d_tex_coord_scale_invalidate(&g3d_state_tex_coord_scale_cache);
    }
    if (flag & 2) {
        g3d_tlut_obj_cache_invalidate(&g3d_state_tlut_obj_cache);
    }
    if (flag & 4) {
        g3d_tex_coord_scale_invalidate(&g3d_state_tex_coord_scale_cache);
        g3d_tev_cache_invalidate(&g3d_state_cached_tev.tev);
    }
    if (flag & 8) {
        g3d_gen_mode_cache_reset(&g3d_state_gen_mode_cache);
    }
    if (flag & 0x10) {
        g3d_vtx_desc_cache_invalidate(&g3d_state_vtx_desc_cache);
    }
    if (flag & 0x20) {
        g3d_current_mtx_reset_clear(&g3d_state_current_mtx_reset.reset);
    }
    if (flag & 0x40) {
        g3d_tex_mtx_func_types_clear(g3d_state_tex_mtx_func_types.funcType);
        g3d_tex_mtx_flags_clear(&g3d_state_tex_mtx_flags.flags);
    }
    if (flag & 0x80) {
        g3d_zcomp_cache_invalidate(&g3d_state_zcomp_cache);
    }
    if (flag & 0x100) {
        g3d_fog_table_invalidate(&g3d_state_fog_table);
    }
    if (flag & 0x200) {
        fn_80085478(&g3d_state_light_table);
    }
    if (flag & 0x400) {
        g3d_view_mtx_arrays_clear(&g3d_state_view_mtx_arrays);
    }
    g3d_dl_dirty_set(&g3d_state_dl_dirty);
}
#pragma pop

/* 0x800886DC (0xC): marks the display lists dirty. */
void g3d_dl_dirty_set(G3dDlDirty* pSelf) {
    pSelf->dirty = true;
}

/* 0x800886E8 (0x14): forgets the three view matrix arrays. */
void g3d_view_mtx_arrays_clear(G3dViewMtxArrays* pSelf) {
    pSelf->pViewPosMtx = NULL;
    pSelf->pViewNrmMtx = NULL;
    pSelf->pViewTexMtx = NULL;
}

/* 0x800886FC (0x4): forgets the cached z-compare location. */
void g3d_zcomp_cache_invalidate(G3dZCompCache* pSelf) {
    g3d_zcomp_cache_clear(pSelf);
}

/* 0x80088700 (0x10): clears the z-compare cache's valid bit. */
static void g3d_zcomp_cache_clear(G3dZCompCache* pSelf) {
    pSelf->flags &= ~1;
}

/* 0x80088710 (0xC): clears the texture-matrix flag word. */
void g3d_tex_mtx_flags_clear(u32* pFlags) {
    *pFlags = 0;
}

/* 0x8008871C (0x28): clears the eight texture matrices' scene-dependent function types. */
void g3d_tex_mtx_func_types_clear(s32* pFuncTypes) {
    pFuncTypes[7] = 0;
    pFuncTypes[6] = 0;
    pFuncTypes[5] = 0;
    pFuncTypes[4] = 0;
    pFuncTypes[3] = 0;
    pFuncTypes[2] = 0;
    pFuncTypes[1] = 0;
    pFuncTypes[0] = 0;
}

/* 0x80088744 (0x4): forgets the loaded vertex description. */
void g3d_vtx_desc_cache_invalidate(G3dVtxDescCache* pSelf) {
    g3d_vtx_desc_cache_clear(pSelf);
}

/* 0x80088748 (0x14): clears the vertex description. */
void g3d_vtx_desc_cache_clear(G3dVtxDescCache* pSelf) {
    pSelf->word[2] = 0;
    pSelf->word[1] = 0;
    pSelf->word[0] = 0;
}

/* 0x8008875C (0x28): resets the generation mode cache to one texgen, one TEV stage and back-face culling. */
void g3d_gen_mode_cache_reset(G3dGenModeCache* pSelf) {
    pSelf->numTexGens = 1;
    pSelf->numChans = 0;
    pSelf->numTevStages = 1;
    pSelf->numIndStages = 0;
    pSelf->cullMode = 2;
    pSelf->flags = 0;
}

/* 0x80088784 (0xC): forgets the cached TEV. */
void g3d_tev_cache_invalidate(u32* pCached) {
    *pCached = 0;
}

/* 0x80088790 (0xC): forgets the loaded TLUT objects. */
void g3d_tlut_obj_cache_invalidate(G3dTlutObjCache* pSelf) {
    pSelf->validMask = 0;
}

/* 0x8008879C (0xC): forgets the texture-coordinate scales. */
void g3d_tex_coord_scale_invalidate(StatePairTable* pSelf) {
    pSelf->mFlags = 0;
}

/* 0x800887A8 (0xC): forgets the loaded texture objects. */
void g3d_tex_obj_cache_invalidate(G3dTexObjCache* pSelf) {
    pSelf->mMask = 0;
}

} /* extern "C" */

/* 0x800887B4 (0x94): constructs the callback with three identity matrices, none enabled. */
G3dIndMtxCallbackStd::G3dIndMtxCallbackStd() {
    nw4r::math::MTX34* pMtx = mtx;
    do {
        MTX34_ctor(pMtx);
        pMtx++;
    } while (pMtx < &mtx[3]);
    mtx34_identity(&mtx[0]);
    mtx34_identity(&mtx[1]);
    mtx34_identity(&mtx[2]);
    enabled[2] = 0;
    enabled[1] = 0;
    enabled[0] = 0;
    pad_0x07 = 0;
}

/* 0x80088848 (0x10): constructs the callback base. */
G3dIndMtxCallback::G3dIndMtxCallback() {
}

#pragma push
#pragma peephole on
/* 0x80088858 (0x1A0): builds the normal-map matrix of indirect matrix `id` from the light direction and the normal
 * matrix (the specular method adds the half vector's row), or clears it for a method that needs none. */
void G3dIndMtxCallbackStd::SetNrmMapMtx(_GXIndTexMtxID id, const nw4r::math::VEC3* pLightDir,
                                        const nw4r::math::MTX34* pNrmMtx,
                                        nw4r::g3d::ResMatMiscData::IndirectMethod method) {
    if (!(id == GX_ITM_0 || id == GX_ITM_1 || id == GX_ITM_2)) {
        nw4r::db::Panic("g3d_state.cpp", 0x9E4,
                        "NW4R:Failed assertion id == GX_ITM_0 || id == GX_ITM_1 || id == GX_ITM_2");
    }
    u32 idx = id - GX_ITM_0;
    if (idx <= 2 && method != nw4r::g3d::ResMatMiscData::WARP && method != nw4r::g3d::ResMatMiscData::FUR) {
        enabled[idx] = 1;
        if (pLightDir != NULL) {
            nw4r::math::MTX34* pMtx = &mtx[idx];
            pMtx->m[0][0] = 0.5f * -pLightDir->x;
            mtx[idx].m[0][1] = 0.5f * -pLightDir->y;
            mtx[idx].m[0][2] = 0.5f * -pLightDir->z;
            if (method == nw4r::g3d::ResMatMiscData::NORMAL_MAP_SPEC) {
                nw4r::math::VEC3 half;
                setVec3(&half, pLightDir->x, pLightDir->y, pLightDir->z - 1.0f);
                vec3_normalize_into(&half, &half);
                mtx[idx].m[1][0] = 0.5f * -half.x;
                mtx[idx].m[1][1] = 0.5f * -half.y;
                mtx[idx].m[1][2] = 0.5f * -half.z;
            } else {
                mtx[idx].m[1][2] = 0.0f;
                mtx[idx].m[1][1] = 0.0f;
                mtx[idx].m[1][0] = 0.0f;
            }
            if (pNrmMtx == NULL) {
                nw4r::db::Panic("g3d_state.cpp", 0xA06, "NW4R:Pointer must not be NULL (pNrmMtx)");
            }
            mtx34_concat(pMtx, &mtx[idx], pNrmMtx);
        } else {
            nw4r::math::MTX34Zero(&mtx[idx]);
        }
    }
}
#pragma pop

/* 0x800889F8 (0x50): resets the three matrices to identity and disables them. */
void G3dIndMtxCallbackStd::Reset() {
    mtx34_identity(&mtx[0]);
    mtx34_identity(&mtx[1]);
    mtx34_identity(&mtx[2]);
    enabled[2] = 0;
    enabled[1] = 0;
    enabled[0] = 0;
}

/* 0x80088A48 (0x88): hands each enabled matrix to the indirect-matrix operation. */
void G3dIndMtxCallbackStd::Exec(G3dIndMtxOp* pOp) {
    if (enabled[0]) {
        g3d_ind_mtx_op_set(pOp, 1, &mtx[0]);
    }
    if (enabled[1]) {
        g3d_ind_mtx_op_set(pOp, 2, &mtx[1]);
    }
    if (enabled[2]) {
        g3d_ind_mtx_op_set(pOp, 3, &mtx[2]);
    }
}

/* ------------------------------------------------------------------------------------------------ */
/* The state's file-scope objects and their constructors (the static initialiser calls them out of line) */
/* ------------------------------------------------------------------------------------------------ */

/* 0x80088BA4 (0x10): an invalid z-compare cache. */
inline G3dZCompCache::G3dZCompCache() {
    flags = 0;
    beforeTex = 0;
}

/* 0x80088BB4 (0x74): no fog loaded, the 32 fogs reset. */
inline G3dFogTable::G3dFogTable() {
    flags = 0;
    fogIdx = -1;
    for (u32 i = 0; i < 32; i++) {
        G3dFog handle;
        fog_init(fog_ctor((s32)&handle, &fog[i]));
    }
}

/* 0x80088C28 (0xE0): the scene-dependent projections of the map modes: environment (camera and light), projection
 * and specular environment; the identity for the rest. */
inline G3dTexProjTable::G3dTexProjTable() {
    entry[0].func = g3d_tex_proj_identity;
    entry[0].mtxMode = 0;
    entry[1].func = g3d_tex_proj_env;
    entry[1].mtxMode = 1;
    entry[2].func = g3d_tex_proj_projection;
    entry[2].mtxMode = 2;
    entry[3].func = g3d_tex_proj_env;
    entry[3].mtxMode = 1;
    entry[4].func = g3d_tex_proj_env_spec;
    entry[4].mtxMode = 1;
    for (u32 i = 5; i < 0x100; i++) {
        entry[i].func = g3d_tex_proj_identity;
        entry[i].mtxMode = 0;
    }
}

/* 0x80088D08 (0x14): no view matrix arrays. */
inline G3dViewMtxArrays::G3dViewMtxArrays() {
    pViewPosMtx = NULL;
    pViewNrmMtx = NULL;
    pViewTexMtx = NULL;
}

/* 0x80088D1C (0xC): no TEV loaded. */
inline G3dTevCache::G3dTevCache() {
    tev = 0;
}

/* 0x80088D28 (0x30): every texture matrix without a scene-dependent function. */
inline G3dTexMtxFuncTypes::G3dTexMtxFuncTypes() {
    g3d_tex_mtx_func_types_clear(funcType);
}

/* 0x80088D58 (0xC): no texture matrix loaded as the identity. */
inline G3dTexMtxFlags::G3dTexMtxFlags() {
    flags = 0;
}

/* 0x80088D64 (0xC): the current matrices not reset. */
inline G3dCurrentMtxReset::G3dCurrentMtxReset() {
    reset = 0;
}

/* 0x80088D70 (0x30): no vertex description loaded. */
inline G3dVtxDescCache::G3dVtxDescCache() {
    g3d_vtx_desc_cache_clear(this);
}

/* 0x80088DA0 (0xC): no TLUT object loaded. */
inline G3dTlutObjCache::G3dTlutObjCache() {
    validMask = 0;
}

/* 0x80088DAC (0xC): no texture object loaded. */
inline G3dTexObjCache::G3dTexObjCache() {
    mMask = 0;
}

/* 0x80088DB8 (0x38): no texture coordinate scale loaded. */
inline StatePairTable::StatePairTable() {
    memset(this, 0, sizeof(*this) - sizeof(pad_0x6C));
}

/* 0x80088DF0 (0x28): one texture coordinate generator, one TEV stage, back-face culling, nothing loaded. */
inline G3dGenModeCache::G3dGenModeCache() {
    numTexGens = 1;
    numChans = 0;
    numTevStages = 1;
    numIndStages = 0;
    cullMode = 2;
    flags = 0;
}

/* 0x80088E18 (0xC): the display lists dirty. */
inline G3dDlDirty::G3dDlDirty() {
    dirty = true;
}

extern "C" {

G3dDlDirty g3d_state_dl_dirty;
G3dGenModeCache g3d_state_gen_mode_cache;
StatePairTable g3d_state_tex_coord_scale_cache;
G3dTexObjCache g3d_state_tex_obj_cache;
G3dTlutObjCache g3d_state_tlut_obj_cache;
G3dVtxDescCache g3d_state_vtx_desc_cache;
G3dCurrentMtxReset g3d_state_current_mtx_reset;
G3dTexMtxFlags g3d_state_tex_mtx_flags;
G3dTexMtxFuncTypes g3d_state_tex_mtx_func_types;
G3dTevCache g3d_state_cached_tev;
G3dViewMtxArrays g3d_state_view_mtx_arrays;
s8 g3d_state_nrm_mtx_scratch_init;
Mat33 g3d_state_nrm_mtx_scratch;
G3dTexProjTable g3d_state_tex_proj_table;
G3dFogTable g3d_state_fog_table;
G3dLightTable g3d_state_light_table;
G3dCameraTable g3d_state_camera_table;
GXRenderModeObj g3d_state_render_mode;
G3dZCompCache g3d_state_zcomp_cache;
G3dIndMtxCallbackStd g3d_state_ind_mtx_callback_std;
G3dIndMtxCallback* g3d_state_ind_mtx_hook = &g3d_state_ind_mtx_callback_std;

} /* extern "C" */

/* Retail calls the objects' constructors out of line from the static initialiser. */
#pragma dont_inline on
