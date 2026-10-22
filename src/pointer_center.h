/* `pointer_center`, defined by `mh3_pad.cpp` (leaf header, rule 2). */
#ifndef MHTRI_POINTER_CENTER_H
#define MHTRI_POINTER_CENTER_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x806694B8 (.bss): the pointer's screen position (x, y) and a third word. */
extern f32 pointer_center[3];

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_POINTER_CENTER_H */
