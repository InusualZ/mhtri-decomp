/*
 * BTE/btm_discovery.c - the BTM discovery database: init, reset and the discovery timeout.
 *
 * RANGE. .text 0x80484828-0x8048495C (3 functions, 0x134 B); no data. Weak: the three btm_discovery_* functions read
 *   no data, and the .data pad seam 0x806141E0 allows any cut in [0x8048453C, 0x8048495C], so they may be the tail of
 *   BTE/btm_link_key.c or the head of BTE/btm_inq.c; only the name family separates them.
 * FLAGS. the `OS` lib default `cflags_os`, as BTE/gki_time.c (registered flags; codegen evidence in BTE/sdp_* points to
 *   GC/3.0a5.2 + cflags_nw4r (unmeasured here)).
 * NAMES. the map's. The file name is a GUESS from the btm_discovery_* family.
 * RESIDUALS. every body is unwritten: 0x80484828-0x8048495C (3 functions).
 * SHAPES. plain C; the BTE declarations the bodies need live in BTE/gki_time.h until the per-owner headers are split.
 */
