/* Leaf header (docs/plan.md 6.5 rule 2): the `menu/menu_message` unit's symbol `menu/menu_item.cpp` calls, for a
 * consumer that cannot include the owner's full header.  C++ linkage (the map row is this declaration's mangling, rule 9). */
#ifndef MHTRI_MENU_PUT_MESSAGE_SYS_H
#define MHTRI_MENU_PUT_MESSAGE_SYS_H

#include "types.h"

/* 0x802A8CC0 - draws a system message box with the given text */
void put_message_sys(s8* text, u32 a, u32 b, u32 c);

#endif /* MHTRI_MENU_PUT_MESSAGE_SYS_H */
