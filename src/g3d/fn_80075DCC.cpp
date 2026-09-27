/*
 * nw4r g3d render/dispatch cluster - `.text` 0x80075DCC-0x8007C540 (216 functions, 26484 B).
 *
 * Registered once, at its final home (docs/plan.md 12), from the pooled proposal `80075DCC`.
 *
 * Name (evidence class 4, "nothing supports a single name"): the run is a maximal unclaimed run that
 * spans more than one original translation unit, so no one `__FILE__` string names it.  The discovery's
 * own attribution probe (`tools/units/attribution-queue.json`) reports
 * `sources: [g3d_dcc.cpp, g3d_draw.cpp, g3d_draw1mat1shp.cpp, g3d_fog.cpp, g3d_light.cpp], conflict: true`
 * - five C++ source files in one linker run.  The first function that cites a `__FILE__` string
 * (`fn_80075E9C`) cites `g3d_dcc.cpp`, the tail (`fn_8007A8E0` onward) cites `g3d_light.cpp`.  The
 * dominant TU names already have homes elsewhere, so the cluster keeps the map's own `fn_80075DCC` stem
 * and the module is `g3d` (every covered TU is nw4r g3d).
 *
 * Seam: unproven.  The left edge 0x80075DCC is where `g3d/g3d_camera.cpp`'s last function ends and is a
 * `tudiscover` strong cut (a `.sdata2` pool run jump); the right edge 0x8007C540 is the proposal
 * boundary the discovery capped at `--max-bytes`, NOT a TU seam - the next proposal (`8007C540`) owns
 * the rest of this linker run.  This unit was registered with the proposal's exact extent so it can be
 * measured; the re-cut rides the batch.
 *
 * Sections: .text 0x80075DCC-0x8007C540, extab 0x800081F8-0x80008588,
 * extabindex 0x8002091C-0x80020E74.
 *
 * rule 7 deferred: the symbol map has only `fn_XXXXXXXX` for this range (checked with
 * `python tools/symbols/dumpmap.py lookup <addr>`: every unnamed entry is a bare `zz_XXXXXXXX_`
 * placeholder in the runtime dump too), so there is no real name to recover for those functions and
 * stylelint's rule 7 refuses the landing without this line.
 *
 * Reconstruction status: the bodies are m2c's decompilation of the target object, mechanically typed
 * (each `->unkNN` becomes a `RawView_N` struct field `field_0xNN` with its offset and size stated).
 * Per-function scores are in the outbox and the notes file.
 */

#include "types.h"


/* The indirect-dispatch object and vtable the five `bctr` trampolines share. */
typedef struct {
    /* +0x00 */ u32 pad_0x00[5];
    /* +0x14 */ u32 (*method_0x14)(...);
    /* +0x18 */ u8 pad_0x18[4];
    /* +0x1C */ u32 (*method_0x1C)(...);
} DispatchVtbl; /* size: 0x20 */
typedef struct {
    /* +0x00 */ u8 pad_0x00[0xD4];
    /* +0xD4 */ void* field_0xD4;
    /* +0xD8 */ u8 field_0xD8;
    /* +0xD9 */ u8 pad_0xD9[1];
    /* +0xDA */ u16 field_0xDA;
} DispatchObj; /* size: 0xDC */

#include "sys_mem.h" /* operator delete (rule 9: call through the owner) */
#include "nw4r/g3d/scnmdl.h" /* nw4r::g3d::ScnMdl::CopiedMatAccess - the owner of the two mangled members (rule 1/9) */
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the symbols deleted above (rule 2) */

#define M2C_ERROR(x) /* unknown instruction */



/* Data objects the range references (unsplit, address-only). */
extern u32 lbl_8056F658;
extern u32 lbl_8056F668;
extern u32 lbl_8056F678;
extern u32 lbl_8056F688;
extern u32 lbl_8056F6A0;
extern u32 lbl_8056F6B0;
extern u32 lbl_8056F6C0;
extern u32 lbl_8058E570;
extern u32 lbl_8058E57C;
extern u32 lbl_8058E5B0;
extern u32 lbl_8058E5D8;
extern u32 lbl_8058E608;
extern u32 lbl_8058E620;
extern u32 lbl_8058E694;
extern u32 lbl_8058E6A4;
extern u32 lbl_8058E6C0;
extern u32 lbl_8058E6D0;
extern u32 lbl_8058E6F0;
extern u32 lbl_8058E700;
extern u32 lbl_8058E720;
extern u32 lbl_8058E730;
extern u32 lbl_8058E758;
extern u32 lbl_8058E768;
extern u32 lbl_8058E790;
extern u32 lbl_8058E7A0;
extern u32 lbl_8058E7C8;
extern u32 lbl_8058E7D8;
extern u32 lbl_8058E7E4;
extern u32 lbl_8058E800;
extern u32 lbl_8058E810;
extern u32 lbl_8058E838;
extern u32 lbl_8058E848;
extern u32 lbl_8058E870;
extern u32 lbl_8058E880;
extern u32 lbl_8058E890;
extern u32 lbl_8058E8B8;
extern u32 lbl_8058E8E8;
extern u32 lbl_8058E91C;
extern u32 lbl_8058E950;
extern u32 lbl_8058E984;
extern u32 lbl_8058E9C8;
extern u32 lbl_8058E9F4;
extern u32 lbl_8058EA20;
extern u32 lbl_8058EA30;
extern u32 lbl_8058EA50;
extern u32 lbl_8058EA64;
extern u32 lbl_8058EA90;
extern u32 lbl_8058EAA0;
extern u32 lbl_8058EAC8;
extern u32 lbl_8058EAD8;
extern u32 lbl_8058EAE8;
extern u32 lbl_8058EB08;
extern u32 lbl_8058EB80;
extern u32 lbl_8058EB90;
extern u32 lbl_8058EBF8;
extern u32 lbl_8058EC30;
extern u32 lbl_8058EC70;
extern u32 lbl_8058ECB0;
extern u32 lbl_8058ECF8;
extern u32 lbl_8058ED18;
extern u32 lbl_8058ED38;
extern u32 lbl_8058ED64;
extern u32 lbl_8058ED90;
extern u32 lbl_8058F4C8;
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
u32 MEMFreeToAllocator(void);
u32 OSRegisterVersion(s32);
u32 PPCSync(void);
u32 TheBeatMatchOutput(void);
u32 VIGetTvFormat(void);
u32 dtor_800813B8(u32);
u32 fn_8004C4F0(s32, void*);
u32 fn_800504D4(s32);
u32 fn_80050850(void*, void*, f32, f32);
u32 fn_800514FC(void*, s32, void*);
u32 fn_8005A8E0(void*, void*);
s32 fn_8005A91C(s32);
s32 fn_8005A950(void*, s32);
s32 fn_8005A9BC(void);
s32 fn_8005AA30(void*);
s32 fn_8005AAEC(s32);
s32 fn_8005AB00(void*);
void* fn_8005CEEC(void);
s32 fn_8005D050(void*);
u32 fn_8005D0CC(void*, void*);
u32 fn_8005D1AC(void*, u32);
s32 fn_8005D218(void*);
u32 fn_8005D2C0(void*, void*);
u32 fn_8005D3E0(void*);
u32 fn_8005DC24(void*);
void* fn_8005DCD0(void*, void*);
void* fn_800600C0(void);
void* fn_80062DEC(s32);
void* fn_800638B8(void*, void*);
s32 fn_80063964(s32, void*);
s32 fn_800639D0(void*, void*);
s32 fn_8006405C(void*);
s32 fn_800640E4(s32);
s32 fn_80064820(void*);
u32 fn_80064BD4(void*);
s32 fn_8006518C(void);
u32 fn_800651A4(void*);
s32 fn_800651E0(s32);
s32 fn_80065204(void*);
s32 fn_800659C4(s32);
void* fn_80067A54(s32);
u32 fn_800696C0(void*);
u32 fn_8006E2A8(s32, s32);
s32 fn_8006E324(void*);
s32 fn_8006E6B4(void*);
u32 fn_8006F0E8(void*, void*);
s32 fn_8006F124(s32);
s32 fn_8006F158(void*, s32);
s32 fn_8006F1C4(void);
u32 fn_8006F228(void*, void*);
s32 fn_8006F264(s32);
s32 fn_8006F298(void*, s32);
s32 fn_8006F340(s32);
s32 fn_8006F374(void*, s32);
s32 fn_8006F3E8(s32);
s32 fn_8006F41C(void*, s32);
s32 fn_8006F488(s32);
s32 fn_8006F4BC(void*, s32);
u32 fn_8006FDCC(void*);
s32 fn_8006FEC8(void*, s32);
s32 fn_80070020(s32);
u32 fn_8007100C(void*, s32);
u32 fn_800710BC(void*, void*, void*);
u32 fn_80071C38(u32);
u32 fn_800731EC(void*, s32);
u32 fn_80073404(void*, s32);
u32 fn_800735A8(void*, s32);
s32 fn_80074074(void*);
u32 fn_80074620(void*);
void* fn_80074A54(void);
s32 fn_80075844(s32);
u32 fn_80085238(void*);
u32 fn_80085D4C(void*);
u32 fn_80085DF4(void*);
u32 fn_80085FF0(void*);
u32 fn_80086194(void*);
u32 fn_80086568(void*);
u32 fn_80086980(void*);
u32 fn_800869D4(void*);
u32 fn_80086A28(void*);
u32 fn_80086AA8(void*, void*);
u32 fn_80086B2C(void*, s32, s32, s32, s32, void*, s32);
u32 fn_800870E4(void*);
u32 fn_8008715C(void*);
u32 fn_80087978(void*);
u32 fn_80087AD0(void*, s32, s32);
u32 fn_80087DF8(s32, s32, s32);
void* fn_80087F08(s32);
void* fn_80088048(void);
u32 fn_8008818C(void);
s32 fn_80088260(s8);
s8 fn_80088270(s8);
u32 fn_800882A0(void*, void*, void*, void*, void*);
s32 fn_8008831C(void);
u32 fn_80088574(void*);
u32 fn_80088590(u32);
u32 fn_80088E24(void);
u32 fn_80088F9C(void);
u32 fn_80089114(u32);
u32 fn_800895B8(void*, u16);
s32 fn_80094094(s32);
u32 fn_800958C8(void*);
u32 fn_80095940(void*);
u32 fn_80095ADC(void*, s32, void*, void*);
f32 fn_80095D54(void*, u16);
u32 fn_800963C0(void*, u32, void*);
u32 fn_80096968(void*, s32, void*, void*);
u32 fn_80096E8C(void*, u32, void*);
u32 fn_80096F9C(void*, s32, u32, u32, void*, u32, u32, void*);
s32 fn_8009754C(s32);
s32 fn_800975D4(void*);
s32 fn_80097EB0(void*, s32);
s32 fn_80097F18(void*, s32);
s32 fn_80098798(s32, s32);
s32 fn_800988B0(s32, s32);
s32 fn_80099974(void*);
s32 fn_80099BB0(void*);
u32 fn_8009A748(void*, void*, u32);
u32 fn_8009A910(void*, u32);
u32 fn_8009AB48(u32);
s32 fn_800D74E8(s32, s32, s32, s32);
s32 fn_800D79B4(s32, s32, s32, s32);
u32 fn_80463EBC(void);
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
u32 fn_80500EF4(f32);
u32 fn_805015C8(void*, s32, void*);
u32 fn_80502678(void);
u32 fn_805026D8(void);
/* internal */ void fn_80075DCC(f32 farg0);
/* internal */ void fn_80075DD8(void* a0);
/* internal */ u32 fn_80075E98(void* a0);
/* internal */ void fn_80075E9C(s32 arg0, s32 arg1, s32 arg2, s32 arg3, s32 arg4, u32 arg_sp0);
/* internal */ void fn_80076050(s32 arg0, s32 arg1, s32 arg2, s32 arg3);
/* internal */ u32 fn_800761BC(s32 arg0, s32 arg1, void *arg2, void **arg3, s32 arg4);
/* internal */ void* fn_8007663C(s32 *arg0);
/* internal */ s32 fn_8007669C(void* a0);
/* internal */ s32 fn_800766D0(s32 arg0, s32 arg1);
/* internal */ u32 fn_80076734(s32 *arg0, s32 arg1);
/* internal */ s32 fn_8007673C(s32 *arg0);
/* internal */ s32 fn_80076750(s32 *arg0);
/* internal */ s32 fn_80076764(void* a0);
/* internal */ s32 fn_80076794(s32 arg0, s32 arg1);
/* internal */ u32 fn_800767F8(s32 *arg0, s32 arg1);
/* internal */ s32 fn_80076800(s32 *arg0);
/* internal */ s32 fn_80076814(s32 *arg0);
/* internal */ s32 fn_80076828(void* a0);
/* internal */ s32 fn_8007685C(s32 arg0, s32 arg1);
/* internal */ u32 fn_800768C0(s32 *arg0, s32 arg1);
/* internal */ s32 fn_800768C8(s32 *arg0);
/* internal */ s32 fn_800768DC(s32 *arg0);
/* internal */ s32 fn_800768F0(s32 *arg0);
/* internal */ s32 fn_80076904(s32 arg0, void* a1);
/* internal */ u32 fn_80076934(s32 *arg0, s32 *arg1);
/* internal */ s32 fn_80076940(void* a0);
/* internal */ s32 fn_80076974(s32 *arg0);
/* internal */ s32 fn_80076988(s32 arg0, s32 arg1);
/* internal */ u32 fn_800769EC(s32 *arg0, s32 arg1);
/* internal */ u32 fn_800769F4(s32 *arg0, s32 *arg1, s32 arg2, s32 arg3, s32 arg4, void *arg5, s32 arg6);
/* internal */ void* fn_80077398(s32 arg0);
/* internal */ s32 fn_800773FC(s32 *arg0);
/* internal */ void* fn_80077404(void* a0);
/* internal */ u32 fn_8007740C(s32 arg0, s32 *arg1);
/* internal */ u32 fn_80077420(u16 arg0, s32 arg1);
/* internal */ u32 fn_80077474(s32 arg0);
/* internal */ u32 fn_80077480(u16 arg0);
/* internal */ u32 fn_80077490(u8 arg0);
/* internal */ f32 fn_800774A0(void* a0, void* a1);
/* internal */ s32 fn_800774A4(void* a0);
/* internal */ void* fn_800774C8(s32 arg0);
/* internal */ s32 fn_8007752C(s32 *arg0);
/* internal */ void* fn_80077534(void* a0);
/* internal */ s32 fn_80077540(s32 *arg0);
/* internal */ s32 fn_80077554(s32 arg0, void* a1);
/* internal */ u32 fn_80077584(s32 *arg0, s32 *arg1);
/* internal */ s32 fn_80077590(s32 arg0, void* a1);
/* internal */ u32 fn_800775C0(s32 *arg0, s32 *arg1);
/* internal */ void* fn_800775CC(s32 arg0);
/* internal */ s32 fn_80077630(s32 *arg0);
/* internal */ void* fn_80077638(void* a0);
/* internal */ s32 fn_80077644(void* a0);
/* internal */ void* fn_80077674(s32 arg0);
/* internal */ s32 fn_800776D8(s32 *arg0);
/* internal */ s32 fn_800776E0(s32 *arg0);
/* internal */ s32 fn_800776F4(s32 *arg0);
/* internal */ s32 fn_80077708(s32 arg0, void* a1);
/* internal */ u32 fn_80077738(s32 *arg0, s32 *arg1);
/* internal */ s32 fn_80077744(s32 arg0, s32 arg1);
/* internal */ u32 fn_800777A8(s32 *arg0, s32 arg1);
/* internal */ void* fn_800777B0(s32 arg0, u32 *arg1, s32 arg2);
/* internal */ s32 fn_80077D4C(s32 *arg0);
/* internal */ s32 fn_80077D64(s32 arg0);
/* internal */ u32 fn_80077DBC(void *arg1, void* a1);
/* internal */ s32 fn_80077DD8(s32 *arg0);
/* internal */ u32 fn_80077DF0(void *arg0, f32 farg0, f32 farg1, f32 farg2, f32 farg3, f32 farg4, f32 farg5, f32 farg6, f32 farg7, f32 arg_sp8, f32 arg_spC, f32 arg_sp10, f32 arg_sp14);
/* internal */ s32 fn_80077E34(s32 arg0, void* a1);
/* internal */ u32 fn_80077E64(s32 *arg0, s32 *arg1);
/* internal */ u32 fn_80077E70(s32 arg0, s32 arg1, s32 arg2);
/* internal */ s32 fn_800783B0(s32 arg0, void* a1);
/* internal */ u32 fn_800783E0(s32 *arg0, s32 *arg1);
/* internal */ s32 fn_800783EC(s32 arg0, s32 arg1);
/* internal */ u32 fn_80078450(s32 *arg0, s32 arg1);
/* internal */ s32 fn_80078458(s32 arg0, void* a1);
/* internal */ u32 fn_80078488(s32 *arg0, s32 *arg1);
/* internal */ s32 fn_80078494(s32 arg0, void* a1);
/* internal */ u32 fn_800784C4(s32 *arg0, s32 *arg1);
/* internal */ s32 fn_800784D0(s32 arg0, void* a1);
/* internal */ u32 fn_80078500(s32 *arg0, s32 *arg1);
/* internal */ s32 fn_8007850C(s32 arg0, void* a1);
/* internal */ u32 fn_8007853C(s32 *arg0, s32 *arg1);
/* internal */ s32 fn_80078548(s32 arg0, void* a1);
/* internal */ u32 fn_80078578(s32 *arg0, s32 *arg1);
/* internal */ s32 fn_80078584(s32 arg0, void* a1);
/* internal */ u32 fn_800785B4(s32 *arg0, s32 *arg1);
/* internal */ s32 fn_800785C0(s32 arg0, s32 arg1);
/* internal */ u32 fn_80078608(s32 *arg0, s32 *arg1);
/* internal */ s32 fn_80078614(s32 arg0, void* a1);
/* internal */ u32 fn_80078644(s32 *arg0, s32 *arg1);
/* internal */ s32 fn_80078650(s32 arg0, void* a1);
/* internal */ u32 fn_80078680(s32 *arg0, s32 *arg1);
/* internal */ u32 fn_8007868C(s32 arg0, void *arg1, s32 arg2, u32 arg_sp0);
/* internal */ s32 fn_80078878(s32 arg0, s32 arg1);
/* internal */ s32 fn_800788C8(s32 arg0, void* a1);
/* internal */ u32 fn_800788F8(s32 *arg0, s32 *arg1);
/* internal */ s32 fn_80078904(s32 arg0);
/* internal */ s32 fn_80078988(s32 arg0, void* a1);
/* internal */ u32 fn_800789B8(s32 *arg0, s32 *arg1);
/* internal */ s32 fn_800789C4(s32 arg0, s32 arg1);
/* internal */ u32 fn_80078A28(s32 *arg0, s32 arg1);
/* internal */ s32 fn_80078A30(s32 arg0, s32 arg1);
/* internal */ u32 fn_80078A94(s32 *arg0, s32 arg1);
/* internal */ u32 fn_80078A9C(s32 arg0, void *arg1, s32 arg2, s32 arg3, u32 arg_sp0);
/* internal */ void* fn_80078DC0(void *arg0);
/* internal */ u32 fn_80078E7C(s32 arg0, void *arg1, u32 arg2, s32 arg3, u32 arg_sp0);
/* internal */ u32 fn_80079018(s32 arg0, void *arg1, u32 arg2, s32 arg3, s32 arg4, u32 arg_sp0);
/* internal */ void* fn_800791D8(u32 *arg0, s32 arg1, s32 arg2, void *arg3, void *arg4);
/* internal */ void fn_800793A4(s32 *arg0, s32 arg1, s32 arg2, s32 arg3, s32 arg4, s32 arg5, s32 arg6, s32 arg7, u32 arg_sp0);
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
/* internal */ s32 fn_80079FB4(s32 arg0, void* a1);
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
/* internal */ void* dtor_8007B2D4(void *arg0, s16 arg1);
/* internal */ u32 fn_8007B340(void* a0, void* a1);
/* internal */ void fn_8007B348(void **arg0);
/* internal */ s32 fn_8007B3BC(void* a0);
/* internal */ void fn_8007B3EC(void **arg0);
/* internal */ s32 fn_8007B424(void *arg0);
/* internal */ void fn_8007B42C(u32 *arg0, u32 arg1, u32 *arg2);
/* internal */ void fn_8007B450(void* a0);
/* internal */ s32 fn_8007B47C(s32 arg0, s32 arg1);
/* internal */ void fn_8007B4E4(void* a0);
/* internal */ void fn_8007B540(void* a0);
/* internal */ s32 fn_8007B544(s32 arg0, u32 arg1);
/* internal */ void fn_8007B5BC(void* a0);
/* internal */ s32 fn_8007B5C0(void *arg0, u32 arg1);
/* internal */ s32 fn_8007B5F4(s32 arg0, s32 *arg1);
/* internal */ s32 fn_8007B660(s32 arg0, s32 *arg1);
/* internal */ s32 fn_8007B6CC(void* a0);
/* internal */ s32 fn_8007B6FC(void* a0);
/* internal */ void* fn_8007B72C(s32 *arg0, s32 arg1);
/* internal */ s32 fn_8007B734(void* a0);
/* internal */ s32 fn_8007B764(void* a0);
/* internal */ s32 fn_8007B794(s32 arg0, s16 arg1);
/* internal */ s32 dtor_8007B7F0(s32 arg0, s16 arg1);
/* internal */ s32 fn_8007B834(s32 arg0);
/* internal */ u32 fn_8007B864(s32 *arg0, s32 *arg1);
/* internal */ s32 fn_8007B870(s32 arg1);
/* internal */ s32 fn_8007B878(s32 arg0, s32 arg1);
/* internal */ u32 fn_8007B8DC(s32 *arg0, s32 arg1);
/* internal */ void fn_8007B93C(void* a0);
/* internal */ void fn_8007B998(void* a0);
/* internal */ void fn_8007B99C(u32 **arg0);
/* internal */ void fn_8007B9AC(void *arg0);
/* internal */ s32 fn_8007B9BC(void *arg0, s32 arg1);
/* internal */ void fn_8007B9D4(void *arg0, s32 arg1, s32 arg2);
/* internal */ s32 fn_8007BA00(void* a0);
/* internal */ s32 fn_8007BA08(s32 arg0);
/* internal */ u32 fn_8007BA38(s32 *arg0, s32 *arg1);
/* internal */ s32 dtor_8007BA44(s32 arg0, s16 arg1);
/* internal */ void* fn_8007BAA0(u32 **arg0);
/* internal */ s32 fn_8007BAF0(s32 arg0, s32 *arg1);
/* internal */ s32 fn_8007BB5C(void* a0);
/* internal */ void* fn_8007BB8C(s32 *arg0, s32 arg1);
/* internal */ u32 fn_8007BB94(void *arg0, s32 arg1, s32 arg2);
/* internal */ s32 fn_8007BC2C(void *arg0, s32 arg1);
/* internal */ s32 fn_8007BCAC(void *arg0, s32 arg1);
/* internal */ s32 fn_8007BD2C(void *arg0, s32 arg1);
/* internal */ s32 fn_8007BDAC(void *arg0, s32 arg1);
/* internal */ s32 fn_8007BE2C(void *arg0, s32 arg1);
/* internal */ s32 fn_8007C3CC(void *arg0, s32 arg1);
/* internal */ u32 fn_8007C464(void *arg0);
/* internal */ void* fn_8007C474(void *arg0, void *arg1, s32 arg2);


void fn_80075DCC(f32 farg0) {
    fn_80500EF4((f32)(lbl_80795DFC * farg0));
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
        var_r26 = fn_800D74E8((s32)(arg0), (s32)(arg1), (s32)(arg2), (s32)(arg3)) == 0;
    } else {
        nw4r::db::Panic((const char*)&lbl_8058E570, 0x3C, (const char*)&lbl_8058E5D8);
    }
    if ((var_r26 != 0) && (arg1 != 0)) {
        fn_800504D4((s32)(arg0));
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
        fn_800504D4((s32)(arg0));
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
u32 fn_800761BC(s32 arg0, s32 arg1, void *arg2, void **arg3, s32 arg4) {
    u32 sp75;
    u32 sp76;
    u32 sp77;

    u32 sp90;
    s32 sp8C;
    s32 sp88;
    s32 sp84;
    s32 sp80;
    s32 sp7C;
    u8 sp7B;
    u8 sp7A;
    u8 sp79;
    u8 sp78;
    u8 sp74;
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
    s32 sp28;
    s32 sp24;
    s32 sp20;
    s32 sp1C;
    s32 sp18;
    s32 sp14;
    s32 sp10;
    s32 spC;
    s32 sp8;

    if (arg4 == 0) {
        fn_80076988((s32)(&sp8C), (s32)(0));
        if ((arg2 == NULL) || (fn_80076974((s32 *)(&((RawView_2*)arg2)->field_0x24)) == 0)) {
            sp70 = fn_80076940((void*)(arg0));
            fn_80076904((s32)(&sp8C), (void*)(&sp70));
        } else {
            fn_80076904((s32)(&sp8C), (void*)(&((RawView_2*)arg2)->field_0x24));
        }
        fn_80095940((void*)(&sp8C));
        fn_8008818C();
        sp6C = sp8C;
        fn_80085D4C((void*)(&sp6C));
        if ((arg2 == NULL) || (fn_800768F0((s32 *)(&((RawView_2*)arg2)->field_0x04)) == 0)) {
            sp68 = fn_8006F3E8((s32)(arg0));
            fn_80085FF0((void*)(&sp68));
        } else {
            sp64 = ((RawView_2*)arg2)->field_0x04;
            fn_80085FF0((void*)(&sp64));
        }
        if ((arg2 == NULL) || (fn_800768DC((s32 *)(arg2)) == 0)) {
            sp60 = fn_8006F488((s32)(arg0));
            fn_80085DF4((void*)(&sp60));
        } else {
            sp5C = ((RawView_2*)arg2)->field_0x00;
            fn_80085DF4((void*)(&sp5C));
        }
        if ((arg2 == NULL) || (fn_800768C8((s32 *)(&((RawView_2*)arg2)->field_0x08)) == 0)) {
            sp58 = fn_80076828((void*)(arg0));
            fn_80086194((void*)(&sp58));
        } else {
            sp54 = ((RawView_2*)arg2)->field_0x08;
            fn_80086194((void*)(&sp54));
        }
        if ((arg2 == NULL) || (fn_80076814((s32 *)(&((RawView_2*)arg2)->field_0x0C)) == 0)) {
            sp50 = fn_8009754C((s32)(arg0));
            fn_80086568((void*)(&sp50));
        } else {
            sp4C = ((RawView_2*)arg2)->field_0x0C;
            fn_80086568((void*)(&sp4C));
        }
        if ((arg2 == NULL) || (fn_80076800((s32 *)(&((RawView_2*)arg2)->field_0x10)) == 0)) {
            sp48 = fn_80076764((void*)(arg0));
            fn_80086980((void*)(&sp48));
        } else {
            sp44 = ((RawView_2*)arg2)->field_0x10;
            fn_80086980((void*)(&sp44));
        }
        if ((arg2 == NULL) || (fn_80076750((s32 *)(&((RawView_2*)arg2)->field_0x14)) == 0)) {
            sp40 = fn_8006F124((s32)(arg0));
            fn_800869D4((void*)(&sp40));
        } else {
            sp3C = ((RawView_2*)arg2)->field_0x14;
            fn_800869D4((void*)(&sp3C));
        }
        if ((arg2 == NULL) || (fn_8006E6B4((void*)(&((RawView_2*)arg2)->field_0x18)) == 0)) {
            if (arg3 != NULL) {
                sp38 = fn_8006F264((s32)(arg0));
                fn_80086AA8((void*)(&sp38), (void*)(arg3));
            } else {
                sp34 = fn_8006F264((s32)(arg0));
                fn_80086A28((void*)(&sp34));
            }
        } else if (arg3 != NULL) {
            sp30 = ((RawView_2*)arg2)->field_0x18;
            fn_80086AA8((void*)(&sp30), (void*)(arg3));
        } else {
            sp2C = ((RawView_2*)arg2)->field_0x18;
            fn_80086A28((void*)(&sp2C));
        }
        fn_800958C8((void*)(&sp8C));
        fn_800882A0((void*)(&sp88), (void*)(&sp84), (void*)(&sp80), (void*)(&sp7C), (void*)(&sp74));
        sp78 = sp74;
        sp79 = sp75;
        sp7A = sp76;
        sp7B = sp77;
        if ((arg2 == NULL) || (fn_80064820((void*)(&((RawView_2*)arg2)->field_0x1C)) == 0)) {
            sp24 = (s32) sp78;
            sp28 = fn_8005A91C((s32)(arg0));
            fn_80086B2C((void*)(&sp28), (s32)(sp88), (s32)(sp84), (s32)(sp80), (s32)(sp7C), (void*)(&sp24), (s32)((arg1 & 8) != 0));
        } else {
            sp1C = (s32) sp78;
            sp20 = ((RawView_2*)arg2)->field_0x1C;
            fn_80086B2C((void*)(&sp20), (s32)(sp88), (s32)(sp84), (s32)(sp80), (s32)(sp7C), (void*)(&sp1C), (s32)((arg1 & 8) != 0));
        }
        if ((arg2 == NULL) || (fn_8007673C((s32 *)(&((RawView_2*)arg2)->field_0x20)) == 0)) {
            sp18 = fn_8007669C((void*)(arg0));
            fn_800870E4((void*)(&sp18));
        } else {
            sp14 = ((RawView_2*)arg2)->field_0x20;
            fn_800870E4((void*)(&sp14));
        }
        if ((arg2 == NULL) || (fn_8006E324((void*)(&((RawView_2*)arg2)->field_0x28)) == 0)) {
            sp10 = fn_8006F340((s32)(arg0));
            fn_8008715C((void*)(&sp10));
            return;
        }
        spC = ((RawView_2*)arg2)->field_0x28;
        fn_8008715C((void*)(&spC));
        return;
    }
    if (arg3 != NULL) {
        if ((arg2 == NULL) || (fn_8006E6B4((void*)(&((RawView_2*)arg2)->field_0x18)) == 0)) {
            fn_8007663C((s32 *)(&sp90));
            ((RawView_3*)(*arg3))->field_0x08(arg3, &sp90);
            fn_80085238((void*)(&sp90));
            return;
        }
        sp8 = ((RawView_2*)arg2)->field_0x18;
        fn_80086AA8((void*)(&sp8), (void*)(arg3));
    }
}

typedef struct {
    /* +0x00 */ u8 pad_0x00[0x30];
    /* +0x30 */ u8 field_0x30;
} RawView_4; /* size: 0x31 */
void* fn_8007663C(s32 *arg0) {
    void *var_r30;

    *arg0 = 0;
    var_r30 = arg0 + 4;
    do {
        MTX34_ctor((void*)(var_r30));
        var_r30 = &((RawView_4*)var_r30)->field_0x30;
    } while ((u32)var_r30 < (u32)(arg0 + 0x94));
    return arg0;
}

s32 fn_8007669C(void* a0) {
    u32 sp8;

    return *((s32*)fn_800766D0((s32)(&sp8), (s32)(fn_8006F1C4() + 0xE0)));
}

s32 fn_800766D0(s32 arg0, s32 arg1) {
    fn_80076734(0, 0);
    if ((s32) (arg1 & 0x1F) != 0) {
        nw4r::db::Panic((const char*)&lbl_8058E758, 0x201, (const char*)&lbl_8058E730);
    }
    return arg0;
}

u32 fn_80076734(s32 *arg0, s32 arg1) {
    *arg0 = arg1;
}

s32 fn_8007673C(s32 *arg0) {
    return *arg0 != 0;
}

s32 fn_80076750(s32 *arg0) {
    return *arg0 != 0;
}

s32 fn_80076764(void* a0) {
    u32 sp8;

    return *((s32*)fn_80076794((s32)(&sp8), (s32)(fn_8006F1C4())));
}

s32 fn_80076794(s32 arg0, s32 arg1) {
    fn_800767F8(0, 0);
    if ((s32) (arg1 & 0x1F) != 0) {
        nw4r::db::Panic((const char*)&lbl_8058E790, 0x154, (const char*)&lbl_8058E768);
    }
    return arg0;
}

u32 fn_800767F8(s32 *arg0, s32 arg1) {
    *arg0 = arg1;
}

s32 fn_80076800(s32 *arg0) {
    return *arg0 != 0;
}

s32 fn_80076814(s32 *arg0) {
    return *arg0 != 0;
}

s32 fn_80076828(void* a0) {
    u32 sp8;

    return *((s32*)fn_8007685C((s32)(&sp8), (s32)(fn_8005A9BC() + 0x14)));
}

s32 fn_8007685C(s32 arg0, s32 arg1) {
    fn_800768C0(0, 0);
    if ((s32) (arg1 & 3) != 0) {
        nw4r::db::Panic((const char*)&lbl_8058E870, 0xAF, (const char*)&lbl_8058E848);
    }
    return arg0;
}

u32 fn_800768C0(s32 *arg0, s32 arg1) {
    *arg0 = arg1;
}

s32 fn_800768C8(s32 *arg0) {
    return *arg0 != 0;
}

s32 fn_800768DC(s32 *arg0) {
    return *arg0 != 0;
}

s32 fn_800768F0(s32 *arg0) {
    return *arg0 != 0;
}

s32 fn_80076904(s32 arg0, void* a1) {
    fn_80076934(0, 0);
    return arg0;
}

u32 fn_80076934(s32 *arg0, s32 *arg1) {
    *arg0 = *arg1;
}

s32 fn_80076940(void* a0) {
    u32 sp8;

    return *((s32*)fn_80076988((s32)(&sp8), (s32)(fn_8005A9BC() + 0x1C)));
}

s32 fn_80076974(s32 *arg0) {
    return *arg0 != 0;
}

s32 fn_80076988(s32 arg0, s32 arg1) {
    fn_800769EC(0, 0);
    if ((s32) (arg1 & 3) != 0) {
        nw4r::db::Panic((const char*)&lbl_8058E838, 0xF3, (const char*)&lbl_8058E810);
    }
    return arg0;
}

u32 fn_800769EC(s32 *arg0, s32 arg1) {
    *arg0 = arg1;
}

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


void* fn_80077398(s32 arg0) {
    if (fn_800776E0(0) == 0) {
        nw4r::db::Panic((const char*)&lbl_8058E6F0, 0x3A, (const char*)&lbl_8058E6D0, fn_80077404(0), &lbl_807911E8);
    }
    fn_800773FC((s32 *)(arg0));
}

s32 fn_800773FC(s32 *arg0) {
    return *arg0;
}

void* fn_80077404(void* a0) {
    return &lbl_807911F0;
}

u32 fn_8007740C(s32 arg0, s32 *arg1) {
    fn_80077420((u16)((u16) ((arg0 & 1) + 0x100C)), (s32)(*arg1));
}

u32 fn_80077420(u16 arg0, s32 arg1) {
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

f32 fn_800774A0(void* a0, void* a1) {
    fn_80463EBC();
}

typedef struct {
    /* +0x00 */ u8 pad_0x00[0x20];
    /* +0x20 */ u32 field_0x20;
} RawView_14; /* size: 0x24 */
s32 fn_800774A4(void* a0) {
    return ((RawView_14*)fn_800774C8(0))->field_0x20;
}

void* fn_800774C8(s32 arg0) {
    if (fn_80077540(0) == 0) {
        nw4r::db::Panic((const char*)&lbl_8058E6C0, 0x13E, (const char*)&lbl_8058E6A4, fn_80077534(0), &lbl_807911EC);
    }
    fn_8007752C((s32 *)(arg0));
}

s32 fn_8007752C(s32 *arg0) {
    return *arg0;
}

void* fn_80077534(void* a0) {
    return &lbl_8058E694;
}

s32 fn_80077540(s32 *arg0) {
    return *arg0 != 0;
}

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
    if (fn_800776F4(0) == 0) {
        nw4r::db::Panic((const char*)&lbl_8058E800, 0x11F, (const char*)&lbl_8058E7E4, fn_80077638(0), &lbl_807911E0);
    }
    fn_80077630((s32 *)(arg0));
}

s32 fn_80077630(s32 *arg0) {
    return *arg0;
}

void* fn_80077638(void* a0) {
    return &lbl_8058E7D8;
}

typedef struct {
    /* +0x00 */ u8 pad_0x00[0x34];
    /* +0x34 */ u32 field_0x34;
} RawView_15; /* size: 0x38 */
s32 fn_80077644(void* a0) {
    return (((RawView_15*)fn_80077674(0))->field_0x34 & 2) == 0;
}

void* fn_80077674(s32 arg0) {
    if (fn_800776E0(0) == 0) {
        nw4r::db::Panic((const char*)&lbl_8058E720, 0x3A, (const char*)&lbl_8058E700, fn_80077404(0), &lbl_807911E4);
    }
    fn_800776D8((s32 *)(arg0));
}

s32 fn_800776D8(s32 *arg0) {
    return *arg0;
}

s32 fn_800776E0(s32 *arg0) {
    return *arg0 != 0;
}

s32 fn_800776F4(s32 *arg0) {
    return *arg0 != 0;
}

s32 fn_80077708(s32 arg0, void* a1) {
    fn_80077738(0, 0);
    return arg0;
}

u32 fn_80077738(s32 *arg0, s32 *arg1) {
    *arg0 = *arg1;
}

s32 fn_80077744(s32 arg0, s32 arg1) {
    fn_800777A8(0, 0);
    if ((s32) (arg1 & 3) != 0) {
        nw4r::db::Panic((const char*)&lbl_8058E7C8, 0x11F, (const char*)&lbl_8058E7A0);
    }
    return arg0;
}

u32 fn_800777A8(s32 *arg0, s32 arg1) {
    *arg0 = arg1;
}

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
    if ((fn_8005AA30(0) == 0) || (fn_800776E0((s32 *)(arg2)) == 0)) {
        M2C_ERROR(/* unknown instruction: vmrghb v31, v1, v0 */);
        M2C_ERROR(/* unknown instruction: vmrghb v30, v1, v0 */);
        M2C_ERROR(/* unknown instruction: vmrghb v29, v1, v0 */);
        return NULL;
    }
    sp24 = fn_80076940((void*)(arg0));
    fn_80077590((s32)(&sp34), (void*)(&sp24));
    fn_80095ADC((void*)(&sp34), (s32)(1), (void*)(&sp8C), (void*)(&sp30));
    fn_80095ADC((void*)(&sp34), (s32)(2), (void*)(&sp90), (void*)(&sp31));
    fn_80095ADC((void*)(&sp34), (s32)(3), (void*)(&sp94), (void*)(&sp32));
    if ((sp8C == 0) && (sp90 == 0) && (sp94 == 0)) {
        M2C_ERROR(/* unknown instruction: vmrghb v31, v1, v0 */);
        M2C_ERROR(/* unknown instruction: vmrghb v30, v1, v0 */);
        M2C_ERROR(/* unknown instruction: vmrghb v29, v1, v0 */);
        return NULL;
    }
    var_r29 = 0;
    MTX34_ctor((void*)(&spC8));
    temp_r3 = (void **)(fn_80088048());
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
                if ((s32) ((RawView_17*)fn_800773FC((s32 *)(arg2)))->field_0x08 >= 0) {
                    temp_r20 = fn_8006FDCC((void*)(arg1));
                    if ((u32) ((RawView_17*)fn_800773FC((s32 *)(arg2)))->field_0x08 == temp_r20) {
                        temp_r3_2 = (void *)(fn_80087F08((s32)(((RawView_17*)fn_800773FC((s32 *)(arg2)))->field_0x08)));
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
                        sp1C = fn_80074074((void*)(&sp2C));
                        temp_r3_3 = fn_8006FEC8((void*)(&sp1C), (s32)(((RawView_17*)fn_800773FC((s32 *)(arg2)))->field_0x08));
                        if (temp_r3_3 < 0) {
                            nw4r::db::Panic((const char*)&lbl_8058E880, 0x64, (const char*)&lbl_8058E890);
                        }
                        sp18 = fn_80097EB0((void*)(&sp2C), (s32)(temp_r3_3));
                        fn_8005D2C0((void*)(&sp28), (void*)(&sp18));
                        temp_r20_2 = fn_8005D218((void*)(arg1));
                        fn_800710BC((void*)(&spC8), (void*)(fn_8005D218((void*)(&sp28)) + 0xA0), (void*)(u32)(temp_r20_2 + 0x70));
                        temp_r3_4 = (void *)(fn_80087F08((s32)(((RawView_17*)fn_800773FC((s32 *)(arg2)))->field_0x08)));
                        sp8 = ((RawView_19*)temp_r3_4)->field_0x18;
                        spC = ((RawView_19*)temp_r3_4)->field_0x1C;
                        sp10 = ((RawView_19*)temp_r3_4)->field_0x20;
                        sp14 = temp_f31;
                        fn_80077DF0((void *)(&sp98), (*(f32*)&temp_r3_4), (f32)(((RawView_19*)temp_r3_4)->field_0x00), (f32)(((RawView_19*)temp_r3_4)->field_0x04), (f32)(((RawView_19*)temp_r3_4)->field_0x08), (f32)(temp_f31), (f32)(((RawView_19*)temp_r3_4)->field_0x0C), (f32)(((RawView_19*)temp_r3_4)->field_0x10), (f32)(((RawView_19*)temp_r3_4)->field_0x14), (f32)(temp_f31), 0, 0, 0);
                        fn_800710BC((void*)(&spC8), (void*)(&sp98), (void*)(&spC8));
                    }
                    spF4 = temp_f31;
                    spE4 = temp_f31;
                    spD4 = temp_f31;
                    setVec3((void*)(&sp80), (f32)(spC8), (f32)(spD8), (f32)(spE8));
                    fn_80050850((void*)(&sp80), (void*)(&sp80), 0, 0);
                    spC8 = sp80;
                    spD8 = sp84;
                    spE8 = sp88;
                    setVec3((void*)(&sp74), (f32)(spCC), (f32)(spDC), (f32)(spEC));
                    fn_80050850((void*)(&sp74), (void*)(&sp74), 0, 0);
                    spCC = sp74;
                    spDC = sp78;
                    spEC = sp7C;
                    setVec3((void*)(&sp68), (f32)(spD0), (f32)(spE0), (f32)(spF0));
                    fn_80050850((void*)(&sp68), (void*)(&sp68), 0, 0);
                    spD0 = sp68;
                    spE0 = sp6C;
                    spF0 = sp70;
                } else {
                    fn_8007100C((void*)(&spC8), (s32)(fn_8008831C()));
                }
            }
            temp_r3_5 = fn_80088260((s8)(fn_80088270((s8)((s8) *var_r30))));
            if ((temp_r3_5 != 0) && (fn_8006518C() != 0)) {
                VEC3_ctor((void*)(&sp5C));
                if (fn_80077DD8((s32 *)(temp_r3_5)) != 0) {
                    fn_8007A7E4((s32)(temp_r3_5), (s32)(&sp5C));
                    if ((temp_f31 == sp5C) && (temp_f31 == sp60) && (temp_f31 == sp64)) {
                        fn_8007A7C8((s32)(temp_r3_5), (s32)(&sp5C));
                        fn_80077DBC((void *)(&sp44), (void*)(&sp5C));
                        copyVec3((void*)(&sp5C), (void*)(&sp44));
                        fn_80050850((void*)(&sp5C), (void*)(&sp5C), 0, 0);
                    }
                } else if (fn_80077D64((s32)(temp_r3_5)) != 0) {
                    fn_8007A7C8((s32)(temp_r3_5), (s32)(&sp5C));
                    fn_80077DBC((void *)(&sp38), (void*)(&sp5C));
                    copyVec3((void*)(&sp5C), (void*)(&sp38));
                    fn_80050850((void*)(&sp5C), (void*)(&sp5C), 0, 0);
                } else {
                    VEC3_ctor((void*)(&sp50));
                    if (fn_80077D4C((s32 *)(temp_r3_5)) == 0) {
                        nw4r::db::Panic((const char*)&lbl_8058E880, 0xAB, (const char*)&lbl_8058E8B8);
                    }
                    fn_8007A7E4((s32)(temp_r3_5), (s32)(&sp50));
                    temp_f1 = temp_f29 * sp58;
                    sp5C = temp_f1 * sp50;
                    sp60 = temp_f1 * sp54;
                    sp64 = temp_f30 + (temp_f1 * sp58);
                    fn_80050850((void*)(&sp5C), (void*)(&sp5C), (f32)(temp_f1), (f32)(sp58));
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
    setVec3((void*)(-((RawView_20*)arg1)->field_0x00), (f32)(-((RawView_20*)arg1)->field_0x04), (f32)(-((RawView_20*)arg1)->field_0x08), 0);
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
u32 fn_80077DF0(void *arg0, f32 farg0, f32 farg1, f32 farg2, f32 farg3, f32 farg4, f32 farg5, f32 farg6, f32 farg7, f32 arg_sp8, f32 arg_spC, f32 arg_sp10, f32 arg_sp14) {
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

typedef struct {
    /* +0x00 */ u8 pad_0x00[0x2C];
    /* +0x2C */ u32 field_0x2C;
    /* +0x30 */ u32 field_0x30;
    /* +0x34 */ u32 field_0x34;
} RawView_23; /* size: 0x38 */
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
        fn_80078650((s32)(arg0), (void*)(fn_8006F4BC((void*)(&sp5C), (s32)(temp_r4 + (arg2 * 0x104)))));
    } else {
        fn_80078650((s32)(arg0), (void*)(fn_8006F4BC((void*)(&sp58), (s32)(0))));
    }
    temp_r4_2 = ((RawView_22*)arg1)->field_0x0C;
    if (temp_r4_2 != 0) {
        fn_80078614((s32)(arg0 + 4), (void*)(fn_8006F41C((void*)(&sp54), (s32)(temp_r4_2 + (arg2 * 0x64)))));
    } else {
        fn_80078614((s32)(arg0 + 4), (void*)(fn_8006F41C((void*)(&sp50), (s32)(0))));
    }
    temp_r4_3 = ((RawView_22*)arg1)->field_0x10;
    if (temp_r4_3 != 0) {
        fn_800785C0((s32)(arg0 + 0x28), (s32)(fn_8006F374((void*)(&sp4C), (s32)(temp_r4_3 + (arg2 * 0x248)))));
    } else {
        fn_800785C0((s32)(arg0 + 0x28), (s32)(fn_8006F374((void*)(&sp48), (s32)(0))));
    }
    temp_r4_4 = ((RawView_22*)arg1)->field_0x14;
    if (temp_r4_4 != 0) {
        fn_80078584((s32)(arg0 + 0x1C), (void*)(fn_8005A950((void*)(&sp44), (s32)(temp_r4_4 + (arg2 * 0x28)))));
    } else {
        fn_80078584((s32)(arg0 + 0x1C), (void*)(fn_8005A950((void*)(&sp40), (s32)(0))));
    }
    temp_r4_5 = ((RawView_22*)arg1)->field_0x18;
    if (temp_r4_5 != 0) {
        fn_80078548((s32)(arg0 + 8), (void*)(fn_8007685C((s32)(&sp3C), (s32)(temp_r4_5 + (arg2 * 8)))));
    } else {
        fn_80078548((s32)(arg0 + 8), (void*)(fn_8007685C((s32)(&sp38), (s32)(0))));
    }
    temp_r4_6 = ((RawView_22*)arg1)->field_0x1C;
    if (temp_r4_6 != 0) {
        fn_80076904((s32)(arg0 + 0x24), (void*)(fn_80076988((s32)(&sp34), (s32)(temp_r4_6 + (arg2 * 0xC)))));
    } else {
        fn_80076904((s32)(arg0 + 0x24), (void*)(fn_80076988((s32)(&sp30), (s32)(0))));
    }
    temp_r4_7 = ((RawView_22*)arg1)->field_0x20;
    if (temp_r4_7 != 0) {
        fn_8007850C((s32)(arg0 + 0x10), (void*)(fn_80076794((s32)(&sp2C), (s32)(temp_r4_7 + (arg2 << 5)))));
    } else {
        fn_8007850C((s32)(arg0 + 0x10), (void*)(fn_80076794((s32)(&sp28), (s32)(0))));
    }
    temp_r4_8 = ((RawView_22*)arg1)->field_0x24;
    if (temp_r4_8 != 0) {
        fn_800784D0((s32)(arg0 + 0x14), (void*)(fn_8006F158((void*)(&sp24), (s32)(temp_r4_8 + (arg2 << 7)))));
    } else {
        fn_800784D0((s32)(arg0 + 0x14), (void*)(fn_8006F158((void*)(&sp20), (s32)(0))));
    }
    temp_r4_9 = ((RawView_22*)arg1)->field_0x28;
    if (temp_r4_9 != 0) {
        fn_80078494((s32)(arg0 + 0x18), (void*)(fn_8006F298((void*)(&sp1C), (s32)(temp_r4_9 + (arg2 << 6)))));
    } else {
        fn_80078494((s32)(arg0 + 0x18), (void*)(fn_8006F298((void*)(&sp18), (s32)(0))));
    }
    temp_r4_10 = ((RawView_22*)arg1)->field_0x2C;
    if (temp_r4_10 != 0) {
        fn_80078458((s32)(arg0 + 0x20), (void*)(fn_800766D0((s32)(&sp14), (s32)(temp_r4_10 + (arg2 * 0xA0)))));
    } else {
        fn_80078458((s32)(arg0 + 0x20), (void*)(fn_800766D0((s32)(&sp10), (s32)(0))));
    }
    temp_r4_11 = ((RawView_22*)arg1)->field_0x30;
    if (temp_r4_11 != 0) {
        fn_800783B0((s32)(arg0 + 0xC), (void*)(fn_800783EC((s32)(&spC), (s32)(temp_r4_11 + (arg2 << 9)))));
    } else {
        fn_800783B0((s32)(arg0 + 0xC), (void*)(fn_800783EC((s32)(&sp8), (s32)(0))));
    }
    if ((s32) (((RawView_22*)arg1)->field_0x00 & 1) != 0) {
        ((RawView_23*)arg0)->field_0x2C = 0;
        ((RawView_23*)arg0)->field_0x30 = 0;
        ((RawView_23*)arg0)->field_0x34 = 0;
        return;
    }
    ((RawView_23*)arg0)->field_0x2C = (s32) ((RawView_22*)arg1)->field_0x34;
    ((RawView_23*)arg0)->field_0x30 = (s32) ((RawView_22*)arg1)->field_0x38;
    ((RawView_23*)arg0)->field_0x34 = (s32) ((RawView_22*)arg1)->field_0x3C;
}

s32 fn_800783B0(s32 arg0, void* a1) {
    fn_800783E0(0, 0);
    return arg0;
}

u32 fn_800783E0(s32 *arg0, s32 *arg1) {
    *arg0 = *arg1;
}

s32 fn_800783EC(s32 arg0, s32 arg1) {
    fn_80078450(0, 0);
    if ((s32) (arg1 & 0x1F) != 0) {
        nw4r::db::Panic((const char*)&lbl_8058EA90, 0x25, (const char*)&lbl_8058EA64);
    }
    return arg0;
}

u32 fn_80078450(s32 *arg0, s32 arg1) {
    *arg0 = arg1;
}

s32 fn_80078458(s32 arg0, void* a1) {
    fn_80078488(0, 0);
    return arg0;
}

u32 fn_80078488(s32 *arg0, s32 *arg1) {
    *arg0 = *arg1;
}

s32 fn_80078494(s32 arg0, void* a1) {
    fn_800784C4(0, 0);
    return arg0;
}

u32 fn_800784C4(s32 *arg0, s32 *arg1) {
    *arg0 = *arg1;
}

s32 fn_800784D0(s32 arg0, void* a1) {
    fn_80078500(0, 0);
    return arg0;
}

u32 fn_80078500(s32 *arg0, s32 *arg1) {
    *arg0 = *arg1;
}

s32 fn_8007850C(s32 arg0, void* a1) {
    fn_8007853C(0, 0);
    return arg0;
}

u32 fn_8007853C(s32 *arg0, s32 *arg1) {
    *arg0 = *arg1;
}

s32 fn_80078548(s32 arg0, void* a1) {
    fn_80078578(0, 0);
    return arg0;
}

u32 fn_80078578(s32 *arg0, s32 *arg1) {
    *arg0 = *arg1;
}

s32 fn_80078584(s32 arg0, void* a1) {
    fn_800785B4(0, 0);
    return arg0;
}

u32 fn_800785B4(s32 *arg0, s32 *arg1) {
    *arg0 = *arg1;
}

s32 fn_800785C0(s32 arg0, s32 arg1) {
    fn_80078608(0, 0);
    fn_8006E2A8((s32)(arg0 + 4), (s32)(arg1 + 4));
    return arg0;
}

u32 fn_80078608(s32 *arg0, s32 *arg1) {
    *arg0 = *arg1;
}

s32 fn_80078614(s32 arg0, void* a1) {
    fn_80078644(0, 0);
    return arg0;
}

u32 fn_80078644(s32 *arg0, s32 *arg1) {
    *arg0 = *arg1;
}

s32 fn_80078650(s32 arg0, void* a1) {
    fn_80078680(0, 0);
    return arg0;
}

u32 fn_80078680(s32 *arg0, s32 *arg1) {
    *arg0 = *arg1;
}

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

    temp_r31 = fn_800640E4((s32)(arg1));
    return fn_800640E4((s32)(arg0)) == temp_r31;
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
    if (fn_8005AAEC(0) == 0) {
        nw4r::db::Panic((const char*)&lbl_8058EA50, 0xA5, (const char*)&lbl_8058EA30);
    }
    if (fn_8005AAEC((s32)(arg0)) != 0) {
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

s32 fn_800789C4(s32 arg0, s32 arg1) {
    fn_80078A28(0, 0);
    if ((s32) (arg1 & 3) != 0) {
        nw4r::db::Panic((const char*)&lbl_8058EA20, 0x3A, (const char*)&lbl_8058E9F4);
    }
    return arg0;
}

u32 fn_80078A28(s32 *arg0, s32 arg1) {
    *arg0 = arg1;
}

s32 fn_80078A30(s32 arg0, s32 arg1) {
    fn_80078A94(0, 0);
    if ((s32) (arg1 & 3) != 0) {
        nw4r::db::Panic((const char*)&lbl_8058EAC8, 0x26D, (const char*)&lbl_8058EAA0);
    }
    return arg0;
}

u32 fn_80078A94(s32 *arg0, s32 arg1) {
    *arg0 = arg1;
}

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
void* fn_80078DC0(void *arg0) {
    fn_8006F4BC((void*)(0), 0);
    fn_8006F41C((void*)(&((RawView_28*)arg0)->field_0x04), (s32)(0));
    fn_8007685C((s32)(&((RawView_28*)arg0)->field_0x08), (s32)(0));
    fn_800783EC((s32)(&((RawView_28*)arg0)->field_0x0C), (s32)(0));
    fn_80076794((s32)(&((RawView_28*)arg0)->field_0x10), (s32)(0));
    fn_8006F158((void*)(&((RawView_28*)arg0)->field_0x14), (s32)(0));
    fn_8006F298((void*)(&((RawView_28*)arg0)->field_0x18), (s32)(0));
    fn_8005A950((void*)(&((RawView_28*)arg0)->field_0x1C), (s32)(0));
    fn_800766D0((s32)(&((RawView_28*)arg0)->field_0x20), (s32)(0));
    fn_80076988((s32)(&((RawView_28*)arg0)->field_0x24), (s32)(0));
    fn_8006F374((void*)(&((RawView_28*)arg0)->field_0x28), (s32)(0));
    ((RawView_28*)arg0)->field_0x2C = 0;
    ((RawView_28*)arg0)->field_0x30 = 0;
    ((RawView_28*)arg0)->field_0x34 = 0;
    return arg0;
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


void fn_800793A4(s32 *arg0, s32 arg1, s32 arg2, s32 arg3, s32 arg4, s32 arg5, s32 arg6, s32 arg7, u32 arg_sp0) {
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

s32 fn_80079FB4(s32 arg0, void* a1) {
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
        return *((s32*)fn_80079FB4((s32)(&spC), (void*)(arg1)));
    }
    return *((s32*)fn_80079FB4((s32)(&sp8), (void*)(0)));
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
            fn_8004C4F0((s32)(arg6), (void*)(&((RawView_45*)temp_r3)->field_0x14));
        }
    }
}

typedef struct {
    /* +0x00 */ u8 pad_0x00[0x1A];
    /* +0x1A */ u8 field_0x1A;
    /* +0x1B */ u8 field_0x1B;
    /* +0x1C */ u8 field_0x1C;
    /* +0x1D */ u8 field_0x1D;
    /* +0x1E */ u8 field_0x1E;
    /* +0x1F */ u8 field_0x1F;
} RawView_46; /* size: 0x20 */
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

void fn_8007A400(s32 arg0) {
    u32 *var_r3;

    OSRegisterVersion((s32)(lbl_80791200));
    if (arg0 != 0) {
        fn_80502678();
    } else {
        fn_805026D8();
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
    fn_80088590((u32)(0x7FF));
}

void* fn_8007A518(s32 *arg0, s32 *arg1) {
    if (arg0 != arg1) {
        *arg0 = *arg1;
        fn_8009A748((arg0 + 4), (arg1 + 4), (u32)(0x40));
    }
    return arg0;
}

u32 fn_8007A564(s32 *arg0) {
    *arg0 = 0;
    fn_8009A910((arg0 + 4), (u32)(0x40));
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
void* dtor_8007B2D4(void *arg0, s16 arg1) {
}


u32 fn_8007B340(void* a0, void* a1) {
    MEMFreeToAllocator();
}

typedef struct {
    /* +0x00 */ u8 pad_0x00[0x10];
    /* +0x10 */ u32 (*field_0x10)(...);
} RawView_63; /* size: 0x14 */
typedef struct {
    /* +0x00 */ u8 pad_0x00[0xC];
    /* +0x0C */ u32 (*field_0x0C)(...);
} RawView_62; /* size: 0x10 */
void fn_8007B348(void **arg0) {
    void **temp_r3;

    temp_r3 = (void **)(fn_800600C0());
    if (temp_r3 != NULL) {
        ((RawView_62*)(*temp_r3))->field_0x0C(0x10001, 0, arg0);
    }
    if (arg0 != NULL) {
        ((RawView_63*)(*arg0))->field_0x10(arg0, 1);
    }
}

s32 fn_8007B3BC(void* a0) {
}


typedef struct {
    /* +0x00 */ u8 pad_0x00[0x14];
    /* +0x14 */ u32 (*field_0x14)(...);
} RawView_64; /* size: 0x18 */
void fn_8007B3EC(void **arg0) {
    s32 sp8;

    sp8 = ((RawView_64*)(*arg0))->field_0x14();
    fn_8005DC24((void*)(&sp8));
}

typedef struct {
    /* +0x00 */ u8 pad_0x00[0xE4];
    /* +0xE4 */ u32 field_0xE4;
} RawView_65; /* size: 0xE8 */
s32 fn_8007B424(void *arg0) {
    return ((RawView_65*)arg0)->field_0xE4;
}

void fn_8007B42C(u32 *arg0, u32 arg1, u32 *arg2) {
}


void fn_8007B450(void* a0) {
    u32 sp8;

    u8 spC;

    spC = sp8;
    fn_8007B47C((s32)(&spC), 0);
}

s32 fn_8007B47C(s32 arg0, s32 arg1) {
    return (s32) (arg1 - arg0) / 4;
}

void fn_8007B4E4(void* a0) {

}

void fn_8007B540(void* a0) {

}

s32 fn_8007B544(s32 arg0, u32 arg1) {
    if (arg1 < 3U) {
        return arg0 + (arg1 * 0x30) + 0xC;
    }
    return 0;
}

void fn_8007B5BC(void* a0) {

}

typedef struct {
    /* +0x00 */ u8 pad_0x00[0xCC];
    /* +0xCC */ u32 field_0xCC;
} RawView_66; /* size: 0xD0 */
s32 fn_8007B5C0(void *arg0, u32 arg1) {
    if ((arg1 < 9U) && ((s32) ((1 << (arg1 - 1)) & ((RawView_66*)arg0)->field_0xCC) != 0)) {
        return 1;
    }
    return 0;
}

s32 fn_8007B5F4(s32 arg0, s32 *arg1) {
    s32 spC;
    s32 sp8;

    spC = fn_8007B6FC(0);
    if (fn_800639D0((void*)(arg1), (void*)(&spC)) != 0) {
        return 1;
    }
    sp8 = *arg1;
    return fn_8007B660((s32)(arg0), (s32 *)(&sp8));
}

s32 fn_8007B660(s32 arg0, s32 *arg1) {
    s32 spC;
    s32 sp8;

    spC = fn_8007B6CC(0);
    if (fn_800639D0((void*)(arg1), (void*)(&spC)) != 0) {
        return 1;
    }
    sp8 = *arg1;
    return fn_80063964((s32)(arg0), (void*)(&sp8));
}

s32 fn_8007B6CC(void* a0) {
}


s32 fn_8007B6FC(void* a0) {
}


void* fn_8007B72C(s32 *arg0, s32 arg1) {
    *arg0 = arg1;
}

s32 fn_8007B734(void* a0) {
}


s32 fn_8007B764(void* a0) {
}


s32 fn_8007B794(s32 arg0, s16 arg1) {
}


s32 dtor_8007B7F0(s32 arg0, s16 arg1) {
}


s32 fn_8007B834(s32 arg0) {
    fn_8007B864(0, 0);
    return arg0;
}

u32 fn_8007B864(s32 *arg0, s32 *arg1) {
    *arg0 = *arg1;
}

s32 fn_8007B870(s32 arg1) {
    return arg1;
}

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
s32 fn_8007B9BC(void *arg0, s32 arg1) {
    return (((RawView_68*)arg0)->field_0xCC & arg1) != 0;
}

typedef struct {
    /* +0x00 */ u8 pad_0x00[0xCC];
    /* +0xCC */ u32 field_0xCC;
} RawView_69; /* size: 0xD0 */
void fn_8007B9D4(void *arg0, s32 arg1, s32 arg2) {
    if (arg2 != 0) {
        ((RawView_69*)arg0)->field_0xCC = (s32) (((RawView_69*)arg0)->field_0xCC | arg1);
        return;
    }
    ((RawView_69*)arg0)->field_0xCC = (s32) (((RawView_69*)arg0)->field_0xCC & ~arg1);
}

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

s32 dtor_8007BA44(s32 arg0, s16 arg1) {
    if (arg0 != 0) {
        dtor_800813B8((u32)(0));
        if (arg1 > 0) {
            fn_8005D3E0((void*)(arg0));
        }
    }
    return arg0;
}

void* fn_8007BAA0(u32 **arg0) {
    TheBeatMatchOutput();
    *arg0 = &lbl_8058F4C8;
    setVec3((arg0 + 0xDC), (f32)(lbl_80795E58), (f32)(lbl_80795E58), (f32)(lbl_80795E58));
    return arg0;
}

s32 fn_8007BAF0(s32 arg0, s32 *arg1) {
    s32 spC;
    s32 sp8;

    spC = fn_8007BB5C(0);
    if (fn_800639D0((void*)(arg1), (void*)(&spC)) != 0) {
        return 1;
    }
    sp8 = *arg1;
    return fn_8007B660((s32)(arg0), (s32 *)(&sp8));
}

s32 fn_8007BB5C(void* a0) {
}


void* fn_8007BB8C(s32 *arg0, s32 arg1) {
    *arg0 = arg1;
}

typedef struct {
    /* +0x00 */ u8 pad_0x00[0x140];
    /* +0x140 */ u32 field_0x140;
} RawView_70; /* size: 0x144 */
u32 fn_8007BB94(void *arg0, s32 arg1, s32 arg2) {
    s32 temp_r3;
    s32 * temp_r6;

    temp_r6 = (s32 *)(((RawView_70*)arg0)->field_0x140);
    temp_r3 = arg1 * 4;
    *(temp_r6 + temp_r3) = *(temp_r6 + temp_r3) | arg2;
}

typedef struct {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ u32 field_0x04;
    /* +0x08 */ u8 pad_0x08[0x8];
    /* +0x10 */ u32 field_0x10;
} RawView_71; /* size: 0x14 */
u32 nw4r::g3d::ScnMdl::CopiedMatAccess::GetResTexSrt(bool arg1) {
    u32 sp8;
    void* arg0 = this;

    if (((s32) ((RawView_71*)arg0)->field_0x00 != 0) && (fn_8006E324((void*)(&((RawView_71*)arg0)->field_0x10)) != 0)) {
        if (arg1 != 0) {
            fn_8007BB94((void *)(((RawView_71*)arg0)->field_0x00), (s32)(((RawView_71*)arg0)->field_0x04), (s32)(4));
        }
        return ((RawView_71*)arg0)->field_0x10;
    }
    return *((s32*)fn_8006F374((void*)(&sp8), (s32)(0)));
}

/* 0x8007BEAC - the ScnMdl::CopiedMatAccess constructor.  m2c could not decompile it (a `bctr` through
 * an unresolved jump table), so it is a compiling stub and a recorded residual. */
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

    if (((s32) ((RawView_72*)arg0)->field_0x00 != 0) && (fn_80064820((void*)(&((RawView_72*)arg0)->field_0x14)) != 0)) {
        if (arg1 != 0) {
            fn_8007BB94((void *)(((RawView_72*)arg0)->field_0x00), (s32)(((RawView_72*)arg0)->field_0x04), (s32)(8));
        }
        return ((RawView_72*)arg0)->field_0x14;
    }
    return *((s32*)fn_8005A950((void*)(&sp8), (s32)(0)));
}

typedef struct {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ u32 field_0x04;
    /* +0x08 */ u8 pad_0x08[0x10];
    /* +0x18 */ u32 field_0x18;
} RawView_73; /* size: 0x1C */
s32 fn_8007BCAC(void *arg0, s32 arg1) {
    u32 sp8;

    if (((s32) ((RawView_73*)arg0)->field_0x00 != 0) && (fn_800768C8((s32 *)(&((RawView_73*)arg0)->field_0x18)) != 0)) {
        if (arg1 != 0) {
            fn_8007BB94((void *)(((RawView_73*)arg0)->field_0x00), (s32)(((RawView_73*)arg0)->field_0x04), (s32)(0x10));
        }
        return ((RawView_73*)arg0)->field_0x18;
    }
    return *((s32*)fn_8007685C((s32)(&sp8), (s32)(0)));
}

typedef struct {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ u32 field_0x04;
    /* +0x08 */ u8 pad_0x08[0x14];
    /* +0x1C */ u32 field_0x1C;
} RawView_74; /* size: 0x20 */
s32 fn_8007BD2C(void *arg0, s32 arg1) {
    u32 sp8;

    if (((s32) ((RawView_74*)arg0)->field_0x00 != 0) && (fn_80076974((s32 *)(&((RawView_74*)arg0)->field_0x1C)) != 0)) {
        if (arg1 != 0) {
            fn_8007BB94((void *)(((RawView_74*)arg0)->field_0x00), (s32)(((RawView_74*)arg0)->field_0x04), (s32)(0x20));
        }
        return ((RawView_74*)arg0)->field_0x1C;
    }
    return *((s32*)fn_80076988((s32)(&sp8), (s32)(0)));
}

typedef struct {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ u32 field_0x04;
    /* +0x08 */ u8 pad_0x08[0x18];
    /* +0x20 */ u32 field_0x20;
} RawView_75; /* size: 0x24 */
s32 fn_8007BDAC(void *arg0, s32 arg1) {
    u32 sp8;

    if (((s32) ((RawView_75*)arg0)->field_0x00 != 0) && (fn_80076800((s32 *)(&((RawView_75*)arg0)->field_0x20)) != 0)) {
        if (arg1 != 0) {
            fn_8007BB94((void *)(((RawView_75*)arg0)->field_0x00), (s32)(((RawView_75*)arg0)->field_0x04), (s32)(0x80));
        }
        return ((RawView_75*)arg0)->field_0x20;
    }
    return *((s32*)fn_80076794((s32)(&sp8), (s32)(0)));
}

typedef struct {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ u32 field_0x04;
    /* +0x08 */ u8 pad_0x08[0x1C];
    /* +0x24 */ u32 field_0x24;
} RawView_76; /* size: 0x28 */
s32 fn_8007BE2C(void *arg0, s32 arg1) {
    u32 sp8;

    if (((s32) ((RawView_76*)arg0)->field_0x00 != 0) && (fn_80076750((s32 *)(&((RawView_76*)arg0)->field_0x24)) != 0)) {
        if (arg1 != 0) {
            fn_8007BB94((void *)(((RawView_76*)arg0)->field_0x00), (s32)(((RawView_76*)arg0)->field_0x04), (s32)(0x100));
        }
        return ((RawView_76*)arg0)->field_0x24;
    }
    return *((s32*)fn_8006F158((void*)(&sp8), (s32)(0)));
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

/* 0x8007B48C / 0x8007B4E8 / 0x8007B564: vtable slot +0x14, guarded by the +0xDA bit 2 and the +0xD8 mask. */
void fn_8007B48C(void* self, u32 mask, void* a5, void* a6)
{
    DispatchObj* o = (DispatchObj*)self;
    void* p = o->field_0xD4;
    if (p == 0 || (o->field_0xDA & 4) == 0)
        return;
    if ((o->field_0xD8 & mask) == 0)
        return;
    ((DispatchVtbl*)*(void**)p)->method_0x14(p, mask, self, a5, a6);
}
void fn_8007B4E8(void* self, u32 mask, void* a5, void* a6)
{
    DispatchObj* o = (DispatchObj*)self;
    void* p = o->field_0xD4;
    if (p == 0 || (o->field_0xDA & 4) == 0)
        return;
    if ((o->field_0xD8 & mask) == 0)
        return;
    ((DispatchVtbl*)*(void**)p)->method_0x14(p, mask, self, a5, a6);
}
void fn_8007B564(void* self, u32 mask, void* a5, void* a6)
{
    DispatchObj* o = (DispatchObj*)self;
    void* p = o->field_0xD4;
    if (p == 0 || (o->field_0xDA & 4) == 0)
        return;
    if ((o->field_0xD8 & mask) == 0)
        return;
    ((DispatchVtbl*)*(void**)p)->method_0x14(p, mask, self, a5, a6);
}
/* 0x8007B8E4 / 0x8007B940: vtable slot +0x1C, guarded by the +0xDA bit 5 / bit 4 and the +0xD8 mask. */
void fn_8007B8E4(void* self, u32 mask, void* a5, void* a6)
{
    DispatchObj* o = (DispatchObj*)self;
    void* p = o->field_0xD4;
    if (p == 0 || (o->field_0xDA & 0x20) == 0)
        return;
    if ((o->field_0xD8 & mask) == 0)
        return;
    ((DispatchVtbl*)*(void**)p)->method_0x1C(p, mask, self, a5, a6);
}
void fn_8007B940(void* self, u32 mask, void* a5, void* a6)
{
    DispatchObj* o = (DispatchObj*)self;
    void* p = o->field_0xD4;
    if (p == 0 || (o->field_0xDA & 0x10) == 0)
        return;
    if ((o->field_0xD8 & mask) == 0)
        return;
    ((DispatchVtbl*)*(void**)p)->method_0x1C(p, mask, self, a5, a6);
}
}
