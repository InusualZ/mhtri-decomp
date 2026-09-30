/*
 * `Psw` - the one declaration of the object `Psw` (rule 2), defined by `src/mh3_pad.cpp` (its `.bss`
 * 0x806585B8-0x806694E8 is that unit's own).  The type `PlayerPad` lives in `mh3_pad/player_pad.h` (rule 1); consumers
 * include this header for the object.
 */
#ifndef MHTRI_MH3_PAD_PSW_H
#define MHTRI_MH3_PAD_PSW_H

#include "mh3_pad/player_pad.h"

#ifdef __cplusplus
extern "C" {
#endif

extern PlayerPad Psw[4];

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MH3_PAD_PSW_H */
