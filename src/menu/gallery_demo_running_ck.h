/* Leaf header (docs/plan.md 6.5 rule 2): a symbol `menu/menu_plsearch.cpp` defines, for a consumer that cannot include
 * the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_MENU_GALLERY_DEMO_RUNNING_CK_H
#define MHTRI_MENU_GALLERY_DEMO_RUNNING_CK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8044FBDC - 1 while move work 0 plays a gallery demo (mode 2, sub-mode 2). */
u32 gallery_demo_running_ck(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_GALLERY_DEMO_RUNNING_CK_H */
