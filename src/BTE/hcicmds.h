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
void btsnd_hcic_accept_conn(BT_HDR* p_buf, u8* dest, u8 role);
void btsnd_hcic_reject_conn(BT_HDR* p_buf, u8* dest, u8 reason);

/* 0x8048F188 - sets the automatic flush timeout (0.625 ms slots) of the link `handle`, in `p_buf`. */
void btsnd_hcic_write_auto_flush_tout(BT_HDR* p_buf, u16 handle, u16 timeout);

/* 0x8048EFA4 - writes HCI Write_Scan_Enable with `flag` into `p_buf` and sends it. */
void btsnd_hcic_write_scan_enable(BT_HDR* p_buf, u8 flag);

/* 0x8048D6F4 / 0x8048DD88 / 0x8048DE94 - disconnect `handle`, set its packet types and switch its encryption; each
 * returns whether the command was queued. */
u8 btsnd_hcic_disconnect(u16 handle, u8 reason);
u8 btsnd_hcic_change_conn_type(u16 handle, u16 packet_types);
u8 btsnd_hcic_set_conn_encrypt(u16 handle, u8 enable);

/* 0x8048E098 / 0x8048E114 / 0x8048E190 - read the remote features, version and clock offset of the link `handle`. */
u8 btsnd_hcic_rmt_features_req(u16 handle);
u8 btsnd_hcic_rmt_ver_req(u16 handle);
u8 btsnd_hcic_read_rmt_clk_offset(u16 handle);

/* 0x8048E78C / 0x8048E83C - switch to `role` on the link to `bd_addr`, and set the link policy of `handle`. */
u8 btsnd_hcic_switch_role(u8* bd_addr, u8 role);
u8 btsnd_hcic_write_policy_set(u16 handle, u16 settings);

/* 0x8048E8D0 - resets the controller. */
u8 btsnd_hcic_reset(void);

/* 0x8048EB5C / 0x8048EBBC / 0x8048ED7C - read, write and delete the controller's stored link keys. */
void btsnd_hcic_read_stored_key(BT_HDR* p_buf, u8* bd_addr, u8 read_all_flag);
void btsnd_hcic_write_stored_key(BT_HDR* p_buf, u8 num_keys, u8 (*bd_addr)[6], u8 (*link_key)[16]);
u8 btsnd_hcic_delete_stored_key(u8* bd_addr, u8 delete_all_flag);

/* 0x8048EE2C / 0x8048EF6C / 0x8048F144 - set the local name, the page timeout and the device class, in `p_buf`. */
void btsnd_hcic_change_name(BT_HDR* p_buf, u8* name);
void btsnd_hcic_write_page_tout(BT_HDR* p_buf, u16 timeout);
void btsnd_hcic_write_dev_class(BT_HDR* p_buf, u8* dev_class);

/* 0x8048F1CC / 0x8048F288 - tell the controller the host buffer sizes, and set the supervision timeout of `handle`. */
u8 btsnd_hcic_set_host_buf_size(u16 acl_len, u8 sco_len, u16 acl_num, u16 sco_num);
u8 btsnd_hcic_write_link_super_tout(u16 handle, u16 timeout);

/* 0x8048F38C / 0x8048F3EC / 0x8048F448 / 0x8048F470 - read the local version, features, buffer size and address. */
u8 btsnd_hcic_read_local_ver(void);
u8 btsnd_hcic_read_local_features(void);
void btsnd_hcic_read_buffer_size(BT_HDR* p_buf);
u8 btsnd_hcic_read_bd_addr(void);

/* 0x8048F4D0 / 0x8048F54C - read the link quality and the RSSI of the link `handle`. */
u8 btsnd_hcic_get_link_quality(u16 handle);
u8 btsnd_hcic_read_rssi(u16 handle);

/* 0x8048F5C4 - disables the AFH channels `first` to `last`. */
u8 btsnd_hcic_set_afh_channels(u8 first, u8 last);

/* 0x8048FA74 - sends the vendor specific command `opcode` with `len` parameter bytes, in `p_buf`. */
void btsnd_hcic_vendor_spec_cmd(BT_HDR* p_buf, u16 opcode, u8 len, u8* p_data);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_HCICMDS_H */
