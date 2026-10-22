/* `pointer_update`, defined by `mh3_pad.cpp` (leaf header, rule 2). */
#ifndef MHTRI_POINTER_UPDATE_H
#define MHTRI_POINTER_UPDATE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80044058 (0x128): refreshes the pointer position (pointer_center) and its state word. */
void pointer_update(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_POINTER_UPDATE_H */
