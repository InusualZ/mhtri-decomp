/* RVLGX/GXAbortFrame.h - the declaration of `GXAbortFrame`, which `RVLGX/GXMisc.c` owns (docs/plan.md 6.5 rule 2,
 *   leaf header). */
#ifndef MHTRI_RVLGX_GXABORTFRAME_H
#define MHTRI_RVLGX_GXABORTFRAME_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804B5FA0 - resets the graphics FIFO and the pipeline after an interrupted frame. */
void GXAbortFrame(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_RVLGX_GXABORTFRAME_H */
