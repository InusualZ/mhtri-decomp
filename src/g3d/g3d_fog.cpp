/*
 * g3d/g3d_fog.cpp - nw4r g3d `Fog`: the handle constructor over a FogData record, Init, the copy, the setters and
 *   SetGP (the GX fog and range-adjustment load).
 * RANGE. .text 0x80079FB4-0x8007A400 (8 functions); extab 0x800083D8-0x80008408, extabindex 0x80020BEC-0x80020C34,
 *   .data 0x8058EAD8-0x8058EB38 (opens on "g3d_fog.cpp", read from fog_init on), .sdata2 0x80795E28-0x80795E30 (a
 *   0.0 pooled again after `g3d/g3d_draw.cpp`'s 0x80795E18, so a new TU).  The draw sort helpers 0x80079E6C-0x80079FB4
 *   before it are `g3d/g3d_draw.cpp`'s (called or address-taken only there); `g3d/g3d_init.cpp` follows at the
 *   version-string user 0x8007A400.
 * FLAGS. cflags_g3d (docs/g3d.md); `#pragma peephole off` (playbook idea 106) except fn_8007A1B8, fn_8007A2A8 and
 *   fn_8007A354, which measure better with the pass on.
 * NAMES. fog_ctor and fog_init are GUESSES (nw4r's Fog::Fog(FogData*) and Fog::Init: the handle store and the
 *   reset of the record).
 * RESIDUALS. Partial: fog_ctor, fog_init, fn_8007A0B4, fn_8007A1B8, fn_8007A2A8, fn_8007A354 (m2c views).
 *   flipcheck: `.data` and `.sdata2` are claimed and not emitted (the strings and the pool word are declared).
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

extern const char lbl_8058EAD8[];
extern const char lbl_8058EAE8[];
extern const char lbl_8058EB08[];
extern f32 lbl_80795E28;

extern "C" {
u32 GXSetFog(s32, void*, f32, f32, f32, f32);
u32 GXSetFogRangeAdj(u8, u16, void*);
u32 color_rgba_copy(s32, void*);
s32 fog_is_valid(s32);
void* fog_ptr(s32);
s32 mtx44_ptr(s32);
u32 GXInitFogAdjTable(void*, u16, s32);
/* internal */ s32 fog_ctor(s32 arg0, void* a1);
/* internal */ u32 fn_80079FE4(s32 *arg0, s32 arg1);
/* internal */ void fog_init(s32 arg0);
/* internal */ s32 fog_copy_to(s32 arg0, s32 arg1);
/* internal */ s32 fog_ptr_const(s32 *arg0);
/* internal */ void fn_8007A1B8(s32 arg0, s32 *arg1, f32 *arg2, f32 *arg3, f32 *arg4, f32 *arg5, s32 arg6, u32 arg_sp0);
/* internal */ void fog_set_range_adj_param(s32 arg0, u16 arg1, s16 arg2, s32 arg3);
/* internal */ void fog_set_gp(s32 arg0);
} /* extern "C" */

extern "C" {
s32 fog_ctor(s32 arg0, void* a1) {
    fn_80079FE4(0, 0);
    return arg0;
}

u32 fn_80079FE4(s32 *arg0, s32 arg1) {
    *arg0 = arg1;
}

typedef struct {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ u32 field_0x04;
    /* +0x08 */ u32 field_0x08;
    /* +0x0C */ u32 field_0x0C;
    /* +0x10 */ u32 field_0x10;
    /* +0x14 */ u8 field_0x14;
    /* +0x15 */ u8 field_0x15;
    /* +0x16 */ u8 field_0x16;
    /* +0x17 */ u8 field_0x17;
    /* +0x18 */ u8 field_0x18;
    /* +0x19 */ u8 field_0x19;
    /* +0x1A */ u8 field_0x1A;
    /* +0x1B */ u8 field_0x1B;
    /* +0x1C */ u8 field_0x1C;
    /* +0x1D */ u8 field_0x1D;
    /* +0x1E */ u8 field_0x1E;
    /* +0x1F */ u8 field_0x1F;
    /* +0x20 */ u8 field_0x20;
    /* +0x21 */ u8 field_0x21;
    /* +0x22 */ u8 field_0x22;
    /* +0x23 */ u8 field_0x23;
    /* +0x24 */ u8 field_0x24;
    /* +0x25 */ u8 field_0x25;
    /* +0x26 */ u8 field_0x26;
    /* +0x27 */ u8 field_0x27;
    /* +0x28 */ u8 field_0x28;
    /* +0x29 */ u8 field_0x29;
    /* +0x2A */ u8 field_0x2A;
    /* +0x2B */ u8 field_0x2B;
    /* +0x2C */ u8 field_0x2C;
    /* +0x2D */ u8 field_0x2D;
    /* +0x2E */ u8 field_0x2E;
    /* +0x2F */ u8 field_0x2F;
    /* +0x30 */ u8 field_0x30;
    /* +0x31 */ u8 field_0x31;
} RawView_42; /* size: 0x32 */
void fog_init(s32 arg0) {
    void *temp_r3;

    if (fog_is_valid(0) == 0) {
        nw4r::db::Panic((const char*)&lbl_8058EAD8, 0x20, (const char*)&lbl_8058EAE8);
    }
    if (fog_is_valid((s32)(arg0)) != 0) {
        temp_r3 = (void *)(fog_ptr((s32)(arg0)));
        ((RawView_42*)temp_r3)->field_0x00 = 0;
        ((RawView_42*)temp_r3)->field_0x04 = (f32) lbl_80795E28;
        ((RawView_42*)temp_r3)->field_0x08 = (f32) lbl_80795E28;
        ((RawView_42*)temp_r3)->field_0x0C = (f32) lbl_80795E28;
        ((RawView_42*)temp_r3)->field_0x10 = (f32) lbl_80795E28;
        ((RawView_42*)temp_r3)->field_0x17 = 0;
        ((RawView_42*)temp_r3)->field_0x16 = 0;
        ((RawView_42*)temp_r3)->field_0x15 = 0;
        ((RawView_42*)temp_r3)->field_0x14 = 0;
        ((RawView_42*)temp_r3)->field_0x18 = 0;
        ((RawView_42*)temp_r3)->field_0x19 = 0;
        ((RawView_42*)temp_r3)->field_0x1A = 0;
        ((RawView_42*)temp_r3)->field_0x1C = 0;
        ((RawView_42*)temp_r3)->field_0x1E = 0;
        ((RawView_42*)temp_r3)->field_0x20 = 0;
        ((RawView_42*)temp_r3)->field_0x22 = 0;
        ((RawView_42*)temp_r3)->field_0x24 = 0;
        ((RawView_42*)temp_r3)->field_0x26 = 0;
        ((RawView_42*)temp_r3)->field_0x28 = 0;
        ((RawView_42*)temp_r3)->field_0x2A = 0;
        ((RawView_42*)temp_r3)->field_0x2C = 0;
        ((RawView_42*)temp_r3)->field_0x2E = 0;
    }
}

typedef struct {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ u8 pad_0x04[0x4];
    /* +0x08 */ u32 field_0x08;
    /* +0x0C */ u8 pad_0x0C[0x4];
    /* +0x10 */ u32 field_0x10;
    /* +0x14 */ u8 pad_0x14[0x4];
    /* +0x18 */ u32 field_0x18;
    /* +0x1C */ u8 pad_0x1C[0x4];
    /* +0x20 */ u32 field_0x20;
    /* +0x24 */ u8 pad_0x24[0x4];
    /* +0x28 */ u32 field_0x28;
} RawView_43; /* size: 0x2C */
typedef struct {
    /* +0x00 */ f64 field_0x00;
    /* +0x08 */ f64 field_0x08;
    /* +0x10 */ f64 field_0x10;
    /* +0x18 */ f64 field_0x18;
    /* +0x20 */ f64 field_0x20;
    /* +0x28 */ f64 field_0x28;
} RawView_44; /* size: 0x30 */
s32 fog_copy_to(s32 arg0, s32 arg1) {
    u32 spC;
    u32 sp8;
    void *temp_r3;

    if ((s32) (arg1 & 3) != 0) {
        nw4r::db::Panic((const char*)&lbl_8058EAD8, 0x3D, (const char*)&lbl_8058EB08);
    }
    if (fog_is_valid((s32)(arg0)) == 0) {
        nw4r::db::Panic((const char*)&lbl_8058EAD8, 0x3E, (const char*)&lbl_8058EAE8);
    }
    if ((arg1 != 0) && (fog_is_valid((s32)(arg0)) != 0)) {
        temp_r3 = (void *)(fog_ptr_const((s32 *)(arg0)));
        ((RawView_43*)arg1)->field_0x00 = (f64) ((RawView_44*)temp_r3)->field_0x00;
        ((RawView_43*)arg1)->field_0x08 = (f64) ((RawView_44*)temp_r3)->field_0x08;
        ((RawView_43*)arg1)->field_0x10 = (f64) ((RawView_44*)temp_r3)->field_0x10;
        ((RawView_43*)arg1)->field_0x18 = (f64) ((RawView_44*)temp_r3)->field_0x18;
        ((RawView_43*)arg1)->field_0x20 = (f64) ((RawView_44*)temp_r3)->field_0x20;
        ((RawView_43*)arg1)->field_0x28 = (f64) ((RawView_44*)temp_r3)->field_0x28;
        return *((s32*)fog_ctor((s32)(&spC), (void*)(arg1)));
    }
    return *((s32*)fog_ctor((s32)(&sp8), (void*)(0)));
}

s32 fog_ptr_const(s32 *arg0) {
    return *arg0;
}

typedef struct {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ u32 field_0x04;
    /* +0x08 */ u32 field_0x08;
    /* +0x0C */ u32 field_0x0C;
    /* +0x10 */ u32 field_0x10;
    /* +0x14 */ s32 field_0x14;
} RawView_45; /* size: 0x18 */
#pragma peephole on
void fn_8007A1B8(s32 arg0, s32 *arg1, f32 *arg2, f32 *arg3, f32 *arg4, f32 *arg5, s32 arg6, u32 arg_sp0) {
    void *temp_r3;

    if (fog_is_valid(0) == 0) {
        nw4r::db::Panic((const char*)&lbl_8058EAD8, 0x68, (const char*)&lbl_8058EAE8);
    }
    if (fog_is_valid((s32)(arg0)) != 0) {
        temp_r3 = (void *)(fog_ptr((s32)(arg0)));
        if (arg1 != NULL) {
            *arg1 = ((RawView_45*)temp_r3)->field_0x00;
        }
        if (arg2 != NULL) {
            *arg2 = ((RawView_45*)temp_r3)->field_0x04;
        }
        if (arg3 != NULL) {
            *arg3 = ((RawView_45*)temp_r3)->field_0x08;
        }
        if (arg4 != NULL) {
            *arg4 = ((RawView_45*)temp_r3)->field_0x0C;
        }
        if (arg5 != NULL) {
            *arg5 = ((RawView_45*)temp_r3)->field_0x10;
        }
        if (arg6 != 0) {
            color_rgba_copy((s32)(arg6), (void*)(&((RawView_45*)temp_r3)->field_0x14));
        }
    }
}
#pragma peephole off

typedef struct {
    /* +0x00 */ u8 pad_0x00[0x1A];
    /* +0x1A */ u8 field_0x1A;
    /* +0x1B */ u8 field_0x1B;
    /* +0x1C */ u8 field_0x1C;
    /* +0x1D */ u8 field_0x1D;
    /* +0x1E */ u8 field_0x1E;
    /* +0x1F */ u8 field_0x1F;
} RawView_46; /* size: 0x20 */
#pragma peephole on
void fog_set_range_adj_param(s32 arg0, u16 arg1, s16 arg2, s32 arg3) {
    void *temp_r3;

    if (fog_is_valid(0) == 0) {
        nw4r::db::Panic((const char*)&lbl_8058EAD8, 0x81, (const char*)&lbl_8058EAE8);
    }
    if (fog_is_valid((s32)(arg0)) != 0) {
        temp_r3 = (void *)(fog_ptr((s32)(arg0)));
        ((RawView_46*)temp_r3)->field_0x1A = arg2;
        GXInitFogAdjTable((void*)(&((RawView_46*)temp_r3)->field_0x1C), (u16)(arg1), (s32)(mtx44_ptr((s32)(arg3))));
    }
}
#pragma peephole off

typedef struct {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ u32 field_0x04;
    /* +0x08 */ u32 field_0x08;
    /* +0x0C */ u32 field_0x0C;
    /* +0x10 */ u32 field_0x10;
    /* +0x14 */ u32 field_0x14;
    /* +0x18 */ u8 field_0x18;
    /* +0x19 */ u8 field_0x19;
    /* +0x1A */ u8 field_0x1A;
    /* +0x1B */ u8 field_0x1B;
    /* +0x1C */ u8 field_0x1C;
    /* +0x1D */ u8 field_0x1D;
    /* +0x1E */ u8 field_0x1E;
    /* +0x1F */ u8 field_0x1F;
} RawView_47; /* size: 0x20 */
#pragma peephole on
void fog_set_gp(s32 arg0) {
    s32 sp8;
    void *temp_r3;

    if (fog_is_valid(0) == 0) {
        nw4r::db::Panic((const char*)&lbl_8058EAD8, 0x8F, (const char*)&lbl_8058EAE8);
    }
    if (fog_is_valid((s32)(arg0)) != 0) {
        temp_r3 = (void *)(fog_ptr_const((s32 *)(arg0)));
        if ((s32) ((RawView_47*)temp_r3)->field_0x00 != 0) {
            GXSetFogRangeAdj((u8)(((RawView_47*)temp_r3)->field_0x18), (u16)(((RawView_47*)temp_r3)->field_0x1A), (void*)(&((RawView_47*)temp_r3)->field_0x1C));
        }
        sp8 = ((RawView_47*)temp_r3)->field_0x14;
        GXSetFog((s32)(((RawView_47*)temp_r3)->field_0x00), (void*)(&sp8), (f32)(((RawView_47*)temp_r3)->field_0x04), (f32)(((RawView_47*)temp_r3)->field_0x08), (f32)(((RawView_47*)temp_r3)->field_0x0C), (f32)(((RawView_47*)temp_r3)->field_0x10));
    }
}
#pragma peephole off

} /* extern "C" */
