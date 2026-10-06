/*
 * Pl/pl_act_step_data.cpp - the data of the player action step band: the motion/step tables and the work arrays the
 *   act state machine indexes (data-only, no bodies).
 * RANGE. no .text; .data 0x805BDC48-0x805C15A8 (72 symbols), .bss 0x806AACC0-0x806AB3E0 (10 symbols).  Its readers
 *   are `Pl/pl_act_step.cpp`'s step band and `Pl/pl_act.cpp`; `Pl/pl_act_step_data.h` declares the words they spell.
 * NAMES. The file name follows the unit that reads it.
 * RESIDUALS. Nothing is defined yet: every symbol's type and extent waits for the bodies that read it (flipcheck: the
 *   object emits neither section).
 */

#include "types.h"
