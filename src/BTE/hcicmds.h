/* BTE/hcicmds.h - the HCI command builders `BTE/hcicmds.c` owns that other units call (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_HCICMDS_H
#define MHTRI_BTE_HCICMDS_H

#include "types.h"
#include "BTE/gki_buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8048D614 - pages the device at `dest`; returns whether the command was queued. */
u8 btsnd_hcic_create_conn(u8* dest, u16 packet_types, u8 page_scan_rep_mode, u8 page_scan_mode, u16 clock_offset,
                          u8 allow_switch);

/* 0x8048D808 / 0x8048D868 - accept (with `role`) or reject (with `reason`) an incoming connection, in `p_buf`. */
u8 btsnd_hcic_accept_conn(BT_HDR* p_buf, u8* dest, u8 role);
u8 btsnd_hcic_reject_conn(BT_HDR* p_buf, u8* dest, u8 reason);

/* 0x8048F188 - sets the automatic flush timeout (0.625 ms slots) of the link `handle`, in `p_buf`. */
u8 btsnd_hcic_write_auto_flush_tout(BT_HDR* p_buf, u16 handle, u16 timeout);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_HCICMDS_H */
