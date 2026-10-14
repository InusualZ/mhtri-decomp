/*
 * BTE/gap_utils.c - the GAP utilities: the BTM event callback and its two registered thunks, the inquiry-database
 *   name/address search and the BTM status conversion.
 *
 * RANGE. .text 0x8048CE30-0x8048D44C (6 functions, 0x61C B); .data 0x806152A0-0x80615590. Strong: the .data starts
 *   after a 4-byte pad beyond 4-byte alignment (0x8061529C -> 0x806152A0), and its first reader btm_cback is the first
 *   function after gap_congestion_ind, the last gap_conn.c .data reader - the cut is pinned exactly at 0x8048CE30. It
 *   ends with gap_convert_btm_status; hcicmds.c's btsnd_hcic_inquiry follows. Every .data object (strings and five
 *   switch tables) is read only by this range's functions.
 * FLAGS. the `OS` lib default `cflags_os`, as BTE/gki_time.c: the function starts of this band are packed to 4 bytes
 *   (the alignment scan of the map's function rows).
 * NAMES. the map's; btm_cback is the map's name for the GAP BTM callback. Unnamed rows: 0x8048D008 and 0x8048D014
 *   (thunks calling btm_cback, registered by GAP_Init) and 0x8048D020 (the inquiry-database name search
 *   gap_find_addr_inq_cb calls).
 * RESIDUALS. every body is unwritten: 0x8048CE30-0x8048D44C (6 functions).
 * SHAPES. plain C; the BTE declarations the bodies need live in BTE/gki_time.h until the per-owner headers are split.
 */
