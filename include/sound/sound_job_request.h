/* Leaf header for two entry points `sound/fn_800DD1F0.cpp` owns (0x800E0560, 0x800E3B8C): the units that
 * call them carry their own spellings of the model object and of the request's sixth argument, so the
 * declarations stay out of the owner's full header (rule 2).  They reset a model object and request a sound/effect job.
 */
class MHchar;

#ifndef MHTRI_SOUND_SOUND_JOB_REQUEST_H
#define MHTRI_SOUND_SOUND_JOB_REQUEST_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800E0560 - resets the model object `self` points at (the shell pool runs it over a fresh
 * `shell_chara_heap_tbl` slot). */
void mhchar_reset(MHchar* self);
/* 0x800E3B8C - fills a sound/effect job slot (kind, slot, model handle, sub, flags, an optional position
 * copied into the job, and a payload word) and hands the job back. */
void sound_job_request(s32 kind, u8 slot, s32 handle, s32 sub, s32 flags, void* extra /* untyped: caller-owned payload - a position record or a callback, by caller */, s32 arg);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_SOUND_JOB_REQUEST_H */
