/* sound/snd_track_pause.h - leaf header (docs/plan.md 6.5 rule 2): `sound/fn_800E46E8.cpp`'s track pause and resume, for `menu/movie.cpp`.  GUESS names (the registry carries them). */
#ifndef MHTRI_SOUND_SND_TRACK_PAUSE_H
#define MHTRI_SOUND_SND_TRACK_PAUSE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800E4ACC / 0x800E4B0C - pause and resume the tracks (pending state 2 / 4). */
void snd_track_pause(void);
void snd_track_resume(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_SND_TRACK_PAUSE_H */
