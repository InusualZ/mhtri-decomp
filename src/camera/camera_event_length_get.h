/* Leaf header (docs/plan.md 6.5 rule 2): a symbol `camera/camera_main.cpp` defines, for a consumer that cannot include
 * the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_CAMERA_CAMERA_EVENT_LENGTH_GET_H
#define MHTRI_CAMERA_CAMERA_EVENT_LENGTH_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802BC878 - the event camera's length in frames. */
s16 camera_event_length_get(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_CAMERA_CAMERA_EVENT_LENGTH_GET_H */
