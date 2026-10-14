/*
 * BTE/hidh_conn.c - the HID host connection layer: L2CAP registration, the security-check completions, the L2CAP
 *   callbacks (connect, config, disconnect, congestion, data), data send and connection origination.
 *
 * RANGE. .text 0x80490E54-0x80492E94 (16 functions, 0x2040 B); .rodata 0x80573870-0x80573898; .data
 *   0x806156F8-0x80615AE0. The .text holds the BTE hidh_conn.c functions hidh_conn_reg..hidh_conn_dereg; both ends are
 *   pinned by the .data readers (the start by hidh_api.c's last reader, the end by l2c_api.c's first). The .rodata is
 *   the L2CAP callback table hidh_conn_reg registers; the .data (trace strings) is read only by this range's functions
 *   and ends at the 4-byte pad before 0x80615AE0.
 * FLAGS. the `OS` lib default `cflags_os`, as BTE/gki_time.c: the function starts of this band are packed to 4 bytes
 *   (the alignment scan of the map's function rows).
 * NAMES. the map's.
 * RESIDUALS. every body is unwritten: 0x80490E54-0x80492E94 (16 functions).
 * SHAPES. plain C; the BTE declarations the bodies need live in BTE/gki_time.h until the per-owner headers are split.
 */
