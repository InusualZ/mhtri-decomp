/* Leaf header (docs/plan.md 6.5 rule 2): `enemy/enemy_control.cpp`'s `senko_set`, the one declaration `ai/ai_npc.cpp` needs.  C++
 * linkage: the map row is the mangling. */
#ifndef MHTRI_ENEMY_SENKO_SET_H
#define MHTRI_ENEMY_SENKO_SET_H

#include "types.h"
#include "nw4r/math.h"

/* 0x8014177C (0x80): fills the first free flash record with a position, a scale, an area and a countdown. */
void senko_set(nw4r::math::VEC3* pos, f32 scale, u8 area, s16 countdown);

#endif /* MHTRI_ENEMY_SENKO_SET_H */
