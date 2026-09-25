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

/* The stage's effect size class (0x400 / 0x800 / 0x8 - see fn_800F9380). */
u32 fn_800E3B3C(void);

/* The model's base initialiser: stores the initial joint table's address at +0x00 of `self`
 * (`src/sound/fn_800DD1F0.cpp` defines it as `void fn_800E3B2C(MHchar* self)`; the parameter is
 * `MHchar*` because its own call sites pass the actor's model, and the body only writes +0x00, so the
 * enemy band's 12-byte helper constructor `enemy/fn_80147CE0.cpp`'s `fn_80147E2C` calls it with its
 * own record).  Declared here on landing that consumer (rule 2: the owner's header); the older
 * `unsplit/sound.h` no-argument view stays for the C-side consumer that uses it. */
void fn_800E3B2C(MHchar* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_FN_800DD1F0_H */
