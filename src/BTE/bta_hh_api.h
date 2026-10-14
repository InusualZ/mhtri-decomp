/* BTE/bta_hh_api.h - the BTA HID host API entry points `BTE/bta_hh_api.c` owns (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_BTA_HH_API_H
#define MHTRI_BTE_BTA_HH_API_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8048013C - posts the HID host request that reports the ACL queue information. */
void BTA_HhGetAclQueueInfo(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_BTA_HH_API_H */
