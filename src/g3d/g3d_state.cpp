/*
 * g3d/g3d_state.cpp - the nw4r g3d `G3DState` cluster: GX state writes and the camera matrix table the
 *   `mtxID < NUM_CAMERA` asserts guard.
 * RANGE. .text 0x8008452C-0x80088E24 (170 functions); extab, extabindex, .ctors 0x8056F2D4-0x8056F2D8, .rodata
 *   0x8056F6D0-0x8056F710, .data 0x8058F750-0x8058FCE8 (opens on "g3d_state.cpp"), .bss 0x80682E80-0x80688420,
 *   .sdata 0x80791238-0x80791258, .sbss 0x807948F0-0x80794910, .sdata2 0x80795E70-0x80795E98.  Left seam:
 *   `g3d/g3d_scnroot.cpp`'s `.data` fragment ends at 0x8058F74A; the right edge is `g3d/g3d_resvtx.cpp`, whose
 *   fragment opens at 0x8058FCE8.
 * NAMES. Map stems (`fn_`/`dtor_`, no mangling to derive from) for the unwritten rows;
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
 *   ResGenMode's GXGet*, ResTev's ref/ptr/GetClassName, ResMatChan::ptr and ResTexSrt's
 *   ref/ptr/GetTexMtxMode/GetTexSrtFlag/IsIdentityTexMtx/IsExist, ResShp::IsVtxAttrEnabled
 *   are nw4r's members.
 * RESIDUALS. 29 functions unwritten (objdiff scores them zero) in 12 runs:
 *   0x80084630-0x8008503C and 0x8008715C-0x800873E4 (the projection functions and the texture-SRT load: their
 *     matrix copy is `g3d/g3d_calcview.cpp`'s mtx34_copy_ps, 32 call sites in other units; fn_80084B9C also calls
 *     `MTX/vec.c`'s fn_804C6C60), 0x8008540C-0x80085478 (the light table constructor: the light objects'
 *     constructor and destructor are `g3d/fn_80075DCC.cpp` stems), 0x800854B8-0x8008569C and 0x800856F4-0x80085ACC
 *     (g3d_light_table_set_setting, g3d_light_table_load_light_set: their light load is `EXI/ProbeBarnacle.c`'s
 *     GXLoadLightObjImm), 0x80085C70-0x80085D4C (g3d_camera_table_set_camera: its view-matrix getter
 *     `Camera::GetCameraMtx` is also called by `sound/fn_800E3CBC.cpp`), 0x80087AD0-0x80087C80
 *     (g3d_state_load_shp_prim: its indexed matrix loads are `RVLGX/GXTexture_tail.cpp`'s unnamed fn_804BA560 and
 *     fn_804BA5F0), 0x80087F18-0x80087FA8 (g3d_view_mtx_arrays_nrm_mtx: it
 *     calls `fn_8004CAD8.cpp`'s fn_800516F0 and `hud/pl_frame_sync.cpp`'s mtx34_to_mtx33), 0x80088050-0x8008812C and
 *     0x8008819C-0x80088250 (the fog set and g3d_fog_table_load: the Fog members are `g3d/fn_80075DCC.cpp` m2c
 *     stems, and GXSetFog has no declaration in `RVLGX/GXTexture_tail.cpp`), 0x80088574-0x80088584 (its copy is
 *     `main.cpp`'s render_mode_copy), 0x80088AD0-0x80088E24 (the `.ctors` static constructor fn_80088AD0 and the
 *     globals' constructors: the globals would become definitions).
 *   G3dRandomAccessTag: its inline constructor is empty on purpose (the tag temporary stays uninitialised, and
 *     retail copies an uninitialised byte).
 *   __ct__14G3dCameraTableFv: register choice only (retail alternates r30/r31 through the member array loops).
 *   fn_8008455C: one relocation argument differs.
 *   G3dIndMtxCallback, G3dIndMtxCallbackStd: the constructors' and destructors' empty bodies are complete (the
 *     compiler emits the vtable stores, the base constructor call and the deleting tail).
 *   g3d_tex_obj_cache_load and SetNrmMapMtx save one register more than retail (`_savegpr_23`/`_restgpr_23` for
 *     `_savegpr_24`, `_savegpr_25`/`_restgpr_25` for `_savegpr_26`).
 *   G3dIndMtxCallbackStd::SetNrmMapMtx: retail reloads the light direction for setVec3 where ours reuses the
 *     registers it loaded for the first row (tried: every store through `mtx[idx]`, worse).
 *   The map splits 0x800884AC, 0x80088560 and 0x80088700 out of their callers' rows: each is a static function its
 *     caller tail-branches into, and dtk's analysis had merged it into the caller.
 *   g3d_state_load_tex_obj, g3d_state_load_tlut_obj: the by-value copy's address is formed after the cache's
 *     (retail forms it first).
 *   g3d_tex_obj_cache_load, g3d_tlut_obj_cache_load: retail strides the compared entry with its own offset
 *     register and recomputes the copied one (`slwi`/`mulli`); ours keeps pointers (tried: a separate offset
 *     counter, an inline store helper - no better).
 *   g3d_tev_cache_update, g3d_state_load_mat_ind_mtx_dl, g3d_state_load_mat_ind_mtx: register order only.
 *   flipcheck: `.text` short of the claim; `.ctors`, `.bss`, `.sbss`, `.rodata` and `.sdata2` are claimed and not
 *     emitted, `.data` is 0xAC of 0x598 and `.sdata` 0x4 of 0x20 (the globals and the asserts' strings are declared,
 *     not defined).
 * SHAPES. `#pragma fp_contract off` around g3d_state_load_mat_chan: retail keeps `fmuls` + `fadds` for the scaled
 *   ambient (it took the function from 97.7 to 100).  File-scope `#pragma pool_data off`: retail gives each assert string its own `lis`/`addi` (it took
 *   g3d_tex_proj_build to 100).  File-scope `#pragma peephole off`: retail keeps every `rlwinm`/`clrlwi` + `cmpwi` and unfused
 *   `clrlwi` + `slwi` pairs (it took fn_800856A4/fn_800856B8/fn_80086610/fn_800868E4 to 100 with no row lost).
 *   The generation-mode word is `(ind << 16 | cull << 14) | ((tev - 1) << 10 | (gens | chans << 4))`, that
 *   grouping.
 */

#include "types.h"
#include "nw4r/math.h"
#include "gx.h"              /* GXWGFifo, the 0xCC008000 write window (rule 1) */
#include "g3d/g3d_resmat.h"   /* nw4r::g3d::ResGenMode (rule 2) */
#include "g3d/g3d_anmchr.h"    /* TypeObj::GetTypeName and operator==, owned by g3d/g3d_anmchr.cpp (rule 2) */
#include "g3d/fn_80063888.h"   /* the cluster declarations, owned by g3d/fn_80063888.cpp (rule 2) */
#include "fn_8004CAD8.h"       /* mtx34_identity/MTX34_ctor, owned by fn_8004CAD8.cpp (rule 2) */
#include "g3d/g3d_camera.h"    /* fn_80075390..fn_80075620, owned by g3d/g3d_camera.cpp (rule 2) */
#include "g3d/fn_80075DCC.h"    /* fn_8007B5F4/fn_8007BB8C, GDWriteXFCmd, owned by g3d/fn_80075DCC.cpp (rule 2) */
#include "g3d/g3d_resshp.h"     /* nw4r::g3d::ResTev (rule 2) */
#include "g3d/g3d_state.h"
#include "gx/fn_8009AA78.h"     /* GDSetTexCoordScale2, GDSetGenMode2 (rule 2) */
#include "gx/GDSetIndTexMtx.h"  /* GDSetIndTexMtx, GDResetCurrentMtx (rule 2) */
#include "draw_shape/mtx34_copy.h" /* mtx34_copy (rule 2) */
#include "g3d/g3d_gpu.h"         /* GDSetCurrentMtx, GDLoadTexMtxImm3x3, Array8, Mat33 (rule 2) */
#include "EXI/ProbeBarnacle.h"   /* GXSetArray (rule 2) */
#include "g3d/g3d_calcview.h"    /* mtx34_concat (rule 2) */
#include "mh3_pad.h"             /* setVec3 (rule 2) */
#include "nw4r/fn_805012C4.h"    /* nw4r::math::MTX34Zero (rule 2) */
#include "font/gx_tex_obj_copy.h" /* gx_tex_obj_copy (rule 2) */
#include "RVLGX/GXGetTexObjWidth.h" /* GXGetTexObjWidth, GXGetTexObjHeight (rule 2) */
#include "OS/PSMTXInverse.h"     /* PSMTXInverse (rule 2) */

#pragma peephole off
#pragma pool_data off
#include "RVLGX/GXSetTevOrder.h" /* GXLoadTlut, GXSetZCompLoc (rule 2) */

/* ------------------------------------------------------------------------------------------------ */
/* externs: the SDK and the neighbouring units this one calls (the map owns their names)             */
/* ------------------------------------------------------------------------------------------------ */

/* The panic file/format strings the target references as map symbols, declared here rather than as
 * literals for the same reason `g3d_camera.cpp` does: `-str reuse` would pool a literal. */
extern const char lbl_8056F6D0[]; /* "ScnRoot" */
extern const char lbl_8058F750[]; /* "g3d_state.cpp" */
extern const char lbl_8058F80C[]; /* "NW4R:Failed assertion mtxID < NUM_CAMERA && mtxID >= 0" */

/* `.sdata` and `.sbss` globals the range's small accessors hand back the address of. */
extern u32 lbl_8079124C;

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
            nw4r::db::Panic(lbl_8058F750, line, msg, (ptr));                                     \
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
    /* +0x000 */ GXTexObj texObj[8];
    /* +0x100 */ u8 mMask;
    /* +0x101 */ u8 pad_0x101[0x1F];
};

/* The TLUT-object cache (`g3d_state_tlut_obj_cache`): the loaded objects and a valid bit per slot.  size: 0x64 */
struct G3dTlutObjCache {
    /* +0x00 */ GXTlutObj tlut[8];
    /* +0x60 */ u16 validMask;
    /* +0x62 */ u8 pad_0x62[2];
};

/* The callback the material's indirect matrices are offered to before they load: `Exec` adds the callback's
 * matrices to the operation, `Reset` drops them and `SetNrmMapMtx` builds one from a light.  size: 0x4 */
class G3dIndMtxCallback {
public:
    G3dIndMtxCallback();
    virtual void Exec(struct G3dIndMtxOp* pOp) = 0;
    virtual ~G3dIndMtxCallback();
    virtual void Reset() = 0;
    virtual void SetNrmMapMtx(_GXIndTexMtxID id, const nw4r::math::VEC3* pLightDir,
                              const nw4r::math::MTX34* pNrmMtx,
                              nw4r::g3d::ResMatMiscData::IndirectMethod method) = 0;
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

/* One light set of the light table: the light indices it selects (`fn_80085B54` reads one).  size: 0xC */
struct G3dLightSet {
    /* +0x00 */ u8 mBytes[0xC];
};

/* The light table (`g3d_state_light_table`): the light setting's header, the loaded light set and the masks and
 * light indices it produced, the 128 light objects and the 128 light sets.  size: 0x2A2C */
struct G3dLightTable {
    /* +0x0000 */ u8 pad_0x0000[0x10];
    /* +0x0010 */ s32 loadedLightSet;      /* -1: none (`fn_80085478`); selects the row `fn_80085B54` reads */
    /* +0x0014 */ u32 maskDiffColor;
    /* +0x0018 */ u32 maskDiffAlpha;
    /* +0x001C */ u32 maskSpecColor;
    /* +0x0020 */ u32 maskSpecAlpha;
    /* +0x0024 */ s8 loadedLightIdx[8];    /* -1: none */
    /* +0x002C */ G3dLightObj lightObj[0x80];
    /* +0x222C */ u8 pad_0x222C[0x200];
    /* +0x242C */ G3dLightSet lightSet[0x80];
};

/* The object `fn_80085344` sets the validity byte of. */
struct StateValidFlag {
    /* +0x00 */ u8 mValid;
}; /* size: 0x1 (approximation) */

/* The object `fn_8008455C` dispatches on: its first word is a vtable and the call is slot +0x14. */
class G3dVtObject {
public:
    virtual void m00(); /* +0x00 */
    virtual void m04(); /* +0x04 */
    virtual void m08(); /* +0x08 */
    virtual void m0C(); /* +0x0C */
    virtual void m10(); /* +0x10 */
    virtual u32 m14();  /* +0x14 - the slot fn_8008455C calls */
}; /* size: 0x4 */

extern "C" {

/* ------------------------------------------------------------------------------------------------ */
/* forward declarations (one per function this unit defines; keeps the source order free)             */
/* ------------------------------------------------------------------------------------------------ */

void* fn_8008452C(void);
u32 fn_8008455C(G3dVtObject* pSelf);
u32 fn_80084594(void* pSelf, u32* pArg);
void* fn_80084600(void);
void fn_80085344(StateValidFlag* pSelf);
void fn_80085478(G3dLightTable* pSelf);
u32* fn_8008569C(G3DResRef* pSelf);
BOOL fn_800856A4(StateFlags* pSelf);
BOOL fn_800856B8(StateFlags* pSelf);
u32* fn_800856CC(G3DResRef* pSelf);
G3dLightObj* fn_800856D4(G3dLightTable* pSelf, u32 idx);
u8* g3d_find_s32(u8* p, u8* pEnd, const s32* pValue);
s32 g3d_distance(const s8* first, const s8* last);
s32 g3d_distance_random_access(const s8* first, const s8* last, G3dRandomAccessTag);
s8* g3d_find_s8(s8* p, s8* pEnd, const s8* pDelim);
u8 fn_80085B54(G3dLightTable* pSelf, u32 byteIdx);
void fn_80085F3C(StatePairTable* pSelf, u32 idx, u16 a, u16 b);
void fn_80086118(G3dTexObjCache* pSelf, u32 bit);
void fn_80086138(StateWord3* pDst, const StateWord3* pSrc);
BOOL fn_80086154(const StateWord3* pA, const StateWord3* pB);
void fn_80086610(StatePairTable* pSelf, u8 arg);
u8 fn_80086640(StateByte* pSelf);
bool fn_80086770(G3dDlDirty* pSelf);
void fn_800867A0(G3dDlDirty* pSelf);
u8 fn_80086AA0(StateByte3* pSelf);
u8 fn_80086FFC(StateByte1* pSelf);
void fn_800868D8(u32 value);
void fn_800868E4(u32 value);
void fn_800868A0(u32 value);

/* ------------------------------------------------------------------------------------------------ */
/* g3d_state.cpp                                                                                     */
/* ------------------------------------------------------------------------------------------------ */

/* The `ScnRoot` singleton lookup: `fn_8007BB8C` stores the found object through its out-parameter and
 * returns that parameter, so the result is the word it stored. */
void* fn_8008452C(void) {
    void* pScnRoot;
    return *fn_8007BB8C(&pScnRoot, lbl_8056F6D0);
}

/* The same lookup, emitted a second time for the state object's own caller. */
void* fn_80084600(void) {
    void* pScnRoot;
    return *fn_8007BB8C(&pScnRoot, lbl_8056F6D0);
}

/* A virtual dispatch on slot +0x14 whose result is handed to the `TypeObj::GetTypeName` unwrapper. */
u32 fn_8008455C(G3dVtObject* pSelf) {
    u32 value = pSelf->m14();
    return (u32)reinterpret_cast<const nw4r::g3d::G3dObj::TypeObj*>(&value)->GetTypeName();
}

/* Register `pArg`'s bound object with the state's table, and on a miss insert it under the current
 * `ScnRoot`. */
u32 fn_80084594(void* pSelf, u32* pArg) {
    void* pScnRoot = fn_80084600();
    if ((*reinterpret_cast<const nw4r::g3d::G3dObj::TypeObj*>(pArg) == *reinterpret_cast<const nw4r::g3d::G3dObj::TypeObj*>(&pScnRoot))) {
        return 1;
    }
    u32 key = *pArg;
    return fn_8007B5F4(pSelf, &key);
}

void fn_80085344(StateValidFlag* pSelf) {
    pSelf->mValid = 1;
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

u32* fn_8008569C(G3DResRef* pSelf) {
    return &pSelf->mPayload;
}

u32* fn_800856CC(G3DResRef* pSelf) {
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

/* The bounds-tested table accessor.  The target puts the NULL path last, so the guard is spelled
 * `if (idx <= 0x7F) { return ...; } return NULL;`. */
G3dLightObj* fn_800856D4(G3dLightTable* pSelf, u32 idx) {
    if (idx <= 0x7F) {
        return &pSelf->lightObj[idx];
    }
    return NULL;
}

/* 0x80085ACC (0x2C): returns the number of bytes from `first` to `last`. */
s32 g3d_distance(const s8* first, const s8* last) {
    return g3d_distance_random_access(first, last, G3dRandomAccessTag());
}

/* 0x80085AF8 (0x8): returns the number of bytes from `first` to `last` (the random-access form). */
s32 g3d_distance_random_access(const s8* first, const s8* last, G3dRandomAccessTag) {
    return last - first;
}

u8 fn_80085B54(G3dLightTable* pSelf, u32 byteIdx) {
    return pSelf->lightSet[pSelf->loadedLightSet].mBytes[byteIdx];
}

/* The two byte-scan loops of the state's key tables: walk `p` up to `pEnd` while the byte does not
 * match, and return where it stopped. */
u8* g3d_find_s32(u8* p, u8* pEnd, const s32* pValue) {
    while (p != pEnd && *(s8*)p != *pValue) {
        p++;
    }
    return p;
}

s8* g3d_find_s8(s8* p, s8* pEnd, const s8* pDelim) {
    while (p != pEnd && *p != *pDelim) {
        p++;
    }
    return p;
}

/* Store the pair into the table at `idx` and mark the table dirty (`(flags | 2) & ~1`). */
void fn_80085F3C(StatePairTable* pSelf, u32 idx, u16 a, u16 b) {
    pSelf->mPairs[idx].mA = a;
    pSelf->mPairs[idx].mB = b;
    pSelf->mFlags = (pSelf->mFlags | 2) & ~1;
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

} /* extern "C" */

/* 0x8008631C (0x74): returns the generation mode's cullMode, 3 for an empty handle. */
_GXCullMode nw4r::g3d::ResGenMode::GXGetCullMode() const {
    if (!IsValid()) {
        nw4r::db::Panic("g3d_resmat_ac.h", 0xDF, "NW4R:Failed assertion IsValid()");
    }
    if (IsValid()) {
        return (_GXCullMode)ptr()->cullMode;
    }
    return (_GXCullMode)3;
}

/* 0x80086398 (0x74): returns the generation mode's nInds, 0 for an empty handle. */
u8 nw4r::g3d::ResGenMode::GXGetNumIndStages() const {
    if (!IsValid()) {
        nw4r::db::Panic("g3d_resmat_ac.h", 0xD8, "NW4R:Failed assertion IsValid()");
    }
    if (IsValid()) {
        return ptr()->nInds;
    }
    return 0;
}

/* 0x8008640C (0x74): returns the generation mode's nTevs, 0 for an empty handle. */
u8 nw4r::g3d::ResGenMode::GXGetNumTevStages() const {
    if (!IsValid()) {
        nw4r::db::Panic("g3d_resmat_ac.h", 0xD1, "NW4R:Failed assertion IsValid()");
    }
    if (IsValid()) {
        return ptr()->nTevs;
    }
    return 0;
}

/* 0x80086480 (0x74): returns the generation mode's nChans, 0 for an empty handle. */
u8 nw4r::g3d::ResGenMode::GXGetNumChans() const {
    if (!IsValid()) {
        nw4r::db::Panic("g3d_resmat_ac.h", 0xCA, "NW4R:Failed assertion IsValid()");
    }
    if (IsValid()) {
        return ptr()->nChans;
    }
    return 0;
}

/* 0x800864F4 (0x74): returns the generation mode's nTexGens, 0 for an empty handle. */
u8 nw4r::g3d::ResGenMode::GXGetNumTexGens() const {
    if (!IsValid()) {
        nw4r::db::Panic("g3d_resmat_ac.h", 0xC3, "NW4R:Failed assertion IsValid()");
    }
    if (IsValid()) {
        return ptr()->nTexGens;
    }
    return 0;
}

/* 0x80086390 (0x8): returns the generation-mode block. */
const nw4r::g3d::ResGenModeData* nw4r::g3d::ResGenMode::ptr() const {
    return mpData;
}

extern "C" {

/* The state's dirty-flush hook: only flush when the pending flags say so and the new value differs. */
void fn_80086610(StatePairTable* pSelf, u8 arg) {
    if ((pSelf->mFlags & 2) != 0 && (pSelf->mFlags & 1) == 0 && arg != 0) {
        g3d_tex_coord_scale_load(pSelf, arg);
    }
}

u8 fn_80086640(StateByte* pSelf) {
    return pSelf->mByte;
}

} /* extern "C" */

/* 0x800866FC (0x64): returns the TEV block, panicking on a NULL handle. */
const nw4r::g3d::ResTevData& nw4r::g3d::ResTev::ref() const {
    if (!IsValid()) {
        nw4r::db::Panic("g3d_restev_ac.h", 0x25, "%s::%s: Object not valid.", GetClassName(), "ref");
    }
    return *ptr();
}

/* 0x80086760 (0x8): returns the TEV block. */
const nw4r::g3d::ResTevData* nw4r::g3d::ResTev::ptr() const {
    return mpData;
}

/* 0x80086768 (0x8): returns the class name. */
const char* nw4r::g3d::ResTev::GetClassName() {
    return (const char*)&lbl_8079124C;
}

extern "C" {

/* Read the byte and clear it, returning what it held. */
bool fn_80086770(G3dDlDirty* pSelf) {
    bool value = pSelf->dirty;
    fn_800867A0(pSelf);
    return value;
}

void fn_800867A0(G3dDlDirty* pSelf) {
    pSelf->dirty = false;
}

u8 fn_80086AA0(StateByte3* pSelf) {
    return pSelf->mByte;
}

u8 fn_80086FFC(StateByte1* pSelf) {
    return pSelf->mByte;
}

/* The two write-gather-pipe stores (the window lives in `gx.h`, rule 1). */
void fn_800868D8(u32 value) {
    GXWGFifo.u32 = value;
}

void fn_800868E4(u32 value) {
    u8 narrow = value;
    GXWGFifo.u8 = narrow;
}

/* The `0x61`-tagged pipe command followed by its word. */
void fn_800868A0(u32 value) {
    fn_800868E4(0x61);
    fn_800868D8(value);
}

} /* extern "C" */

/* ------------------------------------------------------------------------------------------------ */
/* The material-state caches: the indirect matrices, the z-compare location, the texture/TLUT object  */
/* caches, the generation mode and the TEV/pixel/indirect display lists                              */
/* ------------------------------------------------------------------------------------------------ */

/* The indirect-matrix operation a material's `ResMatIndMtxAndScale` fills: a flag bit per matrix that is set,
 * and the three matrices.  size: 0x94 */
struct G3dIndMtxOp {
    /* +0x00 */ u32 flags;
    /* +0x04 */ nw4r::math::MTX34 mtx[3];
};

/* The cached z-compare location (`lbl_80794908`).  size: 0x8 */
struct G3dZCompCache {
    /* +0x00 */ u32 flags;
    /* +0x04 */ u8 beforeTex;
};

/* The cached generation mode (`lbl_80682E80`): the four counts and the cull mode `ResGenMode` holds, and the
 * dirty/valid flags.  size: 0xC */
struct G3dGenModeCache {
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
extern u32 g3d_state_cached_tev;
extern G3dZCompCache g3d_state_zcomp_cache;
extern const u8 g3d_state_cull_mode_hw[4];

/* The texture-object cache load (unwritten; its block copy is a `font/flfnt.cpp` helper). */
void g3d_tex_obj_cache_load(G3dTexObjCache* pSelf, nw4r::g3d::ResTexObj texObj);
BOOL g3d_tex_obj_equal(const GXTexObj* pA, const GXTexObj* pB);
void g3d_tlut_obj_cache_load(G3dTlutObjCache* pSelf, nw4r::g3d::ResTlutObj tlutObj);
void g3d_zcomp_cache_set(G3dZCompCache* pSelf, u8 beforeTex);
void g3d_gen_mode_cache_set(G3dGenModeCache* pSelf, nw4r::g3d::ResGenMode genMode);
BOOL g3d_tev_cache_update(u32* pCached, nw4r::g3d::ResTev tev);
void g3d_gen_mode_cache_load(G3dGenModeCache* pSelf);
void g3d_tex_coord_scale_set_tex_maps(StatePairTable* pSelf, nw4r::g3d::ResTev tev);
void g3d_gd_set_gen_mode(u8 numTexGens, u8 numChans, u8 numTevStages, u8 numIndStages, u32 cullMode);

/* Reads the material's three indirect matrices into the operation, one flag bit per matrix found. */
G3dIndMtxOp* g3d_ind_mtx_op_init(G3dIndMtxOp* pSelf, const nw4r::g3d::ResMatIndMtxAndScale* pInd) {
    nw4r::math::MTX34* pMtx = pSelf->mtx;
    do {
        MTX34_ctor(pMtx);
        pMtx++;
    } while (pMtx < &pSelf->mtx[3]);
    pSelf->flags = 0;
    if (pInd->GXGetIndTexMtx(GX_ITM_0, &pSelf->mtx[0])) {
        pSelf->flags |= 1;
    }
    if (pInd->GXGetIndTexMtx(GX_ITM_1, &pSelf->mtx[1])) {
        pSelf->flags |= 2;
    }
    if (pInd->GXGetIndTexMtx(GX_ITM_2, &pSelf->mtx[2])) {
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

/* Loads the material's texture objects through the cache. */
void g3d_state_load_tex_obj(nw4r::g3d::ResTexObj texObj) {
    if (texObj.IsValid()) {
        g3d_tex_obj_cache_load(&g3d_state_tex_obj_cache, texObj);
        fn_80085344((StateValidFlag*)&g3d_state_dl_dirty);
    }
}

/* 0x80085E48 (0xF4): loads every texture object of the material that differs from the cached one and records its
 * size for the coordinate scales. */
void g3d_tex_obj_cache_load(G3dTexObjCache* pSelf, const nw4r::g3d::ResTexObj texObj) {
    u32 i;
    for (i = 0; i < 8; i++) {
        if (texObj.IsValidTexObj((_GXTexMapID)i)) {
            const GXTexObj* pObj = texObj.GetTexObj((_GXTexMapID)i);
            u8 bit = 1 << i;
            if ((pSelf->mMask & bit) == 0 || !g3d_tex_obj_equal(pObj, &pSelf->texObj[i])) {
                pSelf->mMask |= bit;
                gx_tex_obj_copy(&pSelf->texObj[i], pObj);
                GXLoadTexObj(pObj, i);
                u16 height = GXGetTexObjHeight(pObj);
                fn_80085F3C(&g3d_state_tex_coord_scale_cache, i, GXGetTexObjWidth(pObj), height);
            }
        }
    }
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

/* Loads the material's TLUT objects through the cache. */
void g3d_state_load_tlut_obj(nw4r::g3d::ResTlutObj tlutObj) {
    if (tlutObj.IsValid()) {
        g3d_tlut_obj_cache_load(&g3d_state_tlut_obj_cache, tlutObj);
        fn_80085344((StateValidFlag*)&g3d_state_dl_dirty);
    }
}

/* Loads each valid TLUT that is not already cached, and invalidates the texture cached for that slot. */
void g3d_tlut_obj_cache_load(G3dTlutObjCache* pSelf, nw4r::g3d::ResTlutObj tlutObj) {
    u32 i = 0;
    do {
        if (tlutObj.IsValidTlut((_GXTlut)i)) {
            const GXTlutObj* pTlut = static_cast<const nw4r::g3d::ResTlutObj&>(tlutObj).GetTlut((_GXTlut)i);
            u16 bit = 1 << i;
            if ((pSelf->validMask & bit) == 0 || !fn_80086154((const StateWord3*)&pSelf->tlut[i],
                                                             (const StateWord3*)pTlut)) {
                pSelf->validMask |= bit;
                fn_80086138((StateWord3*)&pSelf->tlut[i], (const StateWord3*)pTlut);
                GXLoadTlut(pTlut, i);
                fn_80086118(&g3d_state_tex_obj_cache, i);
            }
        }
        i++;
    } while (i < 8);
}

/* Caches the material's generation mode. */
void g3d_state_set_gen_mode(nw4r::g3d::ResGenMode genMode) {
    if (genMode.IsValid()) {
        g3d_gen_mode_cache_set(&g3d_state_gen_mode_cache, genMode);
    }
}

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

/* Loads the material's TEV display list unless it is the one already loaded. */
void g3d_state_load_tev(nw4r::g3d::ResTev tev) {
    if (tev.IsValid()) {
        if (!g3d_tev_cache_update(&g3d_state_cached_tev, tev)) {
            g3d_gen_mode_cache_load(&g3d_state_gen_mode_cache);
            tev.CallDisplayList(fn_80086770(&g3d_state_dl_dirty));
            g3d_tex_coord_scale_set_tex_maps(&g3d_state_tex_coord_scale_cache, tev);
            fn_80086610(&g3d_state_tex_coord_scale_cache, fn_80086640((StateByte*)&g3d_state_gen_mode_cache));
        }
    }
}

/* Copies the TEV's per-coordinate texture map table into the scale cache when it changed. */
void g3d_tex_coord_scale_set_tex_maps(StatePairTable* pSelf, nw4r::g3d::ResTev tev) {
    const nw4r::g3d::ResTevData& r = tev.ref();
    const u32* pMaps = (const u32*)r.texMapID;
    if (((u32)pMaps & 3) != 0) {
        nw4r::db::Panic(lbl_8058F750, 0x249, "NW4R:Failed assertion ((u32)x & 0x3) == 0");
    }
    if ((pSelf->mFlags & 2) == 0 || pMaps[0] != pSelf->texMapWord[0] || pMaps[1] != pSelf->texMapWord[1]) {
        pSelf->texMapWord[0] = pMaps[0];
        pSelf->texMapWord[1] = pMaps[1];
        pSelf->mFlags = (pSelf->mFlags | 2) & ~1;
    }
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

/* Records `tev` as the loaded TEV, returning whether it already was. */
BOOL g3d_tev_cache_update(u32* pCached, nw4r::g3d::ResTev tev) {
    if (!tev.IsValid()) {
        nw4r::db::Panic(lbl_8058F750, 0x3B2, "NW4R:Failed assertion rhs.IsValid()");
    }
    if (*pCached == (u32)tev.ptr()) {
        return TRUE;
    }
    *pCached = (u32)tev.ptr();
    return FALSE;
}

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
        bool dirty = fn_80086770(&g3d_state_dl_dirty);
        ind.CallDisplayList(fn_80086AA0((StateByte3*)&g3d_state_gen_mode_cache), dirty);
    }
}

/* Loads the material's indirect matrices: the display list, then the operation the callback may edit. */
void g3d_state_load_mat_ind_mtx(nw4r::g3d::ResMatIndMtxAndScale ind, G3dIndMtxCallback* pCallback) {
    if (ind.IsValid()) {
        G3dIndMtxOp op;
        g3d_state_load_mat_ind_mtx_dl(ind);
        nw4r::g3d::ResMatIndMtxAndScale copy = ind;
        g3d_ind_mtx_op_init(&op, &copy);
        pCallback->Exec(&op);
        g3d_ind_mtx_op_load(&op);
    }
}

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
    /* +0x000 */ G3dTexProjEntry entry[0x100];
};

/* The shape's vertex description the pre-primitive display list was last loaded with.  size: 0xC */
struct G3dVtxDescCache {
    /* +0x00 */ u32 word[3];
};

/* The view matrix arrays the indexed position/normal/texture loads read (`g3d_state_view_mtx_arrays`).
 * size: 0xC */
struct G3dViewMtxArrays {
    /* +0x00 */ nw4r::math::MTX34* pViewPosMtx;
    /* +0x04 */ Mat33* pViewNrmMtx;
    /* +0x08 */ nw4r::math::MTX34* pViewTexMtx;
};

extern "C" {

extern G3dViewMtxArrays g3d_state_view_mtx_arrays;
extern u8 g3d_state_current_mtx_reset;
extern G3dIndMtxCallback* g3d_state_ind_mtx_hook;

void g3d_gen_mode_cache_load_full(G3dGenModeCache* pSelf);
Mat33* g3d_state_get_nrm_mtx(u32 idx);
Mat33* g3d_view_mtx_arrays_nrm_mtx(G3dViewMtxArrays* pSelf, u32 idx);
void g3d_vtx_desc_copy(G3dVtxDescCache* pDst, const G3dVtxDescCache* pSrc);
BOOL g3d_vtx_desc_equal(const G3dVtxDescCache* pA, const G3dVtxDescCache* pB);
void g3d_current_mtx_reset_clear(u8* pFlag);
void g3d_current_mtx_reset(u8* pFlag);
void g3d_view_mtx_arrays_set(G3dViewMtxArrays* pSelf, nw4r::math::MTX34* pPos, Mat33* pNrm,
                             nw4r::math::MTX34* pTex);
f32* g3d_f32_ref(f32* p);

/* 0x80086F94 (0x68): writes the cached generation mode in full when it is complete and either half is stale. */
void g3d_gen_mode_cache_load_full(G3dGenModeCache* pSelf) {
    if ((pSelf->flags & 4) != 0 && (pSelf->flags & 3) != 3) {
        GDSetGenMode2(pSelf->numTexGens, pSelf->numChans, pSelf->numTevStages, pSelf->numIndStages,
                      pSelf->cullMode);
        pSelf->flags |= 3;
    }
}

} /* extern "C" */

extern "C" {

/* 0x80087004 (0x30): writes a colour channel's control word with its light mask (XF 0x100E + chan). */
void g3d_gd_set_chan_ctrl_lights(u32 chan, u32 ctrl, u32 lightMask) {
    ctrl = (ctrl & ~0x783C) | ((lightMask & 0xF) << 2);
    ctrl |= ((lightMask >> 4) & 0xF) << 11;
    GDWriteXFCmd((chan & 3) + 0x100E, ctrl);
}

/* 0x80087034 (0x30): writes the same word with the lighting enable bit cleared too. */
void g3d_gd_set_chan_ctrl_unlit(u32 chan, u32 ctrl, u32 lightMask) {
    ctrl = (ctrl & ~0x783E) | ((lightMask & 0xF) << 2);
    ctrl |= ((lightMask >> 4) & 0xF) << 11;
    GDWriteXFCmd((chan & 3) + 0x100E, ctrl);
}

/* 0x80087064 (0x14): writes a colour channel's ambient colour (XF 0x100A + chan). */
void g3d_gd_set_chan_amb_color(u32 chan, GXColor color) {
    GDWriteXFCmd((chan & 1) + 0x100A, *(const u32*)&color);
}

} /* extern "C" */

/* 0x80087078 (0x64): returns the channel block, panicking on a NULL handle. */
const nw4r::g3d::ResMatChanData& nw4r::g3d::ResMatChan::ref() const {
    if (!IsValid()) {
        nw4r::db::Panic("g3d_resmat_ac.h", 0x1D1, "%s::%s: Object not valid.", GetClassName(), "ref");
    }
    return *ptr();
}

/* 0x800870DC (0x8): returns the channel block. */
const nw4r::g3d::ResMatChanData* nw4r::g3d::ResMatChan::ptr() const {
    return mpData;
}

extern "C" {

/* 0x800870E4 (0x78): calls the material's texture-coordinate generation display list for the cached number of
 * coordinates. */
void g3d_state_load_tex_coord_gen(nw4r::g3d::ResMatTexCoordGen texCoordGen) {
    if (texCoordGen.IsValid()) {
        bool dirty = fn_80086770(&g3d_state_dl_dirty);
        texCoordGen.CallDisplayList(fn_80086640((StateByte*)&g3d_state_gen_mode_cache), dirty);
        g3d_gen_mode_cache_load_full(&g3d_state_gen_mode_cache);
    }
}

/* 0x800873E4 (0x1C): clears bit `bit` of the word. */
void g3d_bits_clear(u32* pBits, u32 bit) {
    *pBits &= ~(1 << bit);
}

/* 0x80087400 (0x18): sets bit `bit` of the word. */
void g3d_bits_set(u32* pBits, u32 bit) {
    *pBits |= 1 << bit;
}

/* 0x80087418 (0x20): whether bit `bit` of the word is set. */
BOOL g3d_bits_test(const u32* pBits, u32 bit) {
    return (*pBits & (1 << bit)) != 0;
}

/* 0x80087438 (0x60): swaps the two floats. */
void g3d_swap_f32(f32* pA, f32* pB) {
    f32 tmp = *g3d_f32_ref(pA);
    *pA = *g3d_f32_ref(pB);
    *pB = *g3d_f32_ref(&tmp);
}

/* 0x80087498 (0x4): returns its argument. */
f32* g3d_f32_ref(f32* p) {
    return p;
}

} /* extern "C" */

/* 0x8008749C (0x24): returns the texture-matrix mode. */
u32 nw4r::g3d::ResTexSrt::GetTexMtxMode() const {
    return ref().texMtxMode;
}

/* 0x800874C0 (0x3C): returns coordinate `id`'s four transform flag bits. */
u32 nw4r::g3d::ResTexSrt::GetTexSrtFlag(u32 id) const {
    return (ref().flagTexSrt >> (id * 4)) & 0xF;
}

/* 0x800874FC (0x84): whether coordinate `id` has no transform and an identity effect matrix. */
BOOL nw4r::g3d::ResTexSrt::IsIdentityTexMtx(u32 id) const {
    BOOL result = FALSE;
    if (((ref().flagTexSrt >> (id * 4)) & 0xF) == 0xF && (ref().effect[id].misc_flag & 1) != 0) {
        result = TRUE;
    }
    return result;
}

extern "C" {

/* 0x80087580 (0x90): records texture matrix `idx`'s scene-dependent function type. */
void g3d_tex_mtx_func_type_set(s32* pFuncTypes, u32 idx, s32 funcType) {
    if (idx >= 8) {
        nw4r::db::Panic(lbl_8058F750, 0x363, "NW4R:Failed assertion idx >= 0 && idx < 8");
    }
    if (funcType >= 4) {
        nw4r::db::Panic(lbl_8058F750, 0x364,
                        "NW4R:Failed assertion funcType < MAX_SCNDEPENDENT_TEXMTX_FUNCTYPE");
    }
    pFuncTypes[idx] = funcType;
}

/* 0x80087610 (0x70): returns the texture-matrix mode projection `idx` needs. */
s32 g3d_tex_proj_mtx_mode(G3dTexProjTable* pSelf, u32 idx) {
    if (idx >= 0x100) {
        nw4r::db::Panic(lbl_8058F750, 0x469, "NW4R:Failed assertion idx < NUM_SCNDEPENDENT_TEXMTX_FUNCTYPE");
    }
    if (idx >= 0x100) {
        idx = 0;
    }
    return pSelf->entry[idx].mtxMode;
}

/* 0x80087680 (0x1F0): builds projection `idx`'s matrix into `pOut` from camera `refCamera` and light `refLight`. */
void g3d_tex_proj_build(G3dTexProjTable* pSelf, u32 idx, nw4r::math::MTX34* pOut, s8 refCamera, s8 refLight) {
    if (idx >= 0x100) {
        nw4r::db::Panic(lbl_8058F750, 0x459, "NW4R:Failed assertion idx < NUM_SCNDEPENDENT_TEXMTX_FUNCTYPE");
    }
    if (refCamera >= 0x20) {
        nw4r::db::Panic(lbl_8058F750, 0x45A, "NW4R:Failed assertion ref_camera < NUM_CAMERA");
    }
    if (refLight >= 0x80) {
        nw4r::db::Panic(lbl_8058F750, 0x45B, "NW4R:Failed assertion ref_light < NUM_LIGHT");
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

/* 0x80087870 (0x64): returns the texture-SRT block, panicking on a NULL handle. */
const nw4r::g3d::ResTexSrtData& nw4r::g3d::ResTexSrt::ref() const {
    if (!IsValid()) {
        nw4r::db::Panic("g3d_resmat_ac.h", 0x6B, "%s::%s: Object not valid.", GetClassName(), "ref");
    }
    return *ptr();
}

/* 0x800878D4 (0x8): returns the texture-SRT block. */
const nw4r::g3d::ResTexSrtData* nw4r::g3d::ResTexSrt::ptr() const {
    return static_cast<const ResTexSrtData*>(mpData);
}

/* 0x800878DC (0x9C): whether coordinate `id` has a transform. */
BOOL nw4r::g3d::ResTexSrt::IsExist(u32 id) const {
    if (!IsValid()) {
        nw4r::db::Panic("g3d_resmat_ac.h", 0x71, "NW4R:Failed assertion IsValid()");
    }
    if (IsValid()) {
        return ((1 << (id * 4)) & ptr()->flagTexSrt) != 0;
    }
    return FALSE;
}

extern "C" {

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
            g3d_current_mtx_reset(&g3d_state_current_mtx_reset);
            return;
        }
        GDSetCurrentMtx((Array8*)setting);
        g3d_current_mtx_reset_clear(&g3d_state_current_mtx_reset);
    }
}

/* 0x80087D9C (0x14): whether the view position matrix array is set. */
BOOL g3d_view_mtx_arrays_has_pos(const G3dViewMtxArrays* pSelf) {
    return pSelf->pViewPosMtx != NULL;
}

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
        nw4r::db::Panic(lbl_8058F750, 0x3EE, "NW4R:Pointer must not be NULL (mpViewPosMtxArray)");
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

/* 0x80088048 (0x8): returns the hook the indirect-matrix builder calls. */
G3dIndMtxCallback* g3d_state_get_ind_mtx_hook(void) {
    return g3d_state_ind_mtx_hook;
}

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

/* The fog table (`g3d_state_fog_table`): the valid flags, the loaded fog's index and the 32 fogs.  size: 0x608 */
struct G3dFogTable {
    /* +0x000 */ u32 flags;
    /* +0x004 */ s32 fogIdx;
    /* +0x008 */ u8 pad_0x008[0x600];   /* the 32 0x30-byte fogs */
};

extern "C" {

extern G3dCameraTable g3d_state_camera_table;
extern G3dFogTable g3d_state_fog_table;
extern G3dLightTable g3d_state_light_table;
extern struct RenderModeObj g3d_state_render_mode;
extern G3dVtxDescCache g3d_state_vtx_desc_cache;
extern s32 g3d_state_tex_mtx_func_types[8];
extern u32 g3d_state_tex_mtx_flags;

nw4r::math::MTX34* g3d_camera_table_camera_mtx(G3dCameraTable* pSelf);
nw4r::math::MTX34* g3d_camera_table_inv_camera_mtx(G3dCameraTable* pSelf);
nw4r::math::MTX34* g3d_camera_table_camera_mtx_at(G3dCameraTable* pSelf, u32 idx);
nw4r::math::MTX34* g3d_camera_table_proj_tex_mtx_at(G3dCameraTable* pSelf, u32 idx);
void g3d_fog_table_invalidate(G3dFogTable* pSelf);
void g3d_dl_dirty_set(G3dDlDirty* pSelf);
void g3d_view_mtx_arrays_clear(G3dViewMtxArrays* pSelf);
void g3d_zcomp_cache_invalidate(G3dZCompCache* pSelf);
void g3d_tex_mtx_flags_clear(u32* pFlags);
void g3d_tex_mtx_func_types_clear(s32* pFuncTypes);
void g3d_vtx_desc_cache_invalidate(G3dVtxDescCache* pSelf);
void g3d_vtx_desc_cache_clear(G3dVtxDescCache* pSelf);
void g3d_gen_mode_cache_reset(G3dGenModeCache* pSelf);
void g3d_tev_cache_invalidate(u32* pCached);
void g3d_tlut_obj_cache_invalidate(G3dTlutObjCache* pSelf);
void g3d_tex_coord_scale_invalidate(StatePairTable* pSelf);
void g3d_tex_obj_cache_invalidate(G3dTexObjCache* pSelf);

/* 0x8008812C (0xC): forgets which fogs are loaded. */
void g3d_fog_table_invalidate(G3dFogTable* pSelf) {
    pSelf->flags = 0;
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
            nw4r::db::Panic(lbl_8058F750, 0x69B, "NW4R:Failed assertion result");
        }
        pSelf->flags |= 1;
    }
    return &pSelf->invCameraMtx;
}

/* Whether `idx` names one of the 32 cameras. */
static inline BOOL g3d_camera_idx_valid(u32 idx) {
    BOOL valid = FALSE;
    if (idx <= 31) {
        valid = TRUE;
    }
    return valid;
}

/* 0x800883C4 (0x48): inverts `pSrc` into `pOut`; 0 when `pSrc` is singular. */
u32 mtx34_inverse(MTX34* pOut, const MTX34* pSrc) {
    MTX34* pDst = (MTX34*)mtx34_get_ptr(pOut);
    return PSMTXInverse((const MTX34*)mtx34_const_ptr((u32)pSrc), pDst);
}

/* 0x8008840C (0x10): returns camera `idx`'s matrix. */
nw4r::math::MTX34* g3d_state_get_camera_mtx_at(u32 idx) {
    return g3d_camera_table_camera_mtx_at(&g3d_state_camera_table, idx);
}

/* 0x8008841C (0x84): returns camera `idx`'s matrix, NULL past the table. */
nw4r::math::MTX34* g3d_camera_table_camera_mtx_at(G3dCameraTable* pSelf, u32 idx) {
    if (!g3d_camera_idx_valid(idx)) {
        nw4r::db::Panic(lbl_8058F750, 0x66A, lbl_8058F80C);
    }
    if (idx <= 31) {
        return &pSelf->cameraMtx[idx];
    }
    return NULL;
}

static nw4r::math::MTX34* g3d_camera_table_proj_tex_mtx(G3dCameraTable* pSelf);

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
        nw4r::db::Panic(lbl_8058F750, 0x67E, lbl_8058F80C);
    }
    if (idx <= 31) {
        return &pSelf->projTexMtx[idx];
    }
    return NULL;
}

static nw4r::math::MTX34* g3d_camera_table_env_tex_mtx(G3dCameraTable* pSelf);

/* 0x80088554 (0xC): returns the current camera's environment texture matrix. */
nw4r::math::MTX34* g3d_state_get_env_tex_mtx(void) {
    return g3d_camera_table_env_tex_mtx(&g3d_state_camera_table);
}

/* 0x80088560 (0x14): returns the current camera's environment texture matrix. */
static nw4r::math::MTX34* g3d_camera_table_env_tex_mtx(G3dCameraTable* pSelf) {
    return &pSelf->envTexMtx[pSelf->currentCamera];
}

/* 0x80088584 (0xC): returns the render mode the state keeps. */
struct RenderModeObj* g3d_state_get_render_mode(void) {
    return &g3d_state_render_mode;
}

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
        g3d_tev_cache_invalidate(&g3d_state_cached_tev);
    }
    if (flag & 8) {
        g3d_gen_mode_cache_reset(&g3d_state_gen_mode_cache);
    }
    if (flag & 0x10) {
        g3d_vtx_desc_cache_invalidate(&g3d_state_vtx_desc_cache);
    }
    if (flag & 0x20) {
        g3d_current_mtx_reset_clear(&g3d_state_current_mtx_reset);
    }
    if (flag & 0x40) {
        g3d_tex_mtx_func_types_clear(g3d_state_tex_mtx_func_types);
        g3d_tex_mtx_flags_clear(&g3d_state_tex_mtx_flags);
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

static void g3d_zcomp_cache_clear(G3dZCompCache* pSelf);

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

/* 0x80087FA8 (0x5C): destroys the callback. */
G3dIndMtxCallbackStd::~G3dIndMtxCallbackStd() {
}

/* 0x80088004 (0x44): destroys the callback base. */
G3dIndMtxCallback::~G3dIndMtxCallback() {
}

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

/* 0x80088858 (0x1A0): builds the normal-map matrix of indirect matrix `id` from the light direction and the normal
 * matrix (the specular method adds the half vector's row), or clears it for a method that needs none. */
void G3dIndMtxCallbackStd::SetNrmMapMtx(_GXIndTexMtxID id, const nw4r::math::VEC3* pLightDir,
                                        const nw4r::math::MTX34* pNrmMtx,
                                        nw4r::g3d::ResMatMiscData::IndirectMethod method) {
    if (!(id == GX_ITM_0 || id == GX_ITM_1 || id == GX_ITM_2)) {
        nw4r::db::Panic(lbl_8058F750, 0x9E4,
                        "NW4R:Failed assertion id == GX_ITM_0 || id == GX_ITM_1 || id == GX_ITM_2");
    }
    u32 idx = id - GX_ITM_0;
    if (idx <= 2 && method != nw4r::g3d::ResMatMiscData::WARP && method != nw4r::g3d::ResMatMiscData::FUR) {
        enabled[idx] = 1;
        if (pLightDir != NULL) {
            nw4r::math::MTX34* pMtx = &mtx[idx];
            pMtx->m[0][0] = 0.5f * -pLightDir->x;
            pMtx->m[0][1] = 0.5f * -pLightDir->y;
            pMtx->m[0][2] = 0.5f * -pLightDir->z;
            if (method == nw4r::g3d::ResMatMiscData::NORMAL_MAP_SPEC) {
                nw4r::math::VEC3 half;
                setVec3(&half, pLightDir->x, pLightDir->y, pLightDir->z - 1.0f);
                vec3_normalize_into(&half, &half);
                mtx[idx].m[1][0] = 0.5f * -half.x;
                mtx[idx].m[1][1] = 0.5f * -half.y;
                mtx[idx].m[1][2] = 0.5f * -half.z;
            } else {
                pMtx->m[1][2] = 0.0f;
                pMtx->m[1][1] = 0.0f;
                pMtx->m[1][0] = 0.0f;
            }
            if (pNrmMtx == NULL) {
                nw4r::db::Panic(lbl_8058F750, 0xA06, "NW4R:Pointer must not be NULL (pNrmMtx)");
            }
            mtx34_concat(pMtx, &mtx[idx], pNrmMtx);
        } else {
            nw4r::math::MTX34Zero(&mtx[idx]);
        }
    }
}

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
/* The identity projection, the channel load and the fog accessors                                   */
/* ------------------------------------------------------------------------------------------------ */

/* The fog data a `G3dFog` handle points at (`g3d_fog_ref`); only used through the pointer. */
struct G3dFogData;

/* A fog handle: the fog data's address.  size: 0x4 */
struct G3dFog {
    /* +0x00 */ G3dFogData* mpData;
};

extern "C" {

void g3d_fog_table_load(G3dFogTable* pSelf, s32 idx);

/* 0x8008503C (0x124): the projection of a map mode with no scene dependency: the identity. */
void g3d_tex_proj_identity(nw4r::math::MTX34* pM, s8 refCamera, s8 refLight) {
    G3D_STATE_POINTER_ASSERT(pM, 0x140, "NW4R:Pointer Error\npM(=%p) is not valid pointer.");
    if (pM != NULL) {
        mtx34_identity(pM);
    }
}

#pragma fp_contract off
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
#pragma fp_contract on

/* 0x80088138 (0x54): returns the fog's data, asserting the handle is set. */
G3dFogData* g3d_fog_ref(const G3dFog* pFog) {
    if (pFog->mpData == NULL) {
        nw4r::db::Panic("g3d_rescommon_ac.h", 0x8F, "NW4R:Pointer must not be NULL (mpData)");
    }
    return pFog->mpData;
}

/* 0x8008818C (0x10): loads fog `idx` of the fog table. */
void g3d_state_load_fog(s32 idx) {
    g3d_fog_table_load(&g3d_state_fog_table, idx);
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

/* ------------------------------------------------------------------------------------------------ */
/* The light table's state entry points                                                              */
/* ------------------------------------------------------------------------------------------------ */

/* The light setting `g3d_state_set_light_setting` copies in; only used through the pointer. */
struct G3dLightSetting;

extern "C" {

void g3d_light_table_set_setting(G3dLightTable* pSelf, const G3dLightSetting* pSetting);
void g3d_light_table_load_light_set(G3dLightTable* pSelf, s32 idx, u32* pMaskDiffColor, u32* pMaskDiffAlpha,
                                    u32* pMaskSpecColor, u32* pMaskSpecAlpha, GXColor* pAmb);

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

} /* extern "C" */

extern "C" {

void g3d_camera_table_set_camera(G3dCameraTable* pSelf, const nw4r::g3d::Camera& camera, u32 id, bool bUpdate);

/* 0x800882FC (0x20): records camera `id`'s matrices from `camera`, making it current when `bUpdate`. */
void g3d_state_set_camera(const nw4r::g3d::Camera& camera, u32 id, bool bUpdate) {
    g3d_camera_table_set_camera(&g3d_state_camera_table, camera, id, bUpdate);
}

} /* extern "C" */

extern "C" {

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

} /* extern "C" */

/* 0x80087DB0 (0x48): whether the shape carries GX vertex attribute `attr`. */
bool nw4r::g3d::ResShp::IsVtxAttrEnabled(u32 attr) const {
    return (ref().vtxAttrFlags & (1 << attr)) != 0;
}
