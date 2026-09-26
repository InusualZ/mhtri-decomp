/* The player action unit `Pl/pl_act.cpp`: the two condition predicates the enemy units test, plus
 * two plain `fn_` helpers.
 *
 * `Pl_condition_ck`/`Pl_dm_condition_ck` are C++ free functions (the map names are their manglings),
 * moved here from `enemy/fn_8012BDF4.cpp` (docs/plan.md 6.5 rule 2).  A C++ consumer calls the real
 * declaration and the front-end mangles it back to the map's name; a C consumer gets the map's
 * spelling under `extern "C"` (rule 9's C limitation).
 */
#ifndef MHTRI_PL_PL_ACT_H
#define MHTRI_PL_PL_ACT_H

#include "types.h"
#include "nw4r/math.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

s32 fn_8027AC18(void* arg);
u32 fn_8027BC48(s32 arg);

/* Declarations added with `Pl/fn_80262940.cpp` (the player main/control cluster 0x80262940-0x802693C4),
 * which calls them; all four are in this unit's `.text` range (0x80276B58-0x8027D684). */
void fn_80276B58(struct _PLW* self, s32 value);
void fn_802770E8(struct _PLW* self, u32 table, s32 arg2);
void fn_8027AC00(struct _PLW* self);
void fn_8027AC0C(struct _PLW* self);
void fn_8027D4F0(struct _PLW* self);
void fn_8027D510(struct _PLW* self);
s32 fn_80277DAC(struct _PLW* self, s32 a, f32 b, f32 c);
u32 fn_8027A198(struct _PLW* self);
void fn_8027A17C(struct _PLW* self);
void fn_8027A190(struct _PLW* self, s32 a);
u32 fn_802790E4(struct _PLW* self, u32 mask);
u32 fn_8027BCE0(struct _PLW* self);
u32 fn_8027D40C(struct _PLW* self, s32 arg);

/* 0x8027D310 - the three-argument target-check helper `ai/fn_802CC794.cpp` and the enemy units
 * call; C linkage (unmangled `fn_80278310`), added with its first consumer (rule 2). */
u32 fn_80278310(u8 a, nw4r::math::VEC3* v, u8 b);

/* 0x8027D050 - the actor's stored carve value, scanned out of the item table.  The owner defines the
 * return `u8`; the retail consumer `Pl/fn_80229ECC.cpp` keeps the raw return in a register and masks
 * it per use (`clrlwi r0,r29,24` before each shift), which MWCC only emits when the declaration is
 * wider than a byte, so the consumer view declared here is 32-bit.  Unmangled (`fn_8027D050`), so the
 * wider return changes no link name. */
u32 fn_8027D050(struct _PLW* self);

/* 0x80277C58 / 0x80278674 - the two unmangled motion helpers `Pl/fn_8024F200.cpp` drives; both are
 * defined by this unit (`Pl/pl_act.cpp:1073` and `:3526`), so their declarations live here. */
void fn_80277C48(struct _PLW* self, s32 arg);
void fn_80277C58(struct _PLW* self);
void fn_80278674(struct _PLW* self, s16 motion, u8 a);

#ifdef __cplusplus
}

u32 Pl_condition_ck(struct _PLW* work, u32 condition);    /* -> Pl_condition_ck__FP4_PLWUl */
u32 Pl_dm_condition_ck(struct _PLW* work, u32 condition); /* -> Pl_dm_condition_ck__FP4_PLWUl */
#else
u32 Pl_condition_ck__FP4_PLWUl(struct _PLW* work, u32 condition);
u32 Pl_dm_condition_ck__FP4_PLWUl(struct _PLW* work, u32 condition);
#endif

#endif /* MHTRI_PL_PL_ACT_H */
