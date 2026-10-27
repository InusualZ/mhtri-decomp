/* sound/mhchar_file_release.h - leaf header (docs/plan.md 6.5 rule 2): `sound/mhchar.cpp`'s `mhchar_file_release`,
 * for `Pl/player_control.cpp`. */
#ifndef MHTRI_SOUND_MHCHAR_FILE_RELEASE_H
#define MHTRI_SOUND_MHCHAR_FILE_RELEASE_H

#include "types.h"
#include "pl.h"   /* MHchar */

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800E2678 (0x8): releases the model's resource file. */
void mhchar_file_release(MHchar* chr);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_MHCHAR_FILE_RELEASE_H */
