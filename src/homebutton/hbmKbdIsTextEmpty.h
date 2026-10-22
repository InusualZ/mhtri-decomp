/* Leaf header (docs/plan.md 6.5 rule 2): `homebutton/hbm_value.cpp`'s `hbmKbdIsTextEmpty`, the one declaration `menu/menu_sysmsg.cpp` needs.
 * GUESS name, from the body (listed on the NAMES line of menu/menu_sysmsg.cpp). */
#ifndef MHTRI_HOMEBUTTON_HBMKBDISTEXTEMPTY_H
#define MHTRI_HOMEBUTTON_HBMKBDISTEXTEMPTY_H

#include "types.h"

#ifdef __cplusplus
class SysKbdView;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8055E69C (0xDC): returns 1 when the keyboard object holds no text to commit */
s32 hbmKbdIsTextEmpty(SysKbdView* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HOMEBUTTON_HBMKBDISTEXTEMPTY_H */
