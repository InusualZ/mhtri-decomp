/* Leaf header (docs/plan.md 6.5 rule 2): a symbol `sound/fn_800F2A94.cpp` defines, for a consumer that cannot include
 * the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_SOUND_DEMO_STREAM_STOP_H
#define MHTRI_SOUND_DEMO_STREAM_STOP_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800F2F74 - releases demo `demo`'s stopped streams.  GUESS name. */
void demo_stream_stop(u8 demo);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_DEMO_STREAM_STOP_H */
