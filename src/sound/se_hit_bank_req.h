/* sound/se_hit_bank_req.h - leaf header: `se_hit_bank_req` (0x800DC0A8), owned by `sound/fn_800D7F54.cpp`. C linkage. */
#ifndef MHTRI_SOUND_SE_HIT_BANK_REQ_H
#define MHTRI_SOUND_SE_HIT_BANK_REQ_H

#include "types.h"
#include "nw4r/math.h"

struct _ENEMY_WORK;
struct _PLW;
struct _HIT_W;

#ifdef __cplusplus
extern "C" {
#endif
/* Requests the hit sound the hit's result byte selects from the player's SE bank tables (none for result 0xFF, or
 * when `em` is outside the stage area). */
void se_hit_bank_req(struct _ENEMY_WORK* em, struct _PLW* plw, struct _HIT_W* hit, VEC3* pos);
#ifdef __cplusplus
}
#endif

#endif
