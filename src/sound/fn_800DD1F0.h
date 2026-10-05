/*
 * Declarations owned by `sound/fn_800DD1F0.cpp` (docs/plan.md 6.5 rule 2).  The effect manager's stage
 * size class comes from the sound unit's quest/challenge data.
 */
#ifndef MHTRI_SOUND_FN_800DD1F0_H
#define MHTRI_SOUND_FN_800DD1F0_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
class MHchar;
#endif

#ifdef __cplusplus
extern "C" {
#endif
/* 0x800E2F40 - writes the TEV alpha pair of material `mat` on the character model `self` (nothing when the
 * model has no resource).  `mode` is the stage selector, `a`/`d` the two 8-bit levels; the trailing float
 * is not read by the body.  Added with `enemy/em024_ai.cpp`.  The name is a GUESS from the body. */
#ifdef __cplusplus
void mhchar_mat_tev_set(MHchar* self, s32 mat, s32 mode, u8 a, s32 b, s32 c, u8 d, f32 e);
#endif


/* The stage's effect size class (0x400 / 0x800 / 0x8 - see fn_800F9380).
 *
 * Two consumers spell this differently, and both are right for their own call site: `ef/eft_res.cpp`'s
 * `fn_800F9380` calls it with no argument at all (retail performs no `r3` setup there, so the actor
 * pointer is not live), while `Pl/fn_80224AC4.cpp` passes the actor (`lwz r3,0(r29)` before the `bl`).
 * The two spellings cannot coexist, and changing the no-argument one would cost `fn_800F9380` its
 * match, so the parameterised view is behind this switch, set by the unit that needs it. */
#ifdef MHTRI_FN_800E3B3C_TAKES_ACTOR
struct _PLW;
u32 fn_800E3B3C(struct _PLW* self);
#else
u32 fn_800E3B3C(void);
#endif

/* The model's base initialiser: stores the initial joint table's address at +0x00 of `self`
 * (`src/sound/fn_800DD1F0.cpp` defines it as `void fn_800E3B2C(MHchar* self)`; the parameter is
 * `MHchar*` because its own call sites pass the actor's model, and the body only writes +0x00, so the
 * enemy band's 12-byte helper constructor `enemy/fn_80147CE0.cpp`'s `em_res_user_data_ctor` calls it with its
 * own record).  Declared here on landing that consumer (rule 2: the owner's header); the older
 * `unsplit/sound.h` no-argument view stays for the C-side consumer that uses it. */
void fn_800E3B2C(MHchar* self);

/* 0x800E3264 - the per-track value setter (defined here at src/sound/fn_800DD1F0.cpp:300).
 * The older `(void*, void*)` view in `unsplit/sound.h` stays for that band's other consumers. */
void fn_800E3264(MHchar* track, u32 arg);

/* 0x800E1640 / 0x800E16DC / 0x800E2198 - the model-block setters the Pl part accessors tail-call.
 * The addresses sit inside this unit's range (0x800DD1F0-0x800E3CBC), so the declarations belong to
 * this header (rule 2); `Pl/fn_802693c4.cpp` and `Pl/pl_act.cpp` are the call sites.  The value
 * setters' return is `u32` because `Pl_frame_check`/`Pl_motion_end_ck` pass it straight back to their own
 * callers, which compare it against 1. */
void fn_800E1640(MHchar* chr, f32 value);
u32 fn_800E16DC(MHchar* chr, u16 motion, s32 flag);
u32 fn_800E2198(MHchar* chr, s32 flag);
void fn_800E26B4(MHchar* chr, u32 index, f32 value);
/* 0x800E0A14 - the joint's world position through the model; the same signature `unsplit/sound.h`
 * carries, so a TU that includes both headers sees one declaration, not an overload. */
void fn_800E0A14(void* chr, u32 joint, Mtx34* out);
/* 0x800DD514 - the fixed SE request the effect step makes when its joint window is hit (the body
 * loads 46/13 and tail-calls the SE request path).  Added with `ef/fn_8030681C.cpp`. */
void fn_800DD514(nw4r::math::VEC3* pos);
/* 0x800DDCB8 - request the map's SE `id` at `pos`.  Its map name is the global C++ mangling
 * `map_se_req__FUcPQ34nw4r4math4VEC3`, so the declaration has to sit at global scope with exactly
 * these parameter types to reproduce it.  Added with `ef/eft053.cpp`; `ef/eft004.cpp` carries an
 * older local copy of the same declaration. */
void map_se_req(u8 id, nw4r::math::VEC3* pos);
/* 0x800E26C4 - releases the model/SE handle the caller's record holds.  Added with
 * `lobby/lb_quest_ui.cpp`: its per-row effect step hands the record's pointer here and then flags
 * the record through `ef/eft_res.cpp`'s `fn_800F8A44`, so the declaration belongs with this unit
 * (rule 2) rather than in the lobby source. */
void fn_800E26C4(void* handle);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_FN_800DD1F0_H */
