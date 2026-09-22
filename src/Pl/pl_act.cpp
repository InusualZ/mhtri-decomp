/*
 * Player action module (Pl_act): the largest of the three Pl clusters. .text 0x80276B58-0x8027D684
 * (115 functions, 0x6B2C B) with its own exception tables - extab 0x8001294C-0x80012B54, extabindex
 * 0x8002FC70-0x8002FF7C.
 *
 * Left edge pinned by the `.sdata2` pool run (`lbl_8079A0AC`), right edge by the closure; the reasoning is in
 * configure.py beside the Pl lib entry.
 *
 * It is C++ (the map holds the mangled `Pl_attack_set_sub__FP4_PLWP9_HIT_DATAP6_HIT_WUs`,
 * `Pl_suimen_ck__FP4_PLW`, `Pl_get_gunner_pos__FP4_PLWPQ34nw4r4math4VEC3l`, ...), so the actor type is
 * `_PLW` - that spelling is what the map's mangling encodes - and every unmangled `fn_*` callee is
 * `extern "C"`.
 *
 * Flags - measured against `build/RMHE08/obj/Pl/pl_act.o`, and **the store needs two changes** (both are a
 * this-unit `cflags_pl_act`, never `cflags_base`):
 *   * `-O4,p` -> **`-O3`**. `-O4,p` implies `-func_align 16`; the retail function starts are packed on 4 B
 *     (+0x23c, +0x588, +0xcbc), which alone rules it out. On the codegen axis `-O4,p` also loses badly:
 *     fn_80276B58 85.9 / fn_80276CE8 84.7 / fn_80276D94 86.1 / fn_80276E08 87.9, against `-O3`'s
 *     97.3 / 90.0 / 96.4 / 89.6 (same source).
 *   * peephole **off** (`-opt nopeephole`, i.e. `-O3 -opt nopeephole`): the retail object carries exactly
 *     one record-form instruction in all 115 functions (`andi. r0,r0,20` at 0xcbc, an `x & 0x20`
 *     truth test from instruction selection), so the peephole's sign/zero-extend compare fusion never
 *     fired in the retail build. With it on, fn_80276B58 = 97.3 / fn_80276CE8 = 90.0 /
 *     fn_80276D94 = 96.4 / fn_80276E08 = 89.6; with it off, 100.0 / 92.7 / 100.0 / 94.0.
 *     `-O4,p -opt nopeephole` is worse still (92.5 / 87.4 / 78.7 / 93.9), so the level is `-O3`.
 *   * `-inline auto` -> **`-inline noauto`** (playbook 28): with `auto` the 46-instruction
 *     `fn_802770E8` is inlined into all three arms of `fn_802771A0` (57 -> 229 instructions, 0.00 %);
 *     `noauto` puts it back at 100.00 % and moves nothing else. `-inline off` measures identically here,
 *     but `noauto` is the spelling the sibling `main.cpp` needed, so it is the one to commit.
 *   * `-func_align` stays out: `-O3` already packs on 4 B, which is what the retail starts want.
 *   * the s16-parameter convention: retail sign-extends a `s16` parameter at its first use and keeps the
 *     *raw* register live, which MWCC only does when the parameter is declared `s32` and cast to `s16` at
 *     each use (`fn_80276D94`: `s16 arg1` truncates the `+= 150` and costs 96.4 %, `s32 arg1` + `(s16)`
 *     casts is byte-identical). The unmangled `fn_*` symbols are free to be declared this way, but a
 *     callee's *declaration* has to match its definition's type, so the shared `fn_80276CE8` is declared
 *     `s32` and its call sites cast explicitly.
 *
 * Residual (work in progress - the functions below 100 %, each measured with the flags above, i.e.
 * `-O3 -opt nopeephole -inline noauto`):
 *   * the unclaimed `.sdata2` pool (playbook 23) is the single largest residual class: MWCC synthesises
 *     the `(f32)(s32)` magic constant as a private `@NN` local where retail `lfd f1, lbl_8079A0A0@sda21`.
 *     That one ARG-only row holds down every function that converts an integer to float (B58, 7A08C,
 *     7D5A4, 7C8B4, Pl_get_gunner_vec, 7CD0C, Get_Shell_rate_adj) and is its only residual in four of
 *     them. Naming it needs the unit's `.sdata2` range in `splits.txt`, not a source change - the
 *     constants are already `extern`-declared under their map names.
 *   * `int -> s16`/`s8` store conversion (A044, CA48, 76CE8, 76E08): retail stores the *raw* int sum
 *     (`add r5,r0,r4; sth r5`) and sign-extends only for the compare that follows (`extsh r0,r5`); our
 *     build materialises the s16 value at the store and reuses it. A compound assignment stores raw only
 *     when the RHS is already the field's type (`+= (s8)arg1`) or the operator is `--`/`++`; with a wider
 *     RHS MWCC inserts the conversion. Eight source shapes per function gave the same or an extra row.
 *   * a dead second test whose condition register retail reuses (AF34): keeping the pre-decrement value
 *     in an `s16` local and writing an unreachable `if (v < 0) return;` after the store reproduces
 *     `blelr ... sth ... bltlr` exactly, and is load-bearing - do not "clean it up".
 *   * load-before-pointer-formation (79414, Get_Shell_rate_adj): retail does `lbz r0,0x1e8(r30)` before
 *     `addi r3,r30,0x1e8`, ours forms the pointer first. Two shapes tried (field vs offset cast, a local
 *     for the byte) do not move it.
 *   * boolean-chain layout (7BC48, 784B8, CB1C, D40C): the `||`/`&&` short-circuit blocks, and the two
 *     induction variables of the 32-bit scan, come out in different registers/order; codes are equivalent
 *     but the rows do not pair. 7CFC0 additionally gets two extra `extsh` before its clamp stores.
 *   * `mulli` vs the shift/subf/shift form of `* 14` (78590): MWCC folds `* 7 * 2` before strength
 *     reduction, so the three-instruction form cannot be recovered from a constant product.
 *
 * Written so far: 90 of the 115 bodies, 64 of them byte-exact; the residual list above names the rest.
 * The 25 unwritten bodies are all >= 170 B: 78D1C (920), A57C (1668), C208 (1684), B358 (1472), B918
 * (816), D0D4 (796), 79C20 (668), B0BC (668), 7993C (584), AC2C (508), BE4C (484), 79490 (480),
 * Get_Shell_bure_type (476), 77974 (464), A340 (428), C064 (420), 78674 (416), 791FC (356), 77FF8 (332),
 * 79EBC (324), 78310 (320), 7885C (312), 7AF88-done, 78144 (372), 77DAC (276), 77C94-done, 7D40C-done,
 * Pl_attack_set_sub (1776). `Pl_attack_set_sub` is the seam between the seven original bodies and the
 * appended run.
 *
 * Source-order caveat: the bodies were appended in per-batch address order, not as one address-ordered
 * list, so the file is *not* in `.text` order any more. The object's function layout is source order, so
 * the file has to be sorted into address order (and the 25 missing bodies added) before this unit can be
 * linked, even at 100 %.
 *
 * Load-bearing source shapes (do not "simplify"):
 *   * an `s16`/`s8` *parameter* (not `s32` + a cast) is what keeps the raw register live for a store while
 *     the comparison still sign-extends at its use (79154, 79194, C89C).
 *   * a `u16` id equality needs a signed local (`s32 id = self->unk00C;`) to pair as `cmpwi` rather than
 *     `cmplwi` (A198, C030, 78C7C, 78CD0, A4EC, BCE0); the range tests stay `(u32)(id - lo) <= n`.
 *   * counted loops unroll with `mtctr`: `i < 10` -> by 1, `i < 24` -> by 4 (BCE0), `i < 32` -> by 8 with
 *     `mtctr 4` (D40C).
 *   * one-off field offsets are written as `*(s16*)((u8*)self + N)` (789EC, BCE0, AF88); the struct only
 *     names fields more than one function touches. 0x278 is a 24-entry {u16 id; s16 count} table.
 *   * the exported helpers are C++ functions so the compiler emits the map's mangled names (Pl_bari_ck,
 *     Pl_condition_ck, Pl_dm_condition_ck, Pl_suimen_ck, Pl_get_gunner_pos/vec, Pl_zanzo_set,
 *     Get_Shell_rate_adj); every `fn_*` callee and the SDK entry points are `extern "C"`.
 *   * MWCC emits `b <callee>` plus a dead `blr` for a void tail call (BE2C).
 */

#include "types.h"

/* The actor the whole Pl_* family takes as its first argument. Only the offsets this unit touches are named;
 * everything in between is padding. */
/* The gunner helpers take the engine's vector types; the names are what the map's mangling encodes. */
struct _CP_VECTOR {
    u32 x;
    u32 y;
    u32 z;
};

namespace nw4r {
namespace math {
struct VEC3 {
    f32 x;
    f32 y;
    f32 z;
};
struct MTX34 {
    f32 m[3][4];
};
}  // namespace math
}  // namespace nw4r

extern u8 lbl_806AB848[];

struct _PLW {
    /* 0x000 */ u8 unk000[2];
    /* 0x002 */ u8 unk002;
    /* 0x003 */ u8 unk003[0x08 - 0x03];
    /* 0x008 */ u8 unk008;
    /* 0x009 */ u8 unk009;
    /* 0x00A */ u8 unk00A;
    /* 0x00B */ u8 unk00B;
    /* 0x00C */ u16 unk00C;
    /* 0x00E */ u8 unk00E[0x15 - 0x0E];
    /* 0x015 */ u8 unk015;
    /* 0x016 */ u8 unk016;
    /* 0x017 */ u8 unk017[0x18 - 0x17];
    /* 0x018 */ u8 unk018;
    /* 0x019 */ u8 unk019[0x20 - 0x19];
    /* 0x020 */ u32 unk020;
    /* 0x024 */ u8 unk024[0x3C - 0x24];
    /* 0x03C */ f32 unk03C;
    /* 0x040 */ f32 unk040;
    /* 0x044 */ f32 unk044;
    /* 0x048 */ u8 unk048[0x54 - 0x48];
    /* 0x054 */ u32 unk054;
    /* 0x058 */ u32 unk058;
    /* 0x05C */ u8 unk05C[0x64 - 0x5C];
    /* 0x064 */ f32 unk064;
    /* 0x068 */ u8 unk068[0x74 - 0x68];
    /* 0x074 */ u8 unk074;
    /* 0x075 */ u8 unk075[0x90 - 0x75];
    /* 0x090 */ f32 unk090[3];
    /* 0x09C */ f32 unk09C;
    /* 0x0A0 */ f32 unk0A0;
    /* 0x0A4 */ f32 unk0A4;
    /* 0x0A8 */ u8 unk0A8[0xB4 - 0xA8];
    /* 0x0B4 */ s16 unk0B4;
    /* 0x0B6 */ u8 unk0B6[0x13C - 0xB6];
    /* 0x13C */ u8* unk13C;
    /* 0x140 */ u8 unk140[0x264 - 0x140];
    /* 0x264 */ s16 unk264;
    /* 0x266 */ u8 unk266[0x269 - 0x266];
    /* 0x269 */ u8 unk269;
    /* 0x26A */ u8 unk26A;
    /* 0x26B */ u8 unk26B;
    /* 0x26C */ u8 unk26C;
    /* 0x26D */ u8 unk26D;
    /* 0x26E */ u16 unk26E;
    /* 0x270 */ s16 unk270;
    /* 0x272 */ u8 unk272[0x276 - 0x272];
    /* 0x276 */ u8 unk276;
    /* 0x277 */ u8 unk277[0x306 - 0x277];
    /* 0x306 */ u16 unk306;
    /* 0x308 */ u8 unk308[0x30C - 0x308];
    /* 0x30C */ u8 unk30C;
    /* 0x30D */ u8 unk30D;
    /* 0x30E */ u8 unk30E;
    /* 0x30F */ u8 unk30F[0x313 - 0x30F];
    /* 0x313 */ s8 unk313;
    /* 0x314 */ u8 unk314;
    /* 0x315 */ u8 unk315[0x318 - 0x315];
    /* 0x318 */ u32 unk318;
    /* 0x31C */ s16 unk31C;
    /* 0x31E */ s16 unk31E;
    /* 0x320 */ s16 unk320;
    /* 0x322 */ u8 unk322[16];
    /* 0x332 */ u8 unk332[0x354 - 0x332];
    /* 0x354 */ f32 unk354;
    /* 0x358 */ f32 unk358;
    /* 0x35C */ u8 unk35C[0x364 - 0x35C];
    /* 0x364 */ u32 unk364;
    /* 0x368 */ u8 unk368;
    /* 0x369 */ u8 unk369;
    /* 0x36A */ u8 unk36A;
    /* 0x36B */ u8 unk36B;
    /* 0x36C */ s16 unk36C;
    /* 0x36E */ u8 unk36E[0x370 - 0x36E];
    /* 0x370 */ s16 unk370;
    /* 0x372 */ u8 unk372[0x376 - 0x372];
    /* 0x376 */ s16 unk376;
    /* 0x378 */ s16 unk378;
    /* 0x37A */ s16 unk37A;
    /* 0x37C */ s16 unk37C;
    /* 0x37E */ u8 unk37E[0x384 - 0x37E];
    /* 0x384 */ s16 unk384;
    /* 0x386 */ s16 unk386;
    /* 0x388 */ u8 unk388;
    /* 0x389 */ u8 unk389[0x38A - 0x389];
    /* 0x38A */ s16 unk38A;
    /* 0x38C */ s16 unk38C;
    /* 0x38E */ s16 unk38E;
    /* 0x390 */ s16 unk390;
    /* 0x392 */ s16 unk392;
    /* 0x394 */ s16 unk394;
    /* 0x396 */ s16 unk396;
    /* 0x398 */ u8 unk398[0x39E - 0x398];
    /* 0x39E */ u8 unk39E;
    /* 0x39F */ u8 unk39F[0x3A2 - 0x39F];
    /* 0x3A2 */ s8 unk3A2;
    /* 0x3A3 */ s8 unk3A3;
    /* 0x3A4 */ u8 unk3A4[0x3AC - 0x3A4];
    /* 0x3AC */ u32 unk3AC;
    /* 0x3B0 */ u8 unk3B0[0x3B4 - 0x3B0];
    /* 0x3B4 */ u8 unk3B4;
    /* 0x3B5 */ u8 unk3B5;
    /* 0x3B6 */ u8 unk3B6[0x3D8 - 0x3B6];
    /* 0x3D8 */ u32 unk3D8;
    /* 0x3DC */ u32 unk3DC;
    /* 0x3E0 */ u32 unk3E0;
    /* 0x3E4 */ u8 unk3E4[0x3EC - 0x3E4];
    /* 0x3EC */ s16 unk3EC;
    /* 0x3EE */ u8 unk3EE[0x3F2 - 0x3EE];
    /* 0x3F2 */ s16 unk3F2;
    /* 0x3F4 */ u8 unk3F4[0x3F8 - 0x3F4];
    /* 0x3F8 */ s16 unk3F8;
    /* 0x3FA */ u8 unk3FA[0x3FE - 0x3FA];
    /* 0x3FE */ s16 unk3FE;
    /* 0x400 */ u8 unk400[0x404 - 0x400];
    /* 0x404 */ s16 unk404;
    /* 0x406 */ u8 unk406[0x40E - 0x406];
    /* 0x40E */ s16 unk40E;
    /* 0x410 */ u8 unk410[0x414 - 0x410];
    /* 0x414 */ s16 unk414;
    /* 0x416 */ s16 unk416;
    /* 0x418 */ u8 unk418[0x41A - 0x418];
    /* 0x41A */ s16 unk41A;
    /* 0x41C */ s16 unk41C;
    /* 0x41E */ u8 unk41E[0x420 - 0x41E];
    /* 0x420 */ s16 unk420;
    /* 0x422 */ s16 unk422;
    /* 0x424 */ s16 unk424;
    /* 0x426 */ s16 unk426;
    /* 0x428 */ s16 unk428;
    /* 0x42A */ s16 unk42A;
    /* 0x42C */ s16 unk42C;
    /* 0x42E */ u8 unk42E[0x448 - 0x42E];
    /* 0x448 */ s8 unk448;
    /* 0x449 */ s8 unk449;
    /* 0x44A */ u8 unk44A[0x44C - 0x44A];
    /* 0x44C */ s8 unk44C;
    /* 0x44D */ s8 unk44D;
    /* 0x44E */ u8 unk44E[0x45A - 0x44E];
    /* 0x45A */ s16 unk45A;
    /* 0x45C */ u8 unk45C[0x466 - 0x45C];
    /* 0x466 */ s16 unk466;
    /* 0x468 */ s16 unk468;
    /* 0x46A */ s16 unk46A;
    /* 0x46C */ u8 unk46C;
    /* 0x46D */ u8 unk46D;
    /* 0x46E */ u8 unk46E;
    /* 0x46F */ u8 unk46F;
    /* 0x470 */ s16 unk470;
    /* 0x472 */ u8 unk472[0x489 - 0x472];
    /* 0x489 */ u8 unk489;
    /* 0x48A */ u8 unk48A[0x492 - 0x48A];
    /* 0x492 */ u8 unk492;
    /* 0x493 */ u8 unk493[0x4DC - 0x493];
    /* 0x4DC */ u8 unk4DC;
    /* 0x4DD */ u8 unk4DD[0x4E5 - 0x4DD];
    /* 0x4E5 */ u8 unk4E5;
    /* 0x4E6 */ u8 unk4E6[0x4EE - 0x4E6];
    /* 0x4EE */ u8 unk4EE;
    /* 0x4EF */ u8 unk4EF[0x538 - 0x4EF];
    /* 0x538 */ u8 unk538;
    /* 0x539 */ u8 unk539[0x580 - 0x539];
    /* 0x580 */ s16 unk580;
    /* 0x582 */ u8 unk582;
    /* 0x583 */ s8 unk583;
    /* 0x584 */ u8 unk584[0x59C - 0x584];
    /* 0x59C */ u16 unk59C;
    /* 0x59E */ u16 unk59E;
    /* 0x5A0 */ u16 unk5A0;
    /* 0x5A2 */ u8 unk5A2[0x5A4 - 0x5A2];
    /* 0x5A4 */ u16 unk5A4;
    /* 0x5A6 */ u8 unk5A6;
    /* 0x5A7 */ u8 unk5A7[0x5BB - 0x5A7];
    /* 0x5BB */ u8 unk5BB;
    /* 0x5BC */ u8 unk5BC[0x5C4 - 0x5BC];
    /* 0x5C4 */ u8 unk5C4;
    /* 0x5C5 */ u8 unk5C5[0x5E5 - 0x5C5];
    /* 0x5E5 */ u8 unk5E5;
    /* 0x5E6 */ u8 unk5E6;
    /* 0x5E7 */ u8 unk5E7;
    /* 0x5E8 */ s16 unk5E8;
    /* 0x5EA */ u8 unk5EA[0x64F - 0x5EA];
    /* 0x64F */ s8 unk64F;
    /* 0x650 */ u8 unk650[0x65E - 0x650];
    /* 0x65E */ u8 unk65E;
    /* 0x65F */ u8 unk65F[0x662 - 0x65F];
    /* 0x662 */ s16 unk662;
    /* 0x664 */ s16 unk664;
    /* 0x666 */ u16 unk666;
};

/* The actors this unit calls into; the mangling of the source names reproduces the map's spellings. */
s32 Pl_master_ck(_PLW*);
u32 Pl_Skill_ck(_PLW*, u16);
u32 Pl_cat_skill_ck(_PLW*, u16);

extern "C" u32 fn_8027681C(_PLW*);
extern "C" void fn_80276868(_PLW*, s16);
extern "C" void fn_80276CE8(_PLW*, s32);
extern "C" u8 fn_802B0598(u8);
extern "C" s32 fn_80331104(void);
extern "C" void fn_8010D688(_PLW*);
extern "C" u32 fn_8026FE44(_PLW*);
extern "C" void fn_800E1640(u8*, f32);
extern "C" u32 fn_802B0688(void*);
extern "C" void fn_8026FEF0(_PLW*, s32);
extern "C" void fn_80275AC4(_PLW*, s32, u16, u16);
u8* get_move_work_adrs(u8);

/* In-unit callees that the appended bodies reference before their own definition. */
extern "C" void fn_802789EC(_PLW*, s32);
extern "C" void fn_8027A17C(_PLW*);
extern "C" u32 fn_80278144(u8, u8*, u8);
extern "C" u32 fn_80278310(u8, u8*, u8);
extern "C" void fn_80278B58(_PLW*, s32, s32);

/* condition-bit tests over the actor's flag words, one per flag set. */
u32 Pl_condition_ck(_PLW*, u32);
u32 Pl_dm_condition_ck(_PLW*, u32);

s16 Get_motion_no(_PLW*);

u32 Pl_act_ck(_PLW*, u8, u16);
u32 Pl_bari_ck(_PLW*, s32);
void Pl_get_gunner_vec(_PLW*, _CP_VECTOR*);
void Pl_get_gunner_pos(_PLW*, nw4r::math::VEC3*, s32);
void cpSetRotMatrixZXY(_CP_VECTOR*, nw4r::math::MTX34*);
void rotVecXYZ(nw4r::math::VEC3*, _CP_VECTOR*);

extern "C" void fn_80043EA8(nw4r::math::VEC3*);
extern "C" void fn_800FC0D4(_CP_VECTOR*, void*);
extern "C" f32 fn_80050EF4(void*, void*);
extern "C" u32 fn_80114C20(_PLW*, s32);
extern "C" u32 fn_802B0668(u8);
extern "C" u32 fn_802753E4(_PLW*, s32);
extern "C" void fn_80278D1C(_PLW*);
u8 get_now_mapno(void);
u32 Pl_frame_check(_PLW*, u32, f32, f32);
f32 GetGroundHit2(nw4r::math::VEC3*, u32, u8, u8*);
void rotVecY(nw4r::math::VEC3*, u32);

extern "C" s32 fn_802E5CFC(s32);
extern "C" u32 fn_8042CB9C(s32);
extern "C" u32 fn_800CF384(void);
extern "C" void fn_80338E04(s32, u8, u8);
extern "C" void fn_80272E30(_PLW*, u16, s16);
extern "C" void fn_802E5D68(u16);

extern "C" s32 fn_8026A644(_PLW*, s32);
extern "C" u8 fn_802748C8(_PLW*);
u32 GetItemData(u16);

extern "C" u32 fn_8027BCE0(_PLW*);
extern "C" s32 fn_80272C80(_PLW*, u8);
extern "C" u8 fn_80274B20(u16);
extern "C" s16 fn_80272CC8(_PLW*, u8);
extern "C" u8 fn_80274D98(_PLW*, u8);
extern "C" u32 fn_8028732C(_PLW*);
extern "C" u8* fn_8027ED18(void*);
extern "C" u8* fn_80279360(u8*, u8);

extern const f32 lbl_8079A088;
extern const f32 lbl_8079A08C;
extern const f32 lbl_8079A090;
extern const f32 lbl_8079A094;
extern const f32 lbl_8079A098;
extern const f32 lbl_8079A09C;
extern const f64 lbl_8079A0A0;
extern const f32 lbl_8079A084;
extern const f32 lbl_8079A0C8;
extern const f32 lbl_8079A0CC;
extern const f32 lbl_8079A0D0;
extern const f32 lbl_8079A0DC;
extern const f32 lbl_8079A0E0;
extern const f32 lbl_8079A0E4;
extern const f32 lbl_8079A0E8;
extern const f32 lbl_8079A0FC;
extern const f32 lbl_8079A0F4;
extern const f32 lbl_8079A100;
extern const f32 lbl_8079A104;
extern const f32 lbl_8079A108;
extern const f32 lbl_8079A10C;
extern const f32 lbl_8079A0C4;
extern const f32 lbl_8079A0EC;
extern const f32 lbl_8079A110;
extern u8* lbl_805BFFA8[];

extern u32 lbl_805BF448[];
extern u32 lbl_805BF46C[];
extern u32 lbl_805E2248[];
extern u32 lbl_805E25D0[];
extern u8 lbl_805BF5E0[];

/* 0x80276B58: the attack-scale multiplier the actor's active skills grant for a signed modifier. */
extern "C" void fn_80276B58(_PLW* self, s32 arg1)
{
    if (Pl_master_ck(self) != 0) {
        f32 f = (f32)(s16)arg1;
        if ((s16)arg1 < 0) {
            if (Pl_Skill_ck(self, 0xB9) == 1) {
                if (Pl_cat_skill_ck(self, 4) == 1) {
                    f *= lbl_8079A088;
                } else {
                    f *= lbl_8079A08C;
                }
            } else if (Pl_Skill_ck(self, 0xBA) == 1) {
                f *= lbl_8079A088;
            } else if (Pl_Skill_ck(self, 0xBB) == 1) {
                if (Pl_cat_skill_ck(self, 4) == 1) {
                    f *= lbl_8079A090;
                } else {
                    f *= lbl_8079A094;
                }
            } else if (Pl_Skill_ck(self, 0xBC) == 1) {
                if (Pl_cat_skill_ck(self, 4) == 1) {
                    f *= lbl_8079A098;
                } else {
                    f *= lbl_8079A09C;
                }
            } else if (Pl_cat_skill_ck(self, 4) == 1) {
                f *= lbl_8079A08C;
            }
        }
        fn_80276868(self, (s16)f);
    }
}

/* 0x80276CE8: adds a signed amount to the actor's stamina pool and clamps it, with a lower bound reset. */
extern "C" void fn_80276CE8(_PLW* self, s32 arg1)
{
    if (Pl_master_ck(self) == 0) {
        return;
    }
    if ((s16)arg1 < 0 && fn_8027681C(self) == 1) {
        return;
    }
    {
        s16 v = self->unk37A + (s16)arg1;
        self->unk37A = v;
        if (v <= 0x96) {
            self->unk37A = 0x96;
        } else if (v > 0x384) {
            self->unk37A = 0x384;
            self->unk37C = 0x2A30;
        }
        v = self->unk37A;
        if (self->unk378 > v) {
            self->unk378 = v;
        }
    }
}

/* 0x80276D94: applies the stamina bonus the two armour skills grant, then the signed amount. */
extern "C" void fn_80276D94(_PLW* self, s32 arg1)
{
    if ((s16)arg1 > 0 && (Pl_Skill_ck(self, 0x48) == 1 || Pl_Skill_ck(self, 0x49) == 1)) {
        arg1 += 150;
    }
    fn_80276CE8(self, (s16)arg1);
}

/* 0x80276E08: recomputes the actor's attack-range/level modifier from its weapon class and skills. */
extern "C" void fn_80276E08(_PLW* self)
{
    if (Pl_master_ck(self) != 0) {
        s16 cls = fn_802B0598(self->unk016);
        s32 v = 0;
        if (cls == 2 || cls == 4) {
            if (self->unk466 == 0 && (cls != 2 || (Pl_Skill_ck(self, 0x80) != 1 && Pl_Skill_ck(self, 0x81) != 1))
                && (cls != 4 || Pl_Skill_ck(self, 0x81) != 1)) {
                if (cls == 2) {
                    if (Pl_Skill_ck(self, 0x82) == 1) {
                        v = 3;
                    } else if (Pl_Skill_ck(self, 0x83) == 1) {
                        v = 4;
                    } else {
                        v = 2;
                    }
                } else if (Pl_Skill_ck(self, 0x80) == 1) {
                    v = 1;
                } else if (Pl_Skill_ck(self, 0x82) == 1) {
                    v = 4;
                } else if (Pl_Skill_ck(self, 0x83) == 1) {
                    v = 6;
                } else {
                    v = 3;
                }
            }
        } else {
            v = 0;
        }
        if (Pl_Skill_ck(self, 0x45) == 0) {
            if (self->unk009 == 3) {
                if ((self->unk020 & 1) == 0 && (Pl_Skill_ck(self, 0x44) != 1 || (self->unk020 & 3) != 0)) {
                    v += 1;
                    if (Pl_Skill_ck(self, 0x47) == 1) {
                        v += 1;
                    } else if (Pl_Skill_ck(self, 0x46) == 1 && (self->unk020 & 3) == 0) {
                        v += 1;
                    }
                }
            } else if (Pl_Skill_ck(self, 0x44) != 1 || (self->unk020 & 1) != 0) {
                v += 1;
                if (Pl_Skill_ck(self, 0x47) == 1) {
                    v += 1;
                } else if (Pl_Skill_ck(self, 0x46) == 1 && (self->unk020 & 1) == 0) {
                    v += 1;
                }
            }
        }
        if ((s16)v > 0) {
            s32 t = self->unk37C - v;
            self->unk37C = t;
            if ((s16)t <= 0) {
                if (fn_8027681C(self) == 1) {
                    self->unk37C = 1;
                    return;
                }
                self->unk37C = 0x2A30;
                fn_80276CE8(self, -0x96);
            }
        }
    }
}

/* 0x802770E0 */
extern "C" s32 fn_802770E0(void)
{
    return 0;
}

/* 0x802770E8: initialises the actor's attack-range state - the id/count words, the zeroed tail fields and
 * the 16-byte per-range table. */
extern "C" void fn_802770E8(_PLW* self, u32 table, s32 arg2)
{
    self->unk318 = table;
    self->unk313 = (s8)arg2;
    self->unk314 = 0;
    self->unk31C = 0;
    self->unk320 = 0;
    self->unk31E = 0;
    for (s16 i = 0; i < 48; i++) {
        self->unk322[i] = 0;
    }
}

/* 0x802771A0: picks the attack-range table for the actor's weapon class and initialises it. */
extern "C" void fn_802771A0(_PLW* self, s32 arg1)
{
    if (self->unk009 != 3) {
        if (self->unk002 == 8 && fn_80331104() == 0) {
            fn_802770E8(self, (u32)lbl_805E2248, (s16)arg1);
        } else {
            fn_802770E8(self, lbl_805BF448[self->unk002], (s16)arg1);
        }
    } else {
        if (self->unk002 == 8 && fn_80331104() == 0) {
            fn_802770E8(self, (u32)lbl_805E25D0, (s16)arg1);
        } else {
            fn_802770E8(self, lbl_805BF46C[self->unk002], (s16)arg1);
        }
    }
}

/* 0x80277B44 */
extern "C" void fn_80277B44(_PLW* self, s16 arg1)
{
    self->unk396 = arg1;
}

/* 0x80277B4C: scales the actor's attack-range modifier by the two armour skills. */
extern "C" void fn_80277B4C(_PLW* self, s16 arg1)
{
    self->unk396 = arg1;
    if (Pl_cat_skill_ck(self, 28) == 1) {
        self->unk396 = (s16)(self->unk396 * 3);
    } else if (Pl_cat_skill_ck(self, 29) == 1) {
        self->unk396 = (s16)(self->unk396 * 2);
    }
}

/* 0x80277C48 */
extern "C" void fn_80277C48(_PLW* self, s16 arg1)
{
    self->unk580 = arg1;
}

/* 0x80277C50 */
extern "C" void fn_80277C50(_PLW* self, s16 arg1)
{
    self->unk45A = arg1;
}

/* 0x80277FE0 */
extern "C" s32 fn_80277FE0(_PLW* self)
{
    return 1;
}

/* 0x80277FE8 */
extern "C" u32 fn_80277FE8(_PLW* self)
{
    return (u32)(self->unk30C - 1) >> 31;
}

/* 0x802784A8 */
extern "C" s32 fn_802784A8(_PLW* self)
{
    return (u32)(self->unk30D - 3) >> 31;
}

/* 0x80278564 */
extern "C" void fn_80278564(_PLW* self, u32 arg1)
{
    self->unk388 |= (u8)arg1;
}

/* 0x80278578 */
extern "C" s32 fn_80278578(_PLW* self, u32 arg1)
{
    return (self->unk388 & (u16)arg1) == 0;
}

/* 0x80276E08 sibling: whether the actor is inside the invulnerability window the suimen skill
 * grants (a +/- one-unit band around its current height). */
s32 Pl_suimen_ck(_PLW* self)
{
    if (self->unk074 != 0) {
        if (self->unk040 >= self->unk064 - lbl_8079A0CC && self->unk040 <= lbl_8079A0CC + self->unk064) {
            return 1;
        }
    }
    return 0;
}

/* 0x8027594C: whether any of the given condition bits is set in the actor's condition word. */
u32 Pl_condition_ck(_PLW* self, u32 mask)
{
    return (self->unk3D8 & mask) != 0;
}

/* 0x8027594C: whether any of the given bits is set in the actor's damage-condition word. */
u32 Pl_dm_condition_ck(_PLW* self, u32 mask)
{
    return (self->unk3DC & mask) != 0;
}

/* 0x802790E4 */
extern "C" u32 fn_802790E4(_PLW* self, u32 mask)
{
    return (self->unk3E0 & mask) != 0;
}

/* 0x80279154 */
extern "C" void fn_80279154(_PLW* self, s32 arg1, s8 arg2)
{
    if (arg1 == 0) {
        if (self->unk448 < arg2) {
            self->unk448 = arg2;
        }
    } else {
        if (self->unk449 < arg2) {
            self->unk449 = arg2;
        }
    }
}

/* 0x802798FC */
extern "C" s32 fn_802798FC(_PLW* self)
{
    if ((self->unk269 == 0 || (self->unk269 < self->unk26A && self->unk269 < self->unk270))
        && self->unk270 > 0) {
        return 1;
    }
    return 0;
}

/* 0x8027A000: adds a signed amount to the actor's charge counter and clamps it to +/-100. */
extern "C" void fn_8027A000(_PLW* self, s32 arg1)
{
    self->unk64F += (s8)arg1;
    if (self->unk64F >= 100) {
        self->unk64F = 100;
    }
    if (self->unk64F <= -100) {
        self->unk64F = -100;
    }
}

/* 0x8027A044: adds a signed amount to the actor's aim angle and clamps it to +/-90 degrees. */
extern "C" void fn_8027A044(_PLW* self, s32 arg1)
{
    self->unk5E8 = self->unk5E8 + arg1;
    if ((s16)arg1 >= 0) {
        if (self->unk5E8 >= 8192) {
            self->unk5E8 = 8192;
        }
    } else {
        if (self->unk5E8 <= -8192) {
            self->unk5E8 = -8192;
        }
    }
}

/* 0x8027A17C */
extern "C" void fn_8027A17C(_PLW* self)
{
    self->unk5E6 = 0;
    self->unk5E5 = 0;
    self->unk5E8 = 0;
}

/* 0x8027A190 */
extern "C" s32 fn_8027A190(_PLW* self)
{
    return 1;
}

/* 0x8027A198 */
extern "C" s32 fn_8027A198(_PLW* self)
{
    if (self->unk00A == 0) {
        s32 id = self->unk00C;
        if ((u32)(id - 68) <= 1 || id == 71) {
            return 1;
        }
    }
    return 0;
}

/* 0x8027A554 */
extern "C" s32 fn_8027A554(_PLW* self)
{
    if (self->unk018 == 1 && self->unk5E6 != 0) {
        return 0;
    }
    return 1;
}

/* 0x8027AC00 */
extern "C" void fn_8027AC00(_PLW* self)
{
    self->unk5BB = 1;
}

/* 0x8027AC0C */
extern "C" void fn_8027AC0C(_PLW* self)
{
    self->unk5BB = 0;
}

/* 0x8027AF34: ticks the actor's residual-velocity timer and integrates its position deltas. */
extern "C" void fn_8027AF34(_PLW* self)
{
    s16 v = self->unk0B4;
    if (v > 0) {
        self->unk0B4--;
        /* unreachable - it makes the compiler reuse the first test's condition register */
        if (v < 0) {
            return;
        }
        self->unk03C += self->unk09C;
        self->unk040 += self->unk0A0;
        self->unk044 += self->unk0A4;
    }
}

/* 0x8027AF80 */
extern "C" s32 fn_8027AF80(_PLW* self)
{
    return 1;
}

/* 0x8027C030 */
extern "C" s32 fn_8027C030(_PLW* self)
{
    if (self->unk00A == 7) {
        s32 id = self->unk00C;
        if ((u32)(id - 5) <= 1 || id == 2) {
            return 1;
        }
    }
    return 0;
}

/* 0x8027C89C: raises the actor's stored action id to the given one. */
extern "C" void fn_8027C89C(_PLW* self, s16 arg1)
{
    if (self->unk468 < arg1) {
        self->unk468 = arg1;
    }
}

/* 0x8027CA04 */
extern "C" s32 fn_8027CA04(_PLW* self, s32 arg1)
{
    if (self->unk002 != 7) {
        return 0;
    }
    if (self->unk468 > 0) {
        return 1;
    }
    return self->unk384 >= (s16)arg1;
}

/* 0x8027CC2C */
extern "C" u32 fn_8027CC2C(_PLW* self)
{
    return (self->unk5A4 & 0x40) != 0;
}

/* 0x8027CF24 */
extern "C" s32 fn_8027CF24(_PLW* self)
{
    if (fn_8026FE44(self) == 1 && self->unk276 != 0) {
        return 1;
    }
    return 0;
}

/* 0x8027CF70 */
extern "C" s32 fn_8027CF70(_PLW* self)
{
    return Pl_condition_ck(self, 0x20000);
}

/* 0x8027CFB4: starts a zanzo (afterimage) trail on the actor's current motion. */
void Pl_zanzo_set(_PLW* self, s32 arg1, u8 arg2)
{
    self->unk662 = (s16)arg1;
    self->unk65E = arg2;
    self->unk664 = Get_motion_no(self);
    self->unk666 = 0xFFFF;
}

/* 0x8027D3F0 */
extern "C" void fn_8027D3F0(_PLW* self, u8 arg1)
{
    self->unk3AC |= 1 << arg1;
}

/* 0x8027D4F0 */
extern "C" void fn_8027D4F0(_PLW* self)
{
    self->unk3B4 = 1;
}

/* 0x8027D4FC */
extern "C" s32 fn_8027D4FC(_PLW* self)
{
    return self->unk3B4 != 0;
}

/* 0x8027D510 */
extern "C" void fn_8027D510(_PLW* self)
{
    self->unk3B5 = 1;
}

/* 0x8027D51C */
extern "C" s32 fn_8027D51C(_PLW* self)
{
    return self->unk3B5 != 0;
}

/* 0x8027D584 */
extern "C" void fn_8027D584(_PLW* self, u8* arg1)
{
    self->unk59C = arg1[3] | 0x8000;
    self->unk59E = 0;
    self->unk5A0 = 0;
}

/* 0x80277C58: writes the actor's remaining vertical range from its motion frame data. */
extern "C" void fn_80277C58(_PLW* self)
{
    u8* p = self->unk13C;
    f32 v = *(f32*)(p + 72);
    if (v < lbl_8079A084) {
        v = lbl_8079A084;
    }
    self->unk264 = (s16)(*(f32*)(p + 120) - v);
}

/* 0x80277BC4: the attack-range modifier the actor's two armour skills set. */
extern "C" void fn_80277BC4(_PLW* self, u8 arg1)
{
    s32 v;
    if (arg1 == 0) {
        v = 6;
        if (Pl_Skill_ck(self, 0xA1) == 1) {
            v = 10;
        } else if (Pl_Skill_ck(self, 0xA2) == 1) {
            v = 12;
        }
    } else {
        v = 12;
    }
    fn_80277B44(self, v);
}

/* 0x80277EC0: hands the actor's vertical speed to its motion frame and caches the result. */
extern "C" void fn_80277EC0(_PLW* self)
{
    if (self->unk3A2 > 0) {
        fn_800E1640(self->unk13C + 4, lbl_8079A084);
    } else if (self->unk3A3 > 0) {
        fn_800E1640(self->unk13C + 4, lbl_8079A0C8 * self->unk354);
    } else {
        fn_800E1640(self->unk13C + 4, self->unk354);
    }
    self->unk358 = *(f32*)(self->unk13C + 0x60);
}

/* 0x80277F54: whether the actor's current motion is still free to be interrupted. */
extern "C" s32 fn_80277F54(_PLW* self)
{
    if (self->unk30E >= 2) {
        return 0;
    }
    s32 kind = self->unk015;
    if (kind != 9) {
        u8* p = get_move_work_adrs(0);
        if (p != 0 && self->unk016 == p[0xF6]) {
            return 0;
        }
    } else {
        if (fn_802B0688((u8*)self + 60) == 1) {
            return 0;
        }
    }
    return 1;
}

/* 0x802782B8 */
extern "C" s32 fn_802782B8(_PLW* self)
{
    if (fn_80277FE8(self) == 1 && fn_80278144(self->unk016, (u8*)self + 60, self->unk5A6) == 1) {
        return 1;
    }
    return 0;
}

/* 0x80278450 */
extern "C" s32 fn_80278450(_PLW* self)
{
    if (fn_80277FE8(self) == 1 && fn_80278310(self->unk016, (u8*)self + 60, self->unk5A6) == 1) {
        return 1;
    }
    return 0;
}

/* 0x80278994: clears the actor's stored per-motion scratch values. */
extern "C" void fn_80278994(_PLW* self)
{
    self->unk39E = 0;
    self->unk38A = 0;
    self->unk3EC = 0;
    self->unk3F8 = 0;
    self->unk3FE = 0;
    self->unk3F2 = 0;
    self->unk40E = 0;
    self->unk414 = 0;
    self->unk41A = 0;
    self->unk420 = 0;
    self->unk38C = 0;
    self->unk424 = 0;
    self->unk38E = 0;
    self->unk426 = 0;
    self->unk390 = 0;
    self->unk428 = 0;
    self->unk392 = 0;
    self->unk42A = 0;
    self->unk394 = 0;
    self->unk42C = 0;
}

/* 0x80278B58: starts the actor's dodge/step motion. */
extern "C" void fn_80278B58(_PLW* self, s32 arg1, s32 arg2)
{
    fn_8026FEF0(self, 4);
    fn_8026FEF0(self, 16);
    self->unk370 = 0;
    self->unk376 = 0;
    fn_802789EC(self, 1);
    fn_8027A17C(self);
    fn_80275AC4(self, 8, (u16)arg1, (u16)(arg2 | 32));
}

/* 0x80278BE4 */
extern "C" void fn_80278BE4(_PLW* self)
{
    if (self->unk00A == 8) {
        return;
    }
    fn_8026FEF0(self, 4);
    fn_8026FEF0(self, 16);
    self->unk370 = 0;
    self->unk376 = 0;
    fn_802789EC(self, 1);
    fn_8027A17C(self);
    if (self->unk009 == 3) {
        fn_80278B58(self, 1, 0);
    } else {
        fn_80278B58(self, 0, 0);
    }
}

/* 0x80278C7C */
extern "C" s32 fn_80278C7C(_PLW* self)
{
    if (self->unk416 > 0) {
        return 1;
    }
    if (self->unk00A == 6) {
        s32 id = self->unk00C;
        if ((u32)(id - 46) <= 5 || (u32)(id - 43) <= 1 || id == 73) {
            return 1;
        }
    }
    return 0;
}

/* 0x80278CD0 */
extern "C" s32 fn_80278CD0(_PLW* self)
{
    if (self->unk41C > 0) {
        return 1;
    }
    if (self->unk00A == 6) {
        s32 id = self->unk00C;
        if ((u32)(id - 66) <= 6 || (u32)(id - 63) <= 1) {
            return 1;
        }
    }
    return 0;
}

/* 0x802790FC */
extern "C" s32 fn_802790FC(_PLW* self)
{
    if (self->unk46C == 1 && self->unk46C != self->unk46D && self->unk46E == 0) {
        self->unk46E = 1;
        if (self->unk470 == 0) {
            self->unk470 = 1800;
            return 1;
        }
        return 0;
    }
    return 0;
}

/* 0x80279194 */
extern "C" s32 fn_80279194(_PLW* self, s32 arg1, s8 arg2)
{
    if (arg1 == 0) {
        self->unk422 = 0;
        if (self->unk44C < arg2) {
            self->unk44C = arg2;
        }
    } else {
        if (self->unk422 > 0) {
            self->unk422 = 0;
            return 0;
        }
        if (self->unk44D < arg2) {
            self->unk44D = arg2;
        }
    }
    return 1;
}

/* 0x8027A08C: applies a charge delta to the actor's aim and converts the counter to an angle. */
extern "C" void fn_8027A08C(_PLW* self, s8 arg1)
{
    f32 v;
    self->unk583 += arg1;
    if (self->unk583 > 100) {
        self->unk583 = 100;
    }
    if (self->unk583 < -100) {
        self->unk583 = -100;
    }
    v = (f32)self->unk583 / lbl_8079A0D0;
    if (self->unk583 >= 0) {
        v = lbl_8079A0E0 * v * lbl_8079A0DC / lbl_8079A0E4 + lbl_8079A088;
        self->unk5E8 = (s16)(u16)v;
    } else {
        v = lbl_8079A0E8 * v * lbl_8079A0DC / lbl_8079A0E4 + lbl_8079A088;
        self->unk5E8 = (s16)(u16)v;
    }
}

/* 0x8027BE2C */
extern "C" void fn_8027BE2C(_PLW* self)
{
    if ((self->unk5C4 & 0xF) != 0) {
        self->unk5C4 &= 0xF0;
        fn_8010D688(self);
    }
}

/* 0x8027AC18 */
extern "C" s32 fn_8027AC18(_PLW* self)
{
    return self->unk5BB != 0;
}

/* 0x8027BDD4 */
extern "C" s32 fn_8027BDD4(_PLW* self)
{
    if (Pl_Skill_ck(self, 93) == 1) {
        return 1;
    }
    return (u16)fn_8027BCE0(self) == 395;
}

/* 0x8027CBC8: the highest carve-slot value the actor has available. */
extern "C" u8 fn_8027CBC8(_PLW* self)
{
    u8 v = 0;
    if (self->unk489 != 0 && self->unk492 != 255) {
        if (v < self->unk4DC) {
            v = self->unk4DC;
        }
    }
    if (self->unk4E5 != 0 && self->unk4EE != 255) {
        if (v < self->unk538) {
            v = self->unk538;
        }
    }
    return v;
}

/* 0x8027D530 */
extern "C" s32 fn_8027D530(_PLW* self)
{
    if (Pl_act_ck(self, 0, 64) == 1 && self->unk306 == 206) {
        return 1;
    }
    return 0;
}

/* 0x8027A4EC */
extern "C" s32 fn_8027A4EC(_PLW* self)
{
    s32 v = 0;
    if (self->unk00A == 4) {
        s32 id = self->unk00C;
        if ((u32)(id - 19) <= 30 || (u32)(id - 5) <= 8) {
            v = 1;
        }
    }
    if (fn_8028732C(self) == 1) {
        v = 1;
    }
    return v;
}

/* 0x80279414: looks the given id up in the actor's two reaction tables. */
extern "C" u8* fn_80279414(_PLW* self, u8 arg1)
{
    u8* p = fn_80279360(fn_8027ED18((u8*)self + 464), arg1);
    if (p != 0) {
        return p;
    }
    u8 n = *((u8*)self + 488);
    if (n == 12) {
        u8* q = fn_80279360(fn_8027ED18((u8*)self + 488), arg1);
        if (q != 0) {
            return q;
        }
    }
    return 0;
}

/* 0x80279360: finds the reaction-table entry the given motion id maps to. */
extern "C" u8* fn_80279360(u8* self, u8 arg1)
{
    for (s32 i = 0; i < 4; i++) {
        u8 idx = self[0x18 + i];
        if (idx == 0) {
            break;
        }
        u8* p = lbl_805BF5E0 + idx * 4;
        if (p[0] == arg1) {
            return p;
        }
    }
    return 0;
}

/* 0x80279B84: refreshes the actor's held-item state from its item id. */
extern "C" void fn_80279B84(_PLW* self)
{
    if (fn_8026FE44(self) != 0 && self->unk26E != 255) {
        (void)GetItemData((u16)fn_80272C80(self, (u8)self->unk26E));
        self->unk26C = fn_80274B20((u16)fn_80272C80(self, (u8)self->unk26E));
        self->unk270 = fn_80272CC8(self, (u8)self->unk26E);
        self->unk26A = fn_80274D98(self, self->unk26C);
        self->unk269 = 0;
    }
}

/* 0x8027A2A0: whether the actor may still act - master/rage state, the bari timer, the stun flag
 * and the held-item lock all have to agree. */
extern "C" s32 fn_8027A2A0(_PLW* self, s32 arg1)
{
    s32 v = 1;
    if (Pl_master_ck(self) == 0) {
        v = 0;
    }
    if (Pl_bari_ck(self, 1) == 0) {
        v = 0;
    }
    if (self->unk5E6 != 0) {
        v = 0;
    }
    if (fn_8026A644(self, 55) == 0 && (u8)arg1 == 0) {
        v = 0;
    }
    return v;
}

/* 0x8027BC48: whether the current motion still takes directional input. */
extern "C" s32 fn_8027BC48(s32 arg1)
{
    u8* p = get_move_work_adrs(0);
    if (p == 0) {
        return 1;
    }
    s32 x = p[0xFA];
    if ((u32)(x - 6) <= 2) {
        return 1;
    }
    if (x == 3) {
        if (arg1 == 0) {
            return 1;
        }
    } else if (x == 5) {
        if (arg1 != 2) {
            return 1;
        }
    } else if (x == 4) {
        return 1;
    }
    return 0;
}

/* 0x8027CFC0: ticks down the actor's stun timer and clears the stun condition at zero. */
extern "C" void fn_8027CFC0(_PLW* self)
{
    if (Pl_master_ck(self) != 0 && self->unk404 > 0) {
        if (Pl_dm_condition_ck(self, 2) == 1) {
            self->unk404 = self->unk404 - 90;
        } else {
            self->unk404 = self->unk404 - 120;
        }
        if (self->unk404 <= 0) {
            self->unk404 = 0;
            self->unk3DC &= 0x3FFFFFFF;
        }
    }
}

/* 0x8027D050: the actor's stored carve value, scanned out of the item table. */
extern "C" u8 fn_8027D050(_PLW* self)
{
    u8 v = 0;
    if (self->unk369 == 0) {
        return 0;
    }
    if ((self->unk364 & 0xE0000007) != 0) {
        for (s32 i = 0; i < 10; i++) {
            if (lbl_806AB848[i * 24 + self->unk008 * 264 + 20] != 0) {
                v = lbl_806AB848[self->unk008 * 264 + i * 24 + 20];
                break;
            }
        }
    }
    return v;
}

/* 0x8027D5A4: applies a signed charge delta to the actor's stored charge, scaled by the two
 * charge skills, and clamps it to 0..100. */
extern "C" void fn_8027D5A4(_PLW* self, s32 arg1)
{
    f32 f = (f32)(s16)arg1;
    if (f > lbl_8079A084) {
        if (Pl_Skill_ck(self, 191) == 1) {
            f *= lbl_8079A094;
        } else if (Pl_Skill_ck(self, 192) == 1) {
            f *= lbl_8079A0FC;
        }
    }
    self->unk36C += (s32)f;
    if (self->unk36C < 0) {
        self->unk36C = 0;
    } else if (self->unk36C > 100) {
        self->unk36C = 100;
    }
}

/* 0x8027CE94: the gunner's world-space origin, biased by the charge counter. */
void Pl_get_gunner_vec(_PLW* self, _CP_VECTOR* out)
{
    u32 x = self->unk054;
    f32 t = (f32)self->unk64F / lbl_8079A0D0;
    t = lbl_8079A110 * t * lbl_8079A0DC / lbl_8079A0E4 + lbl_8079A088;
    out->x = x + (u16)t;
    out->y = self->unk058;
    out->z = 0;
}

/* 0x8027CE74: whether the actor's current motion is one of the gunner charge motions. */
extern "C" s32 fn_8027CE74(_PLW* self)
{
    if (fn_8026FE44(self) == 1) {
    switch ((u16)Get_motion_no(self)) {
    case 1104:
    case 1114:
    case 1115:
    case 1130:
    case 1131:
    case 1132:
    case 1153:
    case 1163:
    case 1164:
    case 1180:
    case 1181:
    case 1182:
        return 1;
    }
    }
    return 0;
}

/* 0x8027CA48: applies a charge delta to the weapon's charge timer. */
extern "C" void fn_8027CA48(_PLW* self, s32 arg1)
{
    if (self->unk002 != 7) {
        return;
    }
    self->unk386 += arg1;
    if ((s16)arg1 >= 0) {
        self->unk46A = 1800;
        if (self->unk386 >= 100) {
            self->unk386 = 100;
        }
        switch ((u8)fn_802748C8(self)) {
        case 1:
            fn_8027C89C(self, 900);
            break;
        case 2:
            fn_8027C89C(self, 900);
            break;
        case 3:
            fn_8027C89C(self, 900);
            break;
        }
    } else {
        if (self->unk386 < 0) {
            self->unk386 = 0;
        }
    }
}

/* 0x8027CB1C: the carve slot to select, sentinel 255 meaning "none". */
extern "C" u8 fn_8027CB1C(_PLW* self)
{
    u8 v = 255;
    if (self->unk489 != 0 && self->unk492 != 255 && (v == 255 || v < self->unk492)) {
        v = self->unk492;
    }
    if (self->unk4E5 != 0 && self->unk4EE != 255 && (v == 255 || v < self->unk4EE)) {
        v = self->unk4EE;
    }
    if (Pl_Skill_ck(self, 24) == 1 && v == 0) {
        v = 2;
    }
    return v;
}

/* 0x8027D40C: the number of set bits in the actor's action-lock word. */
extern "C" s32 fn_8027D40C(_PLW* self)
{
    s32 n = 0;
    for (s32 i = 0; i < 32; i++) {
        if (self->unk3AC & (1 << i)) {
            n++;
        }
    }
    return n;
}

/* 0x8027CD10: builds the gunner's aim matrix from its rotation and gun position. */
extern "C" void fn_8027CD0C(_PLW* self, nw4r::math::MTX34* mtx)
{
    nw4r::math::VEC3 v;
    _CP_VECTOR pos;
    fn_80043EA8(&v);
    fn_800FC0D4(&pos, (u8*)self + 84);
    f32 t = (f32)self->unk64F / lbl_8079A0D0;
    t = lbl_8079A110 * t * lbl_8079A0DC / lbl_8079A0E4 + lbl_8079A088;
    pos.x = pos.x + (u16)t;
    cpSetRotMatrixZXY(&pos, mtx);
    Pl_get_gunner_pos(self, &v, 0);
    mtx->m[0][3] = v.x;
    mtx->m[1][3] = v.y;
    mtx->m[2][3] = v.z;
}

/* 0x8027CD18: the gunner's gun position in actor-local space, biased by the charge counter. */
void Pl_get_gunner_pos(_PLW* self, nw4r::math::VEC3* out, s32 arg2)
{
    if (self->unk009 == 3) {
        out->x = lbl_8079A100;
        out->y = lbl_8079A104;
        out->z = lbl_8079A108;
    } else {
        out->x = lbl_8079A100;
        out->y = lbl_8079A10C;
        out->z = lbl_8079A108;
    }
    if (arg2 != 0) {
        out->z = out->z + lbl_8079A0CC;
    }
    rotVecXYZ(out, (_CP_VECTOR*)((u8*)self + 84));
    out->x = out->x + self->unk03C;
    out->y = out->y + self->unk040;
    out->z = out->z + self->unk044;
}

/* 0x8027AE28: sets the actor's residual-velocity timer from a target position over the given
 * number of frames. */
extern "C" void fn_8027AE28(_PLW* self, s32 arg1)
{
    if (fn_80050EF4((u8*)self + 60, (u8*)self + 144) >= lbl_8079A0F4 || (s16)arg1 == 0) {
        self->unk0B4 = 0;
        self->unk03C = self->unk090[0];
        self->unk040 = self->unk090[1];
        self->unk044 = self->unk090[2];
        self->unk09C = lbl_8079A084;
        self->unk0A0 = lbl_8079A084;
        self->unk0A4 = lbl_8079A084;
    } else {
        f32 n = (f32)(s16)arg1;
        self->unk0B4 = arg1;
        self->unk09C = (self->unk090[0] - self->unk03C) / n;
        self->unk0A0 = (self->unk090[1] - self->unk040) / n;
        self->unk0A4 = (self->unk090[2] - self->unk044) / n;
    }
}

/* 0x8027C8B4: applies a signed charge delta to the actor's attack-range counter, with the two
 * charge skills scaling a positive delta. */
extern "C" void fn_8027C8B4(_PLW* self, s32 arg1)
{
    if (self->unk002 != 7) {
        return;
    }
    if ((s16)arg1 < 0 && self->unk468 > 0) {
        return;
    }
    f32 f = (f32)(s16)arg1;
    if (f > lbl_8079A084) {
        if (Pl_Skill_ck(self, 191) == 1) {
            f *= lbl_8079A094;
        } else if (Pl_Skill_ck(self, 192) == 1) {
            f *= lbl_8079A0FC;
        }
    }
    self->unk384 += (s32)f;
    if (f >= lbl_8079A084) {
        if (self->unk384 >= 100) {
            self->unk384 = 100;
            if (Pl_master_ck(self) == 1 && self->unk468 == 0) {
                fn_80114C20(self, 1);
            }
            fn_8027C89C(self, 900);
        }
    } else {
        if (self->unk384 < 0) {
            self->unk384 = 0;
        }
    }
}

/* 0x8027A2A0 sibling: whether the actor's bari (rage) state covers the given action class. */
u32 Pl_bari_ck(_PLW* self, s32 arg1)
{
    s32 v = 0;
    if (self->unk00A == 0) {
        u16 id = self->unk00C;
        if ((u32)(id - 169) <= 1 || (u32)(id - 172) <= 1) {
            if (arg1 != 2) {
                v = 1;
            } else {
                u16 m = (u16)Get_motion_no(self);
                if ((m == 314 || m == 365)
                    && Pl_frame_check(self, 1, lbl_8079A0EC, lbl_8079A084) == 1) {
                    v = 1;
                }
            }
        } else if ((id == 171 || id == 174) && (u32)(arg1 - 1) <= 1) {
            v = 1;
        }
    }
    return v;
}

/* 0x802784B8 */
extern "C" s32 fn_802784B8(_PLW* self)
{
    s32 m = (u8)fn_802B0668((u8)get_now_mapno());
    if ((m == 6 || m == 17) && self->unk016 == 2) {
        return 0;
    }
    if (m == 9) {
        if (fn_802B0688((u8*)self + 60) == 1) {
            return 0;
        }
    } else {
        u8* p = get_move_work_adrs(0);
        if (p != 0 && self->unk016 == p[0xF6]) {
            return 0;
        }
    }
    return 1;
}

/* 0x80278590: the actor's attack-range tier for the current weapon class. */
extern "C" s32 fn_80278590(_PLW* self)
{
    if (fn_8026FE44(self) == 1) {
        return 0;
    }
    s16* t = (s16*)(lbl_805BFFA8[self->unk002]
                    + *((u8*)self + 0x56A) * 7 * 2);
    s16 v = *((s16*)self + 0x2B6);
    if (v <= t[0]) {
        return 0;
    }
    if (v <= t[1]) {
        return 1;
    }
    if (v <= t[2]) {
        return 2;
    }
    if (v <= t[3]) {
        return 3;
    }
    if (v <= t[4]) {
        return 4;
    }
    return 5 + (v > t[5]);
}

/* 0x802789EC: resets the actor's whole action state and re-applies the armour skill values. */
extern "C" void fn_802789EC(_PLW* self, s32 arg1)
{
    fn_80278994(self);
    *(u32*)((u8*)self + 988) = 0;
    *(s16*)((u8*)self + 1042) = 0;
    *(s16*)((u8*)self + 1002) = 0;
    *(s16*)((u8*)self + 1006) = 0;
    *(s16*)((u8*)self + 1024) = 0;
    *(s16*)((u8*)self + 1020) = 0;
    *(s16*)((u8*)self + 1008) = 0;
    *(s16*)((u8*)self + 1012) = 0;
    *(s16*)((u8*)self + 1014) = 0;
    *(s16*)((u8*)self + 1018) = 0;
    *(s16*)((u8*)self + 1026) = 0;
    *(s16*)((u8*)self + 900) = 0;
    *(s16*)((u8*)self + 902) = 0;
    *(s16*)((u8*)self + 1040) = 0;
    *(s16*)((u8*)self + 1048) = 0;
    *(s16*)((u8*)self + 1054) = 0;
    *(s16*)((u8*)self + 1046) = 0;
    *(s16*)((u8*)self + 1052) = 0;
    *(s16*)((u8*)self + 1118) = 0;
    *(s16*)((u8*)self + 1120) = 0;
    *(s16*)((u8*)self + 1122) = 0;
    *(s16*)((u8*)self + 1124) = 0;
    *(s16*)((u8*)self + 1126) = 0;
    *(u8*)((u8*)self + 1096) = 0;
    *(u8*)((u8*)self + 1100) = 0;
    *(u8*)((u8*)self + 1097) = 0;
    *(u8*)((u8*)self + 1101) = 0;
    *(s16*)((u8*)self + 1098) = 0;
    *(s16*)((u8*)self + 1102) = 0;
    *(s16*)((u8*)self + 1028) = 0;
    *(s16*)((u8*)self + 1030) = 0;
    *(s16*)((u8*)self + 1032) = 0;
    *(s16*)((u8*)self + 1034) = 0;
    *(s16*)((u8*)self + 1036) = 0;
    *(s16*)((u8*)self + 1108) = 0;
    *(u8*)((u8*)self + 1104) = 0;
    *(s16*)((u8*)self + 1112) = 0;
    *(u8*)((u8*)self + 1106) = 0;
    *(s16*)((u8*)self + 1128) = 0;
    *(s16*)((u8*)self + 1130) = 0;
    *(s16*)((u8*)self + 1070) = 0;
    *(s16*)((u8*)self + 1080) = 0;
    *(s16*)((u8*)self + 1072) = 0;
    *(s16*)((u8*)self + 1082) = 0;
    *(s16*)((u8*)self + 1074) = 0;
    *(s16*)((u8*)self + 1084) = 0;
    *(s16*)((u8*)self + 1076) = 0;
    *(s16*)((u8*)self + 1086) = 0;
    *(s16*)((u8*)self + 1078) = 0;
    *(s16*)((u8*)self + 1088) = 0;
    *(s16*)((u8*)self + 1058) = 0;
    *(s16*)((u8*)self + 1114) = 0;
    *(s16*)((u8*)self + 1116) = 0;
    if (Pl_master_ck(self) == 1 && (arg1 == 0 || Pl_cat_skill_ck(self, 39) == 1)) {
        self->unk448 = (s8)fn_802753E4(self, 4);
        self->unk449 = (s8)fn_802753E4(self, 5);
    }
    fn_80278D1C(self);
}

/* 0x8027BCE0: the highest of the actor's 24 stored item ids that is in the carve set. */
extern "C" u32 fn_8027BCE0(_PLW* self)
{
    u16 v = 0xFFFF;
    for (s32 i = 0; i < 24; i++) {
        if (*(s16*)((u8*)self + 0x27A + i * 4) > 0) {
            s32 id = *(u16*)((u8*)self + 0x278 + i * 4);
            if ((u32)(id - 381) <= 1 || id == 139 || id == 395) {
                v = (u16)id;
            }
        }
    }
    return v;
}

/* 0x80275A80: the shell multiplier adjustment the two shell tables give. */
f32 Get_Shell_rate_adj(_PLW* self, u8 arg1)
{
    f32 v = (f32)*(s16*)(fn_8027ED18((u8*)self + 464) + 10);
    if (*((u8*)self + 488) == 12) {
        v = v * (f32)*(s16*)(fn_8027ED18((u8*)self + 488) + 10) / lbl_8079A0D0;
    }
    return v / lbl_8079A0D0;
}

/* 0x80277C94: ground-height probe along the actor's facing, returning whether the probe hit
 * inside the requested band. */
extern "C" s32 fn_80277C94(_PLW* self, nw4r::math::VEC3* out, f32 arg2, f32 arg3)
{
    nw4r::math::VEC3 v;
    u8 hit;
    fn_80043EA8(&v);
    out->x = lbl_8079A084;
    v.x = lbl_8079A084;
    v.y = arg2 + arg3;
    v.z = lbl_8079A0C4;
    rotVecY(&v, self->unk058);
    v.x = v.x + self->unk03C;
    v.y = v.y + self->unk040;
    v.z = v.z + self->unk044;
    f32 h = GetGroundHit2(&v, 0xEFFA, self->unk016, &hit);
    f32 c = self->unk040 + arg2;
    if (h >= c && h <= arg3 + c && hit != 0) {
        out->x = h;
        return 1;
    }
    return 0;
}

/* 0x8027AF88: refreshes the actor's shell/clutch state from its move work. */
extern "C" void fn_8027AF88(_PLW* self)
{
    *(s16*)((u8*)self + 1474) = 30;
    u8* p = get_move_work_adrs(0);
    if (p == 0) {
        return;
    }
    u8* q = *(u8**)(p + 220);
    if (q == 0) {
        return;
    }
    if (fn_8042CB9C(fn_802E5CFC(*(s8*)(q + 1505))) == 1) {
        *(s16*)((u8*)self + 1626) = 900;
        *((u8*)self + 1625) = 1;
        fn_80338E04(1, (u8)fn_800CF384(), *(u8*)(q + 1505));
        return;
    }
    *(s16*)((u8*)self + 1626) = 0;
    *((u8*)self + 1625) = 0;
    fn_80272E30(self, *(u16*)(q + 1506 + *(s8*)(q + 1505) * 4),
                *(s16*)(q + 1508 + *(s8*)(q + 1505) * 4));
    *(u32*)(q + 1668 + (*(s8*)(q + 1505) >> 5) * 4) |= 1 << (*(s8*)(q + 1505) & 31);
    fn_802E5D68(*(u16*)(q + 1506 + *(s8*)(q + 1505) * 4));
    *(s16*)(q + 1506 + *(s8*)(q + 1505) * 4) = 0;
    *(s16*)(q + 1508 + *(s8*)(q + 1505) * 4) = 0;
}
