/* BTE/bta_dm_api.h - the BTA device manager API entry points `BTE/bta_dm_api.c` owns that BTE/gki_time.h does not
 *   declare yet (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_BTA_DM_API_H
#define MHTRI_BTE_BTA_DM_API_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8047D750 - whether the controller is up (asked with the GKI scheduler locked). */
u8 BTA_DmIsDeviceUp(void);

/* 0x8047DAC4 - posts BTA_DM_API_SEND_HCI_RESET_EVT; its action resets the controller. */
void BTA_DmSendHciReset(void);

/* 0x8047D750 - whether the controller is up. */
u8 BTA_DmIsDeviceUp(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_BTA_DM_API_H */
