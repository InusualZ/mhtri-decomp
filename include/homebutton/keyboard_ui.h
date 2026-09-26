/* The interfaces `src/homebutton/keyboard_ui.cpp` owns (docs/plan.md 6.5, rule 2).
 *
 * That unit is the 0x8055C894-0x805632BC slice of the home-button software-keyboard block.
 * `homebutton/fn_8054E894.cpp` calls fn_8056083C, so its declaration lives here with the owning unit.
 */
#ifndef MHTRI_HOMEBUTTON_KEYBOARD_UI_H
#define MHTRI_HOMEBUTTON_KEYBOARD_UI_H

#include "types.h"

extern "C" void fn_8056083C(void* self);

#endif
