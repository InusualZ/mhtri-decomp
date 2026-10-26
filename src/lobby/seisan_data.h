/* `seisan_data`, defined by `lobby/lb_screen_step.cpp` (leaf header, rule 2). */
#ifndef MHTRI_LOBBY_SEISAN_DATA_H
#define MHTRI_LOBBY_SEISAN_DATA_H

#include "types.h"

struct LbSmithRecipe;
struct LbSmithKakouRecipe;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x806BF3C0 / 0x806BF400 / 0x80794BE8 - the make recipe tables by equipment kind (gun parts use 0x24-byte rows), the weapon upgrade tables by kind, and the decoration make table. */
extern struct LbSmithRecipe* seisan_data[16];
extern struct LbSmithKakouRecipe* kakou_data[16];
extern struct LbSmithRecipe* deco_sei_tbl_ptr;

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_SEISAN_DATA_H */
