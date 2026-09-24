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

void fn_800E0914(struct MHchar* ch);
void fn_800E09D0(struct MHchar* ch, const Vec3* v, f32 arg2);
void fn_800E25B0(void* arg0, void* arg1);
u32 fn_800E28E4(struct MHchar* ch);
void fn_800E2EBC(struct MHchar* ch, u32 index, s32 arg2);
void fn_800E30DC(struct MHchar* ch, u32 index, s32 arg2);
void fn_800E3264(void* arg0, u32 arg1);
void move__6MHcharFUs(struct MHchar* ch, u16 arg1);
void setScaleAll__6MHcharFf(struct MHchar* ch, f32 scale);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_UNSPLIT_SOUND_H */
