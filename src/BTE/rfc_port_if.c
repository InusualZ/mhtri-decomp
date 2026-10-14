/*
 * BTE/rfc_port_if.c - the RFCOMM port interface: the RFCOMM_* requests and responses the port layer sends into the
 *   state machines.
 *
 * RANGE. .text 0x8049D0F8-0x8049D624 (12 functions, 0x52C B). The .text holds the BTE rfc_port_if.c functions
 *   RFCOMM_StartRsp..RFCOMM_DlcReleaseReq and the data request at 0x8049D5EC; rfc_ts_frames.c starts at 0x8049D624 with
 *   the SABME sender. No data.
 * FLAGS. the `OS` lib default `cflags_os`, as BTE/gki_time.c: the function starts of this band are packed to 4 bytes
 *   (the alignment scan of the map's function rows).
 * NAMES. the map's. Unnamed row, by the BTE source order only (GUESS, not applied): 0x8049D5EC RFCOMM_DataReq.
 * RESIDUALS. every body is unwritten: 0x8049D0F8-0x8049D624 (12 functions).
 * SHAPES. plain C; the BTE declarations the bodies need live in BTE/gki_time.h until the per-owner headers are split.
 */
