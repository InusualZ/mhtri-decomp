/* Leaf header (docs/plan.md 6.5 rule 2): `lobby/fn_801E0ADC.cpp`'s sprite-record copy, on the layout record type.
 * C linkage (the map row is a plain name). */
#ifndef MHTRI_LOBBY_SPR_DATA_COPY_H
#define MHTRI_LOBBY_SPR_DATA_COPY_H

#include "types.h"
#include "hud/layout_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x801E6850 - copies sprite record `src` into `dst`. */
void spr_data_copy(_SPR_DATA_* dst, const _SPR_DATA_* src);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_SPR_DATA_COPY_H */
