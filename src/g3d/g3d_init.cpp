/*
 * g3d/g3d_init.cpp - nw4r g3d's library start-up: G3dInit (registers the version string, enables or disables the
 *   locked cache, sets the fast-cast quantisers, picks the render mode) and G3dReset (invalidates the G3D state).
 * RANGE. .text 0x8007A400-0x8007A518 (7 functions); extab 0x80008408-0x80008418, extabindex 0x80020C34-0x80020C4C,
 *   .data 0x8058EB38-0x8058EB80 (the "<< NW4R    - G3D ... >>" version string), .sdata 0x80791200-0x80791208 (its
 *   pointer, which fn_8007A400 hands to OSRegisterVersion).  The string sits between `g3d/g3d_fog.cpp`'s last
 *   string and "g3d_light.cpp"; the GQR setters fn_8007A46C..OSInitFastCast are the inline InitFastCast the start-up
 *   calls, and fn_8007A510 (`g3d_state_invalidate(0x7FF)`) is G3dReset.
 * FLAGS. cflags_g3d (docs/g3d.md); `#pragma peephole off` throughout (playbook idea 106).
 * NAMES. Map stems; fn_8007A400 is nw4r's G3dInit and fn_8007A510 its G3dReset (by the body and the version string).
 * RESIDUALS. Unwritten (empty stub): fn_8007A468.  Partial: fn_8007A400, fn_8007A46C, fn_8007A494, fn_8007A4B8,
 *   OSInitFastCast (the `mtspr` GQR writes m2c could not express).
 *   flipcheck: `.data` and `.sdata` are claimed and not emitted.
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


extern u32 GXNtsc480IntDf;
extern u32 GXPal528IntDf;
extern u32 lbl_80791200;

extern "C" {
u32 OSRegisterVersion(s32);
u32 VIGetTvFormat(void);
u32 fn_80088574(void*);
/* internal */ void fn_8007A400(s32 arg0);
/* internal */ u32 fn_8007A468(void* a0);
/* internal */ u32 fn_8007A46C(void* a0);
/* internal */ u32 fn_8007A494(void* a0);
/* internal */ u32 fn_8007A4B8(void* a0);
/* internal */ s32 OSInitFastCast(void* a0);
/* internal */ void fn_8007A510(void* a0);
} /* extern "C" */

extern "C" {
void fn_8007A400(s32 arg0) {
    u32 *var_r3;

    OSRegisterVersion((s32)(lbl_80791200));
    if (arg0 != 0) {
        nw4r::ut::LC::Enable();
    } else {
        nw4r::ut::LC::Disable();
    }
    fn_8007A468(0);
    var_r3 = (u32 *)(&GXNtsc480IntDf);
    if (VIGetTvFormat() == 1U) {
        var_r3 = (u32 *)(&GXPal528IntDf);
    }
    fn_80088574((void*)(var_r3));
}

u32 fn_8007A468(void* a0) {
    fn_8007A46C(0);
}

u32 fn_8007A46C(void* a0) {
    OSInitFastCast(0);
    fn_8007A4B8(0);
    fn_8007A494(0);
}

u32 fn_8007A494(void* a0) {
    M2C_ERROR(/* unknown instruction: mtspr 0x397, $r0 */);
}

u32 fn_8007A4B8(void* a0) {
    M2C_ERROR(/* unknown instruction: mtspr 0x396, $r0 */);
}

s32 OSInitFastCast(void* a0) {
    M2C_ERROR(/* unknown instruction: mtspr 0x392, $r3 */);
    M2C_ERROR(/* unknown instruction: mtspr 0x393, $r3 */);
    M2C_ERROR(/* unknown instruction: mtspr 0x394, $r3 */);
    M2C_ERROR(/* unknown instruction: mtspr 0x395, $r3 */);
    return 0x70007;
}

void fn_8007A510(void* a0) {
    g3d_state_invalidate(0x7FF);
}


} /* extern "C" */
