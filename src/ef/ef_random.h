/*
 * ef/ef_random.h - the effect library's random state word (nw4r's `ef::Random`).  `Srand` is declared here and
 *   defined inline by `ef/ef_effectsystem.cpp`, whose weak copy is the one retail keeps at 0x800A5900;
 *   `ef/ef_emitter.cpp` calls it out of line.
 * NAMES. GUESS: `nw4r::ef::Random` / `Srand` / `mState` (nw4r's effect library layout; the word is seeded by
 *   `EffectSystem::Initialize` and the emitter's initialiser and stepped by `ef_random_float`).
 */
#ifndef MHTRI_EF_EF_RANDOM_H
#define MHTRI_EF_EF_RANDOM_H

#include "types.h"

namespace nw4r {
namespace ef {

class Random {
public:
    void Srand(u32 seed);

    /* +0x00 */ u32 mState;
}; /* size: 0x04 */

} // namespace ef
} // namespace nw4r

#endif
