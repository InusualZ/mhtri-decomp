/* Leaf header (docs/plan.md 6.5 rule 2): `menu/menu_plsearch.cpp`'s `kbd_modifier_bits`, the one declaration `menu/menu_sysmsg.cpp` needs.
 * GUESS name, from the body (listed on the NAMES line of menu/menu_sysmsg.cpp). */
#ifndef MHTRI_MENU_KBD_MODIFIER_BITS_H
#define MHTRI_MENU_KBD_MODIFIER_BITS_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804513B0 (0x44): returns the Ctrl/Shift/Alt bits of the USB keyboard's held modifier counters */
u32 kbd_modifier_bits(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_KBD_MODIFIER_BITS_H */
