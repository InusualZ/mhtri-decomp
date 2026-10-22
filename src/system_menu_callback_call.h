/* Leaf header (docs/plan.md 6.5 rule 2): the `fn_80040598` unit's symbol `menu/menu_item.cpp` calls, for a
 * consumer that cannot include the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_SYSTEM_MENU_CALLBACK_CALL_H
#define MHTRI_SYSTEM_MENU_CALLBACK_CALL_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80040874 - calls system_w's +0x908 callback when its +0x900 word is set, else returns 0; the menu opens on a 1 */
u32 system_menu_callback_call(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SYSTEM_MENU_CALLBACK_CALL_H */
