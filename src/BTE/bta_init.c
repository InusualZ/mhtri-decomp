/*
 * BTE/bta_init.c - the start and stop of the Broadcom BTE Bluetooth stack on the Wii: BTA_Init brings up GKI, the
 *   BTE core and the HCI transport and drives the BTU task from a periodic alarm; BTA_CleanUp stops it.
 *
 * RANGE. .text 0x8047A610-0x8047A780 (4 functions); .bss 0x80738140-0x80739170; .sdata 0x80793D70-0x80793D78;
 *   .sbss 0x8079502C-0x80795030. Medium start: 0x8047A610 switches to the fiber stack 0x80738140 (0x1000 bytes) and
 *   runs the BTU task; its address is taken only by BTA_Init, which installs it on the periodic alarm 0x80739140,
 *   and its stack lies directly before that alarm in the .bss. Strong .sdata seam: BTA_Init's 0x80793D70 follows
 *   the 4-byte hole after BTE/bte_logmsg.c's "%s\n". Strong end: btu_task_init (0x8047A780) is the first reader of
 *   BTE/btu_task1.c's .sdata 0x80793D78 and .sbss 0x80795030. The cleanup callback 0x8079502C is set by BTA_CleanUp
 *   and called by bta_usb_close_evt.
 * FLAGS. Wii/1.3 with `cflags_base` (-O4,p, 16-byte function alignment), as BTE/gki_time.c measured; the lib default
 *   `cflags_os` packs functions to 4 bytes.
 * NAMES. the map's BTA_Init, BTA_CleanUp and bta_usb_close_evt; the file name is a GUESS from BTA_Init. GUESS:
 *   0x8047A610 bta_alarm_handler (the alarm handler that runs the BTU task on its fiber), 0x80738140 bta_task_stack,
 *   0x80739140 bta_task_alarm, 0x8079502C bta_cleanup_cback and 0x80793D70 bta_hcisu_usb_cfg (the Broadcom USB
 *   vendor 0x0A5C / product 0x2101 BTA_Init hands to the transport).
 * RESIDUALS. none known.
 * SHAPES. plain C; the declarations live in BTE/bta_init.h.
 */

#include "BTE/bta_init.h"
#include "BTE/bta_dm_api.h"
#include "BTE/bte_hcisu.h"
#include "BTE/btu_task1.h"
#include "BTE/gki_ppc.h"
#include "BTE/hcisu_h2.h"
#include "BTE/uusb_ppc.h"
#include "OS/OS.h"
#include "OS/OSSetAlarm.h"
#include "OS/OSSetPeriodicAlarm.h"
#include "OS/OSSwitchFiber.h"
#include "OS/OSTime.h"
#include "BTE/btu_init.h"   /* BTE_Init */

#define OS_TIMER_CLOCK (OS_BUS_CLOCK / 4)
#define OSMillisecondsToTicks(msec) ((msec) * (OS_TIMER_CLOCK / 1000))

/* The period of the BTU task's alarm in milliseconds. */
#define BTA_TASK_PERIOD_MS 2

#define BTA_TASK_STACK_SIZE 0x1000

#define BTA_SUCCESS 0
#define BTA_FAILURE 1

/* .bss 0x80738140 - the fiber stack the BTU task runs on. */
u8 bta_task_stack[BTA_TASK_STACK_SIZE];

/* .bss 0x80739140 - the periodic alarm that runs the BTU task. */
OSAlarm bta_task_alarm;

/* .sdata 0x80793D70 - the USB device BTA_Init asks the transport to open. */
tHCISU_CFG bta_hcisu_usb_cfg = {0x0A5C, 0x2101, 0};

/* .sbss 0x8079502C - the callback BTA_CleanUp installed. */
BtaCleanUpCallback bta_cleanup_cback;

/* 0x8047A610 (0x18): Runs one pass of the BTU task on its own stack. */
void bta_alarm_handler(OSAlarm* alarm, OSContext* context) {
    OSSwitchFiber(btu_task, bta_task_stack + BTA_TASK_STACK_SIZE);
}

/* 0x8047A630 (0xDC): Initialises GKI, the stack and the USB transport, starts the BTU task's alarm and waits until the
 * controller is up. */
u8 BTA_Init(void) {
    bta_cleanup_cback = NULL;

    GKI_init();
    GKI_enable();
    BTE_Init();

    bte_hcisu_if = &hcisu_h2_interface;
    bte_hcisu_cfg = &bta_hcisu_usb_cfg;
    bte_hcisu_task(0);

    while (uusb_open_pending) {
        GKI_delay(100);
    }

    btu_task_init();

    OSCreateAlarm(&bta_task_alarm);
    OSSetPeriodicAlarm(&bta_task_alarm, OSGetTime(), OSMillisecondsToTicks(BTA_TASK_PERIOD_MS), bta_alarm_handler);

    GKI_run(0);

    while (!BTA_DmIsDeviceUp()) {
        GKI_delay(2000);
    }

    return BTA_SUCCESS;
}

/* 0x8047A710 (0x34): Stores `callback`, stops the BTU task's alarm, closes the transport and shuts GKI down. */
void BTA_CleanUp(BtaCleanUpCallback callback) {
    bta_cleanup_cback = callback;
    OSCancelAlarm(&bta_task_alarm);
    bte_hcisu_close();
    GKI_shutdown();
}

/* 0x8047A750 (0x30): Reports the close of the USB device to the BTA_CleanUp callback. */
void bta_usb_close_evt(s8 status) {
    if (bta_cleanup_cback != NULL) {
        if (status >= 0) {
            bta_cleanup_cback(BTA_SUCCESS);
        } else {
            bta_cleanup_cback(BTA_FAILURE);
        }
    }
}
