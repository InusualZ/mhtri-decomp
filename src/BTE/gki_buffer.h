/* BTE/gki_buffer.h - the Bluetooth host stack entry points `BTE/gki_buffer.cpp` owns that the Wii remote driver calls
 *   (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_GKI_BUFFER_H
#define MHTRI_BTE_GKI_BUFFER_H

#include "types.h"

/* size: 0x08 - the header of a Bluetooth stack buffer; `offset` bytes of room follow it before the payload. */
typedef struct BT_HDR {
    /* +0x00 */ u16 event;
    /* +0x02 */ u16 len;
    /* +0x04 */ u16 offset;
    /* +0x06 */ u16 layer_specific;
} BT_HDR; /* size: 0x08 */

/* size: 0x09 - a stack buffer with the first payload byte; the payload starts `offset` bytes into `data`. */
typedef struct BT_BUF {
    /* +0x00 */ BT_HDR hdr;
    /* +0x08 */ u8 data[1];
} BT_BUF; /* size: 0x09 */

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804777E0 - takes a buffer of at least `size` bytes from the stack's pools. */
BT_HDR* GKI_getbuf(u8 size);

/* 0x8047FF98 - sends the output report in `buf` to the HID device `handle`. */
void BTA_HhSendData(u8 handle, BT_HDR* buf);

/* 0x8047FE80 - closes the HID connection of `handle`. */
void BTA_HhClose(u8 handle);

/* 0x804824F8 - drops the ACL link to the device at `bdAddr`. */
void btm_remove_acl(u8* bdAddr);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_GKI_BUFFER_H */
