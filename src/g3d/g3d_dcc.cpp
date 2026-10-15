/*
 * g3d/g3d_dcc.cpp - nw4r g3d's DCC texture-matrix builders: the texture SRT matrix for the Maya/XSI/3ds Max modes
 *   (the Maya and 3ds Max paths assert they are compiled out) and its overload.
 * RANGE. .text 0x80075E9C-0x800761BC (2 functions); extab 0x80008200-0x80008210, extabindex 0x80020928-0x80020940,
 *   .data 0x8058E570-0x8058E608 (opens on "g3d_dcc.cpp", read by both functions; then the pointer error and the
 *   NW4R_G3D_MAYA_DISABLE / NW4R_G3D_3DSMAX_DISABLE messages).  No `.sdata2`: neither function loads a pool word.
 *   `g3d/g3d_camera.cpp`'s tail precedes it; `g3d/g3d_draw1mat1shp.cpp` follows at fn_800761BC, which only
 *   fn_800769F4 calls and whose inline copies read that unit's strings.
 * FLAGS. cflags_g3d (docs/g3d.md); `#pragma peephole off` throughout (playbook idea 106).
 * NAMES. Map stems (nw4r's CalcTexMtx overloads, by the asserts).
 * RESIDUALS. Partial: fn_80075E9C, g3d_calc_tex_mtx_dcc (m2c views).
 *   flipcheck: `.data` is claimed and not emitted (the strings are declared).
 */

#include "types.h"


#include "sys_mem.h" /* operator delete (rule 9: call through the owner) */
#include "nw4r/g3d/scnmdl.h" /* nw4r::g3d::ScnMdl::CopiedMatAccess - the owner of the two mangled members (rule 1/9) */
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the matrix helpers (rule 2) */
#include "g3d/g3d_xsi.h" /* g3d_calc_tex_mtx_xsi (rule 2) */
#include "g3d/g3d_obj.h" /* nw4r::g3d::G3dObj (rule 2) */
#include "MEM/MEMAllocFromAllocator.h" /* MEMFreeToAllocator (rule 2) */
#include "g3d/g3d_anmchr.h" /* type_obj_set_name (rule 2) */
#include "g3d/g3d_cpu.h"
#include "g3d/g3d_resvtx.h"
#include "g3d/g3d_resmat.h"
#include "g3d/g3d_resnode.h"
#include "g3d/g3d_resshp.h"
#include "g3d/g3d_state.h"     /* the material loaders (rule 2) */
#include "nw4r/fn_805012C4.h"
#include "g3d/g3d_scnobj.h"   /* nw4r::g3d::ScnObj / ScnLeaf / ScnGroup (rule 1) */
#include "g3d/g3d_scnmdlsmpl.h" /* nw4r::g3d::ScnMdlSimple (rule 1) */
#include "MSL/algorithm.h"     /* std::find / std::distance */
#include "unsplit/g3d.h"      /* the scene objects' type-name records no unit owns (rule 2) */

#pragma peephole off

#define M2C_ERROR(x) /* unknown instruction */


namespace nw4r { namespace db {
void Panic(const char* file, int line, const char* fmt, ...);
}}

extern const char lbl_8058E570[];
extern const char lbl_8058E57C[];
extern const char lbl_8058E5B0[];
extern const char lbl_8058E5D8[];

extern "C" {
u32 mtx34_identity(s32);
s32 fn_800D79B4(s32, s32, s32, s32);
/* internal */ void fn_80075E9C(s32 arg0, s32 arg1, s32 arg2, s32 arg3, s32 arg4, u32 arg_sp0);
/* internal */ void g3d_calc_tex_mtx_dcc(s32 arg0, s32 arg1, s32 arg2, s32 arg3);
} /* extern "C" */

extern "C" {
void fn_80075E9C(s32 arg0, s32 arg1, s32 arg2, s32 arg3, s32 arg4, u32 arg_sp0) {
    s32 temp_r11;
    s32 var_r10;
    s32 var_r26;
    s32 var_r5;
    s32 var_r6;
    s32 var_r7;
    s32 var_r8;
    s32 var_r9;

    var_r5 = 1;
    var_r6 = 1;
    var_r7 = 1;
    var_r8 = 1;
    var_r9 = 1;
    var_r10 = 1;
    temp_r11 = arg0 & 0xFF000000;
    if (((u32) (temp_r11 + 0x80000000) != 0U) && ((u32) ((arg0 & 0xFF800000) + 0x7F000000) != 0U)) {
        var_r10 = 0;
    }
    if ((var_r10 == 0) && ((u32) ((arg0 & 0xF8000000) + 0x70000000) != 0U)) {
        var_r9 = 0;
    }
    if ((var_r9 == 0) && ((u32) (temp_r11 + 0x40000000) != 0U)) {
        var_r8 = 0;
    }
    if ((var_r8 == 0) && ((u32) ((arg0 & 0xFF800000) + 0x3F000000) != 0U)) {
        var_r7 = 0;
    }
    if ((var_r7 == 0) && ((u32) ((arg0 & 0xF8000000) + 0x30000000) != 0U)) {
        var_r6 = 0;
    }
    if ((var_r6 == 0) && ((u32) ((arg0 & 0xFFFFC000) + 0x20000000) != 0U)) {
        var_r5 = 0;
    }
    if (var_r5 == 0) {
        nw4r::db::Panic((const char*)&lbl_8058E570, 0x23, (const char*)&lbl_8058E57C, arg0);
    }
    var_r26 = 1;
    if (arg4 == 0) {
        nw4r::db::Panic((const char*)&lbl_8058E570, 0x2C, (const char*)&lbl_8058E5B0);
    } else if (arg4 == 1) {
        var_r26 = g3d_calc_tex_mtx_xsi((nw4r::math::MTX34*)(arg0), (BOOL)(arg1), (const nw4r::g3d::TexSrt*)(arg2), (u32)(arg3)) == 0;
    } else {
        nw4r::db::Panic((const char*)&lbl_8058E570, 0x3C, (const char*)&lbl_8058E5D8);
    }
    if ((var_r26 != 0) && (arg1 != 0)) {
        mtx34_identity((s32)(arg0));
    }
}

void g3d_calc_tex_mtx_dcc(s32 arg0, s32 arg1, s32 arg2, s32 arg3) {
    s32 temp_r11;
    s32 var_r10;
    s32 var_r5;
    s32 var_r6;
    s32 var_r7;
    s32 var_r8;
    s32 var_r9;

    var_r5 = 1;
    var_r6 = 1;
    var_r7 = 1;
    var_r8 = 1;
    var_r9 = 1;
    var_r10 = 1;
    temp_r11 = arg0 & 0xFF000000;
    if (((u32) (temp_r11 + 0x80000000) != 0U) && ((u32) ((arg0 & 0xFF800000) + 0x7F000000) != 0U)) {
        var_r10 = 0;
    }
    if ((var_r10 == 0) && ((u32) ((arg0 & 0xF8000000) + 0x70000000) != 0U)) {
        var_r9 = 0;
    }
    if ((var_r9 == 0) && ((u32) (temp_r11 + 0x40000000) != 0U)) {
        var_r8 = 0;
    }
    if ((var_r8 == 0) && ((u32) ((arg0 & 0xFF800000) + 0x3F000000) != 0U)) {
        var_r7 = 0;
    }
    if ((var_r7 == 0) && ((u32) ((arg0 & 0xF8000000) + 0x30000000) != 0U)) {
        var_r6 = 0;
    }
    if ((var_r6 == 0) && ((u32) ((arg0 & 0xFFFFC000) + 0x20000000) != 0U)) {
        var_r5 = 0;
    }
    if (var_r5 == 0) {
        nw4r::db::Panic((const char*)&lbl_8058E570, 0x4C, (const char*)&lbl_8058E57C, arg0);
    }
    if (((s32) (fn_800D79B4((s32)(arg0), (s32)(arg1), (s32)(arg2), (s32)(arg3)) == 0) != 0) && (arg1 != 0)) {
        mtx34_identity((s32)(arg0));
    }
}

} /* extern "C" */
