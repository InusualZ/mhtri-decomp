/* Leaf header (docs/plan.md 6.5 rule 2): `fn_80047398.cpp`'s `face_buf_reset` (0x8004882C), for `mh3_pad.cpp`'s
 * task loop.  C linkage (the map row is unmangled). */
#ifndef MHTRI_FACE_BUF_RESET_H
#define MHTRI_FACE_BUF_RESET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8004882C: clears both face buffers and marks every slot free (-1). */
void face_buf_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_FACE_BUF_RESET_H */
