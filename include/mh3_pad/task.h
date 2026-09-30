/*
 * mh3_pad/task.h - `TaskSlot`, the task table entry `src/mh3_pad.cpp` owns.  Split out of `mh3_pad.h`
 * (like `vec3.h` and `control.h`) so the unit itself can include it: the owner header's other declarations
 * still disagree with the unit's own definitions.
 */
#ifndef MHTRI_MH3_PAD_TASK_H
#define MHTRI_MH3_PAD_TASK_H

#include "types.h"

/* The 0x20-byte task slot table at `task_slot_table` (`Tsk_Change`/`fn_800417F0`/`fn_8004192C` index it by
 * `slot << 5`, `fn_80041694` walks all 16).  `func` is the task body, called with its own slot; the
 * bytes after it are the body's private state (`quest/arenatask.cpp`'s `arena_task` keeps its step,
 * countdown and a flag there).  size: 0x20 */
typedef struct TaskSlot {
    /* +0x00 */ s16 state;
    /* +0x02 */ s16 timer;
    /* +0x04 */ void (*func)(struct TaskSlot*);
    /* +0x08 */ u8 step_0x08;         /* the task body's own step byte */
    /* +0x09 */ u8 pad_0x09[0x03];
    /* +0x0C */ s16 wait_0x0C;        /* the body's frame countdown */
    /* +0x0E */ u8 pad_0x0E[0x06];
    /* +0x14 */ u8 flag_0x14;         /* cleared when the arena task leaves its first step */
    /* +0x15 */ u8 pad_0x15[0x0B];
} TaskSlot; /* size: 0x20 */

#endif /* MHTRI_MH3_PAD_TASK_H */
