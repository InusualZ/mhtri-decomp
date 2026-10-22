/* Leaf header (docs/plan.md 6.5 rule 2): `eft_em_spawn_tinted_ex` (0x80305930), which `lobby/fn_8030121C.cpp`
 * defines - the entry of the tinted enemy-effect spawner that passes the caller's colour and trailing word through
 * (its body narrows `id` and branches into the shared spawner `eft_em_spawn_tinted` also enters). */
#ifndef MHTRI_LOBBY_EFT_EM_SPAWN_TINTED_EX_H
#define MHTRI_LOBBY_EFT_EM_SPAWN_TINTED_EX_H

#include "types.h"
#include "nw4r/math.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif

void eft_em_spawn_tinted_ex(struct _ENEMY_WORK* self, u8 id, u32 joint, nw4r::math::VEC3* pos, f32 scale, u32 color,
                            u32 arg);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_EFT_EM_SPAWN_TINTED_EX_H */
