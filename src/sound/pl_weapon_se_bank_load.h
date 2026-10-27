/* sound/pl_weapon_se_bank_load.h - leaf header (docs/plan.md 6.5 rule 2): `sound/snd_bank_loader.cpp`'s weapon SE bank
 * load, for `Pl/player_control.cpp`.  GUESS name (the registry carries it). */
#ifndef MHTRI_SOUND_PL_WEAPON_SE_BANK_LOAD_H
#define MHTRI_SOUND_PL_WEAPON_SE_BANK_LOAD_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800EFDD8 (0x42C): loads the SE bank of the player's weapon type. */
void pl_weapon_se_bank_load(struct _PLW* plw);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_PL_WEAPON_SE_BANK_LOAD_H */
