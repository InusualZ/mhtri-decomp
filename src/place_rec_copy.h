/* `place_rec_copy`, defined by `fn_8004CAD8.cpp` (leaf header, rule 2). */
#ifndef MHTRI_PLACE_REC_COPY_H
#define MHTRI_PLACE_REC_COPY_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8004EAF4 (0x24): copies a 16-byte help record (four words); the prototype is fn_8004CAD8.h's. */
/* untyped: byte range - the four words the helper copies */
void place_rec_copy(void* dst, const void* src);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PLACE_REC_COPY_H */
