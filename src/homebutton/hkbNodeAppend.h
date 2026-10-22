/* Leaf header (docs/plan.md 6.5 rule 2): `homebutton/tiHKBManager.cpp`'s `hkbNodeAppend`, the one declaration `menu/menu_sysmsg.cpp` needs.
 * GUESS name, from the body (listed on the NAMES line of menu/menu_sysmsg.cpp). */
#ifndef MHTRI_HOMEBUTTON_HKBNODEAPPEND_H
#define MHTRI_HOMEBUTTON_HKBNODEAPPEND_H

#include "types.h"

struct CNode;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8056BBD0 (0x20): appends `node` at the tail of the chain rooted at `head` */
void hkbNodeAppend(struct CNode* head, struct CNode* node);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HOMEBUTTON_HKBNODEAPPEND_H */
