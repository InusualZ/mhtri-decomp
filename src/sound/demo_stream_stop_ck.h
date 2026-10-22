/* Leaf header (docs/plan.md 6.5 rule 2): a symbol `sound/fn_800F2A94.cpp` defines, for a consumer that cannot include
 * the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_SOUND_DEMO_STREAM_STOP_CK_H
#define MHTRI_SOUND_DEMO_STREAM_STOP_CK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800F2E38 - fades demo `demo`'s streams out; 1 once both have stopped.  GUESS name. */
u32 demo_stream_stop_ck(u8 demo);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_DEMO_STREAM_STOP_CK_H */
