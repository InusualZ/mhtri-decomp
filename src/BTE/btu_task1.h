/* BTE/btu_task1.h - the BTU control block, the BTU task and the BTU timer entry points `BTE/btu_task1.c` owns
 *   (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_BTU_TASK1_H
#define MHTRI_BTE_BTU_TASK1_H

#include "types.h"
#include "BTE/gki_buffer.h"
#include "BTE/gki_time.h"

#define BTU_MAX_REG_TIMER 2
#define BTU_MAX_REG_EVENT 6

/* size: 0x08 - a timer a layer registered with the BTU: the entry and the function its expiry calls. */
typedef struct tBTU_TIMER_REG {
    /* +0x00 */ TIMER_LIST_ENT* p_tle;
    /* +0x04 */ void (*timer_cb)(TIMER_LIST_ENT* p_tle);
} tBTU_TIMER_REG; /* size: 0x08 */

/* size: 0x08 - a message range a layer registered with the BTU: the event's high byte and its handler. */
typedef struct tBTU_EVENT_REG {
    /* +0x00 */ u16 event_range;
    /* +0x02 */ u8 pad_0x02[2];
    /* +0x04 */ void (*event_cb)(BT_HDR* p_msg);
} tBTU_EVENT_REG; /* size: 0x08 */

/* size: 0x88 - the BTU control block: the registered timers and message ranges, the timer list and the ACL sizes
 *   the L2CAP layer and the HCI transport read. */
typedef struct tBTU_CB {
    /* +0x00 */ tBTU_TIMER_REG timer_reg[BTU_MAX_REG_TIMER];
    /* +0x10 */ tBTU_EVENT_REG event_reg[BTU_MAX_REG_EVENT];
    /* +0x40 */ TIMER_LIST_Q timer_queue;
    /* +0x4C */ u8 unused_0x4C[0x30];
    /* +0x7C */ u16 hcit_acl_data_size;
    /* +0x7E */ u16 hcit_acl_pkt_size;
    /* +0x80 */ u8 unused_0x80[8];
} tBTU_CB; /* size: 0x88 */

#ifdef __cplusplus
extern "C" {
#endif

/* .bss 0x80739170 - the BTU control block. */
extern tBTU_CB btu_cb;

/* 0x8047A780 - resets the BTU task's bookkeeping and initialises the core, the protocol layers and BTA. */
void btu_task_init(void);

/* 0x8047A7C0 - one pass of the BTU task: serves the HCI and BTA mailboxes and the timer lists; BTE/bta_init.c runs it
 * on a fiber from a periodic alarm. */
void btu_task(void);

/* 0x8047AB40 / 0x8047ABB0 - start the timer entry with `timeout` seconds of type `type`, or stop it. */
void btu_start_timer(TIMER_LIST_ENT* p_tle, u16 type, u32 timeout);
void btu_stop_timer(TIMER_LIST_ENT* p_tle);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_BTU_TASK1_H */
