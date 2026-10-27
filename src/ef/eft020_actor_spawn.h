/* ef/eft020_actor_spawn.h - leaf header: `eft020_actor_spawn` (0x80114D28), owned by `ef/eft019.cpp`. C linkage. */
#ifndef MHTRI_EF_EFT020_ACTOR_SPAWN_H
#define MHTRI_EF_EFT020_ACTOR_SPAWN_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
/* Spawns an eft020 record of `type` on `actor` (of kind `kind`), scaled by `scale`. */
void eft020_actor_spawn(void* actor /* untyped: opaque handle - the player, enemy or NPC record `kind` names */, u8 kind,
                        u8 type, f32 scale);
#ifdef __cplusplus
}
#endif

#endif
