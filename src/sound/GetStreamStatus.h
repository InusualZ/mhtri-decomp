/* sound/GetStreamStatus.h - leaf: `sound/fn_800E46E8.cpp`'s `GetStreamStatus__FUl` (0x800E4EF8, a C++ free function), the status of stream
 * channel `channel` (0 idle, 1 loading, 2 playing, 3 ended). */
#ifndef MHTRI_SOUND_GETSTREAMSTATUS_H
#define MHTRI_SOUND_GETSTREAMSTATUS_H

#include "types.h"

#ifdef __cplusplus
s32 GetStreamStatus(u32 channel);
#endif

#endif /* MHTRI_SOUND_GETSTREAMSTATUS_H */
