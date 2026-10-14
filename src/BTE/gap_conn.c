/*
 * BTE/gap_conn.c - the GAP connection layer: gap_conn_init and the L2CAP callbacks it registers (connect, config,
 *   disconnect, data and congestion indications/confirmations).
 *
 * RANGE. .text 0x8048C2F4-0x8048CE30 (8 functions, 0xB3C B); .data 0x806151D0-0x806152A0; .sdata 0x80793DA0-0x80793DA8.
 *   The .text holds the BTE gap_conn.c functions gap_conn_init..gap_congestion_ind; gap_utils.c's BTM callback follows
 *   at 0x8048CE30, pinned by the .data. The .data starts with no pad beyond 4-byte alignment where the readers change;
 *   the .sdata (an 8-byte string read by gap_connect_ind) starts after a 4-byte pad beyond 4-byte alignment (0x80793D9C
 *   -> 0x80793DA0).
 * FLAGS. the `OS` lib default `cflags_os`, as BTE/gki_time.c: the function starts of this band are packed to 4 bytes
 *   (the alignment scan of the map's function rows).
 * NAMES. the map's. GUESS gap_data_ind (0x8048CBEC: the data-indication slot of the L2CAP callback table gap_conn_init
 *   fills, between the disconnect and congestion slots), not applied.
 * RESIDUALS. every body is unwritten: 0x8048C2F4-0x8048CE30 (8 functions).
 * SHAPES. plain C; the BTE declarations the bodies need live in BTE/gki_time.h until the per-owner headers are split.
 */
