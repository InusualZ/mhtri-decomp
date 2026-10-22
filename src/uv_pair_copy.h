/* `uv_pair_copy`, defined by `src/fn_8004CAD8.cpp` (leaf header, rule 2). */
#ifndef MHTRI_UV_PAIR_COPY_H
#define MHTRI_UV_PAIR_COPY_H

#include "types.h"

struct _SPR_DATA_;

#ifdef __cplusplus
extern "C" {
#endif

/* Copies the sprite record's texture coordinate pair. */
void uv_pair_copy(struct _SPR_DATA_* dst, const struct _SPR_DATA_* src);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_UV_PAIR_COPY_H */
