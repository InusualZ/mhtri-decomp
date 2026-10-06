/* sound/snd_bank_loader.h - the declarations `sound/snd_bank_loader.cpp` owns that other units call (docs/plan.md 6.5 rule 2).  The signatures are the ones the callers use;
 * a function the unit has not written yet is declared from its callers. */
#ifndef MHTRI_SOUND_SND_BANK_LOADER_H
#define MHTRI_SOUND_SND_BANK_LOADER_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
void fn_800F0F9C(u8 id);
#ifdef __cplusplus
}
#endif

/* 0x800EF270 / 0x800EF5C8 - set the quest stage's BGM (track, two bytes) and load the lobby quest's BGM (GUESS
 * names from the quest start). */
#ifdef __cplusplus
extern "C" {
#endif
void snd_quest_bgm_set(u8 track, u8 a, u8 b);
void snd_quest_bgm_load(void);
/* 0x800EF064 / 0x800EFAC0 - load the stage's sound bank for the map, area and rank byte, and every player slot's
 * voice banks (GUESS names from those loads; the owner spells an unused `u8` parameter on the second, its callers
 * pass none). */
void snd_stage_bank_load(u8 map, u8 area, u8 rank);
void snd_player_banks_load(void);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
/* 0x800EEEFC - resets the quest sound work (C++ scope: the map row is `quest_snd_wk_init__Fv`). */
void quest_snd_wk_init(void);
#endif

#endif
