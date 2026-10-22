/* Leaf header (docs/plan.md 6.5 rule 2): symbols `pad_connect.cpp` defines, for `hud/cockpit_quest.cpp`. */
#ifndef MHTRI_SCREEN_SPLIT_VIEW_GET_H
#define MHTRI_SCREEN_SPLIT_VIEW_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800470B4: the split-screen view number, -1 when the screen is not split. */
s8 screen_split_view_get(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SCREEN_SPLIT_VIEW_GET_H */
