/* Leaf header for `sound_job_request` (0x800E3B8C), which `sound/sound_job.cpp` owns: several consumers still
 * carry their own spellings of it (one in the band header `unsplit/sound.h`), so its typed declaration stays
 * out of the owner's full header `sound/sound_job.h` (rule 2).
 */
#ifndef MHTRI_SOUND_SOUND_JOB_REQUEST_H
#define MHTRI_SOUND_SOUND_JOB_REQUEST_H

#include "types.h"
#include "nw4r/math.h"

struct PrimRec;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800E3B8C - takes a record from list `list` and fills it (layer, engine handle, draw kind, flags, an optional
 * position copied in with flag 0x1 set, and the per-frame callback); returns the record, or null when the list
 * is full. */
struct PrimRec* sound_job_request(u8 list, u8 layer, s32 handle, u8 kind, u16 flags, const VEC3* pos,
                                  void (*callback)(struct PrimRec*));

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_SOUND_JOB_REQUEST_H */
