/*
 * KBD/kbd_region_us.h - declarations of the symbols owned by `KBD/kbd_region_us.c` that other units call.
 */
#ifndef KBD_KBD_REGION_US_H
#define KBD_KBD_REGION_US_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Installs the international map 0 and the US maps 1, 2, 10 and 17. */
void KBDInitRegionUS(void);

#ifdef __cplusplus
}
#endif

#endif
