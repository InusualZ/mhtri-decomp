/*
 * Declarations for the symbols `src/BTE/btm_link_key.c` owns that other units use (docs/plan.md 6.5, rule 2).
 */
#ifndef MHTRI_BTE_BTM_LINK_KEY_H
#define MHTRI_BTE_BTM_LINK_KEY_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8048453C */
void btm_read_stored_link_key_complete(u8* p);
/* 0x804845B8 */
void btm_write_stored_link_key_complete(u8* p);
/* 0x80484614 */
void btm_delete_stored_link_key_complete(u8* p);
/* 0x8048467C */
void btm_return_link_keys_evt(u8* p_result);
/* 0x80484808 */
void btm_report_device_status(u8 status);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_BTM_LINK_KEY_H */
