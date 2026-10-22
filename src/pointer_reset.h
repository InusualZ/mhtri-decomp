/* `pointer_reset`, defined by `mh3_pad.cpp` (leaf header, rule 2). */
#ifndef MHTRI_POINTER_RESET_H
#define MHTRI_POINTER_RESET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80043BAC (0x10C): clears the pointer history and recentres the pointer. */
void pointer_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_POINTER_RESET_H */
