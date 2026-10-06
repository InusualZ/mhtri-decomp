/* Leaf header (docs/plan.md 6.5 rule 2): `stage/stg_w.cpp`'s current-map getter, a C++ free function (the map row is
 * `get_now_mapno__Fv`), kept out of `stage/stg_w.h` because the owner reads that header inside a namespace, where a
 * C++ declaration would take the namespace into its mangling. */
#ifndef MHTRI_STAGE_GET_NOW_MAPNO_H
#define MHTRI_STAGE_GET_NOW_MAPNO_H

#include "types.h"

#ifdef __cplusplus
/* 0x802AFC74 - the current map number. */
u8 get_now_mapno(void);
#endif

#endif /* MHTRI_STAGE_GET_NOW_MAPNO_H */
