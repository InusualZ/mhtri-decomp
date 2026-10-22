/* sound/se_bank_req.h - leaf header (docs/plan.md 6.5 rule 2): `sound/fn_800D7F54.cpp`'s bank sound request, for `menu/movie.cpp`.  GUESS name (the registry carries it). */
#ifndef MHTRI_SOUND_SE_BANK_REQ_H
#define MHTRI_SOUND_SE_BANK_REQ_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800DBB78 - plays SE `id` of bank `bank`; returns its handle. */
s32 se_bank_req(s32 bank, s32 id);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_SE_BANK_REQ_H */
