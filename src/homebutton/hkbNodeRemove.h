/* Leaf header (docs/plan.md 6.5 rule 2): `homebutton/tiHKBManager.cpp`'s `hkbNodeRemove`, the one declaration `menu/menu_sysmsg.cpp` needs.
 * GUESS name, from the body (listed on the NAMES line of menu/menu_sysmsg.cpp). */
#ifndef MHTRI_HOMEBUTTON_HKBNODEREMOVE_H
#define MHTRI_HOMEBUTTON_HKBNODEREMOVE_H

#include "types.h"

struct HkbNode;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8056BBF0 (0x6C): unlinks `node` from `list` (panics when it is not on the list) */
void hkbNodeRemove(struct HkbNode* list, struct HkbNode* node);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HOMEBUTTON_HKBNODEREMOVE_H */
