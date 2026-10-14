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

/* 0x8048D44C - starts an inquiry on the access code `inq_lap`. */
u8 btsnd_hcic_inquiry(const u8* inq_lap, u8 duration, u8 response_cnt);

/* 0x8048D4F4 - cancels the running inquiry. */
u8 btsnd_hcic_inq_cancel(void);

/* 0x8048D554 - starts periodic inquiries every `min_period`..`max_period` (1.28 s units). */
u8 btsnd_hcic_per_inq_mode(u16 max_period, u16 min_period, const u8* inq_lap, u8 duration, u8 response_cnt);

/* 0x8048D77C - adds an SCO link with `packet_types` to the ACL link `handle`. */
u8 btsnd_hcic_add_SCO_conn(u16 handle, u16 packet_types);

/* 0x8048D8C4 - answers a link key request of `bd_addr` with `link_key`. */
u8 btsnd_hcic_link_key_req_reply(u8* bd_addr, u8* link_key);

/* 0x8048DA64 - answers a link key request of `bd_addr` with no key. */
u8 btsnd_hcic_link_key_neg_reply(u8* bd_addr);

/* 0x8048DB04 - answers a PIN request of `bd_addr` with the `pin_code_len` bytes of `p_pin_code`, zero-padded. */
u8 btsnd_hcic_pin_code_req_reply(u8* bd_addr, u8 pin_code_len, u8* p_pin_code);

/* 0x8048DCE8 - refuses a PIN request of `bd_addr`. */
u8 btsnd_hcic_pin_code_neg_reply(u8* bd_addr);

/* 0x8048DE18 - starts the authentication of the link `handle`. */
u8 btsnd_hcic_auth_request(u16 handle);

/* 0x8048DF20 - requests the name of the device at `bd_addr`. */
u8 btsnd_hcic_rmt_name_req(u8* bd_addr, u8 page_scan_rep_mode, u8 page_scan_mode, u16 clock_offset);

/* 0x8048DFF8 - cancels the name request to `bd_addr`. */
u8 btsnd_hcic_rmt_name_req_cancel(u8* bd_addr);

/* 0x8048E20C - sets up an eSCO link on the ACL link `handle`. */
u8 btsnd_hcic_setup_esco_conn(u16 handle, u32 tx_bw, u32 rx_bw, u16 max_latency, u16 voice, u8 retrans_effort,
                              u16 packet_types);

/* 0x8048E308 - accepts the incoming eSCO connection from `bd_addr`, in `p_buf`. */
void btsnd_hcic_accept_esco_conn(BT_HDR* p_buf, u8* bd_addr, u32 tx_bw, u32 rx_bw, u16 max_latency, u16 content_fmt,
                                 u8 retrans_effort, u16 packet_types);

/* 0x8048E3F0 - rejects the incoming eSCO connection from `bd_addr` with `reason`, in `p_buf`. */
void btsnd_hcic_reject_esco_conn(BT_HDR* p_buf, u8* bd_addr, u8 reason);

/* 0x8048E450 - puts the link `handle` in hold mode, in `p` or a new buffer. */
u8 btsnd_hcic_hold_mode(BT_HDR* p_buf, u16 handle, u16 max_hold_period, u16 min_hold_period);

/* 0x8048E504 - puts the link `handle` in sniff mode, in `p` or a new buffer. */
u8 btsnd_hcic_sniff_mode(BT_HDR* p_buf, u16 handle, u16 max_sniff_period, u16 min_sniff_period, u16 sniff_attempt,
                         u16 sniff_timeout);

/* 0x8048E5D0 - takes the link `handle` out of sniff mode, in `p` or a new buffer. */
u8 btsnd_hcic_exit_sniff_mode(BT_HDR* p_buf, u16 handle);

/* 0x8048E654 - puts the link `handle` in park mode, in `p` or a new buffer. */
u8 btsnd_hcic_park_mode(BT_HDR* p_buf, u16 handle, u16 beacon_max_interval, u16 beacon_min_interval);

/* 0x8048E708 - takes the link `handle` out of park mode, in `p` or a new buffer. */
u8 btsnd_hcic_exit_park_mode(BT_HDR* p_buf, u16 handle);

/* 0x8048E92C - sets the event filter `filt_type` with the `filt_cond_len`-byte condition `filt_cond`, in `p_buf`. */
void btsnd_hcic_set_event_filter(BT_HDR* p_buf, u8 filt_type, u8 filt_cond_type, u8* filt_cond, u8 filt_cond_len);

/* 0x8048EAE8 - sets whether the host's PIN is fixed or variable. */
u8 btsnd_hcic_write_pin_type(u8 type);

/* 0x8048EFD4 - sets the page scan interval and window, in `p_buf`. */
void btsnd_hcic_write_pagescan_cfg(BT_HDR* p_buf, u16 interval, u16 window);

/* 0x8048F018 - sets the inquiry scan interval and window, in `p_buf`. */
void btsnd_hcic_write_inqscan_cfg(BT_HDR* p_buf, u16 interval, u16 window);

/* 0x8048F05C - turns authentication of every connection on or off. */
u8 btsnd_hcic_write_auth_enable(u8 flag);

/* 0x8048F0D0 - sets the encryption mode of every connection. */
u8 btsnd_hcic_write_encr_mode(u8 mode);

/* 0x8048F31C - sets the `num_cur_iac` inquiry access codes the host scans for, in `p_buf`. */
void btsnd_hcic_write_cur_iac_lap(BT_HDR* p_buf, u8 num_cur_iac, u8 (*iac_lap)[3]);

/* 0x8048F9E4 - sets the inquiry scan type, in `p_buf`. */
void btsnd_hcic_write_inqscan_type(BT_HDR* p_buf, u8 type);

/* 0x8048FA14 - sets the inquiry result format, in `p_buf`. */
void btsnd_hcic_write_inquiry_mode(BT_HDR* p_buf, u8 mode);

/* 0x8048FA44 - sets the page scan type, in `p_buf`. */
void btsnd_hcic_write_pagescan_type(BT_HDR* p_buf, u8 type);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_HCICMDS_H */
