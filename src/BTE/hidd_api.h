/* BTE/hidd_api.h - the HID device control block and the entry point `BTE/hidd_api.c` owns (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_HIDD_API_H
#define MHTRI_BTE_HIDD_API_H

#include "BTE/hid_int.h"

#ifdef __cplusplus
extern "C" {
#endif

/* .bss 0x8073C210 - the HID device control block (GUESS name hd_cb, the BTE source's). */
extern tHID_DEV_CTB hd_cb;

/* 0x8048FB68 - clears the control block, sets the default channel MTUs and arms the repage timer handler. */
void HID_DevInit(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_HIDD_API_H */
