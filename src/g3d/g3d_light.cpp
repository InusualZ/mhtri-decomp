/*
 * g3d/g3d_light.cpp - nw4r g3d's lights: the LightObj copy, clear and GX light initialisers, LightSet, and the
 *   LightSetting constructor, copy and selection over its light-object, light-animation and ambient buffers.
 * RANGE. .text 0x8007A518-0x8007B2D4 (25 functions); extab 0x80008418-0x800084A0, extabindex 0x80020C4C-0x80020D18,
 *   .data 0x8058EB80-0x8058ED18 (opens on "g3d_light.cpp", read from light_setting_ctor, fn_8007B160 and
 *   fn_8007B224), .sdata2 0x80795E30-0x80795E58 (a 0.0 pooled again after `g3d/g3d_fog.cpp`'s 0x80795E28; read from
 *   fn_8007A768 on).  `g3d/g3d_init.cpp` precedes it; `g3d/g3d_obj.cpp`'s vtable (0x8058ED18) follows the last
 *   string, and the ScnLeaf constructor's second pooled 1.0 (0x80795E58 against this unit's 0x80795E34) puts the
 *   scene inlines after 0x8007B2D4 in another TU.
 * FLAGS. cflags_g3d (docs/g3d.md); `#pragma peephole off` throughout (playbook idea 106).
 * NAMES. light_obj_get_light_pos and light_obj_get_light_dir are GUESSES (nw4r's LightObj::GetLightPos/GetLightDir:
 *   tail calls to GXGetLightPos/GXGetLightDir on the object's GXLightObj).  light_setting_ctor is a GUESS (the LightSetting constructor: it asserts the light and light-set counts and
 *   the three buffers by name).
 * RESIDUALS. Unwritten (empty stubs): light_setting_ctor, dtor_8007AF28, fn_8007AF6C, fn_8007B074, fn_8007B0A0,
 *   fn_8007A814, fn_8007B160, fn_8007B224.  Partial: fn_8007A518..fn_8007A800 (m2c views).
 *   flipcheck: `.data` and `.sdata2` are claimed and not emitted.
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


extern f32 lbl_80795E30;
extern f32 lbl_80795E34;
extern f32 lbl_80795E38;

extern "C" {
u32 GXInitLightAttn(s32, f32, f32, f32, f32, f32, f32);
u32 GXInitLightDir(s32);
u32 GXInitLightAttnA(s32);
u32 GXInitLightAttnK(s32);
u32 GXInitLightSpot(s32);
u32 GXInitLightDistAttn(s32);
u32 GXInitLightPos(s32);
u32 GXGetLightPos(s32, s32, s32);
u32 GXGetLightDir(s32, s32, s32);
u32 GXInitLightColor(s32, void*);
u32 GXGetLightColor(s32);
/* internal */ void* fn_8007A518(s32 *arg0, s32 *arg1);
/* internal */ u32 fn_8007A564(s32 *arg0);
/* internal */ u32 fn_8007A578(s32 arg0, s32 *arg1);
/* internal */ u32 fn_8007A5A8(s32 *arg0, void* a1, void* a2, void* a3);
/* internal */ u32 fn_8007A5E4(s32 *arg0, void* a1, void* a2, void* a3);
/* internal */ u32 fn_8007A624(s32 *arg0, void* a1, void* a2);
/* internal */ void fn_8007A664(s32 *arg0);
/* internal */ u32 fn_8007A6A4(s32 *arg0, void* a1, void* a2, void* a3);
/* internal */ void fn_8007A6E4(s32 *arg0);
/* internal */ u32 fn_8007A724(s32 *arg0, void* a1, void* a2, void* a3);
/* internal */ u32 fn_8007A768(s32 *arg0, f32 farg0);
/* internal */ u32 light_obj_get_light_pos(s32 arg0, s32 arg1);
/* internal */ u32 light_obj_get_light_dir(s32 arg0, s32 arg1);
/* internal */ void fn_8007A800(s32 arg0, s32 arg1);
/* internal */ u32 fn_8007A814(s32 *arg0, s32 arg1);
/* internal */ void* light_setting_ctor(void *arg0, s32 arg1, s32 arg2, u32 arg3, s32 arg4, u32 arg5);
/* internal */ u32 fn_8007AEF8(void *arg0, void *arg1);
/* internal */ u32 fn_8007AF1C(s32 *arg0);
/* internal */ s32 dtor_8007AF28(s32 arg0, s16 arg1);
/* internal */ s32 fn_8007AF6C(void *arg0, void *arg1, u32 arg_sp0);
/* internal */ u32 fn_8007B074(void *arg0, void *arg1);
/* internal */ void fn_8007B0A0(void *arg0, s32 arg1, u32 arg2);
/* internal */ void* fn_8007B148(u32 *arg0, u32 *arg1);
/* internal */ s32 fn_8007B160(void *arg0, u32 arg1, s8 arg2);
/* internal */ s32 fn_8007B224(void *arg0, s8 arg1);
} /* extern "C" */

extern "C" {
void* fn_8007A518(s32 *arg0, s32 *arg1) {
    if (arg0 != arg1) {
        *arg0 = *arg1;
        nw4r::g3d::detail::Copy32ByteBlocks((arg0 + 4), (arg1 + 4), (u32)(0x40));
    }
    return arg0;
}

u32 fn_8007A564(s32 *arg0) {
    *arg0 = 0;
    nw4r::g3d::detail::ZeroMemory32ByteBlocks((arg0 + 4), (u32)(0x40));
}

u32 fn_8007A578(s32 arg0, s32 *arg1) {
    s32 sp8;

    sp8 = *arg1;
    GXInitLightColor((s32)(arg0 + 4), (void*)(&sp8));
}

u32 fn_8007A5A8(s32 *arg0, void* a1, void* a2, void* a3) {
    GXInitLightPos((s32)(arg0 + 4));
    *arg0 &= 0xFFFFFFFD;
}

u32 fn_8007A5E4(s32 *arg0, void* a1, void* a2, void* a3) {
    GXInitLightDir((s32)(arg0 + 4));
    *arg0 = (*arg0 & 0xFFFFFFFD) | 1;
}

u32 fn_8007A624(s32 *arg0, void* a1, void* a2) {
    GXInitLightSpot((s32)(arg0 + 4));
    *arg0 = (*arg0 & 0xFFFFFFFD) | 1;
}

void fn_8007A664(s32 *arg0) {
    GXInitLightAttnA((s32)(arg0 + 4));
    *arg0 = (*arg0 & 0xFFFFFFFD) | 1;
}

u32 fn_8007A6A4(s32 *arg0, void* a1, void* a2, void* a3) {
    GXInitLightDistAttn((s32)(arg0 + 4));
    *arg0 = (*arg0 & 0xFFFFFFFD) | 1;
}

void fn_8007A6E4(s32 *arg0) {
    GXInitLightAttnK((s32)(arg0 + 4));
    *arg0 = (*arg0 & 0xFFFFFFFD) | 1;
}

u32 fn_8007A724(s32 *arg0, void* a1, void* a2, void* a3) {
    GXInitLightDir((s32)(arg0 + 4));
    *arg0 = (*arg0 & 0xFFFFFFFE) | 2 | 8;
}

u32 fn_8007A768(s32 *arg0, f32 farg0) {
    f32 temp_f4;

    temp_f4 = farg0 * lbl_80795E38;
    GXInitLightAttn((s32)(arg0 + 4), (f32)(lbl_80795E30), (f32)(lbl_80795E30), (f32)(lbl_80795E34), (f32)(temp_f4), (f32)(lbl_80795E30), (f32)(lbl_80795E34 - temp_f4));
    *arg0 = (*arg0 & 0xFFFFFFFE) | 2;
}

u32 light_obj_get_light_pos(s32 arg0, s32 arg1) {
    if (arg1 != 0) {
        GXGetLightPos((s32)(arg0 + 4), (s32)(arg1 + 4), (s32)(arg1 + 8));
    }
}

u32 light_obj_get_light_dir(s32 arg0, s32 arg1) {
    if (arg1 != 0) {
        GXGetLightDir((s32)(arg0 + 4), (s32)(arg1 + 4), (s32)(arg1 + 8));
    }
}

void fn_8007A800(s32 arg0, s32 arg1) {
    if (arg1 != 0) {
        GXGetLightColor((s32)(arg0 + 4));
    }
}

u32 fn_8007A814(s32 *arg0, s32 arg1) {
}


typedef struct {
    /* +0x00 */ u16 field_0x00;
    /* +0x02 */ u16 field_0x02;
    /* +0x04 */ u8* field_0x04;
    /* +0x08 */ u8* field_0x08;
    /* +0x0C */ u8* field_0x0C;
} RawView_48; /* size: 0x10 */
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
    /* +0x0A */ u8 field_0x0A;
    /* +0x0B */ u8 field_0x0B;
} RawView_49; /* size: 0xC */
typedef struct {
    /* +0x00 */ u8 field_0x00;
    /* +0x01 */ u8 field_0x01;
    /* +0x02 */ u8 field_0x02;
    /* +0x03 */ u8 field_0x03;
    /* +0x04 */ u8 field_0x04;
    /* +0x05 */ u8 pad_0x05[0x3];
    /* +0x08 */ u32 field_0x08;
} RawView_50; /* size: 0xC */
void* light_setting_ctor(void *arg0, s32 arg1, s32 arg2, u32 arg3, s32 arg4, u32 arg5) {
}


typedef struct {
    /* +0x00 */ u8 field_0x00;
    /* +0x01 */ u8 field_0x01;
    /* +0x02 */ u8 field_0x02;
    /* +0x03 */ u8 field_0x03;
    /* +0x04 */ u8 field_0x04;
    /* +0x05 */ u8 field_0x05;
    /* +0x06 */ u8 field_0x06;
} RawView_51; /* size: 0x7 */
typedef struct {
    /* +0x00 */ u8 field_0x00;
    /* +0x01 */ u8 field_0x01;
    /* +0x02 */ u8 field_0x02;
    /* +0x03 */ u8 field_0x03;
} RawView_52; /* size: 0x4 */
u32 fn_8007AEF8(void *arg0, void *arg1) {
    ((RawView_51*)arg0)->field_0x00 = (u8) ((RawView_52*)arg1)->field_0x00;
    ((RawView_51*)arg0)->field_0x01 = (u8) ((RawView_52*)arg1)->field_0x01;
    ((RawView_51*)arg0)->field_0x02 = (u8) ((RawView_52*)arg1)->field_0x02;
    ((RawView_51*)arg0)->field_0x03 = (u8) ((RawView_52*)arg1)->field_0x03;
}

u32 fn_8007AF1C(s32 *arg0) {
    *arg0 = 0;
}

s32 dtor_8007AF28(s32 arg0, s16 arg1) {
}


typedef struct {
    /* +0x00 */ u16 field_0x00;
    /* +0x02 */ u16 field_0x02;
    /* +0x04 */ u8* field_0x04;
    /* +0x08 */ u8* field_0x08;
    /* +0x0C */ u8* field_0x0C;
} RawView_53; /* size: 0x10 */
typedef struct {
    /* +0x00 */ u16 field_0x00;
    /* +0x02 */ u16 field_0x02;
    /* +0x04 */ u8* field_0x04;
    /* +0x08 */ u8* field_0x08;
    /* +0x0C */ u8* field_0x0C;
} RawView_54; /* size: 0x10 */
s32 fn_8007AF6C(void *arg0, void *arg1, u32 arg_sp0) {
}


typedef struct {
    /* +0x00 */ u8 pad_0x00[0x9];
    /* +0x09 */ u8 field_0x09;
    /* +0x0A */ u8 field_0x0A;
    /* +0x0B */ u8 field_0x0B;
    /* +0x0C */ u8 field_0x0C;
    /* +0x0D */ u8 field_0x0D;
    /* +0x0E */ u8 field_0x0E;
} RawView_55; /* size: 0xF */
typedef struct {
    /* +0x00 */ u8 pad_0x00[0x9];
    /* +0x09 */ u16 field_0x09;
    /* +0x0B */ u8 field_0x0B;
} RawView_56; /* size: 0xC */
u32 fn_8007B074(void *arg0, void *arg1) {
}


typedef struct {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ u8* field_0x04;
} RawView_57; /* size: 0x8 */
void fn_8007B0A0(void *arg0, s32 arg1, u32 arg2) {
}


void* fn_8007B148(u32 *arg0, u32 *arg1) {
    if ((u32) *arg0 < (u32) *arg1) {
        return arg1;
    }
    return arg0;
}

typedef struct {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ u8* field_0x04;
} RawView_58; /* size: 0x8 */
s32 fn_8007B160(void *arg0, u32 arg1, s8 arg2) {
}


typedef struct {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ u8* field_0x04;
} RawView_59; /* size: 0x8 */
typedef struct {
    /* +0x00 */ u8 pad_0x00[0x8];
    /* +0x08 */ u32 field_0x08;
} RawView_60; /* size: 0xC */
s32 fn_8007B224(void *arg0, s8 arg1) {
}

} /* extern "C" */
