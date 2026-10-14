/* BTE/btm_discovery.h - the discovery database `BTE/btm_discovery.c` keeps in btm_cb and the entry points it owns that
 *   other units call (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_BTM_DISCOVERY_H
#define MHTRI_BTE_BTM_DISCOVERY_H

#include "types.h"
#include "BTE/gki_time.h"
#include "BTE/sdp_int.h"

/* The size of the SDP discovery database memory. */
#define BTM_DISC_DB_SIZE 4000

/* The service class UUID the database filters on (the BTE source's UUID_SERVCLASS_PUBLIC_BROWSE_GROUP). */
#define BTM_DISC_UUID_SERVCLASS 0x1002

/* The completion callback of a discovery: its status. */
typedef void (tBTM_DISC_CMPL_CB)(u16 status);

/* size: 0x1020 - the discovery state (GUESS name tBTM_DISC_VAR): its timer, the UUID filter, the callback, the SDP
 * database memory and the busy flag. */
typedef struct tBTM_DISC_VAR {
    /* +0x0000 */ TIMER_LIST_ENT disc_timer;
    /* +0x0018 */ u16 num_uuid;
    /* +0x001C */ tSDP_UUID uuid;
    /* +0x0030 */ u8 unused_0x0030[0x48];
    /* +0x0078 */ tBTM_DISC_CMPL_CB* p_cmpl_cb;
    /* +0x007C */ u8 db_mem[BTM_DISC_DB_SIZE];
    /* +0x101C */ u8 busy;
    /* +0x101D */ u8 pad_0x101D[3];
} tBTM_DISC_VAR; /* size: 0x1020 */

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80484828 / 0x804848A0 - clear the discovery state and set the database up again; the reset first stops the timer
 * and reports a running discovery as finished. */
void btm_discovery_db_init(void);
void btm_discovery_db_reset(void);

/* 0x80484958 - the discovery timer expiry; nothing to do. */
void btm_discovery_timeout(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_BTM_DISCOVERY_H */
