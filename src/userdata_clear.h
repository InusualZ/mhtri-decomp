/* userdata_clear.h - leaf header (docs/plan.md 6.5 rule 2): `userdata_item.cpp`'s user data reset, for `menu/movie.cpp`.  GUESS names (the registry carries them). */
#ifndef MHTRI_USERDATA_CLEAR_H
#define MHTRI_USERDATA_CLEAR_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800498EC / 0x8004991C - clear the user data / fill it with the new-game defaults. */
void userdata_clear(void);
void userdata_new_game_init(s32 mode);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_USERDATA_CLEAR_H */
