/* Leaf header (docs/plan.md 6.5 rule 2): `main.cpp`'s `vi_next_frame_buffer`, the one declaration `menu/menu_sysmsg.cpp` needs.
 * GUESS name, from the body (listed on the NAMES line of menu/menu_sysmsg.cpp). */
#ifndef MHTRI_VI_NEXT_FRAME_BUFFER_H
#define MHTRI_VI_NEXT_FRAME_BUFFER_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8004026C (0x8): returns the frame buffer the next frame draws into */
/* untyped: byte range - the external frame buffer GXCopyDisp writes */
void* vi_next_frame_buffer(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_VI_NEXT_FRAME_BUFFER_H */
