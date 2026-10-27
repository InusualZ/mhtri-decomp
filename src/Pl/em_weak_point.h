/*
 * Pl/em_weak_point.h - the hit-part record an enemy's part list (`_ENEMY_WORK::field_0x8C8`, ended by joint -1) and a
 *   body slot (`_BODY_W::data`) point at; `Pl/pl_hit.cpp` turns one into a sphere or a capsule (`em_part_sphere_get`)
 *   and tests its flags, `Pl/pl_body_push.cpp` pushes bodies apart with it and `ai/ai_npc.cpp` aims at it (rule 1).
 */
#ifndef MHTRI_PL_EM_WEAK_POINT_H
#define MHTRI_PL_EM_WEAK_POINT_H

#include "types.h"
#include "nw4r/math.h"

/* One hit part: the joint it sits on (1000 for the owner's own position), the second joint a two-joint capsule spans,
 * its shape (0 sphere, 1 capsule on one joint, 2 capsule across `joint` and `joint_b`), its kind, flags (0x002 hidden,
 * 0x010 disabled, 0x100 broken; `hit_push_enabled_ck` wants none of 0xD2), the part `em_flags836_ck` tests, the radius
 * and the two offsets on the joints.  size: 0x2C */
struct EmWeakPoint {
    /* +0x00 */ s16 joint;
    /* +0x02 */ s16 joint_b;
    /* +0x04 */ u16 shape;
    /* +0x06 */ u16 rates_index;    /* the part's damage-percentage record (`em_attacker_rec_get`), and its stun attacker index */
    /* +0x08 */ u16 kind;
    /* +0x0A */ u16 flags;
    /* +0x0C */ u16 part;
    /* +0x0E */ u8 unused_0x0E[0x10 - 0x0E];
    /* +0x10 */ f32 radius;
    /* +0x14 */ nw4r::math::VEC3 offset;
    /* +0x20 */ nw4r::math::VEC3 offset_b;
};

#endif /* MHTRI_PL_EM_WEAK_POINT_H */
