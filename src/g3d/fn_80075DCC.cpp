/*
 * g3d/fn_80075DCC.cpp - nw4r g3d render/dispatch cluster: the `g3d_dcc.cpp`, `g3d_draw1mat1shp.cpp`,
 *   `g3d_draw.cpp`, `g3d_fog.cpp` and `g3d_light.cpp` bodies of one linker run.
 * RANGE. .text 0x80075DCC-0x8007C540 (216 functions); extab, extabindex, .rodata 0x8056F658-0x8056F678, .data
 *   0x8058E570-0x8058EDA0, .sdata 0x807911E0-0x80791208, .sdata2 0x80795DF8-0x80795E60.  fn_80075E9C cites
 *   "g3d_dcc.cpp" and fn_8007A8E0 onward "g3d_light.cpp".  The left edge is a tudiscover strong cut after
 *   `g3d/g3d_camera.cpp`; the right edge is the discovery's byte cap, not a TU seam (`g3d/g3d_scnmdl.cpp` follows).
 * NAMES. fog_ctor is a GUESS (the evidence follows).
 *   res_shp_copy_ctor is a GUESS (the evidence follows).
 *   scn_mdl_set_mat_buffer_flag is a GUESS (it ORs a mask into the ScnMdl's +0x140 buffer-flag word; the
 *   material pass calls it after each applied animation).  The file keeps the map's stem (no one `__FILE__` names the run).  The bodies are m2c's output, typed
 *   mechanically: each `RawView_N` struct's `field_0xNN` states an offset and a size, not a meaning.
 *   GUESS (from the body and its callers): `sin_cos_deg`, `mtx34_set`; g3d_gd_set_chan_mat_color is a GUESS (XF 0x100C
 *   plus the channel); GDWriteXFCmd is a GUESS (the SDK's GD inline that
 *   writes one XF register: opcode 0x10, a zero count, the address, the value); `CopiedMatResources` is a GUESS (the 0x38-byte
 *   set of material handles fn_80077E70 refills and 0x80078DC0 constructs).  The material resource classes'
 *   constructors, assignments, ResMatTexCoordGen::IsValid, ResMatTevColor::IsValid, ResShp's ref/ptr/GetClassName/IsValid and `ResMat` getters are nw4r's `g3d_resmat_ac.h` members.
 *   The ScnObj, ScnLeaf and ScnGroup members (0x8007B424-0x8007BB8C: IsDerivedFrom, GetTypeObjStatic, the flag and
 *   callback helpers, ScnLeaf's constructor and destructor) are nw4r's `g3d_scnobj.h` members (`g3d/g3d_scnobj.h`);
 *   type_obj_set_name_scnleaf and type_obj_set_name_scngroup are GUESSES (the type-name store copies they call).
 *   g3d_draw_res_mdl_directly is a GUESS (0x800793A4: draws a model's opaque or translucent byte code over its view
 *   matrices; ScnMdlSimple's draw passes call it).
 * RESIDUALS. Unwritten (objdiff scores them zero): fn_80075DD8, fn_80077DBC, mtx34_set, fn_80079EE4, fn_8007A468.
 *   Unwritten (empty stub): g3d_draw_res_mdl_directly.
 *   Unwritten (empty stubs, 33 rows, 0x31F0 bytes; objdiff scores them near zero): fn_800769F4, fn_8007868C,
 *   fn_80078A9C, fn_80078E7C, fn_80079018, fn_800791D8, g3d_draw_res_mdl_directly, fn_80079604, fn_80079938, fn_800799BC,
 *   fn_80079A48, fn_80079B38, fn_8007A814, fn_8007A8E0, dtor_8007AF28, fn_8007AF6C, fn_8007B074, fn_8007B0A0,
 *   fn_8007B160, fn_8007B224, dtor_8007B2D4, fn_8007B3BC,
 *   fn_8007B764, fn_8007B794, dtor_8007B7F0, the `ScnMdl::CopiedMatAccess` constructor (0x8007BEAC),
 *   fn_8007C3CC, fn_8007C474.
 *   Partial (101 written bodies): every remaining function except the 77 at 100 %.
 *   flipcheck: `.text` 0x3B28 of 0x6774; `.rodata`, `.data` and `.sdata` are claimed and not emitted; `.sdata2` is
 *   0xC of 0x68, and the `.sdata`/`.sdata2` pools share a literal with `g3d/g3d_camera.cpp` (a candidate fold).
 *   Relocation names that differ from retail (pool constants, save helpers, statics): `lbl_8058E570`,
 *     `lbl_8058E57C`, `lbl_8058E5B0`, `lbl_8058E5D8`, `_savegpr_27`, `_restgpr_27`, `lbl_8058E758`,
 *     `lbl_8058E730`, `lbl_8058E790`, `lbl_8058E768`, `lbl_8058E870`, `lbl_8058E848`, `lbl_8058E838`,
 *     `lbl_8058E810`, `lbl_8058E6F0`, `lbl_8058E6D0`, `lbl_8058E6C0`, `lbl_8058E6A4`, `lbl_8058E800`,
 *     `lbl_8058E7E4`, `lbl_8058E720`, `lbl_8058E700`, `lbl_8058E7C8`, `lbl_8058E7A0`, `_savegpr_22`,
 *     `_savegpr_19`, `_restgpr_22`, `lbl_8058E880`, `lbl_8058E8B8`, `lbl_8058E890`, `_restgpr_19`,
 *     `__cvt_fp2unsigned`, `lbl_8058E8E8`, `lbl_8058E91C`, `lbl_8058EA90`, `lbl_8058EA64`, `lbl_8058EA50`,
 *     `lbl_8058EA30`, `lbl_8058EA20`, `lbl_8058E9F4`, `lbl_8058EAC8`, `lbl_8058EAA0`, `lbl_8058EAD8`,
 *     `lbl_80795E28`, `lbl_8058EAE8`, `lbl_8058EB08`, `fn_80502678`, `Enable__Q34nw4r2ut2LCFv`, `fn_805026D8`,
 *     `Disable__Q34nw4r2ut2LCFv`, `lbl_8061A9C0`, `lbl_8061AA74`, `lbl_8058ED90`, `lbl_8058ED64`, `lbl_8058ED38`.
 *   ScnLeaf's destructor is complete with an empty body (the compiler emits the base call and the deleting tail).
 * SHAPES. The CopiedMatResources constructor is complete: its work is the member initialisers.  The material
 *   resource constructors keep `#pragma peephole off` (retail keeps `clrlwi` + `cmpwi` for the alignment test).
 *   The unit compiles with `#pragma peephole off` (retail's unfused forms; playbook idea 106) except fn_800761BC, fn_800777B0, fn_8007A1B8, fn_8007A2A8, fn_8007A354 and scn_mdl_set_mat_buffer_flag,
 *   which measure better with the pass on.
 */

#include "types.h"


#include "sys_mem.h" /* operator delete (rule 9: call through the owner) */
#include "nw4r/g3d/scnmdl.h" /* nw4r::g3d::ScnMdl::CopiedMatAccess - the owner of the two mangled members (rule 1/9) */
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the matrix helpers (rule 2) */
#include "g3d/g3d_xsi.h" /* g3d_calc_tex_mtx_xsi (rule 2) */
#include "g3d/g3d_obj.h" /* nw4r::g3d::G3dObj (rule 2) */
#include "OS/MEMAllocFromAllocator.h" /* MEMFreeToAllocator (rule 2) */
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


/* Data objects the range references (unsplit, address-only). */
extern u32 lbl_8056F658;
extern const char lbl_8058E570[];
extern const char lbl_8058E57C[];
extern const char lbl_8058E5B0[];
extern const char lbl_8058E5D8[];
extern const char lbl_8058E608[];
extern const char lbl_8058E620[];
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
extern const char lbl_8058E880[];
extern u32 lbl_8058E890;
extern u32 lbl_8058E8B8;
extern const char lbl_8058E8E8[];
extern const char lbl_8058E91C[];
extern const char lbl_8058E950[];
extern const char lbl_8058E984[];
extern const char lbl_8058E9C8[];
extern const char lbl_8058E9F4[];
extern const char lbl_8058EA20[];
extern const char lbl_8058EA30[];
extern const char lbl_8058EA50[];
extern const char lbl_8058EA64[];
extern const char lbl_8058EA90[];
extern const char lbl_8058EAA0[];
extern const char lbl_8058EAC8[];
extern const char lbl_8058EAD8[];
extern const char lbl_8058EAE8[];
extern const char lbl_8058EB08[];
extern const char lbl_8058EB80[];
extern const char lbl_8058EB90[];
extern const char lbl_8058EBF8[];
extern const char lbl_8058EC30[];
extern const char lbl_8058EC70[];
extern const char lbl_8058ECB0[];
extern const char lbl_8058ECF8[];
extern u32 lbl_8058ED38;
extern const char lbl_8058ED64[];
extern const char lbl_8058ED90[];
extern u32 lbl_8061A9C0;
extern u32 lbl_8061AA74;
extern u32 lbl_807911E0;
extern u32 lbl_807911E4;
extern u32 lbl_807911E8;
extern u32 lbl_807911EC;
extern u32 lbl_807911F0;
extern u32 lbl_807911F8;
extern u32 lbl_807911FC;
extern u32 lbl_80791200;
extern f32 lbl_80795DFC;
extern f32 lbl_80795E00;
extern f32 lbl_80795E04;
extern f32 lbl_80795E08;
extern f32 lbl_80795E18;
extern f32 lbl_80795E1C;
extern f32 lbl_80795E20;
extern f32 lbl_80795E28;
extern f32 lbl_80795E30;
extern f32 lbl_80795E34;
extern f32 lbl_80795E38;
extern u32 lbl_80795E3C;
extern u32 lbl_80795E40;
extern f32 lbl_80795E44;
extern f32 lbl_80795E48;
extern f32 lbl_80795E4C;
extern f32 lbl_80795E50;
extern f32 lbl_80795E54;
extern f32 lbl_80795E58;

namespace nw4r { namespace db {
void Panic(const char* file, int line, const char* fmt, ...);
}}

extern "C" {
u32 C_MTXOrtho(f32, f32, f32, f32, f32, f32);
u32 GXInitLightAttn(s32, f32, f32, f32, f32, f32, f32);
u32 GXInitLightDir(s32);
u32 GXSetFog(s32, void*, f32, f32, f32, f32);
u32 GXSetFogRangeAdj(u8, u16, void*);
u32 GXSetIndTexMtx(s32, void*, s8);
u32 GXSetTevKColor(u32, void*);
u32 OSRegisterVersion(s32);
u32 PPCSync(void);
u32 VIGetTvFormat(void);
u32 color_rgba_copy(s32, void*);
u32 mtx34_identity(s32);
u32 vec3_normalize_into(void*, void*, f32, f32);
u32 mtx34_mult_vec3(void*, s32, void*);
u32 res_mat_chan_copy_ctor(void*, void*);
s32 res_node_is_valid(s32);
void* fn_8005CEEC(void);
s32 res_node_get_id(void*);
u32 fn_8005D0CC(void*, void*);
s32 fn_8005D218(void*);
u32 res_node_copy_ctor(void*, void*);
void* fn_80062DEC(s32);
s32 fn_8006405C(void*);
u32 fn_80064BD4(void*);
s32 fn_8006518C(void);
u32 fn_800651A4(void*);
s32 fn_800651E0(s32);
s32 fn_80065204(void*);
s32 fn_800659C4(s32);
void* fn_80067A54(s32);
u32 fn_8006E2A8(s32, s32);
u32 res_mat_tev_color_copy_ctor(void*, void*);
u32 res_mat_ind_mtx_copy_ctor(void*, void*);
u32 fn_8006FDCC(void*);
s32 fn_8006FEC8(void*, s32);
s32 fn_80070020(s32);
u32 mtx34_copy_ps(void*, s32);
u32 mtx34_concat(void*, void*, void*);
u32 g3d_lc_queue_wait(u32);
s32 res_mdl_get_info(void*);
u32 res_mdl_info_num_view_mtx(void*);
void* fn_80074A54(void);
s32 fn_80075844(s32);
u32 fn_8008715C(void*);
u32 fn_80087978(void*);
u32 fn_80087AD0(void*, s32, s32);
u32 fn_80088574(void*);
s32 fn_80094094(s32);
s32 fn_80099BB0(void*);
u32 fn_8009AB48(u32);
s32 fn_800D79B4(s32, s32, s32, s32);
f32 fn_80463EBC(f32 x, f32 y);
u32 fn_804B7800(s32);
u32 fn_804B7810(s32);
u32 fn_804B7820(s32);
u32 fn_804B79C0(s32);
u32 fn_804B7A90(s32);
u32 fn_804B7AA0(s32, s32, s32);
u32 fn_804B7AE0(s32, s32, s32);
u32 fn_804B7B10(void*, f32, f32, f32);
u32 fn_804B7C20(s32, void*);
u32 fn_804B7C30(s32);
u32 fn_804B9C60(void*, u16, s32);
u32 fn_804C6900(f32, f32, f32, f32, f32, f32);
u32 fn_804C69A0(f32, f32, f32, f32);
u32 math_sincos_idx(f32);
/* internal */ void sin_cos_deg(f32 farg0);
/* internal */ void fn_80075DD8(void* a0);
/* internal */ u32 fn_80075E98(void* a0);
/* internal */ void fn_80075E9C(s32 arg0, s32 arg1, s32 arg2, s32 arg3, s32 arg4, u32 arg_sp0);
/* internal */ void fn_80076050(s32 arg0, s32 arg1, s32 arg2, s32 arg3);
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
/* internal */ s32 fn_80077590(s32 arg0, void* a1);
/* internal */ u32 fn_800775C0(s32 *arg0, s32 *arg1);
/* internal */ void* fn_800775CC(s32 arg0);
/* internal */ s32 fn_80077630(s32 *arg0);
/* internal */ s32 fn_80077644(void* a0);
/* internal */ s32 fn_80077708(s32 arg0, void* a1);
/* internal */ u32 fn_80077738(s32 *arg0, s32 *arg1);
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
/* internal */ s32 fog_ctor(s32 arg0, void* a1);
/* internal */ u32 fn_80079FE4(s32 *arg0, s32 arg1);
/* internal */ void fn_80079FEC(s32 arg0);
/* internal */ s32 fn_8007A0B4(s32 arg0, s32 arg1);
/* internal */ s32 fn_8007A1B0(s32 *arg0);
/* internal */ void fn_8007A1B8(s32 arg0, s32 *arg1, f32 *arg2, f32 *arg3, f32 *arg4, f32 *arg5, s32 arg6, u32 arg_sp0);
/* internal */ void fn_8007A2A8(s32 arg0, u16 arg1, s16 arg2, s32 arg3);
/* internal */ void fn_8007A354(s32 arg0);
/* internal */ void fn_8007A400(s32 arg0);
/* internal */ u32 fn_8007A468(void* a0);
/* internal */ u32 fn_8007A46C(void* a0);
/* internal */ u32 fn_8007A494(void* a0);
/* internal */ u32 fn_8007A4B8(void* a0);
/* internal */ s32 OSInitFastCast(void* a0);
/* internal */ void fn_8007A510(void* a0);
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
/* internal */ u32 fn_8007A7C8(s32 arg0, s32 arg1);
/* internal */ u32 fn_8007A7E4(s32 arg0, s32 arg1);
/* internal */ void fn_8007A800(s32 arg0, s32 arg1);
/* internal */ u32 fn_8007A814(s32 *arg0, s32 arg1);
/* internal */ void* fn_8007A8E0(void *arg0, s32 arg1, s32 arg2, u16 arg3, s32 arg4, u16 arg5);
/* internal */ u32 fn_8007AEF8(void *arg0, void *arg1);
/* internal */ u32 fn_8007AF1C(s32 *arg0);
/* internal */ s32 dtor_8007AF28(s32 arg0, s16 arg1);
/* internal */ s32 fn_8007AF6C(void *arg0, void *arg1, u32 arg_sp0);
/* internal */ u32 fn_8007B074(void *arg0, void *arg1);
/* internal */ void fn_8007B0A0(void *arg0, s32 arg1, u32 arg2);
/* internal */ void* fn_8007B148(u32 *arg0, u32 *arg1);
/* internal */ s32 fn_8007B160(void *arg0, u32 arg1, s8 arg2);
/* internal */ s32 fn_8007B224(void *arg0, s8 arg1);
/* internal */ void fn_8007B4E4(void* a0);
/* internal */ void fn_8007B540(void* a0);
/* internal */ void fn_8007B5BC(void* a0);
/* internal */ const u8** type_obj_set_name_scngroup(const u8** out, const u8* v);
/* internal */ s32 fn_8007B794(s32 arg0, s16 arg1);
/* internal */ s32 dtor_8007B7F0(s32 arg0, s16 arg1);
/* internal */ s32 res_shp_copy_ctor(s32 arg0);
/* internal */ u32 fn_8007B864(s32 *arg0, s32 *arg1);
/* internal */ s32 fn_8007B878(s32 arg0, s32 arg1);
/* internal */ u32 fn_8007B8DC(s32 *arg0, s32 arg1);
/* internal */ void fn_8007B93C(void* a0);
/* internal */ void fn_8007B998(void* a0);
/* internal */ void fn_8007B99C(u32 **arg0);
/* internal */ void fn_8007B9AC(void *arg0);
/* internal */ s32 fn_8007BA00(void* a0);
/* internal */ s32 fn_8007BA08(s32 arg0);
/* internal */ u32 fn_8007BA38(s32 *arg0, s32 *arg1);
/* internal */ const u8** type_obj_set_name_scnleaf(const u8** out, const u8* v);
/* internal */ u32 scn_mdl_set_mat_buffer_flag(void *arg0, s32 arg1, s32 arg2);
/* internal */ s32 fn_8007BC2C(void *arg0, s32 arg1);
/* internal */ s32 fn_8007BCAC(void *arg0, s32 arg1);
/* internal */ s32 fn_8007BD2C(void *arg0, s32 arg1);
/* internal */ s32 fn_8007BDAC(void *arg0, s32 arg1);
/* internal */ s32 fn_8007BE2C(void *arg0, s32 arg1);
/* internal */ s32 fn_8007C3CC(void *arg0, s32 arg1);
/* internal */ u32 fn_8007C464(void *arg0);
/* internal */ void* fn_8007C474(void *arg0, void *arg1, s32 arg2);


void sin_cos_deg(f32 farg0) {
    math_sincos_idx((f32)(lbl_80795DFC * farg0));
}

typedef struct {
    /* +0x00 */ u8 pad_0x00[0x30];
    /* +0x30 */ s32 field_0x30;
    /* +0x34 */ u8 pad_0x34[0x3C];
    /* +0x70 */ u32 field_0x70;
    /* +0x74 */ u8 pad_0x74[0x38];
    /* +0xAC */ u32 field_0xAC;
    /* +0xB0 */ u32 field_0xB0;
    /* +0xB4 */ u32 field_0xB4;
    /* +0xB8 */ u32 field_0xB8;
    /* +0xBC */ u32 field_0xBC;
    /* +0xC0 */ u32 field_0xC0;
    /* +0xC4 */ u32 field_0xC4;
    /* +0xC8 */ u32 field_0xC8;
} RawView_1; /* size: 0xCC */
void fn_80075DD8(void* a0) {
    s32 temp_r4;
    void *temp_r3;

    temp_r3 = (void *)(fn_80074A54());
    temp_r4 = ((RawView_1*)temp_r3)->field_0x70;
    if ((s32) (temp_r4 & 0x40) != 0) {
        fn_80075E98((void*)(&((RawView_1*)temp_r3)->field_0x30));
        C_MTXOrtho((f32)(((RawView_1*)temp_r3)->field_0xBC), (f32)(((RawView_1*)temp_r3)->field_0xC0), (f32)(((RawView_1*)temp_r3)->field_0xC4), (f32)(((RawView_1*)temp_r3)->field_0xC8), (f32)(((RawView_1*)temp_r3)->field_0xB4), (f32)(((RawView_1*)temp_r3)->field_0xB8));
    } else if ((s32) (temp_r4 & 0x10) != 0) {
        fn_80075E98((void*)(&((RawView_1*)temp_r3)->field_0x30));
        fn_804C6900((f32)(((RawView_1*)temp_r3)->field_0xBC), (f32)(((RawView_1*)temp_r3)->field_0xC0), (f32)(((RawView_1*)temp_r3)->field_0xC4), (f32)(((RawView_1*)temp_r3)->field_0xC8), (f32)(((RawView_1*)temp_r3)->field_0xB4), (f32)(((RawView_1*)temp_r3)->field_0xB8));
    } else {
        fn_80075E98((void*)(&((RawView_1*)temp_r3)->field_0x30));
        fn_804C69A0((f32)(((RawView_1*)temp_r3)->field_0xAC), (f32)(((RawView_1*)temp_r3)->field_0xB0), (f32)(((RawView_1*)temp_r3)->field_0xB4), (f32)(((RawView_1*)temp_r3)->field_0xB8));
    }
    ((RawView_1*)temp_r3)->field_0x70 = (s32) (((RawView_1*)temp_r3)->field_0x70 | 0x80);
}

u32 fn_80075E98(void* a0) {

}

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

void fn_80076050(s32 arg0, s32 arg1, s32 arg2, s32 arg3) {
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
    return fn_80463EBC(x, y);
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

s32 fn_80077590(s32 arg0, void* a1) {
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
    fn_80077590((s32)(&sp34), (void*)(&sp24));
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
                        temp_r3_3 = fn_8006FEC8((void*)(&sp1C), (s32)(reinterpret_cast<nw4r::g3d::ResShp*>(arg2)->ptr()->curMtxIdx));
                        if (temp_r3_3 < 0) {
                            nw4r::db::Panic((const char*)&lbl_8058E880, 0x64, (const char*)&lbl_8058E890);
                        }
                        sp18 = (s32)reinterpret_cast<nw4r::g3d::ResMdl*>(&sp2C)->GetResNode((int)temp_r3_3).mpData;
                        res_node_copy_ctor((void*)(&sp28), (void*)(&sp18));
                        temp_r20_2 = fn_8005D218((void*)(arg1));
                        mtx34_concat((void*)(&spC8), (void*)(fn_8005D218((void*)(&sp28)) + 0xA0), (void*)(u32)(temp_r20_2 + 0x70));
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
                    fn_8007A7E4((s32)(temp_r3_5), (s32)(&sp5C));
                    if ((temp_f31 == sp5C) && (temp_f31 == sp60) && (temp_f31 == sp64)) {
                        fn_8007A7C8((s32)(temp_r3_5), (s32)(&sp5C));
                        fn_80077DBC((void *)(&sp44), (void*)(&sp5C));
                        copyVec3((nw4r::math::VEC3*)&sp5C, (const nw4r::math::VEC3*)&sp44);
                        vec3_normalize_into((void*)(&sp5C), (void*)(&sp5C), 0, 0);
                    }
                } else if (fn_80077D64((s32)(temp_r3_5)) != 0) {
                    fn_8007A7C8((s32)(temp_r3_5), (s32)(&sp5C));
                    fn_80077DBC((void *)(&sp38), (void*)(&sp5C));
                    copyVec3((nw4r::math::VEC3*)&sp5C, (const nw4r::math::VEC3*)&sp38);
                    vec3_normalize_into((void*)(&sp5C), (void*)(&sp5C), 0, 0);
                } else {
                    VEC3_ctor((nw4r::math::VEC3*)&sp50);
                    if (fn_80077D4C((s32 *)(temp_r3_5)) == 0) {
                        nw4r::db::Panic((const char*)&lbl_8058E880, 0xAB, (const char*)&lbl_8058E8B8);
                    }
                    fn_8007A7E4((s32)(temp_r3_5), (s32)(&sp50));
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
        return (((RawView_25*)fn_80062DEC((s32)(arg0)))->field_0x14 & 0x100) != 0;
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
void fn_80079FEC(s32 arg0) {
    void *temp_r3;

    if (fn_800659C4(0) == 0) {
        nw4r::db::Panic((const char*)&lbl_8058EAD8, 0x20, (const char*)&lbl_8058EAE8);
    }
    if (fn_800659C4((s32)(arg0)) != 0) {
        temp_r3 = (void *)(fn_80067A54((s32)(arg0)));
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
s32 fn_8007A0B4(s32 arg0, s32 arg1) {
    u32 spC;
    u32 sp8;
    void *temp_r3;

    if ((s32) (arg1 & 3) != 0) {
        nw4r::db::Panic((const char*)&lbl_8058EAD8, 0x3D, (const char*)&lbl_8058EB08);
    }
    if (fn_800659C4((s32)(arg0)) == 0) {
        nw4r::db::Panic((const char*)&lbl_8058EAD8, 0x3E, (const char*)&lbl_8058EAE8);
    }
    if ((arg1 != 0) && (fn_800659C4((s32)(arg0)) != 0)) {
        temp_r3 = (void *)(fn_8007A1B0((s32 *)(arg0)));
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

s32 fn_8007A1B0(s32 *arg0) {
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

    if (fn_800659C4(0) == 0) {
        nw4r::db::Panic((const char*)&lbl_8058EAD8, 0x68, (const char*)&lbl_8058EAE8);
    }
    if (fn_800659C4((s32)(arg0)) != 0) {
        temp_r3 = (void *)(fn_80067A54((s32)(arg0)));
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
void fn_8007A2A8(s32 arg0, u16 arg1, s16 arg2, s32 arg3) {
    void *temp_r3;

    if (fn_800659C4(0) == 0) {
        nw4r::db::Panic((const char*)&lbl_8058EAD8, 0x81, (const char*)&lbl_8058EAE8);
    }
    if (fn_800659C4((s32)(arg0)) != 0) {
        temp_r3 = (void *)(fn_80067A54((s32)(arg0)));
        ((RawView_46*)temp_r3)->field_0x1A = arg2;
        fn_804B9C60((void*)(&((RawView_46*)temp_r3)->field_0x1C), (u16)(arg1), (s32)(fn_80075844((s32)(arg3))));
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
void fn_8007A354(s32 arg0) {
    s32 sp8;
    void *temp_r3;

    if (fn_800659C4(0) == 0) {
        nw4r::db::Panic((const char*)&lbl_8058EAD8, 0x8F, (const char*)&lbl_8058EAE8);
    }
    if (fn_800659C4((s32)(arg0)) != 0) {
        temp_r3 = (void *)(fn_8007A1B0((s32 *)(arg0)));
        if ((s32) ((RawView_47*)temp_r3)->field_0x00 != 0) {
            GXSetFogRangeAdj((u8)(((RawView_47*)temp_r3)->field_0x18), (u16)(((RawView_47*)temp_r3)->field_0x1A), (void*)(&((RawView_47*)temp_r3)->field_0x1C));
        }
        sp8 = ((RawView_47*)temp_r3)->field_0x14;
        GXSetFog((s32)(((RawView_47*)temp_r3)->field_0x00), (void*)(&sp8), (f32)(((RawView_47*)temp_r3)->field_0x04), (f32)(((RawView_47*)temp_r3)->field_0x08), (f32)(((RawView_47*)temp_r3)->field_0x0C), (f32)(((RawView_47*)temp_r3)->field_0x10));
    }
}
#pragma peephole off

void fn_8007A400(s32 arg0) {
    u32 *var_r3;

    OSRegisterVersion((s32)(lbl_80791200));
    if (arg0 != 0) {
        nw4r::ut::LC::Enable();
    } else {
        nw4r::ut::LC::Disable();
    }
    fn_8007A468(0);
    var_r3 = (u32 *)(&lbl_8061A9C0);
    if (VIGetTvFormat() == 1U) {
        var_r3 = (u32 *)(&lbl_8061AA74);
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
    fn_804B7C20((s32)(arg0 + 4), (void*)(&sp8));
}

u32 fn_8007A5A8(s32 *arg0, void* a1, void* a2, void* a3) {
    fn_804B7A90((s32)(arg0 + 4));
    *arg0 &= 0xFFFFFFFD;
}

u32 fn_8007A5E4(s32 *arg0, void* a1, void* a2, void* a3) {
    GXInitLightDir((s32)(arg0 + 4));
    *arg0 = (*arg0 & 0xFFFFFFFD) | 1;
}

u32 fn_8007A624(s32 *arg0, void* a1, void* a2) {
    fn_804B7820((s32)(arg0 + 4));
    *arg0 = (*arg0 & 0xFFFFFFFD) | 1;
}

void fn_8007A664(s32 *arg0) {
    fn_804B7800((s32)(arg0 + 4));
    *arg0 = (*arg0 & 0xFFFFFFFD) | 1;
}

u32 fn_8007A6A4(s32 *arg0, void* a1, void* a2, void* a3) {
    fn_804B79C0((s32)(arg0 + 4));
    *arg0 = (*arg0 & 0xFFFFFFFD) | 1;
}

void fn_8007A6E4(s32 *arg0) {
    fn_804B7810((s32)(arg0 + 4));
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

u32 fn_8007A7C8(s32 arg0, s32 arg1) {
    if (arg1 != 0) {
        fn_804B7AA0((s32)(arg0 + 4), (s32)(arg1 + 4), (s32)(arg1 + 8));
    }
}

u32 fn_8007A7E4(s32 arg0, s32 arg1) {
    if (arg1 != 0) {
        fn_804B7AE0((s32)(arg0 + 4), (s32)(arg1 + 4), (s32)(arg1 + 8));
    }
}

void fn_8007A800(s32 arg0, s32 arg1) {
    if (arg1 != 0) {
        fn_804B7C30((s32)(arg0 + 4));
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
void* fn_8007A8E0(void *arg0, s32 arg1, s32 arg2, u16 arg3, s32 arg4, u16 arg5) {
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


typedef struct {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ u8 pad_0x04[0x4];
    /* +0x08 */ u32 field_0x08;
} RawView_61; /* size: 0xC */
} /* extern "C" */

/* 0x8007B2D4 (0x6C): returns the object's memory to the heap it came from. */
nw4r::g3d::G3dObj::~G3dObj() {
    Dealloc(mpHeap, this);
}

/* 0x8007B340 (0x8): returns a block to the allocator. */
void nw4r::g3d::G3dObj::Dealloc(MEMAllocator* pHeap, void* pBlock) { /* untyped: byte range */
    MEMFreeToAllocator(pHeap, pBlock);
}

extern "C" {

typedef struct {
    /* +0x00 */ u8 pad_0x00[0x10];
    /* +0x10 */ u32 (*field_0x10)(...);
} RawView_63; /* size: 0x14 */
typedef struct {
    /* +0x00 */ u8 pad_0x00[0xC];
    /* +0x0C */ u32 (*field_0x0C)(...);
} RawView_62; /* size: 0x10 */
} /* extern "C" */

/* 0x8007B348 (0x74): tells the parent the object is going away, then destroys it. */
void nw4r::g3d::G3dObj::Destroy() {
    G3dObj* pParent = GetParent();
    if (pParent != NULL) {
        pParent->G3dProc(G3DPROC_CHILD_DETACHED, 0, this);
    }
    delete this;
}

extern "C" {

} /* extern "C" */

/* The G3dObj type-name record (this unit's `.rodata`). */
extern "C" u8 anm_typename_G3dObj[];

/* 0x8007B3BC (0x30): returns the type. */
const nw4r::g3d::G3dObj::TypeObj nw4r::g3d::G3dObj::GetTypeObj() const {
    const u8* pName;
    return *reinterpret_cast<const TypeObj*>(type_obj_set_name(&pName, anm_typename_G3dObj));
}

extern "C" {


typedef struct {
    /* +0x00 */ u8 pad_0x00[0x14];
    /* +0x14 */ u32 (*field_0x14)(...);
} RawView_64; /* size: 0x18 */
} /* extern "C" */

/* 0x8007B3EC (0x38): returns the type's name. */
const char* nw4r::g3d::G3dObj::GetTypeName() const {
    return GetTypeObj().GetTypeName();
}

extern "C" {

typedef struct {
    /* +0x00 */ u8 pad_0x00[0xE4];
    /* +0xE4 */ u32 field_0xE4;
} RawView_65; /* size: 0xE8 */
}   /* extern "C": the scene object members below have C++ linkage */

/* 0x8007B424 (0x8): the child count. */
u32 nw4r::g3d::ScnGroup::Size() const
{
    return mNumScnObj;
}

/* 0x8007B42C/0x8007B450/0x8007B47C: the std::find and std::distance instances ScnGroup::Remove(ScnObj*) calls. */
template nw4r::g3d::ScnObj** std::find(nw4r::g3d::ScnObj** first, nw4r::g3d::ScnObj** last,
                                       nw4r::g3d::ScnObj* const& value);
template long std::distance(nw4r::g3d::ScnObj** first, nw4r::g3d::ScnObj** last);
template long std::__distance(nw4r::g3d::ScnObj** first, nw4r::g3d::ScnObj** last, std::random_access_iterator_tag);

extern "C" {


void fn_8007B4E4(void* a0) {

}

void fn_8007B540(void* a0) {

}

}   /* extern "C": the scene object member below has C++ linkage */

/* 0x8007B544 (0x20): the matrix of `type`, or NULL for an unknown type. */
nw4r::math::MTX34* nw4r::g3d::ScnObj::GetMtxPtr(ScnObjMtxType type)
{
    if ((u32)type < MTX_TYPE_MAX) {
        return &mMtxArray[type];
    }
    return NULL;
}

extern "C" {

void fn_8007B5BC(void* a0) {

}

typedef struct {
    /* +0x00 */ u8 pad_0x00[0xCC];
    /* +0xCC */ u32 field_0xCC;
} RawView_66; /* size: 0xD0 */
}   /* extern "C": the scene object members below have C++ linkage */

/* 0x8007B5C0 (0x34): whether the object's flags disable G3dProc pass `task`. */
bool nw4r::g3d::ScnObj::IsG3dProcDisabled(u32 task) const
{
    if (task < 9 && ((1 << (task - 1)) & mScnObjFlags)) {
        return true;
    }
    return false;
}

extern "C" {


}   /* extern "C": the scene object members below have C++ linkage */

/* 0x8007B5F4 (0x6C): whether the object is a ScnGroup or derives from `type`. */
bool nw4r::g3d::ScnGroup::IsDerivedFrom(TypeObj type) const
{
    if (type == GetTypeObjStatic()) {
        return true;
    }
    return ScnObj::IsDerivedFrom(type);
}

extern "C" {


}   /* extern "C": the scene object members below have C++ linkage */

/* 0x8007B660 (0x6C): whether the object is a ScnObj or derives from `type`. */
bool nw4r::g3d::ScnObj::IsDerivedFrom(TypeObj type) const
{
    if (type == GetTypeObjStatic()) {
        return true;
    }
    return G3dObj::IsDerivedFrom(type);
}

extern "C" {


}   /* extern "C": the scene object members below have C++ linkage */

/* 0x8007B6CC (0x30): returns the ScnObj type. */
const nw4r::g3d::G3dObj::TypeObj nw4r::g3d::ScnObj::GetTypeObjStatic()
{
    const u8* pName;
    return *reinterpret_cast<const TypeObj*>(type_obj_set_name(&pName, scn_typename_ScnObj));
}

extern "C" {


}   /* extern "C": the scene object members below have C++ linkage */

/* 0x8007B6FC (0x30): returns the ScnGroup type. */
const nw4r::g3d::G3dObj::TypeObj nw4r::g3d::ScnGroup::GetTypeObjStatic()
{
    const u8* pName;
    return *reinterpret_cast<const TypeObj*>(type_obj_set_name_scngroup(&pName, scn_typename_ScnGroup));
}

extern "C" {


/* 0x8007B72C (0x8): stores `v` through `out` and returns `out`. */
const u8** type_obj_set_name_scngroup(const u8** out, const u8* v)
{
    *out = v;
    return out;
}

}   /* extern "C": the ScnMdlSimple member below has C++ linkage */

/* 0x8007B734 (0x30): returns the ScnMdlSimple type. */
const nw4r::g3d::G3dObj::TypeObj nw4r::g3d::ScnMdlSimple::GetTypeObjStatic()
{
    void* pName;
    return *reinterpret_cast<const TypeObj*>(type_obj_set_name_anmchr(&pName, scn_typename_ScnMdlSimple));
}

extern "C" {


}   /* extern "C": the ScnMdl member below has C++ linkage */

/* 0x8007B764 (0x30): returns the ScnMdl type. */
const nw4r::g3d::G3dObj::TypeObj nw4r::g3d::ScnMdl::GetTypeObjStatic()
{
    void* pName;
    return *reinterpret_cast<const TypeObj*>(type_obj_set_name((const u8**)&pName, scn_typename_ScnMdl));
}

extern "C" {


s32 fn_8007B794(s32 arg0, s16 arg1) {
}


s32 dtor_8007B7F0(s32 arg0, s16 arg1) {
}


s32 res_shp_copy_ctor(s32 arg0) {
    fn_8007B864(0, 0);
    return arg0;
}

u32 fn_8007B864(s32 *arg0, s32 *arg1) {
    *arg0 = *arg1;
}

}   /* extern "C": the placement `operator new` below has C++ linkage (`__nw__FUlPv`) */

/* The placement `operator new`: the caller's address back, unchanged. */
/* untyped: opaque handle passed through - the placement address the caller hands in */
void* operator new(unsigned long size, void* place) {
    return place;
}

extern "C" {

s32 fn_8007B878(s32 arg0, s32 arg1) {
    fn_8007B8DC(0, 0);
    if ((s32) (arg1 & 0x1F) != 0) {
        nw4r::db::Panic((const char*)&lbl_8058ED90, 0x78, (const char*)&lbl_8058ED64);
    }
    return arg0;
}

u32 fn_8007B8DC(s32 *arg0, s32 arg1) {
    *arg0 = arg1;
}

void fn_8007B93C(void* a0) {

}

void fn_8007B998(void* a0) {

}

void fn_8007B99C(u32 **arg0) {
    *arg0 = &lbl_8058ED38;
}

typedef struct {
    /* +0x00 */ u8 pad_0x00[0x10];
    /* +0x10 */ u32 field_0x10;
    /* +0x14 */ u32 field_0x14;
} RawView_67; /* size: 0x18 */
void fn_8007B9AC(void *arg0) {
    ((RawView_67*)arg0)->field_0x14 = 0;
    ((RawView_67*)arg0)->field_0x10 = 0;
}

typedef struct {
    /* +0x00 */ u8 pad_0x00[0xCC];
    /* +0xCC */ u32 field_0xCC;
} RawView_68; /* size: 0xD0 */
}   /* extern "C": the scene object members below have C++ linkage */

/* 0x8007B9BC (0x18): whether any of `flag`'s bits is set. */
u32 nw4r::g3d::ScnObj::TestScnObjFlag(ScnObjFlag flag) const
{
    return (mScnObjFlags & flag) != 0;
}

extern "C" {


typedef struct {
    /* +0x00 */ u8 pad_0x00[0xCC];
    /* +0xCC */ u32 field_0xCC;
} RawView_69; /* size: 0xD0 */
}   /* extern "C": the scene object members below have C++ linkage */

/* 0x8007B9D4 (0x2C): sets or clears `flag`'s bits. */
void nw4r::g3d::ScnObj::SetScnObjFlag(ScnObjFlag flag, u32 on)
{
    if (on) {
        mScnObjFlags |= flag;
    } else {
        mScnObjFlags &= ~flag;
    }
}

extern "C" {


s32 fn_8007BA00(void* a0) {
    return 0;
}

s32 fn_8007BA08(s32 arg0) {
    fn_8007BA38(0, 0);
    return arg0;
}

u32 fn_8007BA38(s32 *arg0, s32 *arg1) {
    *arg0 = *arg1;
}

}   /* extern "C": the scene object members below have C++ linkage */

/* 0x8007BA44 (0x5C): destroys the leaf. */
nw4r::g3d::ScnLeaf::~ScnLeaf()
{
}

extern "C" {


}   /* extern "C": the scene object members below have C++ linkage */

/* 0x8007BAA0 (0x50): constructs the leaf with a unit scale. */
nw4r::g3d::ScnLeaf::ScnLeaf(MEMAllocator* pHeap) : ScnObj(pHeap)
{
    setVec3(&mScale, lbl_80795E58, lbl_80795E58, lbl_80795E58);
}

extern "C" {


}   /* extern "C": the scene object members below have C++ linkage */

/* 0x8007BAF0 (0x6C): whether the object is a ScnLeaf or derives from `type`. */
bool nw4r::g3d::ScnLeaf::IsDerivedFrom(TypeObj type) const
{
    if (type == GetTypeObjStatic()) {
        return true;
    }
    return ScnObj::IsDerivedFrom(type);
}

extern "C" {


}   /* extern "C": the scene object members below have C++ linkage */

/* 0x8007BB5C (0x30): returns the ScnLeaf type. */
const nw4r::g3d::G3dObj::TypeObj nw4r::g3d::ScnLeaf::GetTypeObjStatic()
{
    const u8* pName;
    return *reinterpret_cast<const TypeObj*>(type_obj_set_name_scnleaf(&pName, scn_typename_ScnLeaf));
}

extern "C" {


/* 0x8007BB8C (0x8): stores `v` through `out` and returns `out`. */
const u8** type_obj_set_name_scnleaf(const u8** out, const u8* v)
{
    *out = v;
    return out;
}

typedef struct {
    /* +0x00 */ u8 pad_0x00[0x140];
    /* +0x140 */ u32 field_0x140;
} RawView_70; /* size: 0x144 */
#pragma peephole on
u32 scn_mdl_set_mat_buffer_flag(void *arg0, s32 arg1, s32 arg2) {
    s32 temp_r3;
    s32 * temp_r6;

    temp_r6 = (s32 *)(((RawView_70*)arg0)->field_0x140);
    temp_r3 = arg1 * 4;
    *(temp_r6 + temp_r3) = *(temp_r6 + temp_r3) | arg2;
}
#pragma peephole off

typedef struct {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ u32 field_0x04;
    /* +0x08 */ u8 pad_0x08[0x8];
    /* +0x10 */ u32 field_0x10;
} RawView_71; /* size: 0x14 */
u32 nw4r::g3d::ScnMdl::CopiedMatAccess::GetResTexSrt(bool arg1) {
    u32 sp8;
    void* arg0 = this;

    if (((s32) ((RawView_71*)arg0)->field_0x00 != 0) && (reinterpret_cast<const nw4r::g3d::ResTexSrt*>((void*)(&((RawView_71*)arg0)->field_0x10))->IsValid() != 0)) {
        if (arg1 != 0) {
            scn_mdl_set_mat_buffer_flag((void *)(((RawView_71*)arg0)->field_0x00), (s32)(((RawView_71*)arg0)->field_0x04), (s32)(4));
        }
        return ((RawView_71*)arg0)->field_0x10;
    }
    return (s32)nw4r::g3d::ResTexSrt((void*)NULL).mpData;
}

/* 0x8007BEAC - the ScnMdl::CopiedMatAccess constructor. */
nw4r::g3d::ScnMdl::CopiedMatAccess::CopiedMatAccess(ScnMdl* pMdl, u32 idx)
{
    (void)pMdl;
    (void)idx;
}


typedef struct {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ u32 field_0x04;
    /* +0x08 */ u8 pad_0x08[0xC];
    /* +0x14 */ u32 field_0x14;
} RawView_72; /* size: 0x18 */
s32 fn_8007BC2C(void *arg0, s32 arg1) {
    u32 sp8;

    if (((s32) ((RawView_72*)arg0)->field_0x00 != 0) && (reinterpret_cast<const nw4r::g3d::ResMatChan*>((void*)(&((RawView_72*)arg0)->field_0x14))->IsValid() != 0)) {
        if (arg1 != 0) {
            scn_mdl_set_mat_buffer_flag((void *)(((RawView_72*)arg0)->field_0x00), (s32)(((RawView_72*)arg0)->field_0x04), (s32)(8));
        }
        return ((RawView_72*)arg0)->field_0x14;
    }
    return (s32)nw4r::g3d::ResMatChan((void*)NULL).mpData;
}

typedef struct {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ u32 field_0x04;
    /* +0x08 */ u8 pad_0x08[0x10];
    /* +0x18 */ u32 field_0x18;
} RawView_73; /* size: 0x1C */
s32 fn_8007BCAC(void *arg0, s32 arg1) {
    u32 sp8;

    if (((s32) ((RawView_73*)arg0)->field_0x00 != 0) && (reinterpret_cast<const nw4r::g3d::ResGenMode*>((s32 *)(&((RawView_73*)arg0)->field_0x18))->IsValid() != 0)) {
        if (arg1 != 0) {
            scn_mdl_set_mat_buffer_flag((void *)(((RawView_73*)arg0)->field_0x00), (s32)(((RawView_73*)arg0)->field_0x04), (s32)(0x10));
        }
        return ((RawView_73*)arg0)->field_0x18;
    }
    return (s32)nw4r::g3d::ResGenMode((void*)NULL).mpData;
}

typedef struct {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ u32 field_0x04;
    /* +0x08 */ u8 pad_0x08[0x14];
    /* +0x1C */ u32 field_0x1C;
} RawView_74; /* size: 0x20 */
s32 fn_8007BD2C(void *arg0, s32 arg1) {
    u32 sp8;

    if (((s32) ((RawView_74*)arg0)->field_0x00 != 0) && (reinterpret_cast<const nw4r::g3d::ResMatMisc*>((s32 *)(&((RawView_74*)arg0)->field_0x1C))->IsValid() != 0)) {
        if (arg1 != 0) {
            scn_mdl_set_mat_buffer_flag((void *)(((RawView_74*)arg0)->field_0x00), (s32)(((RawView_74*)arg0)->field_0x04), (s32)(0x20));
        }
        return ((RawView_74*)arg0)->field_0x1C;
    }
    return (s32)nw4r::g3d::ResMatMisc((void*)NULL).mpData;
}

typedef struct {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ u32 field_0x04;
    /* +0x08 */ u8 pad_0x08[0x18];
    /* +0x20 */ u32 field_0x20;
} RawView_75; /* size: 0x24 */
s32 fn_8007BDAC(void *arg0, s32 arg1) {
    u32 sp8;

    if (((s32) ((RawView_75*)arg0)->field_0x00 != 0) && (reinterpret_cast<const nw4r::g3d::ResMatPix*>((s32 *)(&((RawView_75*)arg0)->field_0x20))->IsValid() != 0)) {
        if (arg1 != 0) {
            scn_mdl_set_mat_buffer_flag((void *)(((RawView_75*)arg0)->field_0x00), (s32)(((RawView_75*)arg0)->field_0x04), (s32)(0x80));
        }
        return ((RawView_75*)arg0)->field_0x20;
    }
    return (s32)nw4r::g3d::ResMatPix((void*)NULL).mpData;
}

typedef struct {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ u32 field_0x04;
    /* +0x08 */ u8 pad_0x08[0x1C];
    /* +0x24 */ u32 field_0x24;
} RawView_76; /* size: 0x28 */
s32 fn_8007BE2C(void *arg0, s32 arg1) {
    u32 sp8;

    if (((s32) ((RawView_76*)arg0)->field_0x00 != 0) && (reinterpret_cast<const nw4r::g3d::ResMatTevColor*>((void*)(&((RawView_76*)arg0)->field_0x24))->IsValid() != 0)) {
        if (arg1 != 0) {
            scn_mdl_set_mat_buffer_flag((void *)(((RawView_76*)arg0)->field_0x00), (s32)(((RawView_76*)arg0)->field_0x04), (s32)(0x100));
        }
        return ((RawView_76*)arg0)->field_0x24;
    }
    return (s32)nw4r::g3d::ResMatTevColor((void*)NULL).mpData;
}

typedef struct {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ u8 pad_0x04[0x4];
    /* +0x08 */ u32 field_0x08;
} RawView_77; /* size: 0xC */
s32 fn_8007C3CC(void *arg0, s32 arg1) {
}


typedef struct {
    /* +0x00 */ u8 pad_0x00[0x13C];
    /* +0x13C */ u32 field_0x13C;
} RawView_78; /* size: 0x140 */
u32 fn_8007C464(void *arg0) {
    ((RawView_78*)arg0)->field_0x13C = (s32) (((RawView_78*)arg0)->field_0x13C | 1);
}

typedef struct {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ u32 field_0x04;
    /* +0x08 */ u32 field_0x08;
} RawView_79; /* size: 0xC */
typedef struct {
    /* +0x00 */ u8 pad_0x00[0x148];
    /* +0x148 */ u32 field_0x148;
} RawView_80; /* size: 0x14C */
void* fn_8007C474(void *arg0, void *arg1, s32 arg2) {
}


}
extern "C" {

/* --------------------------------------------------------------------------------------------------
 * The five functions m2c could not decompile: indirect-dispatch trampolines (a `bctr` through a vtable
 * slot, which m2c reports as an unresolved jump table).  Hand-reconstructed from the target
 * disassembly; they are recorded as residuals.
 * -------------------------------------------------------------------------------------------------- */

}   /* extern "C": the scene object members below have C++ linkage */

/* 0x8007B48C (0x58): runs the callback's CALC_VIEW hook at `timing` when it is enabled for the pass and the timing. */
/* untyped: caller-owned payload - the pass's info block */
void nw4r::g3d::ScnObj::CheckCallback_CALC_VIEW(Timing timing, u32 param, void* pInfo)
{
    if (mpFnCallback != NULL && (mCallbackExecOpMask & EXECOP_CALC_VIEW) && (mCallbackTiming & timing)) {
        mpFnCallback->ExecCallback_CALC_VIEW(timing, this, param, pInfo);
    }
}

extern "C" {

}   /* extern "C": the scene object members below have C++ linkage */

/* 0x8007B4E8 (0x58): runs the callback's CALC_MAT hook at `timing` when it is enabled for the pass and the timing. */
/* untyped: caller-owned payload - the pass's info block */
void nw4r::g3d::ScnObj::CheckCallback_CALC_MAT(Timing timing, u32 param, void* pInfo)
{
    if (mpFnCallback != NULL && (mCallbackExecOpMask & EXECOP_CALC_MAT) && (mCallbackTiming & timing)) {
        mpFnCallback->ExecCallback_CALC_MAT(timing, this, param, pInfo);
    }
}

extern "C" {

}   /* extern "C": the scene object members below have C++ linkage */

/* 0x8007B564 (0x58): runs the callback's CALC_WORLD hook at `timing` when it is enabled for the pass and the timing. */
/* untyped: caller-owned payload - the pass's info block */
void nw4r::g3d::ScnObj::CheckCallback_CALC_WORLD(Timing timing, u32 param, void* pInfo)
{
    if (mpFnCallback != NULL && (mCallbackExecOpMask & EXECOP_CALC_WORLD) && (mCallbackTiming & timing)) {
        mpFnCallback->ExecCallback_CALC_WORLD(timing, this, param, pInfo);
    }
}

extern "C" {

}   /* extern "C": the scene object members below have C++ linkage */

/* 0x8007B8E4 (0x58): runs the callback's DRAW_XLU hook at `timing` when it is enabled for the pass and the timing. */
/* untyped: caller-owned payload - the pass's info block */
void nw4r::g3d::ScnObj::CheckCallback_DRAW_XLU(Timing timing, u32 param, void* pInfo)
{
    if (mpFnCallback != NULL && (mCallbackExecOpMask & EXECOP_DRAW_XLU) && (mCallbackTiming & timing)) {
        mpFnCallback->ExecCallback_DRAW_XLU(timing, this, param, pInfo);
    }
}

/* 0x8007B940 (0x58): runs the callback's DRAW_OPA hook at `timing` when it is enabled for the pass and the timing. */
/* untyped: caller-owned payload - the pass's info block */
void nw4r::g3d::ScnObj::CheckCallback_DRAW_OPA(Timing timing, u32 param, void* pInfo)
{
    if (mpFnCallback != NULL && (mCallbackExecOpMask & EXECOP_DRAW_OPA) && (mCallbackTiming & timing)) {
        mpFnCallback->ExecCallback_DRAW_OPA(timing, this, param, pInfo);
    }
}

extern "C" {
}
