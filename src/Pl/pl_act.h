/* Declarations owned by `Pl/pl_act.cpp` (rule 2).  `Pl_condition_ck`/`Pl_dm_condition_ck`, the two condition
 * predicates the enemy units test, are C++ free functions (the map names are their manglings): a C++ consumer calls
 * the real declaration and the front-end mangles it back; a C consumer gets the map's spelling under `extern "C"`
 * (rule 9's C limitation).
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
void pl_pos_blend_start(struct _PLW* self, s32 frames); /* 0x8027AE28 - starts the blend of the actor's position to `target_pos_0x090` over `frames` */
u32 Pl_motion_input_ck(s32 arg);

/* The skill-slot and equipment helpers the lobby page, menu and cockpit units drive (0x802738B8-0x8027EB18). */
u8 fn_802738B8(u8 idx);
void fn_802738E8(struct _PLW* plw, u8* rec);
void fn_80273998(struct _PLW* plw, u8 idx, s32 val);
void fn_802754B4(struct _PLW* plw);
void fn_8027EB18(u8 kind);
u32 fn_8027E120(struct _PLW* worker);
/* untyped: the equipment-slot record the caller casts to its own view */
void* fn_8027E354(u8 kind, u16 index);
void pl_act_enter(struct _PLW* plw, s32 a, u16 b, u16 c);

/* The four action helpers `Pl/pl_act_step.cpp`'s main/control cluster (0x80262940-0x802673A4) calls. */
void fn_80276B58(struct _PLW* self, s32 value);
void Pl_act_set_step_table(struct _PLW* self, u32 table, s32 arg2);   /* the owner's own `void` definition */
void fn_8027AC00(struct _PLW* self);
/* 0x8027D5A4 - applies a signed charge delta to the actor's stored charge, scaled by the two charge skills, and
 * clamps it to 0..100. */
void pl_act_add_charge(struct _PLW* self, s32 delta);
void pl_act_clear_flag5bb(struct _PLW* self);
void fn_8027D4F0(struct _PLW* self);
void fn_8027D510(struct _PLW* self);
s32 fn_80277DAC(struct _PLW* self, s32 a, f32 b, f32 c);
/* 0x80277B44 - arms the actor's `+0x396` gauge/hold word (the field's own `s16`; the sibling
 * `pl_act_set_gauge_arm_skilled` at 0x80277B4C scales it by the cat-skill arms 28/29); `Pl/pl_act_step.cpp`'s
 * act-175/act-89 handlers call them. */
void pl_act_set_gauge_arm(struct _PLW* self, s16 value);
u32 fn_8027A198(struct _PLW* self);
void fn_8027A17C(struct _PLW* self);
void fn_8027A190(struct _PLW* self, s32 a);
u32 fn_802790E4(struct _PLW* self, u32 mask);
u32 fn_8027BCE0(struct _PLW* self);
/* 0x8027D40C - the number of set bits in the actor's action-lock word (one parameter, like the definition). */
s32 fn_8027D40C(struct _PLW* self);

/* The helpers `Pl/pl_act_step.cpp`'s action/handler cluster calls, with their definitions' signatures;
 * `pl_act_set_frame_timer` is declared in the block below. */
void fn_802771A0(struct _PLW* self, s32 value);
s32 fn_8027A340(struct _PLW* self);
void fn_8027A57C(struct _PLW* self, u16 a, u8 b);
void pl_act_clear_mode5c4(struct _PLW* self);
void fn_80277BC4(struct _PLW* self, u8 flag);
void fn_80277C50(struct _PLW* self, s16 value);
void pl_act_arm_flags(struct _PLW* self, u32 value);
void fn_80276CE8(struct _PLW* self, s16 value);

/* 0x80278310 - the three-argument target-check helper `ai/fn_802CC794.cpp` and the enemy units call; C linkage
 * (the map row is unmangled). */
u32 fn_80278310(u8 a, nw4r::math::VEC3* v, u8 b);

/* 0x8027D050 - the actor's stored carve value, scanned out of the item table.  The definition returns `u8`;
 * the retail consumer `Pl/fn_80229ECC.cpp` keeps the raw return in a register and masks
 * it per use (`clrlwi r0,r29,24` before each shift), which MWCC only emits when the declaration is
 * wider than a byte, so the consumer view declared here is 32-bit.  Unmangled (`fn_8027D050`), so the
 * wider return changes no link name. */
u32 fn_8027D050(struct _PLW* self);

/* 0x80277C48 (`pl_act_set_step_time`) / 0x80277C58 / 0x80278674 - the three unmangled motion helpers
 * `Pl/pl_act_step.cpp` drives. */
void pl_act_set_step_time(struct _PLW* self, s32 arg);
void pl_act_set_frame_timer(struct _PLW* self);
void fn_80278674(struct _PLW* self, s16 motion, u8 a);

/* The act entry and frame step call these: 0x8027AC2C is the act's status-block store, 0x80278BE4 the act tail
 * the frame step falls into. */
void fn_8027AC2C(struct _PLW* self, u32 a, u32 b);
void fn_80278BE4(struct _PLW* self);

/* 0x80277974 - the shared attack set-up the shell band's `fn_80284204` fills its attack entry with
 * (`hit` is this unit's `_HIT_W`, spelled `void*` here so the consumer needs no type of its own). */
void fn_80277974(struct _PLW* self, void* hit, u8* base, u16 idx, s32* ids, u16 flags);

/* 0x8027B0BC / 0x8027B358 / 0x8027B918 - the three per-frame act entries the cockpit band (`menu/fn_802E4978.cpp`)
 * drives, with the definitions' `_PLW*` parameter. */
void fn_8027B0BC(struct _PLW* self);
void fn_8027B358(struct _PLW* self);
void fn_8027B918(struct _PLW* self);

#ifdef __cplusplus
}

u32 Pl_condition_ck(struct _PLW* work, u32 condition);    /* -> Pl_condition_ck__FP4_PLWUl */
u32 Pl_dm_condition_ck(struct _PLW* work, u32 condition); /* -> Pl_dm_condition_ck__FP4_PLWUl */

/* 0x80278814 - the gunner predicate `Pl/pl_act_step.cpp` gates a motion on; defined at C++ scope, so this is the
 * callable spelling of the map name `Pl_suimen_ck__FP4_PLW` (rule 9). */
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

/* Further helpers of this unit (rule 2: the owner declares). */
struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

void Pl_act_set_motion(struct _PLW* self, u16 a, u32 b, u32 c);

void Pl_act_set_motion_slot(struct _PLW* self, u32 a, u32 b, u32 c);

/* The helpers `Pl/pl_act_step.cpp`'s act state-machine band calls; each signature is the callee's own body (its
 * prologue's argument saves and the width it narrows them to). */
void pl_act_reenter(struct _PLW* self, s32 a, s32 b, s32 c);

/* The act-entry section's act/motion request entry point, with `Pl/fn_80273B14.h`'s signature (two C-linkage
 * spellings of one name would be an illegal overload).  The `u16` third parameter is what retail's callers narrow to
 * (`pl_act_enter`/`fn_80275ADC` emit `clrlwi r6,r6,16`). */
void pl_act_enter_raw(struct _PLW* self, u8 kind, u16 no, u16 mask);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_ACT_H */
