/* BTE/gki_ppc.h - the GKI operating system glue `BTE/gki_ppc.c` owns: start and shutdown, task events, the interrupt
 *   disable/enable pair and the pool memory allocator (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_GKI_PPC_H
#define MHTRI_BTE_GKI_PPC_H

#include "types.h"

/* The GKI task the BTU runs as; the only task the Wii port creates. */
#define BTU_TASK 2

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80478D20 - clears the GKI control block, builds the pools and timers and marks the BTU task ready. */
void GKI_init(void);

/* 0x80478DB0 - marks the BTU task dead and unwinds every pending GKI_disable. */
void GKI_shutdown(void);

/* 0x80478E60 / 0x80478E70 / 0x80478E80 / 0x80478E90 - the scheduler entry points, empty on the Wii: start the tasks,
 * lock and unlock the scheduler, and wait `timeout` milliseconds. */
void GKI_run(u32 p_task_id);
void GKI_sched_lock(void);
void GKI_sched_unlock(void);
void GKI_delay(u32 timeout);

/* 0x80478EA0 - sets `event` in the wait mask of the task `taskId`; returns 1 for a task id of 8 or more, else 0. */
u8 GKI_send_event(u8 taskId, u16 event);

/* 0x80478F50 / 0x80478F60 / 0x80478F90 - the current task id and the nested interrupt disable/enable pair. */
u8 GKI_get_taskid(void);
void GKI_enable(void);
void GKI_disable(void);

/* 0x80478FE0 - reports the stack error `code` with the message `msg` (empty in retail). */
void GKI_exception(u16 code, char* msg);

/* 0x80478FF0 / 0x80479020 - allocate and free the memory of a buffer pool. */
/* untyped: a byte range - raw pool memory carved into buffers */
void* GKI_os_malloc(u32 size);
/* untyped: a byte range - raw pool memory carved into buffers */
void GKI_os_free(void* memory);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_GKI_PPC_H */
