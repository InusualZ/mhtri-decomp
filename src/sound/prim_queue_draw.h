/* `prim_queue_draw` (0x800E444C), defined by `sound/fn_800E3CBC.cpp` (leaf header, rule 2): draws one layer/pass of
 * the primitive queue. */
#ifndef MHTRI_SOUND_PRIM_QUEUE_DRAW_H
#define MHTRI_SOUND_PRIM_QUEUE_DRAW_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

void prim_queue_draw(u8 idx, u8 mode);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_PRIM_QUEUE_DRAW_H */
