/*
 * Declarations owned by `sound/fn_800DD1F0.cpp` (docs/plan.md 6.5 rule 2).  The effect manager's stage
 * size class comes from the sound unit's quest/challenge data.
 */
#ifndef MHTRI_SOUND_FN_800DD1F0_H
#define MHTRI_SOUND_FN_800DD1F0_H

#include "types.h"

#ifdef __cplusplus
class MHchar;
#endif

#ifdef __cplusplus
extern "C" {
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
 * enemy band's 12-byte helper constructor `enemy/fn_80147CE0.cpp`'s `fn_80147E2C` calls it with its
 * own record).  Declared here on landing that consumer (rule 2: the owner's header); the older
 * `unsplit/sound.h` no-argument view stays for the C-side consumer that uses it. */
void fn_800E3B2C(MHchar* self);

/* 0x800E3264 - the per-track value setter (defined here at src/sound/fn_800DD1F0.cpp:300).
 * The older `(void*, void*)` view in `unsplit/sound.h` stays for that band's other consumers. */
void fn_800E3264(MHchar* track, u32 arg);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_FN_800DD1F0_H */
