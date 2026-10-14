/* BTE/bta_sys_conn.h - the connection-state reports `BTE/bta_sys_conn.c` owns that other units call (docs/plan.md
 *   6.5 rule 2). */
#ifndef MHTRI_BTE_BTA_SYS_CONN_H
#define MHTRI_BTE_BTA_SYS_CONN_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8047ACC8 / 0x8047AD8C - report the open and the close of the connection of subsystem `id` to `peer_addr`. */
void bta_sys_conn_open(u8 id, u8 app_id, u8* peer_addr);
void bta_sys_conn_close(u8 id, u8 app_id, u8* peer_addr);

/* 0x8047AE50 / 0x8047AE84 / 0x8047AEB8 - report a SCO close, an idle and a busy connection (power-manager status
 * 5, 6 and 7). */
void bta_sys_sco_close(u8 id, u8 app_id, u8* peer_addr);
void bta_sys_idle(u8 id, u8 app_id, u8* peer_addr);
void bta_sys_busy(u8 id, u8 app_id, u8* peer_addr);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_BTA_SYS_CONN_H */
