/* BTE/bte_hcisu.h - the HCI services interface, its installed instance and the HCI transport entry points
 *   `BTE/bte_hcisu.c` owns (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_BTE_HCISU_H
#define MHTRI_BTE_BTE_HCISU_H

#include "types.h"
#include "BTE/gki_buffer.h"

/* size: 0x08 - the USB device an HCI services open is asked for (GUESS names: the bytes are Broadcom's USB vendor and
 *   product ids). */
typedef struct tHCISU_CFG {
    /* +0x00 */ u16 vendor_id;
    /* +0x02 */ u16 product_id;
    /* +0x04 */ u32 unused_0x04;
} tHCISU_CFG; /* size: 0x08 */

/* size: 0x18 - the entry points of an HCI services transport: set up its receive task, open and close the device,
 *   send a message and handle a receive event of the BTU task. */
typedef struct tHCISU_IF {
    /* +0x00 */ void (*init)(u8 task_id, u8 mbox, u16 rcv_evt);
    /* +0x04 */ u8 (*open)(tHCISU_CFG* p_cfg);
    /* +0x08 */ void (*close)(void);
    /* +0x0C */ u8 (*send)(BT_HDR* p_msg);
    /* +0x10 */ u8 (*handle_event)(u16 event);
    /* +0x14 */ void (*unused_0x14)(void);
} tHCISU_IF; /* size: 0x18 */

#ifdef __cplusplus
extern "C" {
#endif

/* .sbss 0x80795020 / 0x80795024 - the installed HCI services transport and the device it opens. */
extern const tHCISU_IF* bte_hcisu_if;
extern tHCISU_CFG* bte_hcisu_cfg;

/* 0x8047A370 - hands the message `p_msg` with the event `event` to the HCI transport. */
void bte_hcisu_send(BT_HDR* p_msg, u16 event);

/* 0x8047A390 / 0x8047A400 - set up and open the HCI transport, or close it. */
void bte_hcisu_task(u32 param);
void bte_hcisu_close(void);

/* 0x8047A430 - posts a received HCI message to the BTU task. */
void bte_hcisu_rcv_msg(BT_HDR* p_msg);

/* 0x8047A440 - initialises the protocol layers above L2CAP. */
void BTE_InitStack(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_BTE_HCISU_H */
