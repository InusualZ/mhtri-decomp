/* Leaf header (docs/plan.md 6.5 rule 2): `menu/menu_sysmsg.cpp`'s `kbd_hooks_set` (C++ scope: `kbd_hooks_set__Fv`). */
#ifndef MHTRI_MENU_KBD_HOOKS_SET_H
#define MHTRI_MENU_KBD_HOOKS_SET_H

#include "types.h"

/* 0x804561AC (0x80): stores the software keyboard driver's hooks into `system_w`. */
void kbd_hooks_set(void);

#endif /* MHTRI_MENU_KBD_HOOKS_SET_H */
