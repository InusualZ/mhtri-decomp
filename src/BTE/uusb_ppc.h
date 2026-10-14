/* BTE/uusb_ppc.h - the USB transport `BTE/uusb_ppc.c` owns: the open request, its event callback and the transport
 *   entry points (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_UUSB_PPC_H
#define MHTRI_BTE_UUSB_PPC_H

#include "types.h"
#include "BTE/gki_buffer.h"

/* The pipes of the Bluetooth USB device, the `port` of UUSB_Read and UUSB_Write. */
#define UUSB_PORT_CMD 0
#define UUSB_PORT_EVT 1
#define UUSB_PORT_ACL 2
#define UUSB_PORT_SCO 3

/* The event UUSB_Register's callback receives when the device is closed. */
#define UUSB_EVT_CLOSED 4

/* size: 0x0C - the device an open asks for (GUESS names: only the HCI services fill it, and only the vendor and
 *   product ids and the receiving task are read from the caller). */
typedef struct tUUSB_CFG {
    /* +0x00 */ u32 unused_0x00;
    /* +0x04 */ u16 vendor_id;
    /* +0x06 */ u16 product_id;
    /* +0x08 */ u16 task_id;
    /* +0x0A */ u8 unused_0x0A;
    /* +0x0B */ u8 rcv_mode;
} tUUSB_CFG; /* size: 0x0C */

/* The transport's event callback: `event` is UUSB_EVT_CLOSED with the close result in `status`. */
typedef void (*tUUSB_CBACK)(s32 event, s8 status);

#ifdef __cplusplus
extern "C" {
#endif

/* .sdata 0x80793D28 - set from the start of an open until UUSB_Register has started the receive pipes. */
extern BOOL uusb_open_pending;

/* 0x80479CB0 - opens the IOS USB library and the Bluetooth device and creates the receive pools. */
void UUSB_Open(const tUUSB_CFG* p_cfg);

/* 0x80479EC0 - installs the event callback and starts the HCI event and ACL receive pipes. */
void UUSB_Register(const tUUSB_CFG* p_cfg, tUUSB_CBACK p_cback);

/* 0x8047A050 - reads at most `len` bytes of `port` into `p_data`; the Wii port delivers whole messages instead, so it
 * reads nothing. */
u16 UUSB_Read(u8 port, u8* p_data, u16 len);

/* 0x8047A060 - writes `len` bytes at `p_data` to `port` (UUSB_PORT_CMD or UUSB_PORT_ACL) on behalf of `p_msg`;
 * returns the IOS USB result of the write, 0 when it was queued or no buffer was free. */
u16 UUSB_Write(u8 port, const u8* p_data, u16 len, BT_HDR* p_msg);

/* 0x8047A270 / 0x8047A320 - drop the queued writes and close the device, and close the IOS USB library. */
void UUSB_Close(void);
void UUSB_Unregister(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_UUSB_PPC_H */
