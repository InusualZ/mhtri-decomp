/* ef/GameMode_ck.h - leaf header: `GameMode_ck` (0x800CF208), owned by the `ef/fn_800CDB2C` band. The band's full header
 * `ef/fn_800CDB2C.h` cannot be included beside `ef/eft013_fx.cpp`: it declares `ran_suu` as `u16` where that unit keeps
 * the `s32` view its matched rows need. */
#ifndef MHTRI_EF_GAMEMODE_CK_H
#define MHTRI_EF_GAMEMODE_CK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
/* Returns the current game mode byte. */
u8 GameMode_ck(void);
#ifdef __cplusplus
}
#endif

#endif
