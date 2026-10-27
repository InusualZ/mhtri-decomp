/* sound/snd_fx_hooks_set.h - leaf header (docs/plan.md 6.5 rule 2): `snd_fx_hooks_set`, which `sound/snd_stream_reloc.cpp` owns. */
#ifndef MHTRI_SOUND_SND_FX_HOOKS_SET_H
#define MHTRI_SOUND_SND_FX_HOOKS_SET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800E87E0 (0x4): installs the AXFX allocation hooks. */
/* untyped: opaque handle passed through - the AXFX alloc/free hooks */
void snd_fx_hooks_set(void* alloc, void* free);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_SND_FX_HOOKS_SET_H */
