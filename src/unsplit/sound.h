/* Not-yet-reconstructed `sound`-band symbols (the bracketing registered units both name `sound`).
 *
 * Declarations moved here from the consumer units' `src/` files (docs/plan.md 6.5 rule 2:
 * an extern lives with the TU that owns the symbol).  The signature set is what the
 * consumers used; where only the parameter spelling differed the wider form is kept.
 */
#ifndef MHTRI_UNSPLIT_SOUND_H
#define MHTRI_UNSPLIT_SOUND_H

#include "types.h"
#include "nw4r/math.h"

struct MHchar;

#ifdef __cplusplus
extern "C" {
#endif

void mhchar_model_mtx_update(struct MHchar* ch);
void fn_800E09D0(struct MHchar* ch, const Vec3* v, f32 arg2);
u32 mhchar_mat_num_get(struct MHchar* ch);
void fn_800E2EBC(struct MHchar* ch, u32 index, s32 arg2);
void fn_800E30DC(struct MHchar* ch, u32 index, s32 arg2);
void setScaleAll__6MHcharFf(struct MHchar* ch, f32 scale);


/* Merged 2026-09-24: a second lane formalized into this shared header.  Declarations it
 * needed that the first did not; a symbol both named keeps the first (verified) signature. */
void fn_800DD7E0(struct MHchar* model, Vec3* pos, s32 flag);
void mhchar_joint_mtx_get(void* mhchar, u32 joint, Mtx34* out);
int copied_mat_access_valid_ck(void* handle);
/* Merged 2026-09-24: the 0x800EF7D8 proposal's consumer needed this one. */
void* sound_job_request(s32 a, u8 b, s32 c, s32 d, s32 e, void (*cb)(void));
/* .sdata 0x80791460 - the two primitive-record lists' capacities, read by `sound/sound_job.cpp` and
 * `sound/fn_800E3CBC.cpp`; no registered range covers it.  GUESS name. */
extern u16 prim_rec_capacity[2];
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_UNSPLIT_SOUND_H */
