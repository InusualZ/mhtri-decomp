/*
 * Declarations for the symbols `src/fn_8004CAD8.cpp` owns (docs/plan.md 6.5, rule 2).  The parameters
 * are the owner's `long`, which is the mangling's (`wii_sysmsg_gen__FlPcl`) - `int` would ask for a
 * different symbol.
 */
#ifndef MHTRI_FN_8004CAD8_H
#define MHTRI_FN_8004CAD8_H

#include "types.h"
#include "nw4r/math.h"
#include "fn_8004CAD8/mtx.h" /* MTX34_ctor / set_slot_none - the two this unit's consumers share */

#ifdef __cplusplus
void wii_sysmsg_gen(long id, char* buf, long a);

/* The 0x100 B VS user block `get_vsUser_work` indexes (the two slots at 0x8066A620/0x8066A720).
 * The block's own name is the mangling's (`ck_mydata_vs__FP13_vs_user_dataP13_vs_user_data`), so a
 * caller that reaches a mangled consumer spells it `_vs_user_data`.  Only the fields this project's
 * functions touch are named; every other byte is filler.  size: 0x100 */
struct _vs_user_data {
    /* +0x00 */ u8 cfg_0x00[0x18];  /* the per-index option bytes `get_arena_cfg` reads (the
                                     * `0x803BE30C` band's arena twin of `option_w`) */
    /* +0x18 */ s32 point_0x18;      /* the credit `score_add_clamped` accumulates for the player */
    /* +0x1C */ u8 pad_0x1C[0x2C - 0x1C];
    /* +0x2C */ u32 item_0x2C[0x10]; /* the 16 four-byte item slots `item_count_find`/`item_take` walk */
    /* +0x6C */ u32 slot_a_0x6C[7];  /* the first of the two per-slot flag runs `multi_box_phase_ck` counts */
    /* +0x88 */ u8 pad_0x88[0x94 - 0x88];
    /* +0x94 */ u32 slot_b_0x94[7];  /* its sibling; the two are read as a pair per slot */
    /* +0xB0 */ u8 pad_0xB0[0xBC - 0xB0];
    /* +0xBC */ u16 ready_mask_0xBC; /* the per-player ready bits the phase gate tests against 0x380 */
    /* +0xBE */ u8 pad_0xBE[0x100 - 0xBE];
};

/* 0x8004D14C - one of the two 0x100 B VS user slots, or null when the index is out of range (the
 * owner defines it with the same `long` parameter, which is what its mangling `__Fl` asks for).
 * Added with `menu/multi_result.cpp`, its first consumer (rule 2). */
_vs_user_data* get_vsUser_work(long index);
#endif

/* 0x80050A90 - `calcVecAng2`, the two-vector angle helper: it loads the two vectors' x/z floats and
 * tail-calls 0x80050A40, so the angle comes back in r3.  Added when `enemy/fn_80147CE0.cpp`
 * registered as a consumer (rule 2); the return is the 16-bit angle its three consumers mask
 * (`clrlwi r3,r3,16`) and sign-extend (`extsh`), spelled `s32` here as `enemy/fn_801550FC.cpp` does.
 * C++ linkage (outside the `extern "C"` block below): the map name is the mangling, not a plain
 * name, and `enemy/fn_801550FC.cpp` declares it that way. */
#ifdef __cplusplus
s32 calcVecAng2(VEC3* a, VEC3* b);
/* 0x80050D18 / 0x80051064 - the two nw4r-math helpers this range owns.  C++ linkage (the map names are the
 * manglings of exactly these signatures: `calcVecAngXY__FPQ34nw4r4math4VEC3PUlPUl`, `rotVecY__FPQ34nw4r4math4VEC3Ul`),
 * so they are declared here rather than inside the `extern "C"` block - rule 9.  Consumers used to spell them
 * locally in their own sources. */
void calcVecAngXY(nw4r::math::VEC3* v, u32* x, u32* y);
void rotVecY(nw4r::math::VEC3* v, u32 angle);
#endif

/* C linkage: the target symbol is the unmangled `fn_80051570` (.text 0x80051570, a 4-byte `blr`).
 * Moved out of `src/g3d/g3d_anmchr.cpp` on landing (docs/plan.md 6.5, rule 2): that range was
 * written before this owner registered, so its local `extern "C"` declaration was a boundary
 * artefact. */
#ifdef __cplusplus
extern "C" {
u32 fn_8004D27C(s32 id);
/* 0x8004D70C - the event-flag test that shares the same table `fn_8004D27C` reads; added with
 * `src/lobby/fn_802FA9A0.cpp` as the second consumer (it was declared locally by four
 * `src/lobby/*.cpp` files, which is the rule-2 backlog this declaration closes). */
s32 fn_8004D70C(s32 id);
#endif
/* 0x8004D0E8 - clamp `*value += delta` into [0, 9999999], the score/point accumulator the VS result
 * and skill bands credit.  The owner defines it at C linkage, so the declaration sits inside the
 * `extern "C"` region (rule 2: added with `menu/multi_result.cpp`, its second consumer). */
void score_add_clamped(s32 delta, s32* value);
u32 fn_80051570(u32);
/* 0x8004D140 - the quest-result record accessor this range owns (`get_qResult_work__Fv`, a
 * 12-byte `lis`/`addi`/`blr` over the 0x438 B `qResult` buffer `src/fn_8004CAD8.cpp` defines).
 * Added with `menu/menu_result.cpp`, whose whole band reads the record (rule 2: the declaration
 * belongs with the owner, which had not declared it yet - `src/lobby/fn_801F9CD4.cpp` carried a
 * local copy of this spelling). */
void* get_qResult_work(void);
/* 0x8005220C - an 8-byte `fabs f1,f1; blr` helper (caller: `gx/fn_8009ACE4.c`, rule 2: this range
 * owns the address). */
f32 fn_8005220C(f32 value);
/* The VEC3 helpers this range owns, added when `src/ef/ef_util.cpp` registered as a consumer (rule 2).
 * Signatures are the owners' own bodies, not the call sites' guesses: 0x80050EDC is the squared length
 * (`ps_mul`/`ps_madd`/`ps_sum0` of the vector with itself), 0x80050F24 the length (`PSVECMag`),
 * 0x80051424 the scale-by-scalar (`out = in * s`), 0x80051820 the cross product (returns `out`),
 * 0x80052214 the dot product, 0x80050BC0 the square root (`x * FrSqrt(x)`). */
f32 fn_80050EDC(const f32* v);
f32 fn_80050F24(const f32* v);
void fn_80051424(f32* out, const f32* in, f32 s);
f32* fn_80051820(f32* out, const f32* a, const f32* b);
f32 fn_80052214(const f32* a, const f32* b);
/* 0x80050BC0 - the square root: `x * FrSqrt(x)` for x > 0, 0 for x == 0, and a `nw4r::db::Warning`
 * for x < 0.  ONE float argument, settled from the callee's own body (it reads only f1 and never
 * touches f2), not from the call sites: the `f2` a retail caller materialises before the call is the
 * hoisted common subexpression `1.0f - t` that its `else` branch reuses (ef_disc 0x800CCA7C/0x800CCA98,
 * ef_cylinder 0x800CBD20/0x800CBD3C).  The `(f32, f32)` spelling this symbol used to carry in
 * `include/ef.h` was the wrong view and made every TU including both headers fail to compile. */
f32 fn_80050BC0(f32 x);
/* 0x8005024C - the `SinFIdx` wrapper `enemy/fn_80181E24.cpp`'s alpha computation calls: it narrows
 * its argument to u16 (`clrlwi r3,r3,16`), scales the `anim_tick_angle` result and returns
 * `nw4r::math::SinFIdx`, so the signature is `(u16) -> f32` (settled from the callee's own body,
 * docs/plan.md 6.5 rule 6).  Added with proposal/80181C88. */
f32 fn_8005024C(u16 idx);
/* 0x80050CF4 - the SDK vector subtract (`ps_sub` on two paired loads): `dst = a - b`. */
void PSVECSubtract(f32* dst, const f32* a, const f32* b);
/* 0x800504D4/0x8005050C - the two GX pipe-setup helpers `g3d/g3d_state.cpp` calls (rule 2, moved
 * out of that unit's local extern block on landing, 2026-09-25). */
void fn_800504D4(void* pOut);
/* `MTX34_ctor` (0x8005050C) and `set_slot_none` (0x8004CAD8) live in `fn_8004CAD8/mtx.h`, included
 * below - this header's other declarations still disagree with several consumers, so a band that
 * only needs those two takes the light one. */
/* 0x80052BC0/0x800534B0 - the `ResTex`/`ResPltt` value-type constructors the
 * `g3d/g3d_resanmtexsrt.cpp` `ResFile` accessors use (rule 2: declared in their owner's header). */
void* fn_80052BC0(void* out, u32 v);
void* fn_800534B0(void* out, u32 v);
/* 0x80050508 - the 4-byte `blr` twin of fn_8005050C.  Its body does not touch r3, and the retail
 * call sites use the pointer it hands back as the following call's first argument
 * (`GXLoadTexMtxImm(fn_80050508(&mtx), id, ...)` in `src/g3d/g3d_gpu.cpp`, `GXLoadPosMtxImm(
 * fn_80050508(&mtx), 0)` in `src/ef/ef_drawlinestrategy.cpp`), so the pointer-returning shape is the
 * one the target's call sites require (added when `src/g3d/g3d_gpu.cpp` registered as the consumer). */
void* fn_80050508(void* pOut);
/* 0x80050EF4 - the two-pointer distance helper: r3 and r4 are the two `VEC3*` (its body moves r3
 * into r5 and calls `fn_80050CA0(&out, r4, r3)`, then `fn_80050F24(&out)`), so it takes two
 * pointers and returns the float.  Moved here from `enemy/fn_801550FC.cpp` on landing (rule 2):
 * this unit owns the address, and the three-argument form the consumer used was wrong
 * (`enemy/fn_8015941C` sets only r3/r4). */
f32 fn_80050EF4(void* a, void* b);
/* 0x80050CA0 / 0x80050F80 - the vector difference and the distance between two positions, both owned here.
 * Signatures are the CALLEES' OWN BODIES, not the callers' guesses: `fn_80050CA0(out, a, b)` is
 * `VEC3_ctor(out); PSVECSubtract(out, a, b)`, and `fn_80050F80(a, b)` calls `fn_80050CA0(&local, b, a)`
 * then the length helper `fn_80050F24(&local)`, i.e. `|a - b|`.  Ten consumer files used to declare these
 * locally (four spellings, one of them a `MTX34*` misnomer); they now include this header, so the home is
 * here.  All parameters are pointers - a declaration cannot change a call site's codegen. */
void fn_80050CA0(void* out, const void* a, const void* b);
f32 fn_80050F80(const void* a, const void* b);
/* 0x80050EAC - the SQUARED distance between two positions (the callers compare it against a squared
 * radius constant, e.g. `enemy/fn_801B0010.cpp` against `lbl_80798B3C` = 2250000.0f = 1500^2). */
f32 fn_80050EAC(const void* a, const void* b);
/* 0x80050F48 - the squared xz distance between two vectors, added with the same consumer.
 * C++ linkage: the map name is the mangling (`calcDistanceSqXZ__FPQ34nw4r4math4VEC3PQ34nw4r4math4VEC3`). */
f32 calcDistanceSqXZ(VEC3* a, VEC3* b);
/* 0x80051378 - the three-pointer vector helper this range owns (unmangled `fn_80051378`, so C
 * linkage).  Its body saves r3/r4/r5, zeroes the first through `VEC3_ctor`, then tail-forwards all
 * three to `fn_800513CC`, i.e. `void (VEC3*, VEC3*, VEC3*)`; added when `ai/fn_802CC794.cpp`
 * registered as the first consumer (rule 2) - the owner header did not declare it yet. */
void fn_80051378(VEC3* out, VEC3* a, VEC3* b);
/* 0x80050850 - the in-place normaliser this range owns (unmangled `fn_80050850`, so C
 * linkage).  Its body saves r3/r4 in r29/r30, hands r3 to `PSVECNormalize`'s `src` (r3) and
 * r4 to its `dst` (the target body reads 0(r3)/8(r3) and stores 0(r4)/8(r4)), then restores
 * `mr r3,r29`, so the shape is `dst = normalize(src); return dst;`: two pointers, the source
 * read-only, the destination returned.  Declared here because this unit owns the address
 * (rule 2).  `void*`/`const void*` is the spelling every call site reaches without a cast
 * (`ef`, `g3d` and `enemy` callers mix `VEC3*` and `void*` pointers) - the body moves
 * pointers and cannot distinguish them. */
void* fn_80050850(void* dst, const void* src);
/* 0x80052300 / 0x80052408 / 0x80052370 - the animation key-frame readers this unit owns (the
 * manglings `getKeyData__FPff` / `getKeyData3__FPffPfPfPf` say the real C++ signatures, which is how
 * the consumers call them; rule 9).  Added with `Pl/fn_80224AC4.cpp`. */
f32 getKeyData(f32* keys, f32 frame);
void getKeyData3(f32* keys, f32 frame, f32* out0, f32* out1, f32* out2);
f32 fn_80052370(f32* a, f32* b, f32* c, f32 frame);
/* 0x800513CC / 0x80050028 / 0x80051EE0 / 0x800513F0 - the four vector helpers the `Pl` hit tests and
 * the `ef`/`enemy` effect code call (rule 2: this range owns the addresses).  `fn_800513CC(out, a, b)`
 * is the paired-single add `out = a + b`, `fn_80050028(out, src)` the field-by-field three-float copy,
 * `fn_80051EE0(out, in, s)` the scale (`VEC3_ctor` then `fn_80051424`), and `fn_800513F0` the
 * in-place scale.  Added with `Pl/fn_8028F66C.cpp`, the first consumer to need them here.  The
 * parameter spellings are the ones the consumers that already include this header declare
 * (`ef/fn_801173AC.cpp`, `enemy/fn_801B7020.cpp`, `enemy/fn_8035E034.cpp`): `fn_800513F0`'s return
 * value is ignored at every call site in the tree, so it is declared `void` - a `VEC3*` return here
 * would be a second overload and `(10505) illegal overloading`. */
void fn_800513CC(VEC3* out, VEC3* a, VEC3* b);
void fn_80050028(VEC3* out, const VEC3* src);
void fn_80051EE0(VEC3* out, VEC3* in, f32 scale);
void fn_800513F0(VEC3* v, f32 scale);
/* 0x80053960 / 0x80054178 - the two draw-shape helpers the cockpit band (`menu/fn_802E4978.cpp`,
 * 0x802E4978-0x802E7408) calls (rule 2: this range owns the addresses; the signatures are that
 * consumer's call sites, neither body being written yet).  0x80053960 sets a four-word colour run on
 * the draw-shape state, 0x80054178 takes the 2D vertex pair it rewrites. */
void fn_80053960(u32, s32, s32, u32);
void fn_80054178(s16* pos);
/* Added by `enemy/em_action.cpp` (rule 2: the declaration belongs with the owner TU, which
 * had not declared it yet). */
s32 fn_80050C40(void* a, void* b);
void fn_8004FFC8(void* a, void* b, void* c, f32 d);
void fn_800516F0(void* out);
/* 0x8004EAF4 - the 16-byte struct copy MWCC emits for a four-word assignment (`dst[0..3] = src[0..3]`)
 * as four separate word moves.  Added with `menu/menu_row.cpp`, its consumer (rule 2). */
void fn_8004EAF4(void* dst, const void* src);
/* 0x8004EA58 - the 8-row item table's insert: it fills the first empty row with the caller's 16-byte
 * record, and when every row is full shifts the table down one row and writes the record into the
 * last.  Added with `menu/menu_row.cpp`, its consumer (rule 2). */
void fn_8004EA58(const void* entry);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
/* 0x800514AC - the `out = mtx * v + trans` helper (map name
 * `mulVecMatAddTrans__FPQ34nw4r4math4VEC3PQ34nw4r4math5MTX34`, so C++ linkage at global scope).
 * Added with `enemy/fn_801A4504.cpp`, its consumer (rule 2/9). */
void mulVecMatAddTrans(VEC3* v, MTX34* m);

/* 0x80050990 - build an MTX34 from the Z-X-Y Euler triple `_CP_VECTOR`.  The address sits between the
 * registered `fn_8004C9A0.cpp` and `draw_shape.cpp` units, so no unit owns it yet; the callers are
 * `ef/eft053.cpp` and `Pl/pl_act.cpp` (which declared its own copy before this header carried it). */
struct _CP_VECTOR;
void cpSetRotMatrixZXY(_CP_VECTOR* rot, MTX34* mtx);

/* 0x80050F80 - the squared distance between two 3-float vectors (`fn_8004CAD8.cpp`'s range).  The
 * same signature `ai/fn_802D0F34.h` carries, so a TU including both sees one declaration.  Added
 * with `ef/eft053.cpp`. */
f32 fn_80050F80(const void* a, const void* b);

/* 0x80050E70 - copy the engine's `Vec` into an nw4r `VEC3` (map mangling
 * `vec_to_mh_vec3__FPQ34nw4r4math4VEC3P3Vec`).  Added with `ef/eft053.cpp`, whose state machines
 * convert the family's placement-table entries. */
struct Vec;
void vec_to_mh_vec3(VEC3* dst, struct Vec* src);
#endif

#endif /* MHTRI_FN_8004CAD8_H */
