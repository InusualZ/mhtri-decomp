/* BTE/bta_dm_api.h - the BTA device manager API entry points `BTE/bta_dm_api.c` owns (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_BTA_DM_API_H
#define MHTRI_BTE_BTA_DM_API_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8047DAC4 - posts BTA_DM event 0x10C with no payload (GUESS name: bta_dm action-table slot 12 is
 * `bta_dm_send_hci_reset`). */
void BTA_DmSendHciReset(void);

/* 0x8047D750 - whether the controller is up. */
u8 BTA_DmIsDeviceUp(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_BTA_DM_API_H */
