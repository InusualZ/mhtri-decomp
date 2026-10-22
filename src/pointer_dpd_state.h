/* `pointer_dpd_state`, defined by `mh3_pad.cpp` (leaf header, rule 2). */
#ifndef MHTRI_POINTER_DPD_STATE_H
#define MHTRI_POINTER_DPD_STATE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80794814 (.sbss): the pointer state: nonzero while the remote points at the screen, 1 on a steady frame. */
extern s32 pointer_dpd_state;

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_POINTER_DPD_STATE_H */
