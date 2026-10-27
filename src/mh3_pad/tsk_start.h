/* mh3_pad/tsk_start.h - leaf header (docs/plan.md 6.5 rule 2): `src/mh3_pad.cpp`'s task-slot starters and the strap
 * texture load, for `fn_80059550.cpp`.  C linkage (the map rows are plain names). */
#ifndef MHTRI_MH3_PAD_TSK_START_H
#define MHTRI_MH3_PAD_TSK_START_H

#include "types.h"
#include "mh3_pad/task.h" /* TaskSlot (rule 1) */

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800417F0 (0x60): clears task slot `slot` and starts it running `func` (state 0xC). */
void tsk_start(void (*func)(TaskSlot* task), s16 slot);
/* 0x80041850 (0xC): zeroes a task's state word (the task then stops). */
void tsk_timer_clear(s16* timer);
/* 0x8004132C (0x40): loads the language's strap (health and safety) texture to 0x90F49000. */
void strap_tpl_load(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MH3_PAD_TSK_START_H */
