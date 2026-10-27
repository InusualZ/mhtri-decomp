/* Leaf header (docs/plan.md 6.5 rule 2): `stage/stg_w.cpp`'s `get_stg_w` (C++ scope: `get_stg_w__Fv`). */
#ifndef MHTRI_STAGE_GET_STG_W_H
#define MHTRI_STAGE_GET_STG_W_H

#include "types.h"

/* 0x802AD9C0 (0xC): returns the stage-work block `stage_w`. */
/* untyped: opaque handle - the stage-work block, typed by each caller's view */
void* get_stg_w();

#endif /* MHTRI_STAGE_GET_STG_W_H */
