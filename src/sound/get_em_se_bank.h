/* sound/get_em_se_bank.h - leaf header (docs/plan.md 6.5 rule 2): `sound/snd_bank_loader.cpp`'s `get_em_se_bank`, for
 *   `menu/menu_placeinfo.cpp`.  C++ linkage (the map row is the mangling `get_em_se_bank__FUc`). */
#ifndef MHTRI_SOUND_GET_EM_SE_BANK_H
#define MHTRI_SOUND_GET_EM_SE_BANK_H

#include "types.h"

/* 0x800F134C: the SE bank loaded for enemy kind `kind`. */
s32 get_em_se_bank(u8 kind);

#endif /* MHTRI_SOUND_GET_EM_SE_BANK_H */
