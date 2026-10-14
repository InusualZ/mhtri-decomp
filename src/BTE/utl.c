/*
 * BTE/utl.c - the BTA utility function utl_freebuf.
 *
 * RANGE. .text 0x8047B310-0x8047B354 (1 function, 0x44 B); no data. Medium: utl_freebuf is the one utl.c function this
 *   link keeps; it sits between ptim_stop_timer and bta_dm_enable, the first function that reads the bta_dm data.
 * FLAGS. the `OS` lib default `cflags_os`, as BTE/gki_time.c (registered flags; codegen evidence in BTE/sdp_* points to
 *   GC/3.0a5.2 + cflags_nw4r (unmeasured here)).
 * NAMES. the map's.
 * RESIDUALS. every body is unwritten: 0x8047B310-0x8047B354 (1 function).
 * SHAPES. plain C; the BTE declarations the bodies need live in BTE/gki_time.h until the per-owner headers are split.
 */
