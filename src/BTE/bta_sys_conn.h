/* BTE/bta_sys_conn.h - the BTA connection manager entry points `BTE/bta_sys_conn.c` owns (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_BTA_SYS_CONN_H
#define MHTRI_BTE_BTA_SYS_CONN_H

#include "types.h"
#include "BTE/bta_sys_int.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8047AC98 / 0x8047ACA8 / 0x8047ACB8 - register the resource manager, compression and power manager callbacks. */
void bta_sys_rm_register(tBTA_SYS_CONN_CBACK* p_cback);
void bta_sys_compress_register(tBTA_SYS_CONN_CBACK* p_cback);
void bta_sys_pm_register(tBTA_SYS_CONN_CBACK* p_cback);

/* 0x8047ACC8 / 0x8047AD8C - report that the subsystem `id` (application `app_id`) opened or closed a connection to
 * `peer_addr`. */
void bta_sys_conn_open(u8 id, u8 app_id, u8* peer_addr);
void bta_sys_conn_close(u8 id, u8 app_id, u8* peer_addr);

/* 0x8047AE50 / 0x8047AE84 / 0x8047AEB8 - report an SCO close, an idle connection or a busy connection to the power
 * manager. */
void bta_sys_sco_close(u8 id, u8 app_id, u8* peer_addr);
void bta_sys_idle(u8 id, u8 app_id, u8* peer_addr);
void bta_sys_busy(u8 id, u8 app_id, u8* peer_addr);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_BTA_SYS_CONN_H */
