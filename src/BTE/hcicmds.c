/*
 * BTE/hcicmds.c - the HCI command builders: the btsnd_hcic_* functions, each filling a GKI buffer with one HCI command
 *   and sending it through btu_hcif_send_cmd.
 *
 * RANGE. .text 0x8048D44C-0x8048FB68 (59 functions, 0x271C B); .rodata 0x80573860-0x80573870. Strong by family:
 *   btsnd_hcic_inquiry..the vendor-specific command builder 0x8048FA74 (called by BTM_VendorSpecificCommand);
 *   gap_convert_btm_status precedes it and hidd_api.c's HID_DevInit follows. No .data; the .rodata is one 0x10 B table
 *   read only by btsnd_hcic_set_afh_channels, right after the last BTE/gki_time.c .rodata object.
 * FLAGS. the `OS` lib default `cflags_os`, as BTE/gki_time.c: the function starts of this band are packed to 4 bytes
 *   (the alignment scan of the map's function rows).
 * NAMES. the map's. 27 rows are unnamed (the fn_ rows between the named btsnd_hcic_* builders; the HCI opcode each one
 *   stores names it).
 * RESIDUALS. every body is unwritten: 0x8048D44C-0x8048FB68 (59 functions).
 * SHAPES. plain C; the BTE declarations the bodies need live in BTE/gki_time.h until the per-owner headers are split.
 */
