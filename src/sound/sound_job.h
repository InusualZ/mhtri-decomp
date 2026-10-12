/*
 * sound/sound_job.h - the declarations `src/sound/sound_job.cpp` owns (docs/plan.md 6.5 rule 2), apart from
 * `sound_job_request`, whose typed declaration is the leaf header `sound/sound_job_request.h` (the band header
 * `unsplit/sound.h` still carries a foreign spelling of it, so a unit that includes the band cannot see both).
 * `PrimRec`, the record these functions fill, is defined in `sound/fn_800E3CBC.h`.
 */
#ifndef MHTRI_SOUND_SOUND_JOB_H
#define MHTRI_SOUND_SOUND_JOB_H

#include "types.h"

struct PrimRec;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800E3B3C - the stage's effect size class: 0x800 / 0x400 on a split screen (by the actor's +0x08 byte),
 * 0x8 otherwise.
 *
 * Two consumers spell it differently, and both are right for their own call site: `ef/eft_res.cpp`'s
 * `eft_stage_spawn_flag` calls it with no argument (retail performs no `r3` setup there, so the actor pointer
 * is not live), while `lobby/lb_equip_page.cpp` passes the actor.  The two spellings cannot coexist, and
 * changing the no-argument one would cost `eft_stage_spawn_flag` its match, so the parameterised view is
 * behind this switch, set by the unit that needs it. */
#ifdef MHTRI_SPLIT_SCREEN_SIZE_CLASS_TAKES_ACTOR
struct _PLW;
u32 split_screen_size_class(struct _PLW* self);
#else
u32 split_screen_size_class(void);
#endif

/* 0x800E3C1C - takes the next free record of list `list` and clears it; returns null when the list is full. */
struct PrimRec* prim_rec_alloc(u8 list);

/* 0x800E3C90 - clears a record's state, handle, flags and callback and seeds its two cull bounds. */
void prim_rec_init(struct PrimRec* rec);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_SOUND_JOB_H */
