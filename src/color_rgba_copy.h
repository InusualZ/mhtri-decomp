/* `color_rgba_copy`, defined by `src/fn_80047398.cpp` (leaf header, rule 2). */
#ifndef MHTRI_COLOR_RGBA_COPY_H
#define MHTRI_COLOR_RGBA_COPY_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Copies a 4-byte RGBA colour. */
void color_rgba_copy(u8* dst, const u8* src);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_COLOR_RGBA_COPY_H */
