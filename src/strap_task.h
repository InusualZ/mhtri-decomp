/* Leaf header (docs/plan.md 6.5 rule 2): `fn_80059550.cpp`'s `strap_task`. */
#ifndef MHTRI_STRAP_TASK_H
#define MHTRI_STRAP_TASK_H

#include "types.h"
#include "mh3_pad/task.h" /* TaskSlot (rule 1) */

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800595E4 (0xD88): the boot strap screen and save-check task body (`pad_connect.cpp`'s main-loop frame starts it). */
void strap_task(TaskSlot* task);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_STRAP_TASK_H */
