/*
 * g3d/g3d_draw.cpp - nw4r g3d's model draw: the per-shape setup (fn_800777B0: the matrix, light and specular
 *   state), the material-resource refill (fn_80077E70 with `CopiedMatResources`), the byte-code draw passes over a
 *   ResMdl (fn_8007868C..fn_800791D8), g3d_draw_res_mdl_directly, and the opaque/translucent sort and its helpers.
 * RANGE. .text 0x800777B0-0x80079FB4 (58 functions); extab 0x800082C8-0x800083D8, extabindex 0x80020A54-0x80020BEC,
 *   .data 0x8058E880-0x8058EAD8 (opens on "g3d_draw.cpp", read from fn_800777B0, fn_80077E70, fn_8007868C,
 *   fn_80078A9C, fn_800791D8 and g3d_draw_res_mdl_directly), .sdata 0x807911F8-0x80791200, .sdata2
 *   0x80795E18-0x80795E28 (a 1.0 pooled again after `g3d/g3d_draw1mat1shp.cpp`'s 0x80795E00, so a new TU at
 *   0x800777B0).  The sort helpers 0x80079E6C-0x80079FB4 are called or address-taken only by this unit's sort
 *   passes; `g3d/g3d_fog.cpp` follows ("g3d_fog.cpp", a 0.0 pooled again at 0x80795E28).
 * FLAGS. cflags_g3d (docs/g3d.md); `#pragma peephole off` (playbook idea 106) except fn_800777B0, which measures
 *   better with the pass on.
 * NAMES. mtx34_set is a GUESS (it assembles an MTX34 from twelve floats); g3d_draw_res_mdl_directly is a GUESS
 *   (0x800793A4: draws a model's opaque or translucent byte code over its view matrices; ScnMdlSimple's draw passes
 *   call it); `CopiedMatResources` is a GUESS (the 0x38-byte set of material handles fn_80077E70 refills and
 *   0x80078DC0 constructs).  The ResTev, ResMatXxx, ResTexSrt, ResShp and ResMat handle members are nw4r's `_ac.h` members.
 *   The bodies are m2c's output, typed mechanically: each `RawView_N` struct's `field_0xNN` states an offset and a
 *   size, not a meaning.
 * RESIDUALS. Unwritten (empty stubs): fn_8007868C, fn_80078A9C, fn_80078E7C, fn_80079018, fn_800791D8,
 *   g3d_draw_res_mdl_directly, fn_80079604, fn_80079938, fn_800799BC, fn_80079A48, fn_80079B38.
 *   Scored zero with a body: fn_80077DBC, mtx34_set, fn_80079EE4.  Partial: fn_800777B0, fn_80077D64, fn_80077E34,
 *   fn_80077E70, fn_80078878, fn_800788C8, fn_80078904, fn_80078988, fn_80079E6C, fn_80079F14, fn_80079F78.
 *   flipcheck: `.data`, `.sdata` and `.sdata2` are claimed and not emitted (declared).
 * SHAPES. The CopiedMatResources constructor is complete: its work is the member initialisers.  The material
 *   resource constructors keep `#pragma peephole off` (retail keeps `clrlwi` + `cmpwi` for the alignment test).
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
#include "g3d/g3d_light.h"     /* light_obj_get_light_pos / light_obj_get_light_dir (rule 2) */
#include "g3d/g3d_draw1mat1shp.h" /* res_mat_misc_copy_ctor (rule 2) */

#pragma peephole off

#define M2C_ERROR(x) /* unknown instruction */


/* Data objects the range references (unsplit, address-only). */
extern const char lbl_8058E880[];
extern u32 lbl_8058E890;
extern u32 lbl_8058E8B8;
extern const char lbl_8058E8E8[];
extern const char lbl_8058E91C[];
extern const char lbl_8058E9F4[];
extern const char lbl_8058EA20[];
extern const char lbl_8058EA30[];
extern const char lbl_8058EA50[];
extern const char lbl_8058EA64[];
extern const char lbl_8058EA90[];
extern const char lbl_8058EAA0[];
extern const char lbl_8058EAC8[];
extern f32 lbl_80795E18;
extern f32 lbl_80795E1C;
extern f32 lbl_80795E20;

namespace nw4r { namespace db {
void Panic(const char* file, int line, const char* fmt, ...);
}}

extern "C" {
u32 vec3_normalize_into(void*, void*, f32, f32);
s32 res_node_is_valid(s32);
s32 fn_8006518C(void);
u32 fn_8006E2A8(s32, s32);
u32 fn_8006FDCC(void*);
s32 res_mdl_info_get_node_of_pos_nrm_mtx(void*, s32);
u32 mtx34_copy_ps(void*, s32);
u32 mtx34_concat(void*, void*, void*);
s32 res_mdl_get_info(void*);
s32 fn_80094094(s32);
/* internal */ void* fn_800777B0(s32 arg0, u32 *arg1, s32 arg2);
/* internal */ s32 fn_80077D4C(s32 *arg0);
/* internal */ s32 fn_80077D64(s32 arg0);
/* internal */ u32 fn_80077DBC(void *arg1, void* a1);
/* internal */ s32 fn_80077DD8(s32 *arg0);
/* internal */ u32 mtx34_set(void *arg0, f32 farg0, f32 farg1, f32 farg2, f32 farg3, f32 farg4, f32 farg5, f32 farg6, f32 farg7, f32 arg_sp8, f32 arg_spC, f32 arg_sp10, f32 arg_sp14);
/* internal */ s32 fn_80077E34(s32 arg0, void* a1);
/* internal */ u32 fn_80077E64(s32 *arg0, s32 *arg1);
/* internal */ u32 fn_80077E70(s32 arg0, s32 arg1, s32 arg2);
/* internal */ u32 fn_80078608(s32 *arg0, s32 *arg1);
/* internal */ u32 fn_8007868C(s32 arg0, void *arg1, s32 arg2, u32 arg_sp0);
/* internal */ s32 fn_80078878(s32 arg0, s32 arg1);
/* internal */ s32 fn_800788C8(s32 arg0, void* a1);
/* internal */ u32 fn_800788F8(s32 *arg0, s32 *arg1);
/* internal */ s32 fn_80078904(s32 arg0);
/* internal */ s32 fn_80078988(s32 arg0, void* a1);
/* internal */ u32 fn_800789B8(s32 *arg0, s32 *arg1);
/* internal */ u32 fn_80078A9C(s32 arg0, void *arg1, s32 arg2, s32 arg3, u32 arg_sp0);
/* internal */ u32 fn_80078E7C(s32 arg0, void *arg1, u32 arg2, s32 arg3, u32 arg_sp0);
/* internal */ u32 fn_80079018(s32 arg0, void *arg1, u32 arg2, s32 arg3, s32 arg4, u32 arg_sp0);
/* internal */ void* fn_800791D8(u32 *arg0, s32 arg1, s32 arg2, void *arg3, void *arg4);
/* internal */ void g3d_draw_res_mdl_directly(s32 *arg0, s32 arg1, s32 arg2, s32 arg3, s32 arg4, s32 arg5, s32 arg6, s32 arg7, u32 arg_sp0);
/* internal */ u32 fn_80079604(u32 arg0, u32 arg1, s32 (*arg2)(u32, u32), u32 arg_sp0);
/* internal */ u32 fn_80079938(u32 arg0, u32 arg1, s32 arg2);
/* internal */ u32 fn_800799BC(u32 arg0, u32 arg1, s32 (**arg2)(u32, u32));
/* internal */ u32 fn_80079A48(s32 arg0, s32 arg1, s32 arg2, s32 (**arg3)(s32, s32), u32 arg_sp0);
/* internal */ u32 fn_80079B38(u32 arg0, u32 arg1, s32 (**arg2)(u32, u32), u32 arg_sp0);
/* internal */ u32 fn_80079E6C(s32 *arg0, s32 *arg1);
/* internal */ u32 fn_80079EE4(void *arg0, void *arg1);
/* internal */ void* fn_80079F10(void* a0, void* a1);
/* internal */ s32 fn_80079F14(void *arg0, void *arg1);
/* internal */ s32 fn_80079F78(void *arg0, void *arg1);



} /* extern "C" */
extern "C" {

typedef struct {
    /* +0x00 */ u8 pad_0x00[0x10];
    /* +0x10 */ u32 (*field_0x10)(...);
    /* +0x14 */ u32 (*field_0x14)(...);
} RawView_16; /* size: 0x18 */
typedef struct {
    /* +0x00 */ u8 pad_0x00[0x8];
    /* +0x08 */ u32 field_0x08;
} RawView_17; /* size: 0xC */
typedef struct {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ u32 field_0x04;
    /* +0x08 */ u32 field_0x08;
    /* +0x0C */ u32 field_0x0C;
    /* +0x10 */ u32 field_0x10;
    /* +0x14 */ u32 field_0x14;
    /* +0x18 */ u32 field_0x18;
    /* +0x1C */ u32 field_0x1C;
    /* +0x20 */ u32 field_0x20;
} RawView_18; /* size: 0x24 */
typedef struct {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ u32 field_0x04;
    /* +0x08 */ u32 field_0x08;
    /* +0x0C */ u32 field_0x0C;
    /* +0x10 */ u32 field_0x10;
    /* +0x14 */ u32 field_0x14;
    /* +0x18 */ u32 field_0x18;
    /* +0x1C */ u32 field_0x1C;
    /* +0x20 */ u32 field_0x20;
} RawView_19; /* size: 0x24 */
#pragma peephole on
void* fn_800777B0(s32 arg0, u32 *arg1, s32 arg2) {
    u32 sp54;
    u32 sp58;
    u32 sp6C;
    u32 sp70;
    u32 sp78;
    u32 sp7C;
    u32 sp84;
    u32 sp88;

    f32 spF4;
    f32 spF0;
    f32 spEC;
    f32 spE8;
    f32 spE4;
    f32 spE0;
    f32 spDC;
    f32 spD8;
    f32 spD4;
    f32 spD0;
    f32 spCC;
    f32 spC8;
    u32 sp98;
    s32 sp94;
    s32 sp90;
    s32 sp8C;
    f32 sp80;
    f32 sp74;
    f32 sp68;
    f32 sp64;
    f32 sp60;
    f32 sp5C;
    f32 sp50;
    u32 sp44;
    u32 sp38;
    u32 sp34;
    u8 sp32;
    u8 sp31;
    u8 sp30;
    u32 sp2C;
    u32 sp28;
    s32 sp24;
    s32 sp20;
    s32 sp1C;
    s32 sp18;
    f32 sp14;
    f32 sp10;
    f32 spC;
    f32 sp8;
    f32 temp_f1;
    f32 temp_f29;
    f32 temp_f30;
    f32 temp_f31;
    s32 *var_r31;
    s32 temp_r20_2;
    s32 temp_r26;
    s32 temp_r3_3;
    s32 temp_r3_5;
    s32 var_r29;
    u32 temp_r20;
    u32 var_r28;
    u8 *var_r30;
    void **temp_r3;
    void *temp_r3_2;
    void *temp_r3_4;

    M2C_ERROR(/* unknown instruction: xsmaddmdp vs30, vs1, vs0 */);
    M2C_ERROR(/* unknown instruction: xxsel vs29, vs1, vs0, vs36 */);
    if ((reinterpret_cast<const nw4r::g3d::ResMat*>(0)->IsValid() == 0) || (reinterpret_cast<const nw4r::g3d::ResShp*>(arg2)->IsValid() == 0)) {
        M2C_ERROR(/* unknown instruction: vmrghb v31, v1, v0 */);
        M2C_ERROR(/* unknown instruction: vmrghb v30, v1, v0 */);
        M2C_ERROR(/* unknown instruction: vmrghb v29, v1, v0 */);
        return NULL;
    }
    sp24 = (s32)reinterpret_cast<nw4r::g3d::ResMat*>((void*)(arg0))->GetResMatMisc().mpData;
    res_mat_misc_copy_ctor((s32)(&sp34), (const s32*)(&sp24));
    reinterpret_cast<nw4r::g3d::ResMatMisc*>(&sp34)->GetIndirectMethod(GX_ITM_0, (nw4r::g3d::ResMatMiscData::IndirectMethod*)&sp8C, (s8*)&sp30);
    reinterpret_cast<nw4r::g3d::ResMatMisc*>(&sp34)->GetIndirectMethod(GX_ITM_1, (nw4r::g3d::ResMatMiscData::IndirectMethod*)&sp90, (s8*)&sp31);
    reinterpret_cast<nw4r::g3d::ResMatMisc*>(&sp34)->GetIndirectMethod(GX_ITM_2, (nw4r::g3d::ResMatMiscData::IndirectMethod*)&sp94, (s8*)&sp32);
    if ((sp8C == 0) && (sp90 == 0) && (sp94 == 0)) {
        M2C_ERROR(/* unknown instruction: vmrghb v31, v1, v0 */);
        M2C_ERROR(/* unknown instruction: vmrghb v30, v1, v0 */);
        M2C_ERROR(/* unknown instruction: vmrghb v29, v1, v0 */);
        return NULL;
    }
    var_r29 = 0;
    MTX34_ctor((nw4r::math::MTX34*)&spC8);
    temp_r3 = (void **)(g3d_state_get_ind_mtx_hook());
    ((RawView_16*)(*temp_r3))->field_0x10();
    var_r28 = 0U;
    var_r31 = (s32 *)(&sp8C);
    var_r30 = (u8 *)(&sp30);
    temp_f29 = lbl_80795E1C;
    temp_f30 = lbl_80795E20;
    temp_f31 = lbl_80795E18;
    do {
        temp_r26 = var_r28 + 1;
        if ((s32) *var_r31 != 0) {
            if (var_r29 == 0) {
                var_r29 = 1;
                if ((s32) reinterpret_cast<nw4r::g3d::ResShp*>(arg2)->ptr()->curMtxIdx >= 0) {
                    temp_r20 = fn_8006FDCC((void*)(arg1));
                    if ((u32) reinterpret_cast<nw4r::g3d::ResShp*>(arg2)->ptr()->curMtxIdx == temp_r20) {
                        temp_r3_2 = (void *)(g3d_state_get_nrm_mtx((u32)(reinterpret_cast<nw4r::g3d::ResShp*>(arg2)->ptr()->curMtxIdx)));
                        spC8 = ((RawView_18*)temp_r3_2)->field_0x00;
                        spCC = ((RawView_18*)temp_r3_2)->field_0x04;
                        spD0 = ((RawView_18*)temp_r3_2)->field_0x08;
                        spD8 = ((RawView_18*)temp_r3_2)->field_0x0C;
                        spDC = ((RawView_18*)temp_r3_2)->field_0x10;
                        spE0 = ((RawView_18*)temp_r3_2)->field_0x14;
                        spE8 = ((RawView_18*)temp_r3_2)->field_0x18;
                        spEC = ((RawView_18*)temp_r3_2)->field_0x1C;
                        spF0 = ((RawView_18*)temp_r3_2)->field_0x20;
                    } else {
                        sp20 = fn_80094094((s32)(arg0));
                        fn_80077E34((s32)(&sp2C), (void*)(&sp20));
                        sp1C = res_mdl_get_info((void*)(&sp2C));
                        temp_r3_3 = res_mdl_info_get_node_of_pos_nrm_mtx((void*)(&sp1C), (s32)(reinterpret_cast<nw4r::g3d::ResShp*>(arg2)->ptr()->curMtxIdx));
                        if (temp_r3_3 < 0) {
                            nw4r::db::Panic((const char*)&lbl_8058E880, 0x64, (const char*)&lbl_8058E890);
                        }
                        sp18 = (s32)reinterpret_cast<nw4r::g3d::ResMdl*>(&sp2C)->GetResNode((int)temp_r3_3).mpData;
                        res_node_copy_ctor((void*)(&sp28), (u32*)(&sp18));
                        temp_r20_2 = (s32)res_node_ref_nonconst((ResHandle*)(arg1));
                        mtx34_concat((void*)(&spC8), (void*)((u32)res_node_ref_nonconst((ResHandle*)(&sp28)) + 0xA0), (void*)(u32)(temp_r20_2 + 0x70));
                        temp_r3_4 = (void *)(g3d_state_get_nrm_mtx((u32)(reinterpret_cast<nw4r::g3d::ResShp*>(arg2)->ptr()->curMtxIdx)));
                        sp8 = ((RawView_19*)temp_r3_4)->field_0x18;
                        spC = ((RawView_19*)temp_r3_4)->field_0x1C;
                        sp10 = ((RawView_19*)temp_r3_4)->field_0x20;
                        sp14 = temp_f31;
                        mtx34_set((void *)(&sp98), (*(f32*)&temp_r3_4), (f32)(((RawView_19*)temp_r3_4)->field_0x00), (f32)(((RawView_19*)temp_r3_4)->field_0x04), (f32)(((RawView_19*)temp_r3_4)->field_0x08), (f32)(temp_f31), (f32)(((RawView_19*)temp_r3_4)->field_0x0C), (f32)(((RawView_19*)temp_r3_4)->field_0x10), (f32)(((RawView_19*)temp_r3_4)->field_0x14), (f32)(temp_f31), 0, 0, 0);
                        mtx34_concat((void*)(&spC8), (void*)(&sp98), (void*)(&spC8));
                    }
                    spF4 = temp_f31;
                    spE4 = temp_f31;
                    spD4 = temp_f31;
                    setVec3((nw4r::math::VEC3*)&sp80, (f32)(spC8), (f32)(spD8), (f32)(spE8));
                    vec3_normalize_into((void*)(&sp80), (void*)(&sp80), 0, 0);
                    spC8 = sp80;
                    spD8 = sp84;
                    spE8 = sp88;
                    setVec3((nw4r::math::VEC3*)&sp74, (f32)(spCC), (f32)(spDC), (f32)(spEC));
                    vec3_normalize_into((void*)(&sp74), (void*)(&sp74), 0, 0);
                    spCC = sp74;
                    spDC = sp78;
                    spEC = sp7C;
                    setVec3((nw4r::math::VEC3*)&sp68, (f32)(spD0), (f32)(spE0), (f32)(spF0));
                    vec3_normalize_into((void*)(&sp68), (void*)(&sp68), 0, 0);
                    spD0 = sp68;
                    spE0 = sp6C;
                    spF0 = sp70;
                } else {
                    mtx34_copy_ps((void*)(&spC8), (s32)(g3d_state_get_camera_mtx()));
                }
            }
            temp_r3_5 = (s32)g3d_state_get_light_obj(g3d_state_get_light_set_entry((s8)*var_r30));
            if ((temp_r3_5 != 0) && (fn_8006518C() != 0)) {
                VEC3_ctor((nw4r::math::VEC3*)&sp5C);
                if (fn_80077DD8((s32 *)(temp_r3_5)) != 0) {
                    light_obj_get_light_dir((s32)(temp_r3_5), (s32)(&sp5C));
                    if ((temp_f31 == sp5C) && (temp_f31 == sp60) && (temp_f31 == sp64)) {
                        light_obj_get_light_pos((s32)(temp_r3_5), (s32)(&sp5C));
                        fn_80077DBC((void *)(&sp44), (void*)(&sp5C));
                        copyVec3((nw4r::math::VEC3*)&sp5C, (const nw4r::math::VEC3*)&sp44);
                        vec3_normalize_into((void*)(&sp5C), (void*)(&sp5C), 0, 0);
                    }
                } else if (fn_80077D64((s32)(temp_r3_5)) != 0) {
                    light_obj_get_light_pos((s32)(temp_r3_5), (s32)(&sp5C));
                    fn_80077DBC((void *)(&sp38), (void*)(&sp5C));
                    copyVec3((nw4r::math::VEC3*)&sp5C, (const nw4r::math::VEC3*)&sp38);
                    vec3_normalize_into((void*)(&sp5C), (void*)(&sp5C), 0, 0);
                } else {
                    VEC3_ctor((nw4r::math::VEC3*)&sp50);
                    if (fn_80077D4C((s32 *)(temp_r3_5)) == 0) {
                        nw4r::db::Panic((const char*)&lbl_8058E880, 0xAB, (const char*)&lbl_8058E8B8);
                    }
                    light_obj_get_light_dir((s32)(temp_r3_5), (s32)(&sp50));
                    temp_f1 = temp_f29 * sp58;
                    sp5C = temp_f1 * sp50;
                    sp60 = temp_f1 * sp54;
                    sp64 = temp_f30 + (temp_f1 * sp58);
                    vec3_normalize_into((void*)(&sp5C), (void*)(&sp5C), (f32)(temp_f1), (f32)(sp58));
                }
                ((RawView_16*)(*temp_r3))->field_0x14(temp_r3, temp_r26, &sp5C, &spC8, *var_r31);
            } else {
                ((RawView_16*)(*temp_r3))->field_0x14(temp_r3, temp_r26, 0, &spC8, *var_r31);
            }
        }
        var_r31 += 4;
        var_r30 += 1;
        var_r28 += 1;
    } while (var_r28 < 3U);
    M2C_ERROR(/* unknown instruction: vmrghb v31, v1, v0 */);
    M2C_ERROR(/* unknown instruction: vmrghb v30, v1, v0 */);
    M2C_ERROR(/* unknown instruction: vmrghb v29, v1, v0 */);
    return temp_r3;
}
#pragma peephole off

s32 fn_80077D4C(s32 *arg0) {
    return (*arg0 & 2) != 0;
}

s32 fn_80077D64(s32 arg0) {
    s32 var_r31;

    var_r31 = 0;
    if ((fn_80077DD8(0) == 0) && (fn_80077D4C((s32 *)(arg0)) == 0)) {
        var_r31 = 1;
    }
    return var_r31;
}

typedef struct {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ u32 field_0x04;
    /* +0x08 */ u32 field_0x08;
} RawView_20; /* size: 0xC */
u32 fn_80077DBC(void *arg1, void* a1) {
    setVec3((nw4r::math::VEC3*)(-((RawView_20*)arg1)->field_0x00), (f32)(-((RawView_20*)arg1)->field_0x04), (f32)(-((RawView_20*)arg1)->field_0x08), 0);
}

s32 fn_80077DD8(s32 *arg0) {
    return (*arg0 & 1) != 0;
}

typedef struct {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ u32 field_0x04;
    /* +0x08 */ u32 field_0x08;
    /* +0x0C */ u32 field_0x0C;
    /* +0x10 */ u32 field_0x10;
    /* +0x14 */ u32 field_0x14;
    /* +0x18 */ u32 field_0x18;
    /* +0x1C */ u32 field_0x1C;
    /* +0x20 */ u32 field_0x20;
    /* +0x24 */ u32 field_0x24;
    /* +0x28 */ u32 field_0x28;
    /* +0x2C */ u32 field_0x2C;
} RawView_21; /* size: 0x30 */
u32 mtx34_set(void *arg0, f32 farg0, f32 farg1, f32 farg2, f32 farg3, f32 farg4, f32 farg5, f32 farg6, f32 farg7, f32 arg_sp8, f32 arg_spC, f32 arg_sp10, f32 arg_sp14) {
    ((RawView_21*)arg0)->field_0x00 = farg0;
    ((RawView_21*)arg0)->field_0x04 = farg1;
    ((RawView_21*)arg0)->field_0x08 = farg2;
    ((RawView_21*)arg0)->field_0x0C = farg3;
    ((RawView_21*)arg0)->field_0x10 = farg4;
    ((RawView_21*)arg0)->field_0x14 = farg5;
    ((RawView_21*)arg0)->field_0x18 = farg6;
    ((RawView_21*)arg0)->field_0x1C = farg7;
    ((RawView_21*)arg0)->field_0x20 = arg_sp8;
    ((RawView_21*)arg0)->field_0x24 = arg_spC;
    ((RawView_21*)arg0)->field_0x28 = arg_sp10;
    ((RawView_21*)arg0)->field_0x2C = arg_sp14;
}

s32 fn_80077E34(s32 arg0, void* a1) {
    fn_80077E64(0, 0);
    return arg0;
}

u32 fn_80077E64(s32 *arg0, s32 *arg1) {
    *arg0 = *arg1;
}

} /* extern "C" */

/* The material resource set fn_80077E70 refills from a ScnMdl's copied-material buffers: one handle per
 * resource kind and three trailing words.  size: 0x38 */
struct CopiedMatResources {
    /* +0x00 */ nw4r::g3d::ResTexObj texObj;
    /* +0x04 */ nw4r::g3d::ResTlutObj tlutObj;
    /* +0x08 */ nw4r::g3d::ResGenMode genMode;
    /* +0x0C */ nw4r::g3d::ResTev tev;
    /* +0x10 */ nw4r::g3d::ResMatPix pix;
    /* +0x14 */ nw4r::g3d::ResMatTevColor tevColor;
    /* +0x18 */ nw4r::g3d::ResMatIndMtxAndScale indMtxAndScale;
    /* +0x1C */ nw4r::g3d::ResMatChan chan;
    /* +0x20 */ nw4r::g3d::ResMatTexCoordGen texCoordGen;
    /* +0x24 */ nw4r::g3d::ResMatMisc misc;
    /* +0x28 */ nw4r::g3d::ResTexSrt texSrt;
    /* +0x2C */ u32 word_0x2C;
    /* +0x30 */ u32 word_0x30;
    /* +0x34 */ u32 word_0x34;

    CopiedMatResources();
};

extern "C" {

typedef struct {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ u8 pad_0x04[0x4];
    /* +0x08 */ u32 field_0x08;
    /* +0x0C */ u32 field_0x0C;
    /* +0x10 */ u32 field_0x10;
    /* +0x14 */ u32 field_0x14;
    /* +0x18 */ u32 field_0x18;
    /* +0x1C */ u32 field_0x1C;
    /* +0x20 */ u32 field_0x20;
    /* +0x24 */ u32 field_0x24;
    /* +0x28 */ u32 field_0x28;
    /* +0x2C */ u32 field_0x2C;
    /* +0x30 */ u32 field_0x30;
    /* +0x34 */ u32 field_0x34;
    /* +0x38 */ u32 field_0x38;
    /* +0x3C */ u32 field_0x3C;
} RawView_22; /* size: 0x40 */
u32 fn_80077E70(s32 arg0, s32 arg1, s32 arg2) {
    u32 sp5C;
    u32 sp58;
    u32 sp54;
    u32 sp50;
    u32 sp4C;
    u32 sp48;
    u32 sp44;
    u32 sp40;
    u32 sp3C;
    u32 sp38;
    u32 sp34;
    u32 sp30;
    u32 sp2C;
    u32 sp28;
    u32 sp24;
    u32 sp20;
    u32 sp1C;
    u32 sp18;
    u32 sp14;
    u32 sp10;
    u32 spC;
    u32 sp8;
    s32 temp_r10;
    s32 temp_r11;
    s32 temp_r4;
    s32 temp_r4_10;
    s32 temp_r4_11;
    s32 temp_r4_2;
    s32 temp_r4_3;
    s32 temp_r4_4;
    s32 temp_r4_5;
    s32 temp_r4_6;
    s32 temp_r4_7;
    s32 temp_r4_8;
    s32 temp_r4_9;
    s32 var_r10;
    s32 var_r4;
    s32 var_r5;
    s32 var_r5_2;
    s32 var_r6;
    s32 var_r6_2;
    s32 var_r7;
    s32 var_r7_2;
    s32 var_r8;
    s32 var_r8_2;
    s32 var_r9;
    s32 var_r9_2;

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
        nw4r::db::Panic((const char*)&lbl_8058E880, 0xCD, (const char*)&lbl_8058E8E8, arg0);
    }
    var_r4 = 1;
    var_r5_2 = 1;
    var_r6_2 = 1;
    var_r7_2 = 1;
    var_r8_2 = 1;
    var_r9_2 = 1;
    temp_r10 = arg1 & 0xFF000000;
    if (((u32) (temp_r10 + 0x80000000) != 0U) && ((u32) ((arg1 & 0xFF800000) + 0x7F000000) != 0U)) {
        var_r9_2 = 0;
    }
    if ((var_r9_2 == 0) && ((u32) ((arg1 & 0xF8000000) + 0x70000000) != 0U)) {
        var_r8_2 = 0;
    }
    if ((var_r8_2 == 0) && ((u32) (temp_r10 + 0x40000000) != 0U)) {
        var_r7_2 = 0;
    }
    if ((var_r7_2 == 0) && ((u32) ((arg1 & 0xFF800000) + 0x3F000000) != 0U)) {
        var_r6_2 = 0;
    }
    if ((var_r6_2 == 0) && ((u32) ((arg1 & 0xF8000000) + 0x30000000) != 0U)) {
        var_r5_2 = 0;
    }
    if ((var_r5_2 == 0) && ((u32) ((arg1 & 0xFFFFC000) + 0x20000000) != 0U)) {
        var_r4 = 0;
    }
    if (var_r4 == 0) {
        nw4r::db::Panic((const char*)&lbl_8058E880, 0xCE, (const char*)&lbl_8058E91C, arg1);
    }
    temp_r4 = ((RawView_22*)arg1)->field_0x08;
    if (temp_r4 != 0) {
        ((CopiedMatResources*)arg0)->texObj = nw4r::g3d::ResTexObj((void*)(temp_r4 + (arg2 * 0x104)));
    } else {
        ((CopiedMatResources*)arg0)->texObj = nw4r::g3d::ResTexObj((void*)(0));
    }
    temp_r4_2 = ((RawView_22*)arg1)->field_0x0C;
    if (temp_r4_2 != 0) {
        ((CopiedMatResources*)arg0)->tlutObj = nw4r::g3d::ResTlutObj((void*)(temp_r4_2 + (arg2 * 0x64)));
    } else {
        ((CopiedMatResources*)arg0)->tlutObj = nw4r::g3d::ResTlutObj((void*)(0));
    }
    temp_r4_3 = ((RawView_22*)arg1)->field_0x10;
    if (temp_r4_3 != 0) {
        ((CopiedMatResources*)arg0)->texSrt = nw4r::g3d::ResTexSrt((void*)(temp_r4_3 + (arg2 * 0x248)));
    } else {
        ((CopiedMatResources*)arg0)->texSrt = nw4r::g3d::ResTexSrt((void*)NULL);
    }
    temp_r4_4 = ((RawView_22*)arg1)->field_0x14;
    if (temp_r4_4 != 0) {
        ((CopiedMatResources*)arg0)->chan = nw4r::g3d::ResMatChan((void*)(temp_r4_4 + (arg2 * 0x28)));
    } else {
        ((CopiedMatResources*)arg0)->chan = nw4r::g3d::ResMatChan((void*)(0));
    }
    temp_r4_5 = ((RawView_22*)arg1)->field_0x18;
    if (temp_r4_5 != 0) {
        ((CopiedMatResources*)arg0)->genMode = nw4r::g3d::ResGenMode((void*)(temp_r4_5 + (arg2 * 8)));
    } else {
        ((CopiedMatResources*)arg0)->genMode = nw4r::g3d::ResGenMode((void*)(0));
    }
    temp_r4_6 = ((RawView_22*)arg1)->field_0x1C;
    if (temp_r4_6 != 0) {
        ((CopiedMatResources*)arg0)->misc = nw4r::g3d::ResMatMisc((void*)(temp_r4_6 + (arg2 * 0xC)));
    } else {
        ((CopiedMatResources*)arg0)->misc = nw4r::g3d::ResMatMisc((void*)(0));
    }
    temp_r4_7 = ((RawView_22*)arg1)->field_0x20;
    if (temp_r4_7 != 0) {
        ((CopiedMatResources*)arg0)->pix = nw4r::g3d::ResMatPix((void*)(temp_r4_7 + (arg2 << 5)));
    } else {
        ((CopiedMatResources*)arg0)->pix = nw4r::g3d::ResMatPix((void*)(0));
    }
    temp_r4_8 = ((RawView_22*)arg1)->field_0x24;
    if (temp_r4_8 != 0) {
        ((CopiedMatResources*)arg0)->tevColor = nw4r::g3d::ResMatTevColor((void*)(temp_r4_8 + (arg2 << 7)));
    } else {
        ((CopiedMatResources*)arg0)->tevColor = nw4r::g3d::ResMatTevColor((void*)(0));
    }
    temp_r4_9 = ((RawView_22*)arg1)->field_0x28;
    if (temp_r4_9 != 0) {
        ((CopiedMatResources*)arg0)->indMtxAndScale = nw4r::g3d::ResMatIndMtxAndScale((void*)(temp_r4_9 + (arg2 << 6)));
    } else {
        ((CopiedMatResources*)arg0)->indMtxAndScale = nw4r::g3d::ResMatIndMtxAndScale((void*)(0));
    }
    temp_r4_10 = ((RawView_22*)arg1)->field_0x2C;
    if (temp_r4_10 != 0) {
        ((CopiedMatResources*)arg0)->texCoordGen = nw4r::g3d::ResMatTexCoordGen((void*)(temp_r4_10 + (arg2 * 0xA0)));
    } else {
        ((CopiedMatResources*)arg0)->texCoordGen = nw4r::g3d::ResMatTexCoordGen((void*)(0));
    }
    temp_r4_11 = ((RawView_22*)arg1)->field_0x30;
    if (temp_r4_11 != 0) {
        ((CopiedMatResources*)arg0)->tev = nw4r::g3d::ResTev((void*)(temp_r4_11 + (arg2 << 9)));
    } else {
        ((CopiedMatResources*)arg0)->tev = nw4r::g3d::ResTev((void*)(0));
    }
    if ((s32) (((RawView_22*)arg1)->field_0x00 & 1) != 0) {
        ((CopiedMatResources*)arg0)->word_0x2C = 0;
        ((CopiedMatResources*)arg0)->word_0x30 = 0;
        ((CopiedMatResources*)arg0)->word_0x34 = 0;
        return;
    }
    ((CopiedMatResources*)arg0)->word_0x2C = (s32) ((RawView_22*)arg1)->field_0x34;
    ((CopiedMatResources*)arg0)->word_0x30 = (s32) ((RawView_22*)arg1)->field_0x38;
    ((CopiedMatResources*)arg0)->word_0x34 = (s32) ((RawView_22*)arg1)->field_0x3C;
}

} /* extern "C" */

/* 0x800783B0 (0x30): copies the handle. */
nw4r::g3d::ResTev& nw4r::g3d::ResTev::operator=(const ResTev& rhs) {
    ResCommon<ResTevData>::operator=(rhs);
    return *this;
}

extern "C" {


} /* extern "C" */

/* 0x800783EC (0x64): wraps `pData`, asserting its alignment. */
/* untyped: opaque handle - the block address */
nw4r::g3d::ResTev::ResTev(void* pData) : ResCommon<ResTevData>(pData) {
    if ((u32)pData & 0x1F) {
        nw4r::db::Panic((const char*)&lbl_8058EA90, 0x25, (const char*)&lbl_8058EA64);
    }
}

extern "C" {

} /* extern "C" */

/* 0x80078450 (0x8): stores the block address. */
NW4R_G3D_RESCOMMON_CTOR(nw4r::g3d::ResTevData)

extern "C" {

} /* extern "C" */

/* 0x80078458 (0x30): copies the handle. */
nw4r::g3d::ResMatTexCoordGen& nw4r::g3d::ResMatTexCoordGen::operator=(const ResMatTexCoordGen& rhs) {
    ResCommon<ResMatTexCoordGenData>::operator=(rhs);
    return *this;
}

extern "C" {


} /* extern "C" */

/* 0x80078494 (0x30): copies the handle. */
nw4r::g3d::ResMatIndMtxAndScale& nw4r::g3d::ResMatIndMtxAndScale::operator=(const ResMatIndMtxAndScale& rhs) {
    ResCommon<ResMatIndMtxAndScaleData>::operator=(rhs);
    return *this;
}

extern "C" {


} /* extern "C" */

/* 0x800784D0 (0x30): copies the handle. */
nw4r::g3d::ResMatTevColor& nw4r::g3d::ResMatTevColor::operator=(const ResMatTevColor& rhs) {
    ResCommon<ResMatTevColorData>::operator=(rhs);
    return *this;
}

extern "C" {


} /* extern "C" */

/* 0x8007850C (0x30): copies the handle. */
nw4r::g3d::ResMatPix& nw4r::g3d::ResMatPix::operator=(const ResMatPix& rhs) {
    ResCommon<ResMatPixData>::operator=(rhs);
    return *this;
}

extern "C" {


} /* extern "C" */

/* 0x80078548 (0x30): copies the handle. */
nw4r::g3d::ResGenMode& nw4r::g3d::ResGenMode::operator=(const ResGenMode& rhs) {
    ResCommon<ResGenModeData>::operator=(rhs);
    return *this;
}

extern "C" {


} /* extern "C" */

/* 0x80078584 (0x30): copies the handle. */
nw4r::g3d::ResMatChan& nw4r::g3d::ResMatChan::operator=(const ResMatChan& rhs) {
    ResCommon<ResMatChanData>::operator=(rhs);
    return *this;
}

extern "C" {


} /* extern "C" */

/* 0x800785C0 (0x48): copies the handle word and the word after it. */
nw4r::g3d::ResTexSrt& nw4r::g3d::ResTexSrt::operator=(const ResTexSrt& rhs) {
    fn_80078608((s32*)this, (s32*)&rhs);
    fn_8006E2A8((s32)this + 4, (s32)&rhs + 4);
    return *this;
}

extern "C" {

u32 fn_80078608(s32 *arg0, s32 *arg1) {
    *arg0 = *arg1;
}

} /* extern "C" */

/* 0x80078614 (0x30): copies the handle. */
nw4r::g3d::ResTlutObj& nw4r::g3d::ResTlutObj::operator=(const ResTlutObj& rhs) {
    ResCommon<ResTlutObjData>::operator=(rhs);
    return *this;
}

extern "C" {


} /* extern "C" */

/* 0x80078650 (0x30): copies the handle. */
nw4r::g3d::ResTexObj& nw4r::g3d::ResTexObj::operator=(const ResTexObj& rhs) {
    ResCommon<ResTexObjData>::operator=(rhs);
    return *this;
}

extern "C" {


typedef struct {
    /* +0x00 */ u8 field_0x00;
    /* +0x01 */ u8 field_0x01;
    /* +0x02 */ u8 field_0x02;
    /* +0x03 */ u8 field_0x03;
    /* +0x04 */ u8 field_0x04;
    /* +0x05 */ u8 field_0x05;
    /* +0x06 */ u8 field_0x06;
    /* +0x07 */ u8 field_0x07;
    /* +0x08 */ u8 field_0x08;
    /* +0x09 */ u8 field_0x09;
} RawView_24; /* size: 0xA */
u32 fn_8007868C(s32 arg0, void *arg1, s32 arg2, u32 arg_sp0) {
}


s32 fn_80078878(s32 arg0, s32 arg1) {
    s32 temp_r31;

    temp_r31 = (s32)reinterpret_cast<const nw4r::g3d::ResMat*>((s32)(arg1))->ptr();
    return (s32)reinterpret_cast<const nw4r::g3d::ResMat*>((s32)(arg0))->ptr() == temp_r31;
}

s32 fn_800788C8(s32 arg0, void* a1) {
    fn_800788F8(0, 0);
    return arg0;
}

u32 fn_800788F8(s32 *arg0, s32 *arg1) {
    *arg0 = *arg1;
}

typedef struct {
    /* +0x00 */ u8 pad_0x00[0x14];
    /* +0x14 */ u32 field_0x14;
} RawView_25; /* size: 0x18 */
s32 fn_80078904(s32 arg0) {
    if (res_node_is_valid(0) == 0) {
        nw4r::db::Panic((const char*)&lbl_8058EA50, 0xA5, (const char*)&lbl_8058EA30);
    }
    if (res_node_is_valid((s32)(arg0)) != 0) {
        return (((RawView_25*)res_node_ref((const ResHandle*)(arg0)))->field_0x14 & 0x100) != 0;
    }
    return 0;
}

s32 fn_80078988(s32 arg0, void* a1) {
    fn_800789B8(0, 0);
    return arg0;
}

u32 fn_800789B8(s32 *arg0, s32 *arg1) {
    *arg0 = *arg1;
}

} /* extern "C" */

/* 0x800789C4 (0x64): wraps `pData`, asserting its alignment. */
/* untyped: opaque handle - the block address */
nw4r::g3d::ResShp::ResShp(void* pData) : ResCommon<ResShpData>(pData) {
    if ((u32)pData & 3) {
        nw4r::db::Panic((const char*)&lbl_8058EA20, 0x3A, (const char*)&lbl_8058E9F4);
    }
}

extern "C" {

} /* extern "C" */

/* 0x80078A28 (0x8): stores the block address. */
NW4R_G3D_RESCOMMON_CTOR(nw4r::g3d::ResShpData)

extern "C" {

} /* extern "C" */

/* 0x80078A30 (0x64): wraps `pData`, asserting its alignment. */
/* untyped: opaque handle - the block address */
nw4r::g3d::ResMat::ResMat(void* pData) : ResCommon<ResMatData>(pData) {
    if ((u32)pData & 3) {
        nw4r::db::Panic((const char*)&lbl_8058EAC8, 0x26D, (const char*)&lbl_8058EAA0);
    }
}

extern "C" {

} /* extern "C" */

/* 0x80078A94 (0x8): stores the block address. */
NW4R_G3D_RESCOMMON_CTOR(nw4r::g3d::ResMatData)

extern "C" {

typedef struct {
    /* +0x00 */ u8 pad_0x00[0x4];
    /* +0x04 */ u32 field_0x04;
} RawView_27; /* size: 0x8 */
typedef struct {
    /* +0x00 */ u8 field_0x00;
    /* +0x01 */ u8 field_0x01;
    /* +0x02 */ u8 field_0x02;
    /* +0x03 */ u8 field_0x03;
    /* +0x04 */ u8 field_0x04;
    /* +0x05 */ u8 field_0x05;
    /* +0x06 */ u8 field_0x06;
    /* +0x07 */ u8 field_0x07;
    /* +0x08 */ u8 field_0x08;
    /* +0x09 */ u8 field_0x09;
} RawView_26; /* size: 0xA */
u32 fn_80078A9C(s32 arg0, void *arg1, s32 arg2, s32 arg3, u32 arg_sp0) {
}


typedef struct {
    /* +0x00 */ u8 pad_0x00[0x4];
    /* +0x04 */ s32 field_0x04;
    /* +0x08 */ s32 field_0x08;
    /* +0x0C */ s32 field_0x0C;
    /* +0x10 */ s32 field_0x10;
    /* +0x14 */ s32 field_0x14;
    /* +0x18 */ s32 field_0x18;
    /* +0x1C */ s32 field_0x1C;
    /* +0x20 */ s32 field_0x20;
    /* +0x24 */ s32 field_0x24;
    /* +0x28 */ s32 field_0x28;
    /* +0x2C */ u32 field_0x2C;
    /* +0x30 */ u32 field_0x30;
    /* +0x34 */ u32 field_0x34;
} RawView_28; /* size: 0x38 */
} /* extern "C" */


/* 0x80078DC0 (0xBC): constructs every handle empty and clears the trailing words. */
CopiedMatResources::CopiedMatResources()
    : texObj(NULL), tlutObj(NULL), genMode(NULL), tev(NULL), pix(NULL), tevColor(NULL), indMtxAndScale(NULL),
      chan(NULL), texCoordGen(NULL), misc(NULL), texSrt(NULL), word_0x2C(0), word_0x30(0), word_0x34(0) {}

extern "C" {

typedef struct {
    /* +0x00 */ u8 pad_0x00[0x6];
    /* +0x06 */ u8 field_0x06;
    /* +0x07 */ u8 field_0x07;
    /* +0x08 */ u8 field_0x08;
    /* +0x09 */ u8 field_0x09;
    /* +0x0A */ u8 field_0x0A;
    /* +0x0B */ u8 field_0x0B;
    /* +0x0C */ u8 field_0x0C;
    /* +0x0D */ u8 field_0x0D;
} RawView_29; /* size: 0xE */
u32 fn_80078E7C(s32 arg0, void *arg1, u32 arg2, s32 arg3, u32 arg_sp0) {
}


typedef struct {
    /* +0x00 */ u8 pad_0x00[0x6];
    /* +0x06 */ u8 field_0x06;
    /* +0x07 */ u8 field_0x07;
    /* +0x08 */ u8 field_0x08;
    /* +0x09 */ u8 field_0x09;
    /* +0x0A */ u8 field_0x0A;
    /* +0x0B */ u8 field_0x0B;
    /* +0x0C */ u8 field_0x0C;
    /* +0x0D */ u8 field_0x0D;
} RawView_30; /* size: 0xE */
u32 fn_80079018(s32 arg0, void *arg1, u32 arg2, s32 arg3, s32 arg4, u32 arg_sp0) {
}


typedef struct {
    /* +0x00 */ u8 pad_0x00[0x2C];
    /* +0x2C */ u32 field_0x2C;
} RawView_34; /* size: 0x30 */
typedef struct {
    /* +0x00 */ u8 pad_0x00[0x4];
    /* +0x04 */ u32 field_0x04;
} RawView_32; /* size: 0x8 */
typedef struct {
    /* +0x00 */ u8 field_0x00;
    /* +0x01 */ u8 field_0x01;
    /* +0x02 */ u8 field_0x02;
    /* +0x03 */ u8 field_0x03;
    /* +0x04 */ u8 field_0x04;
    /* +0x05 */ u8 field_0x05;
    /* +0x06 */ u8 field_0x06;
    /* +0x07 */ u8 field_0x07;
    /* +0x08 */ u8 field_0x08;
    /* +0x09 */ u8 field_0x09;
} RawView_31; /* size: 0xA */
typedef struct {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ u8 field_0x04;
    /* +0x05 */ u8 field_0x05;
    /* +0x06 */ u8 field_0x06;
    /* +0x07 */ u8 field_0x07;
    /* +0x08 */ u8 field_0x08;
    /* +0x09 */ u8 field_0x09;
    /* +0x0A */ u8 field_0x0A;
    /* +0x0B */ u8 field_0x0B;
    /* +0x0C */ u8 field_0x0C;
    /* +0x0D */ u8 field_0x0D;
} RawView_33; /* size: 0xE */
void* fn_800791D8(u32 *arg0, s32 arg1, s32 arg2, void *arg3, void *arg4) {
}


void g3d_draw_res_mdl_directly(s32 *arg0, s32 arg1, s32 arg2, s32 arg3, s32 arg4, s32 arg5, s32 arg6, s32 arg7, u32 arg_sp0) {
}


u32 fn_80079604(u32 arg0, u32 arg1, s32 (*arg2)(u32, u32), u32 arg_sp0) {
}


u32 fn_80079938(u32 arg0, u32 arg1, s32 arg2) {
}


u32 fn_800799BC(u32 arg0, u32 arg1, s32 (**arg2)(u32, u32)) {
}


u32 fn_80079A48(s32 arg0, s32 arg1, s32 arg2, s32 (**arg3)(s32, s32), u32 arg_sp0) {
}


u32 fn_80079B38(u32 arg0, u32 arg1, s32 (**arg2)(u32, u32), u32 arg_sp0) {
}


typedef struct {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ u32 field_0x04;
    /* +0x08 */ u32 field_0x08;
} RawView_35; /* size: 0xC */
u32 fn_80079E6C(s32 *arg0, s32 *arg1) {
    s32 sp10;
    s32 spC;
    s32 sp8;
    s32 temp_r4;
    void *temp_r3;

    temp_r3 = (void *)(fn_80079F10(0, 0));
    temp_r4 = ((RawView_35*)temp_r3)->field_0x00;
    sp8 = temp_r4;
    spC = ((RawView_35*)temp_r3)->field_0x04;
    sp10 = ((RawView_35*)temp_r3)->field_0x08;
    fn_80079EE4((void *)(arg0), (void *)(fn_80079F10((void*)(arg1), (void*)(temp_r4))));
    fn_80079EE4((void *)(arg1), (void *)(fn_80079F10((void*)(&sp8), 0)));
}

typedef struct {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ u8 field_0x04;
    /* +0x05 */ u8 field_0x05;
    /* +0x06 */ u8 field_0x06;
    /* +0x07 */ u8 field_0x07;
    /* +0x08 */ u8 field_0x08;
    /* +0x09 */ u8 field_0x09;
    /* +0x0A */ u8 field_0x0A;
    /* +0x0B */ u8 field_0x0B;
    /* +0x0C */ u8 field_0x0C;
    /* +0x0D */ u8 field_0x0D;
} RawView_36; /* size: 0xE */
typedef struct {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ u16 field_0x04;
    /* +0x06 */ u16 field_0x06;
    /* +0x08 */ u16 field_0x08;
    /* +0x0A */ u16 field_0x0A;
} RawView_37; /* size: 0xC */
u32 fn_80079EE4(void *arg0, void *arg1) {
    ((RawView_36*)arg0)->field_0x00 = (f32) ((RawView_37*)arg1)->field_0x00;
    ((RawView_36*)arg0)->field_0x04 = (u16) ((RawView_37*)arg1)->field_0x04;
    ((RawView_36*)arg0)->field_0x06 = (u16) ((RawView_37*)arg1)->field_0x06;
    ((RawView_36*)arg0)->field_0x08 = (u16) ((RawView_37*)arg1)->field_0x08;
    ((RawView_36*)arg0)->field_0x0A = (u16) ((RawView_37*)arg1)->field_0x0A;
}

void* fn_80079F10(void* a0, void* a1) {

}

typedef struct {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ u32 field_0x04;
    /* +0x08 */ u16 field_0x08;
} RawView_39; /* size: 0xA */
typedef struct {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ u32 field_0x04;
    /* +0x08 */ u16 field_0x08;
} RawView_38; /* size: 0xA */
s32 fn_80079F14(void *arg0, void *arg1) {
    f32 temp_f0;
    f32 temp_f1;
    u16 temp_r0;
    u16 temp_r5;

    temp_r0 = ((RawView_38*)arg1)->field_0x04;
    temp_r5 = ((RawView_39*)arg0)->field_0x04;
    if (temp_r5 < temp_r0) {
        return 1;
    }
    if (temp_r5 > temp_r0) {
        return 0;
    }
    temp_f1 = ((RawView_39*)arg0)->field_0x00;
    temp_f0 = ((RawView_38*)arg1)->field_0x00;
    if (temp_f1 < temp_f0) {
        return 1;
    }
    if ((temp_f1 == temp_f0) && ((u16) ((RawView_39*)arg0)->field_0x08 < (u16) ((RawView_38*)arg1)->field_0x08)) {
        return 1;
    }
    return 0;
}

typedef struct {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ u32 field_0x04;
} RawView_41; /* size: 0x8 */
typedef struct {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ u32 field_0x04;
} RawView_40; /* size: 0x8 */
s32 fn_80079F78(void *arg0, void *arg1) {
    u16 temp_r0;
    u16 temp_r5;

    temp_r0 = ((RawView_40*)arg1)->field_0x04;
    temp_r5 = ((RawView_41*)arg0)->field_0x04;
    if (temp_r5 < temp_r0) {
        return 1;
    }
    if (temp_r5 > temp_r0) {
        return 0;
    }
    return ((RawView_41*)arg0)->field_0x00 > ((RawView_40*)arg1)->field_0x00;
}

} /* extern "C" */
