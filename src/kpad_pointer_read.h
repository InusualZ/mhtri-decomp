/* `kpad_pointer_read`, defined by `mh3_pad.cpp` (leaf header, rule 2). */
#ifndef MHTRI_KPAD_POINTER_READ_H
#define MHTRI_KPAD_POINTER_READ_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80044D78 (0x218): reads the remote's pointer and motion samples for this frame. */
void kpad_pointer_read(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_KPAD_POINTER_READ_H */
