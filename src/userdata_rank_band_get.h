/* Leaf header (docs/plan.md 6.5 rule 2): a symbol `fn_8004CAD8.cpp` defines, for a consumer that cannot include
 * the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_USERDATA_RANK_BAND_GET_H
#define MHTRI_USERDATA_RANK_BAND_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8004DD74 - 0 when the user data's +0x3DE4 rank is 0, 1 up to 30, 2 above.  GUESS name. */
u8 userdata_rank_band_get(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_USERDATA_RANK_BAND_GET_H */
