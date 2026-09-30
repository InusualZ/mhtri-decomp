/* The enemy-side consumer of `Pl/fn_80262940.cpp`'s action dispatcher.
 *
 * `pl_model_state_set` (`.text` 0x80267270, inside that unit's range 0x80262940..0x802693C4) was declared
 * in three wrong places before this header existed (docs/plan.md 6.5 rule 2): `enemy/fn_801B0010.cpp`
 * carried a local copy, `include/unsplit/Pl.h` - a fallback band - carried another, and
 * `include/Pl/fn_8025F088.h` carries a third, differently-typed one whose range does not cover the
 * address.  This is the owner's header; the signature is the owner's own definition
 * (`src/Pl/fn_80262940.cpp:478`, `void (_PLW*, u32, s32, u16)`), and the return is `void` there.
 *
 * The copy in `Pl/fn_8025F088.h` is gone: that header includes this one now, so a translation unit
 * that sees both gets one `void` declaration instead of the `u32` one that used to clash with it
 * ((10505) illegal overloading).
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
/* 0x802673A4 - sets the number of players in play; the map's `set_max_player__Fl`.  Added with
 * `quest/arenatask.cpp` (rule 2: this range owns the address). */
void set_max_player(s32 value);

/* The player-work setup/motion entry points the arena and lobby tasks drive (the map's `__Fv` / `__Fl`
 * manglings; owner bodies `src/Pl/fn_80262940.cpp`).  Added with `quest/arenatask.cpp` (rule 2: this range
 * owns the addresses): 0x80267874 builds the players' move work, 0x802681D4 loads their init data,
 * 0x80267D00 runs one movement step, 0x802681EC answers whether every player's start motion finished and
 * 0x80268260 starts the motion (-1 = every player). */
void init_player_work(void);
void player_init_data_load(void);
void player_control_move(void);
u32 player_move_start_ck(void);
void player_move_start(s32 index);
#endif

#endif /* MHTRI_PL_FN_80262940_H */
