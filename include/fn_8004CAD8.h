/*
 * Declarations for the symbols `src/fn_8004CAD8.cpp` owns (docs/plan.md 6.5, rule 2).  The parameters
 * are the owner's `long`, which is the mangling's (`wii_sysmsg_gen__FlPcl`) - `int` would ask for a
 * different symbol.
 */
#ifndef MHTRI_FN_8004CAD8_H
#define MHTRI_FN_8004CAD8_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
void wii_sysmsg_gen(long id, char* buf, long a);
#endif

/* 0x80050A90 - `calcVecAng2`, the two-vector angle helper: it loads the two vectors' x/z floats and
 * tail-calls 0x80050A40, so the angle comes back in r3.  Added when `enemy/fn_80147CE0.cpp`
 * registered as a consumer (rule 2); the return is the 16-bit angle its three consumers mask
 * (`clrlwi r3,r3,16`) and sign-extend (`extsh`), spelled `s32` here as `enemy/fn_801550FC.cpp` does.
 * C++ linkage (outside the `extern "C"` block below): the map name is the mangling, not a plain
 * name, and `enemy/fn_801550FC.cpp` declares it that way. */
#ifdef __cplusplus
s32 calcVecAng2(VEC3* a, VEC3* b);
#endif

/* C linkage: the target symbol is the unmangled `fn_80051570` (.text 0x80051570, a 4-byte `blr`).
 * Moved out of `src/g3d/g3d_anmchr.cpp` on landing (docs/plan.md 6.5, rule 2): that range was
 * written before this owner registered, so its local `extern "C"` declaration was a boundary
 * artefact. */
#ifdef __cplusplus
extern "C" {
#endif
u32 fn_80051570(u32);
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
/* 0x80050CF4 - the SDK vector subtract (`ps_sub` on two paired loads): `dst = a - b`. */
void PSVECSubtract(f32* dst, const f32* a, const f32* b);
/* 0x800504D4/0x8005050C - the two GX pipe-setup helpers `g3d/g3d_state.cpp` calls (rule 2, moved
 * out of that unit's local extern block on landing, 2026-09-25). */
void fn_800504D4(void* pOut);
void fn_8005050C(void* pOut);
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
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
/* 0x800514AC - the `out = mtx * v + trans` helper (map name
 * `mulVecMatAddTrans__FPQ34nw4r4math4VEC3PQ34nw4r4math5MTX34`, so C++ linkage at global scope).
 * Added with `enemy/fn_801A4504.cpp`, its consumer (rule 2/9). */
void mulVecMatAddTrans(VEC3* v, MTX34* m);
#endif

#endif /* MHTRI_FN_8004CAD8_H */
