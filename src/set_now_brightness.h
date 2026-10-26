/* `set_now_brightness`, defined by `src/main.cpp` (leaf header, docs/plan.md 6.5 rule 2).  C++ scope: the map row is
 * the mangling `set_now_brightness__Ff`. */
#ifndef MHTRI_SET_NOW_BRIGHTNESS_H
#define MHTRI_SET_NOW_BRIGHTNESS_H

#include "types.h"

#ifdef __cplusplus
/* 0x8003FC50 - sets the screen brightness (1.0 is full). */
void set_now_brightness(float brightness);
#endif

#endif /* MHTRI_SET_NOW_BRIGHTNESS_H */
