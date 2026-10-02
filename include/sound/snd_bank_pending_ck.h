/* sound/snd_bank_pending_ck.h - the declaration of `snd_bank_pending_ck`, which `sound/fn_800E46E8.cpp` owns (docs/plan.md 6.5 rule 2); the signature is the one its callers use. */
#ifndef MHTRI_SOUND_SND_BANK_PENDING_CK_H
#define MHTRI_SOUND_SND_BANK_PENDING_CK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
/* untyped: the stream manager and the sound objects both keep their flag word at +4: a caller-owned payload */
u32 snd_bank_pending_ck(void* work, u32 mask);
#ifdef __cplusplus
}
#endif

#endif
