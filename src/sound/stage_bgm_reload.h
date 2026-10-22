/* Leaf header (docs/plan.md 6.5 rule 2): a symbol `sound/snd_bank_loader.cpp` defines, for a consumer that cannot include
 * the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_SOUND_STAGE_BGM_RELOAD_H
#define MHTRI_SOUND_STAGE_BGM_RELOAD_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800F1FCC - reloads the current map's BGM banks after a demo.  GUESS name. */
void stage_bgm_reload(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_STAGE_BGM_RELOAD_H */
