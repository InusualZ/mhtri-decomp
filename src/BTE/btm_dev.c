/*
 * BTE/btm_dev.c - the BTM security device records: add, delete and name read, record allocation and the lookups by
 *   handle, by address and the oldest record.
 *
 * RANGE. .text 0x804827D4-0x80482EA0 (7 functions, 0x6CC B); no data. Medium: the seven functions are exactly the BTE
 *   source's btm_dev.c (BTM_SecAddDevice..btm_find_oldest_dev), between btm_acl.c's btm_chg_all_acl_pkt_types and
 *   btm_devctl.c's btm_dev_init; no data on either side pins the cuts.
 * FLAGS. the `OS` lib default `cflags_os`, as BTE/gki_time.c (registered flags; codegen evidence in BTE/sdp_* points to
 *   GC/3.0a5.2 + cflags_nw4r (unmeasured here)).
 * NAMES. the map's.
 * RESIDUALS. every body is unwritten: 0x804827D4-0x80482EA0 (7 functions).
 * SHAPES. plain C; the BTE declarations the bodies need live in BTE/gki_time.h until the per-owner headers are split.
 */
