/* Declarations of the player model-state setter `pl_model_state_set` (0x80267270, `Pl/pl_act_step.cpp`, defined
 * `void (_PLW*, u32, s32, u16)`) and the player-control entry points `Pl/player_control.cpp` defines, for the enemy,
 * arena and lobby consumers.
 */
#ifndef MHTRI_PL_FN_80262940_H
#define MHTRI_PL_FN_80262940_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* r3 the player work, r4 the action, r5/r6 the two scalars the owner's body stores beside it. */
void pl_model_state_set(struct _PLW* self, u32 action, s32 a, u16 b);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
/* 0x802673A4 - sets the number of players in play (the map's `set_max_player__Fl`). */
void set_max_player(s32 value);

/* The player-work setup/motion entry points the arena and lobby tasks drive (the map's `__Fv` / `__Fl`
 * manglings): 0x80267874 builds the players' move work, 0x802681D4 loads their init data,
 * 0x80267D00 runs one movement step, 0x802681EC answers whether every player's start motion finished and
 * 0x80268260 starts the motion (-1 = every player). */
void init_player_work(void);
void player_init_data_load(void);
void player_control_move(void);
u32 player_move_start_ck(void);
void player_move_start(s32 index);
#endif

#endif /* MHTRI_PL_FN_80262940_H */
