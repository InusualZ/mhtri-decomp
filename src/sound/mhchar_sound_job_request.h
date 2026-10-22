/* Leaf header (docs/plan.md 6.5 rule 2): `sound/mhchar.cpp`'s `mhchar_sound_job_request`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_SOUND_MHCHAR_SOUND_JOB_REQUEST_H
#define MHTRI_SOUND_MHCHAR_SOUND_JOB_REQUEST_H

#include "types.h"

struct MHchar;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800E08BC - requests the character's sound job with flags `flags` when the model and channel are live (sound_job_request) */
void mhchar_sound_job_request(struct MHchar* chr, u16 flags);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_MHCHAR_SOUND_JOB_REQUEST_H */
