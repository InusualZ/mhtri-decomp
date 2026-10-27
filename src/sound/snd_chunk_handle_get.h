/* Leaf header (docs/plan.md 6.5 rule 2): `sound/snd_bank_loader.cpp`'s loaded-chunk handle lookup (registry name; GUESS). */
#ifndef MHTRI_SOUND_SND_CHUNK_HANDLE_GET_H
#define MHTRI_SOUND_SND_CHUNK_HANDLE_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800F0C14 - the handle of the loaded sound chunk `chunk`, or -1 when it is not loaded.  The body sign-extends its
 * result, and retail callers compare the whole register without an `extsb`, so the result is spelled `s32`. */
s32 snd_chunk_handle_get(s32 chunk);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_SND_CHUNK_HANDLE_GET_H */
