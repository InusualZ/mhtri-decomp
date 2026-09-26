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
void fn_802770E8(struct _PLW* self, u32 table, s32 arg2);   /* the owner's own `void` definition */
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
/* 0x8027D40C - the number of set bits in the actor's action-lock word.  The owner defines
 * `extern "C" s32 fn_8027D40C(_PLW* self)` (one parameter); the `s32 arg` this declaration used to
 * carry was never read, and `Pl/fn_8025F088.cpp` measured the extra `li r4,2` it cost (rule 2: the
 * owner's signature wins). */
s32 fn_8027D40C(struct _PLW* self);

/* The rest of this unit's `.text` that `Pl/fn_802489D4.cpp` (0x802489D4-0x8024F200) calls; the
 * signatures are the owners' own definitions in `src/Pl/pl_act.cpp` (rule 2: this header is the
 * owner's).  `fn_80277C58`, which both this unit and `Pl/fn_8024F200.cpp` drive, is declared in the
 * block below. */
void fn_802771A0(struct _PLW* self, s32 value);
s32 fn_8027A340(struct _PLW* self);
void fn_8027A57C(struct _PLW* self, u16 a, u8 b);
void fn_8027BE2C(struct _PLW* self);
void fn_80277BC4(struct _PLW* self, u8 flag);
void fn_80277C50(struct _PLW* self, s16 value);
void fn_80278564(struct _PLW* self, u32 value);
void fn_80276CE8(struct _PLW* self, s16 value);

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

/* Added with `Pl/fn_80273B14.cpp`, whose act entry and frame step call them (rule 2):
 * 0x8027AC2C is the act's status-block store, 0x80278BE4 the act tail the frame step falls into. */
void fn_8027AC2C(struct _PLW* self, u32 a, u32 b);
void fn_80278BE4(struct _PLW* self);

/* 0x80277974 - the shared attack set-up the shell band's `fn_80284204` fills its attack entry with
 * (`hit` is this unit's `_HIT_W`, spelled `void*` here so the consumer needs no type of its own);
 * the owner defines it `extern "C"` in `Pl/pl_act.cpp`, so the declaration lives here. */
void fn_80277974(struct _PLW* self, void* hit, u8* base, u16 idx, s32* ids, u16 flags);

#ifdef __cplusplus
}

u32 Pl_condition_ck(struct _PLW* work, u32 condition);    /* -> Pl_condition_ck__FP4_PLWUl */
u32 Pl_dm_condition_ck(struct _PLW* work, u32 condition); /* -> Pl_dm_condition_ck__FP4_PLWUl */

/* 0x80278814 - the gunner predicate `Pl/fn_802489D4.cpp` gates a motion on; the owner defines it at
 * C++ scope in `src/Pl/pl_act.cpp`, so this is the callable spelling of the map name
 * `Pl_suimen_ck__FP4_PLW` (docs/plan.md 6.5 rule 9). */
u32 Pl_suimen_ck(struct _PLW* work);

/* 0x8027CC44 / 0x8027CDF0 - the gunner's aim position and origin the shell band reads; this unit
 * (`Pl/pl_act.cpp`) defines both at C++ scope, so these are the callable spellings of the map names
 * `Pl_get_gunner_pos__FP4_PLWPQ34nw4r4math4VEC3l` and `Pl_get_gunner_vec__FP4_PLWP10_CP_VECTOR`. */
void Pl_get_gunner_pos(struct _PLW* self, nw4r::math::VEC3* out, s32 arg2);
void Pl_get_gunner_vec(struct _PLW* self, struct _CP_VECTOR* out);
#else
u32 Pl_condition_ck__FP4_PLWUl(struct _PLW* work, u32 condition);
u32 Pl_dm_condition_ck__FP4_PLWUl(struct _PLW* work, u32 condition);
#endif

#endif /* MHTRI_PL_PL_ACT_H */
