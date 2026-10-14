/* BTE/btm_link_key.h - the stored link key results `BTE/btm_link_key.c` reports and the entry points it owns that other
 *   units call (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_BTM_LINK_KEY_H
#define MHTRI_BTE_BTM_LINK_KEY_H

#include "types.h"

/* The events of the stored link key callback. */
#define BTM_CB_EVT_RETURN_LINK_KEYS 1
#define BTM_CB_EVT_READ_STORED_LINK_KEYS 2
#define BTM_CB_EVT_WRITE_STORED_LINK_KEYS 3
#define BTM_CB_EVT_DELETE_STORED_LINK_KEYS 4

/* The most link keys one write sends. */
#define BTM_MAX_WRITE_LINK_KEYS 11

/* size: 0x06 - the result of a stored link key read: the event, the HCI status, the controller's capacity and the
 * number of keys it returned. */
typedef struct tBTM_READ_STORED_LINK_KEY_COMPLETE {
    /* +0x00 */ u8 event;
    /* +0x01 */ u8 status;
    /* +0x02 */ u16 max_keys;
    /* +0x04 */ u16 read_keys;
} tBTM_READ_STORED_LINK_KEY_COMPLETE; /* size: 0x06 */

/* size: 0x03 - the result of a stored link key write: the event, the HCI status and the number of keys written. */
typedef struct tBTM_WRITE_STORED_LINK_KEY_COMPLETE {
    /* +0x00 */ u8 event;
    /* +0x01 */ u8 status;
    /* +0x02 */ u8 num_keys;
} tBTM_WRITE_STORED_LINK_KEY_COMPLETE; /* size: 0x03 */

/* size: 0x04 - the result of a stored link key delete: the event, the HCI status and the number of keys deleted. */
typedef struct tBTM_DELETE_STORED_LINK_KEY_COMPLETE {
    /* +0x00 */ u8 event;
    /* +0x01 */ u8 status;
    /* +0x02 */ u16 num_keys;
} tBTM_DELETE_STORED_LINK_KEY_COMPLETE; /* size: 0x04 */

/* size: 0x18 - the returned link keys event: the event, the number of (address, key) pairs and the pairs, 22 bytes
 * each (approximation: one pair is declared, `num_keys` of them follow the head). */
typedef struct tBTM_RETURN_LINK_KEYS_EVT {
    /* +0x00 */ u8 event;
    /* +0x01 */ u8 num_keys;
    /* +0x02 */ u8 key_data[22];
} tBTM_RETURN_LINK_KEYS_EVT; /* size: 0x18 - approximation: the pairs run past the first */

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8048453C / 0x804845B8 / 0x80484614 - the HCI completions of the stored link key read, write and delete (`p` is the
 * event's parameter stream). */
void btm_read_stored_link_key_complete(u8* p);
void btm_write_stored_link_key_complete(u8* p);
void btm_delete_stored_link_key_complete(u8* p);

/* 0x8048467C - turns the returned addresses and keys to the stack's byte order and reports them. */
void btm_return_link_keys_evt(tBTM_RETURN_LINK_KEYS_EVT* result);

/* 0x80484808 - reports the device status `status` to the registered status callback. */
void btm_report_device_status(u32 status);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_BTM_LINK_KEY_H */
