/* Declarations owned by `lobby/fn_8030121C.cpp`: the single-effect spawners the enemy action bands call. */
#ifndef MHTRI_LOBBY_FN_8030121C_H
#define MHTRI_LOBBY_FN_8030121C_H

#include "types.h"
#include "nw4r/math.h"

struct _ENEMY_WORK;

/* One `_tr_flag` record (`lb_tr_flag_get`, the table `lb_tr_flag_data` points at): the goods id, its name and its
 * description.  size: 0xC */
typedef struct LbTrFlag {
    /* +0x00 */ u16 id_0x00;     /* 0xFFFF ends the table */
    /* +0x02 */ u8 pad_0x02[2];
    /* +0x04 */ s8* name_0x04;
    /* +0x08 */ s8* text_0x08;
} LbTrFlag; /* size: 0xC */

#ifdef __cplusplus
extern "C" {

/* 0x80304508 - spawns effect `id` for the enemy work at `joint` (`pos` an optional offset, `scale` the size); its body
 * reads only `id` before tail-calling the effect allocator. */
void eft_em_spawn(struct _ENEMY_WORK* self, u32 id, u32 joint, nw4r::math::VEC3* pos, f32 scale);
}
#endif

/* The tinted spawn the enemy bands reach through this header (`enemy/fn_8011D448.cpp`, `enemy/em020_prog.cpp`) is
 * `ef/eft_em_tint.cpp`'s. */
#include "ef/eft_em_tint.h"

#endif /* MHTRI_LOBBY_FN_8030121C_H */
