/*
 * BTE/gki_ppc.c - the GKI operating system glue of the Broadcom BTE Bluetooth stack on the Wii: GKI_init, the
 *   scheduler lock stubs, task events, the interrupt disable/enable pair and the pool memory allocator.
 *
 * RANGE. .text 0x80478D20-0x80479030 (13 functions); no data. Medium start: GKI_init (0x80478D20, calls
 *   gki_buffer_init and gki_timers_init) follows the timer list functions of BTE/gki_time.c, Broadcom's gki_time.c /
 *   gki_ppc.c order; no data on either side. Strong end: 0x80479030 is a static callback of BTE/hcisu_h2.c (its
 *   address is taken by 0x804795D0 there).
 * FLAGS. Wii/1.3 with `cflags_base` (-O4,p, 16-byte function alignment), as BTE/gki_time.c measured; the lib default
 *   `cflags_os` packs functions to 4 bytes.
 * NAMES. the map's; GKI_send_event (0x80478EA0, sets an event bit of a task below 8 and returns 1 for a bad task),
 *   GKI_exception (0x80478FE0) and GKI_os_free (0x80479020, called by GKI_delete_pool) are the BTE source's names read
 *   from the bodies and their callers in BTE/gki_buffer.c. GUESS names from the BTE source and the callers:
 *   0x80478DB0 GKI_shutdown (its only caller is BTA_CleanUp; marks the BTU task dead and unwinds the interrupt
 *   nesting), 0x80478E60 GKI_run and 0x80478E90 GKI_delay (empty bodies BTA_Init calls with 0 and with 100 / 2000).
 * RESIDUALS. none known in the bodies.
 * SHAPES. plain C; GKI_disable and GKI_enable are spelled as the static inline helpers gki_int_disable and
 *   gki_int_enable, which GKI_init, GKI_shutdown and GKI_send_event inline before the out-of-line pair is defined.
 *   The declarations live in BTE/gki_ppc.h.
 */

#include "BTE/gki_ppc.h"
#include "BTE/gki_buffer.h"
#include "BTE/gki_time.h"
#include "OS/OSInterrupt.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "WUD/wud.h"

#define TASK_DEAD 0
#define TASK_READY 1

#define GKI_SUCCESS 0
#define GKI_FAILURE 1

/* Disables interrupts and saves the previous level at the next nesting depth. */
static inline void gki_int_disable(void) {
    BOOL level = OSDisableInterrupts();

    gki_cb.os.int_level[gki_cb.os.int_nest] = level;
    gki_cb.os.int_nest++;
}

/* Restores the interrupt level saved at the current nesting depth. */
static inline void gki_int_enable(void) {
    gki_cb.os.int_nest--;
    OSRestoreInterrupts(gki_cb.os.int_level[gki_cb.os.int_nest]);
}

/* 0x80478D20 (0x90): Clears the GKI control block, builds the pools and timers and marks the BTU task ready. */
void GKI_init(void) {
    BOOL level;

    memset(&gki_cb, 0, sizeof(gki_cb));
    gki_buffer_init();
    gki_timers_init();

    gki_cb.com.OSRdyTbl[BTU_TASK] = TASK_READY;
    gki_cb.com.OSTName[BTU_TASK] = NULL;
    gki_cb.com.OSWaitTmr[BTU_TASK] = 0;
    gki_cb.com.OSWaitEvt[BTU_TASK] = 0;

    gki_cb.os.int_nest = 0;
    level = OSEnableInterrupts();
    gki_cb.os.int_level[gki_cb.os.int_nest] = level;
    gki_cb.os.int_nest++;
}

/* 0x80478DB0 (0xA8): Marks the BTU task dead and unwinds every pending GKI_disable. */
void GKI_shutdown(void) {
    gki_int_disable();
    gki_cb.com.OSRdyTbl[BTU_TASK] = TASK_DEAD;
    gki_int_enable();

    while (gki_cb.os.int_nest != 0) {
        gki_int_enable();
    }
}

/* 0x80478E60 (0x4): Starts the tasks; the Wii port runs the BTU task from an alarm instead. */
void GKI_run(u32 p_task_id) {
}

/* 0x80478E70 (0x4): Locks the scheduler; nothing to do on the Wii. */
void GKI_sched_lock(void) {
}

/* 0x80478E80 (0x4): Unlocks the scheduler; nothing to do on the Wii. */
void GKI_sched_unlock(void) {
}

/* 0x80478E90 (0x4): Waits `timeout` milliseconds; nothing to do on the Wii. */
void GKI_delay(u32 timeout) {
}

/* 0x80478EA0 (0xA4): Sets `event` in the wait mask of the task `taskId`; returns GKI_FAILURE for a bad task. */
u8 GKI_send_event(u8 taskId, u16 event) {
    if (taskId >= GKI_MAX_TASKS) {
        return GKI_FAILURE;
    }

    gki_int_disable();
    gki_cb.com.OSWaitEvt[taskId] |= event;
    gki_int_enable();
    return GKI_SUCCESS;
}

/* 0x80478F50 (0x8): Returns the calling task's id, always the BTU task. */
u8 GKI_get_taskid(void) {
    return BTU_TASK;
}

/* 0x80478F60 (0x24): Restores the interrupt level of the innermost GKI_disable. */
void GKI_enable(void) {
    gki_int_enable();
}

/* 0x80478F90 (0x44): Disables interrupts, nesting. */
void GKI_disable(void) {
    gki_int_disable();
}

/* 0x80478FE0 (0x4): Reports a stack error; nothing is reported in retail. */
void GKI_exception(u16 code, char* msg) {
}

/* 0x80478FF0 (0x2C): Allocates `size` bytes of pool memory from the application heap. */
/* untyped: a byte range - raw pool memory carved into buffers */
void* GKI_os_malloc(u32 size) {
    void* p_mem = App_MEMalloc(size); /* untyped: a byte range - raw pool memory */

    if (p_mem == NULL) {
        return NULL;
    }
    return p_mem;
}

/* 0x80479020 (0x4): Returns pool memory to the application heap. */
/* untyped: a byte range - raw pool memory carved into buffers */
void GKI_os_free(void* memory) {
    App_MEMfree(memory);
}
