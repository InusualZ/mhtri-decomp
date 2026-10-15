/*
 * g3d/g3d_draw1mat1shp.cpp - nw4r g3d's one-material/one-shape draw: the material state loader fn_800761BC, the
 *   draw itself (fn_800769F4) and the ResMat/ResShp/ResVtxFurPos/ResMatFur handle members it pulls in, the GD XF writer
 *   and FPow.
 * RANGE. .text 0x800761BC-0x800777B0 (55 functions); extab 0x80008210-0x800082C8, extabindex 0x80020940-0x80020A54,
 *   .rodata 0x8056F658-0x8056F668 (fn_800769F4's index table), .data 0x8058E608-0x8058E880 (opens on
 *   "g3d_draw1mat1shp.cpp", read from fn_800769F4; the handle members' `_ac.h` asserts follow), .sdata
 *   0x807911E0-0x807911F8 (the "ref"/"ResShp" names the handle asserts print), .sdata2 0x80795E00-0x80795E18 (a 1.0
 *   pooled again after `g3d/g3d_camera.cpp`'s 0x80795DD0).  fn_800761BC and fn_8007663C are called only from
 *   fn_800769F4's chain, and the handle members before fn_800769F4 read this unit's strings (0x8058E730..0x8058E870),
 *   so the TU opens at 0x800761BC; `g3d/g3d_draw.cpp` opens on "g3d_draw.cpp" and a second pooled 1.0 at 0x800777B0.
 * FLAGS. cflags_g3d (docs/g3d.md); `#pragma peephole off` (playbook idea 106) except fn_800761BC, which measures
 *   better with the pass on.
 * NAMES. res_mat_misc_copy_ctor is a GUESS (the copy constructor of the material's misc block handle, the
 *   res_mat_chan_copy_ctor scheme).  g3d_gd_set_chan_mat_color is a GUESS (XF 0x100C plus the channel); GDWriteXFCmd is a GUESS (the SDK's GD
 *   inline that writes one XF register).  The handle members are nw4r's `g3d_resmat_ac.h`/`g3d_resshp_ac.h`.
 * RESIDUALS. Unwritten (empty stub): fn_800769F4.  Partial: fn_800761BC, fn_8007663C, fn_80077480, fn_80077490,
 *   fn_80077554, res_mat_misc_copy_ctor, fn_800775CC, fn_80077708 (m2c views).
 *   flipcheck: `.rodata`, `.data`, `.sdata` and `.sdata2` are claimed and not emitted (declared).
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

extern const char lbl_8058E694[];
extern const char lbl_8058E6A4[];
extern const char lbl_8058E6C0[];
extern const char lbl_8058E6D0[];
extern const char lbl_8058E6F0[];
extern const char lbl_8058E700[];
extern const char lbl_8058E720[];
extern const char lbl_8058E730[];
extern const char lbl_8058E758[];
extern const char lbl_8058E768[];
extern const char lbl_8058E790[];
extern const char lbl_8058E7A0[];
extern const char lbl_8058E7C8[];
extern const char lbl_8058E7D8[];
extern const char lbl_8058E7E4[];
extern const char lbl_8058E800[];
extern const char lbl_8058E810[];
extern const char lbl_8058E838[];
extern const char lbl_8058E848[];
extern const char lbl_8058E870[];
extern u32 lbl_807911E0;
extern u32 lbl_807911E4;
extern u32 lbl_807911E8;
extern u32 lbl_807911EC;
extern u32 lbl_807911F0;

extern "C" {
u32 fn_8008715C(void*);
f32 powf(f32 x, f32 y);
/* internal */ u32 fn_800761BC(s32 arg0, s32 arg1, void *arg2, void **arg3, s32 arg4);
/* internal */ void* fn_8007663C(s32 *arg0);
/* internal */ u32 fn_800769F4(s32 *arg0, s32 *arg1, s32 arg2, s32 arg3, s32 arg4, void *arg5, s32 arg6);
/* internal */ u32 GDWriteXFCmd(u16 arg0, s32 arg1);
/* internal */ u32 fn_80077474(s32 arg0);
/* internal */ u32 fn_80077480(u16 arg0);
/* internal */ u32 fn_80077490(u8 arg0);
/* internal */ s32 fn_800774A4(void* a0);
/* internal */ s32 fn_80077554(s32 arg0, void* a1);
/* internal */ u32 fn_80077584(s32 *arg0, s32 *arg1);
/* internal */ s32 res_mat_misc_copy_ctor(s32 arg0, const s32* a1);
/* internal */ u32 fn_800775C0(s32 *arg0, s32 *arg1);
/* internal */ void* fn_800775CC(s32 arg0);
/* internal */ s32 fn_80077630(s32 *arg0);
/* internal */ s32 fn_80077644(void* a0);
/* internal */ s32 fn_80077708(s32 arg0, void* a1);
/* internal */ u32 fn_80077738(s32 *arg0, s32 *arg1);
} /* extern "C" */

extern "C" {
typedef struct {
    /* +0x00 */ u8 pad_0x00[0x8];
    /* +0x08 */ u32 (*field_0x08)(...);
} RawView_3; /* size: 0xC */
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
    /* +0x24 */ s32 field_0x24;
    /* +0x28 */ u32 field_0x28;
} RawView_2; /* size: 0x2C */
#pragma peephole on
u32 fn_800761BC(s32 arg0, s32 arg1, void *arg2, void **arg3, s32 arg4) {

    u32 sp90;
    u32 sp88;
    u32 sp84;
    u32 sp80;
    u32 sp7C;
    GXColor sp78;
    GXColor sp74;
    s32 sp70;
    s32 sp6C;
    s32 sp68;
    s32 sp64;
    s32 sp60;
    s32 sp5C;
    s32 sp58;
    s32 sp54;
    s32 sp50;
    s32 sp4C;
    s32 sp48;
    s32 sp44;
    s32 sp40;
    s32 sp3C;
    s32 sp38;
    s32 sp34;
    s32 sp30;
    s32 sp2C;
    s32 sp18;
    s32 sp14;
    s32 sp10;
    s32 spC;
    s32 sp8;

    if (arg4 == 0) {
        nw4r::g3d::ResMatMisc misc((void*)NULL);
        if ((arg2 == NULL) || (reinterpret_cast<const nw4r::g3d::ResMatMisc*>((s32 *)(&((RawView_2*)arg2)->field_0x24))->IsValid() == 0)) {
            sp70 = (s32)reinterpret_cast<nw4r::g3d::ResMat*>((void*)(arg0))->GetResMatMisc().mpData;
            misc = *reinterpret_cast<const nw4r::g3d::ResMatMisc*>(&sp70);
        } else {
            misc = *reinterpret_cast<const nw4r::g3d::ResMatMisc*>(&((RawView_2*)arg2)->field_0x24);
        }
        g3d_state_load_fog(misc.GetFogIdx());
        sp6C = (s32)misc.mpData;
        g3d_state_set_mat_misc(*reinterpret_cast<nw4r::g3d::ResMatMisc*>((void*)(&sp6C)));
        if ((arg2 == NULL) || (reinterpret_cast<const nw4r::g3d::ResTlutObj*>((s32 *)(&((RawView_2*)arg2)->field_0x04))->IsValid() == 0)) {
            sp68 = (s32)reinterpret_cast<nw4r::g3d::ResMat*>((s32)(arg0))->GetResTlutObj().mpData;
            g3d_state_load_tlut_obj(*reinterpret_cast<nw4r::g3d::ResTlutObj*>((void*)(&sp68)));
        } else {
            sp64 = ((RawView_2*)arg2)->field_0x04;
            g3d_state_load_tlut_obj(*reinterpret_cast<nw4r::g3d::ResTlutObj*>((void*)(&sp64)));
        }
        if ((arg2 == NULL) || (reinterpret_cast<const nw4r::g3d::ResTexObj*>((s32 *)(arg2))->IsValid() == 0)) {
            sp60 = (s32)reinterpret_cast<nw4r::g3d::ResMat*>((s32)(arg0))->GetResTexObj().mpData;
            g3d_state_load_tex_obj(*reinterpret_cast<nw4r::g3d::ResTexObj*>((void*)(&sp60)));
        } else {
            sp5C = ((RawView_2*)arg2)->field_0x00;
            g3d_state_load_tex_obj(*reinterpret_cast<nw4r::g3d::ResTexObj*>((void*)(&sp5C)));
        }
        if ((arg2 == NULL) || (reinterpret_cast<const nw4r::g3d::ResGenMode*>((s32 *)(&((RawView_2*)arg2)->field_0x08))->IsValid() == 0)) {
            sp58 = (s32)reinterpret_cast<nw4r::g3d::ResMat*>((void*)(arg0))->GetResGenMode().mpData;
            g3d_state_set_gen_mode(*reinterpret_cast<nw4r::g3d::ResGenMode*>((void*)(&sp58)));
        } else {
            sp54 = ((RawView_2*)arg2)->field_0x08;
            g3d_state_set_gen_mode(*reinterpret_cast<nw4r::g3d::ResGenMode*>((void*)(&sp54)));
        }
        if ((arg2 == NULL) || (reinterpret_cast<const nw4r::g3d::ResTev*>((s32 *)(&((RawView_2*)arg2)->field_0x0C))->IsValid() == 0)) {
            sp50 = (s32)reinterpret_cast<nw4r::g3d::ResMat*>(arg0)->GetResTev().mpData;
            g3d_state_load_tev(*reinterpret_cast<nw4r::g3d::ResTev*>((void*)(&sp50)));
        } else {
            sp4C = ((RawView_2*)arg2)->field_0x0C;
            g3d_state_load_tev(*reinterpret_cast<nw4r::g3d::ResTev*>((void*)(&sp4C)));
        }
        if ((arg2 == NULL) || (reinterpret_cast<const nw4r::g3d::ResMatPix*>((s32 *)(&((RawView_2*)arg2)->field_0x10))->IsValid() == 0)) {
            sp48 = (s32)reinterpret_cast<nw4r::g3d::ResMat*>((void*)(arg0))->GetResMatPix().mpData;
            g3d_state_load_mat_pix(*reinterpret_cast<nw4r::g3d::ResMatPix*>((void*)(&sp48)));
        } else {
            sp44 = ((RawView_2*)arg2)->field_0x10;
            g3d_state_load_mat_pix(*reinterpret_cast<nw4r::g3d::ResMatPix*>((void*)(&sp44)));
        }
        if ((arg2 == NULL) || (reinterpret_cast<const nw4r::g3d::ResMatTevColor*>((void*)(&((RawView_2*)arg2)->field_0x14))->IsValid() == 0)) {
            sp40 = (s32)reinterpret_cast<nw4r::g3d::ResMat*>((s32)(arg0))->GetResMatTevColor().mpData;
            g3d_state_load_mat_tev_color(*reinterpret_cast<nw4r::g3d::ResMatTevColor*>((void*)(&sp40)));
        } else {
            sp3C = ((RawView_2*)arg2)->field_0x14;
            g3d_state_load_mat_tev_color(*reinterpret_cast<nw4r::g3d::ResMatTevColor*>((void*)(&sp3C)));
        }
        if ((arg2 == NULL) || (reinterpret_cast<const nw4r::g3d::ResMatIndMtxAndScale*>((void*)(&((RawView_2*)arg2)->field_0x18))->IsValid() == 0)) {
            if (arg3 != NULL) {
                sp38 = (s32)reinterpret_cast<nw4r::g3d::ResMat*>((s32)(arg0))->GetResMatIndMtxAndScale().mpData;
                g3d_state_load_mat_ind_mtx(*reinterpret_cast<nw4r::g3d::ResMatIndMtxAndScale*>((void*)(&sp38)), (G3dIndMtxCallback*)((void*)(arg3)));
            } else {
                sp34 = (s32)reinterpret_cast<nw4r::g3d::ResMat*>((s32)(arg0))->GetResMatIndMtxAndScale().mpData;
                g3d_state_load_mat_ind_mtx_dl(*reinterpret_cast<nw4r::g3d::ResMatIndMtxAndScale*>((void*)(&sp34)));
            }
        } else if (arg3 != NULL) {
            sp30 = ((RawView_2*)arg2)->field_0x18;
            g3d_state_load_mat_ind_mtx(*reinterpret_cast<nw4r::g3d::ResMatIndMtxAndScale*>((void*)(&sp30)), (G3dIndMtxCallback*)((void*)(arg3)));
        } else {
            sp2C = ((RawView_2*)arg2)->field_0x18;
            g3d_state_load_mat_ind_mtx_dl(*reinterpret_cast<nw4r::g3d::ResMatIndMtxAndScale*>((void*)(&sp2C)));
        }
        g3d_state_load_light_set(misc.GetLightSetIdx(), &sp88, &sp84, &sp80, &sp7C, &sp74);
        sp78 = sp74;
        if ((arg2 == NULL) || (reinterpret_cast<const nw4r::g3d::ResMatChan*>((void*)(&((RawView_2*)arg2)->field_0x1C))->IsValid() == 0)) {
            g3d_state_load_mat_chan(reinterpret_cast<nw4r::g3d::ResMat*>((s32)(arg0))->GetResMatChan(), sp88, sp84, sp80,
                                    sp7C, sp78, (arg1 & 8) != 0);
        } else {
            g3d_state_load_mat_chan(*reinterpret_cast<nw4r::g3d::ResMatChan*>((void*)(&((RawView_2*)arg2)->field_0x1C)),
                                    sp88, sp84, sp80, sp7C, sp78, (arg1 & 8) != 0);
        }
        if ((arg2 == NULL) || (reinterpret_cast<const nw4r::g3d::ResMatTexCoordGen*>((void*)(&((RawView_2*)arg2)->field_0x20))->IsValid() == 0)) {
            sp18 = (s32)reinterpret_cast<nw4r::g3d::ResMat*>((void*)(arg0))->GetResMatTexCoordGen().mpData;
            g3d_state_load_tex_coord_gen(*reinterpret_cast<nw4r::g3d::ResMatTexCoordGen*>((void*)(&sp18)));
        } else {
            sp14 = ((RawView_2*)arg2)->field_0x20;
            g3d_state_load_tex_coord_gen(*reinterpret_cast<nw4r::g3d::ResMatTexCoordGen*>((void*)(&sp14)));
        }
        if ((arg2 == NULL) || (reinterpret_cast<const nw4r::g3d::ResTexSrt*>((void*)(&((RawView_2*)arg2)->field_0x28))->IsValid() == 0)) {
            sp10 = (s32)reinterpret_cast<nw4r::g3d::ResMat*>((s32)(arg0))->GetResTexSrt().mpData;
            fn_8008715C((void*)(&sp10));
            return;
        }
        spC = ((RawView_2*)arg2)->field_0x28;
        fn_8008715C((void*)(&spC));
        return;
    }
    if (arg3 != NULL) {
        if ((arg2 == NULL) || (reinterpret_cast<const nw4r::g3d::ResMatIndMtxAndScale*>((void*)(&((RawView_2*)arg2)->field_0x18))->IsValid() == 0)) {
            fn_8007663C((s32 *)(&sp90));
            ((RawView_3*)(*arg3))->field_0x08(arg3, &sp90);
            g3d_ind_mtx_op_load((G3dIndMtxOp*)((void*)(&sp90)));
            return;
        }
        sp8 = ((RawView_2*)arg2)->field_0x18;
        g3d_state_load_mat_ind_mtx(*reinterpret_cast<nw4r::g3d::ResMatIndMtxAndScale*>((void*)(&sp8)), (G3dIndMtxCallback*)((void*)(arg3)));
    }
}
#pragma peephole off

typedef struct {
    /* +0x00 */ u8 pad_0x00[0x30];
    /* +0x30 */ u8 field_0x30;
} RawView_4; /* size: 0x31 */
void* fn_8007663C(s32 *arg0) {
    void *var_r30;

    *arg0 = 0;
    var_r30 = arg0 + 4;
    do {
        MTX34_ctor((nw4r::math::MTX34*)(var_r30));
        var_r30 = &((RawView_4*)var_r30)->field_0x30;
    } while ((u32)var_r30 < (u32)(arg0 + 0x94));
    return arg0;
}

} /* extern "C" */

/* 0x8007669C (0x34): returns the material's MatTexCoordGen block. */
nw4r::g3d::ResMatTexCoordGen nw4r::g3d::ResMat::GetResMatTexCoordGen() {
    return ResMatTexCoordGen((u8*)GetResMatDLData() + 0xE0);
}

extern "C" {

} /* extern "C" */

/* 0x800766D0 (0x64): wraps `pData`, asserting its alignment. */
/* untyped: opaque handle - the block address */
nw4r::g3d::ResMatTexCoordGen::ResMatTexCoordGen(void* pData) : ResCommon<ResMatTexCoordGenData>(pData) {
    if ((u32)pData & 0x1F) {
        nw4r::db::Panic((const char*)&lbl_8058E758, 0x201, (const char*)&lbl_8058E730);
    }
}

extern "C" {

} /* extern "C" */

/* 0x80076734 (0x8): stores the block address. */
NW4R_G3D_RESCOMMON_CTOR(nw4r::g3d::ResMatTexCoordGenData)

/* 0x80076750 (0x14): whether the handle is set. */
bool nw4r::g3d::ResMatTevColor::IsValid() const {
    return mpData != NULL;
}

/* 0x8007673C (0x14): whether the handle is set. */
bool nw4r::g3d::ResMatTexCoordGen::IsValid() const {
    return mpData != NULL;
}

extern "C" {


} /* extern "C" */

/* 0x80076764 (0x30): returns the material's MatPix block. */
nw4r::g3d::ResMatPix nw4r::g3d::ResMat::GetResMatPix() {
    return ResMatPix(GetResMatDLData());
}

extern "C" {

} /* extern "C" */

/* 0x80076794 (0x64): wraps `pData`, asserting its alignment. */
/* untyped: opaque handle - the block address */
nw4r::g3d::ResMatPix::ResMatPix(void* pData) : ResCommon<ResMatPixData>(pData) {
    if ((u32)pData & 0x1F) {
        nw4r::db::Panic((const char*)&lbl_8058E790, 0x154, (const char*)&lbl_8058E768);
    }
}

extern "C" {

} /* extern "C" */

/* 0x800767F8 (0x8): stores the block address. */
NW4R_G3D_RESCOMMON_CTOR(nw4r::g3d::ResMatPixData)

extern "C" {

} /* extern "C" */

/* 0x80076800 (0x14): tells whether the handle is set. */
bool nw4r::g3d::ResMatPix::IsValid() const {
    return mpData != NULL;
}

/* 0x80076814 (0x14): tells whether the handle is set. */
bool nw4r::g3d::ResTev::IsValid() const {
    return mpData != NULL;
}

extern "C" {

} /* extern "C" */

/* 0x80076828 (0x34): returns the material's GenMode block. */
nw4r::g3d::ResGenMode nw4r::g3d::ResMat::GetResGenMode() {
    return ResGenMode((u8*)&ref() + 0x14);
}

extern "C" {

} /* extern "C" */

/* 0x8007685C (0x64): wraps `pData`, asserting its alignment. */
/* untyped: opaque handle - the block address */
nw4r::g3d::ResGenMode::ResGenMode(void* pData) : ResCommon<ResGenModeData>(pData) {
    if ((u32)pData & 3) {
        nw4r::db::Panic((const char*)&lbl_8058E870, 0xAF, (const char*)&lbl_8058E848);
    }
}

extern "C" {

} /* extern "C" */

/* 0x800768C0 (0x8): stores the block address. */
NW4R_G3D_RESCOMMON_CTOR(nw4r::g3d::ResGenModeData)

extern "C" {

} /* extern "C" */

/* 0x800768C8 (0x14): tells whether the handle is set. */
bool nw4r::g3d::ResGenMode::IsValid() const {
    return mpData != NULL;
}

extern "C" {


} /* extern "C" */

/* 0x800768DC (0x14): tells whether the handle is set. */
bool nw4r::g3d::ResTexObj::IsValid() const {
    return mpData != NULL;
}

extern "C" {


} /* extern "C" */

/* 0x800768F0 (0x14): tells whether the handle is set. */
bool nw4r::g3d::ResTlutObj::IsValid() const {
    return mpData != NULL;
}

extern "C" {


} /* extern "C" */

/* 0x80076904 (0x30): copies the handle. */
nw4r::g3d::ResMatMisc& nw4r::g3d::ResMatMisc::operator=(const ResMatMisc& rhs) {
    ResCommon<ResMatMiscData>::operator=(rhs);
    return *this;
}

extern "C" {


} /* extern "C" */

/* 0x80076940 (0x34): returns the material's MatMisc block. */
nw4r::g3d::ResMatMisc nw4r::g3d::ResMat::GetResMatMisc() {
    return ResMatMisc((u8*)&ref() + 0x1C);
}

extern "C" {

} /* extern "C" */

/* 0x80076974 (0x14): tells whether the handle is set. */
bool nw4r::g3d::ResMatMisc::IsValid() const {
    return mpData != NULL;
}

extern "C" {


} /* extern "C" */

/* 0x80076988 (0x64): wraps `pData`, asserting its alignment. */
/* untyped: opaque handle - the block address */
nw4r::g3d::ResMatMisc::ResMatMisc(void* pData) : ResCommon<ResMatMiscData>(pData) {
    if ((u32)pData & 3) {
        nw4r::db::Panic((const char*)&lbl_8058E838, 0xF3, (const char*)&lbl_8058E810);
    }
}

extern "C" {

} /* extern "C" */

/* 0x800769EC (0x8): stores the block address. */
NW4R_G3D_RESCOMMON_CTOR(nw4r::g3d::ResMatMiscData)

extern "C" {

typedef struct {
    /* +0x00 */ u8 pad_0x00[0x3];
    /* +0x03 */ u32 field_0x03;
} RawView_11; /* size: 0x7 */
typedef struct {
    /* +0x00 */ u8 pad_0x00[0x4];
    /* +0x04 */ u32 field_0x04;
} RawView_10; /* size: 0x8 */
typedef struct {
    /* +0x00 */ u8 pad_0x00[0x4];
    /* +0x04 */ u32 field_0x04;
} RawView_8; /* size: 0x8 */
typedef struct {
    /* +0x00 */ u8 pad_0x00[0x14];
    /* +0x14 */ s32 field_0x14;
    /* +0x18 */ s32 field_0x18;
    /* +0x1C */ s32 field_0x1C;
    /* +0x20 */ u8 pad_0x20[0x4];
    /* +0x24 */ s32 field_0x24;
    /* +0x28 */ u8 pad_0x28[0x4];
    /* +0x2C */ u8* field_0x2C;
    /* +0x30 */ u8* field_0x30;
    /* +0x34 */ u8* field_0x34;
} RawView_12; /* size: 0x38 */
typedef struct {
    /* +0x00 */ u8 pad_0x00[0x48];
    /* +0x48 */ u8 field_0x48;
    /* +0x49 */ u8 field_0x49;
    /* +0x4A */ u8 field_0x4A;
    /* +0x4B */ u8 field_0x4B;
    /* +0x4C */ u8 field_0x4C;
    /* +0x4D */ u8 field_0x4D;
    /* +0x4E */ u8 field_0x4E;
    /* +0x4F */ u8 field_0x4F;
    /* +0x50 */ u8 field_0x50;
    /* +0x51 */ u8 field_0x51;
} RawView_13; /* size: 0x52 */
typedef struct {
    /* +0x00 */ u8 pad_0x00[0xC];
    /* +0x0C */ u32 field_0x0C;
    /* +0x10 */ u32 field_0x10;
} RawView_5; /* size: 0x14 */
typedef struct {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ u32 field_0x04;
    /* +0x08 */ u32 field_0x08;
    /* +0x0C */ u32 field_0x0C;
} RawView_6; /* size: 0x10 */
typedef struct {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ u32 field_0x04;
    /* +0x08 */ u32 field_0x08;
    /* +0x0C */ u8 pad_0x0C[0x4];
    /* +0x10 */ u32 field_0x10;
    /* +0x14 */ u32 field_0x14;
    /* +0x18 */ u32 field_0x18;
} RawView_7; /* size: 0x1C */
typedef struct {
    /* +0x00 */ u8 field_0x00;
    /* +0x01 */ u8 field_0x01;
    /* +0x02 */ u8 field_0x02;
    /* +0x03 */ u8 field_0x03;
    /* +0x04 */ u8 field_0x04;
    /* +0x05 */ u8 field_0x05;
} RawView_9; /* size: 0x6 */
u32 fn_800769F4(s32 *arg0, s32 *arg1, s32 arg2, s32 arg3, s32 arg4, void *arg5, s32 arg6) {
}


} /* extern "C" */

/* 0x80077398 (0x64): returns the shape block, panicking on a NULL handle. */
nw4r::g3d::ResShpData& nw4r::g3d::ResShp::ref() {
    if (!IsValid()) {
        nw4r::db::Panic((const char*)&lbl_8058E6F0, 0x3A, (const char*)&lbl_8058E6D0, GetClassName(),
                        (const char*)&lbl_807911E8);
    }
    return *ptr();
}

/* 0x800773FC (0x8): returns the shape block. */
nw4r::g3d::ResShpData* nw4r::g3d::ResShp::ptr() {
    return mpData;
}

/* 0x80077404 (0x8): returns the class name. */
const char* nw4r::g3d::ResShp::GetClassName() {
    return (const char*)&lbl_807911F0;
}

extern "C" {

/* 0x8007740C (0x14): writes a colour channel's material colour (XF 0x100C + chan). */
void g3d_gd_set_chan_mat_color(u32 chan, GXColor color) {
    GDWriteXFCmd((chan & 1) + 0x100C, *(const u32*)&color);
}

u32 GDWriteXFCmd(u16 arg0, s32 arg1) {
    fn_80077490((u8)(0x10));
    fn_80077480((u16)(0U));
    fn_80077480((u16)(arg0));
    fn_80077474((s32)(arg1));
}

u32 fn_80077474(s32 arg0) {
    *(s32 *)0xCC008000 = arg0;
}

u32 fn_80077480(u16 arg0) {
    *(u16 *)0xCC008000 = arg0;
}

u32 fn_80077490(u8 arg0) {
    *(u8 *)0xCC008000 = arg0;
}

} /* extern "C" */

/* 0x800774A0 (0x4): raises `x` to the power `y` through the C library. */
namespace nw4r {
namespace math {
f32 FPow(f32 x, f32 y) {
    return powf(x, y);
}
}  // namespace math
}  // namespace nw4r

extern "C" {


typedef struct {
    /* +0x00 */ u8 pad_0x00[0x20];
    /* +0x20 */ u32 field_0x20;
} RawView_14; /* size: 0x24 */
/* untyped: opaque handle */
s32 fn_800774A4(void* a0) {
    return ((const RawView_14*)&reinterpret_cast<const nw4r::g3d::ResVtxFurPos*>(a0)->ref())->field_0x20;
}

} /* extern "C" */

/* 0x800774C8 (0x64): returns the fur-position block, panicking on a NULL handle. */
const nw4r::g3d::ResVtxFurPosData& nw4r::g3d::ResVtxFurPos::ref() const {
    if (!IsValid()) {
        nw4r::db::Panic((const char*)&lbl_8058E6C0, 0x13E, (const char*)&lbl_8058E6A4, GetClassName(),
                        (const char*)&lbl_807911EC);
    }
    return *ptr();
}

/* 0x8007752C (0x8): returns the fur-position block. */
const nw4r::g3d::ResVtxFurPosData* nw4r::g3d::ResVtxFurPos::ptr() const {
    return mpData;
}

/* 0x80077534 (0xC): returns the class name. */
const char* nw4r::g3d::ResVtxFurPos::GetClassName() {
    return (const char*)&lbl_8058E694;
}

/* 0x80077540 (0x14): tells whether the handle is set. */
bool nw4r::g3d::ResVtxFurPos::IsValid() const {
    return mpData != NULL;
}

extern "C" {

s32 fn_80077554(s32 arg0, void* a1) {
    fn_80077584(0, 0);
    return arg0;
}

u32 fn_80077584(s32 *arg0, s32 *arg1) {
    *arg0 = *arg1;
}

s32 res_mat_misc_copy_ctor(s32 arg0, const s32* a1) {
    fn_800775C0(0, 0);
    return arg0;
}

u32 fn_800775C0(s32 *arg0, s32 *arg1) {
    *arg0 = *arg1;
}

void* fn_800775CC(s32 arg0) {
    if (reinterpret_cast<const nw4r::g3d::ResMatFur*>(0)->IsValid() == 0) {
        nw4r::db::Panic((const char*)&lbl_8058E800, 0x11F, (const char*)&lbl_8058E7E4, nw4r::g3d::ResMatFur::GetClassName(), &lbl_807911E0);
    }
    fn_80077630((s32 *)(arg0));
}

s32 fn_80077630(s32 *arg0) {
    return *arg0;
}

} /* extern "C" */

/* 0x80077638 (0xC): returns the class name. */
const char* nw4r::g3d::ResMatFur::GetClassName() {
    return (const char*)&lbl_8058E7D8;
}

extern "C" {


typedef struct {
    /* +0x00 */ u8 pad_0x00[0x34];
    /* +0x34 */ u32 field_0x34;
} RawView_15; /* size: 0x38 */
s32 fn_80077644(void* a0) {
    return (reinterpret_cast<const nw4r::g3d::ResShp*>(a0)->ref().flag & 2) == 0;
}

} /* extern "C" */

/* 0x80077674 (0x64): returns the shape block, panicking on a NULL handle. */
const nw4r::g3d::ResShpData& nw4r::g3d::ResShp::ref() const {
    if (!IsValid()) {
        nw4r::db::Panic((const char*)&lbl_8058E720, 0x3A, (const char*)&lbl_8058E700, GetClassName(),
                        (const char*)&lbl_807911E4);
    }
    return *ptr();
}

/* 0x800776D8 (0x8): returns the shape block. */
const nw4r::g3d::ResShpData* nw4r::g3d::ResShp::ptr() const {
    return mpData;
}

/* 0x800776E0 (0x14): whether the handle is set. */
bool nw4r::g3d::ResShp::IsValid() const {
    return mpData != NULL;
}

extern "C" {

} /* extern "C" */

/* 0x800776F4 (0x14): tells whether the handle is set. */
bool nw4r::g3d::ResMatFur::IsValid() const {
    return mpData != NULL;
}

extern "C" {


s32 fn_80077708(s32 arg0, void* a1) {
    fn_80077738(0, 0);
    return arg0;
}

u32 fn_80077738(s32 *arg0, s32 *arg1) {
    *arg0 = *arg1;
}

} /* extern "C" */

/* 0x80077744 (0x64): wraps `pData`, asserting its alignment. */
/* untyped: opaque handle - the block address */
nw4r::g3d::ResMatFur::ResMatFur(void* pData) : ResCommon<ResMatFurData>(pData) {
    if ((u32)pData & 3) {
        nw4r::db::Panic((const char*)&lbl_8058E7C8, 0x11F, (const char*)&lbl_8058E7A0);
    }
}

extern "C" {

} /* extern "C" */

/* 0x800777A8 (0x8): stores the block address. */
NW4R_G3D_RESCOMMON_CTOR(nw4r::g3d::ResMatFurData)
