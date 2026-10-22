/* Leaf header (docs/plan.md 6.5 rule 2): a symbol `sound/snd_bank_loader.cpp` defines, for a consumer that cannot include
 * the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_SOUND_DEMO_BGM_READY_CK_H
#define MHTRI_SOUND_DEMO_BGM_READY_CK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800F1DE0 - 1 once both demo BGM streams are ready (or unused).  GUESS name. */
u32 demo_bgm_ready_ck(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_DEMO_BGM_READY_CK_H */
